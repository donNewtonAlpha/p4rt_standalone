/*
 * Copyright 2020 Google LLC
 * Copyright 2020-present Open Networking Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "p4rt_server.h"
#include "sdn_controller_manager.h"

#include <memory>

#include "absl/status/status.h"
#include "glog/logging.h"
#include "grpcpp/grpcpp.h"
#include "grpcpp/server_context.h"
#include "gutil/status.h"
#include "p4/v1/p4runtime.grpc.pb.h"



namespace p4rt_server{
namespace{
  // Generates a StreamMessageResponse error based on an absl::Status.
   p4::v1::StreamMessageResponse GenerateErrorResponse(absl::Status status) {
     grpc::Status grpc_status = gutil::AbslStatusToGrpcStatus(status);
     p4::v1::StreamMessageResponse response;
     auto error = response.mutable_error();
     error->set_canonical_code(grpc_status.error_code());
     error->set_message(grpc_status.error_message());
     return response;
   }

   // Generates StreamMessageResponse with errors for PacketIO
   p4::v1::StreamMessageResponse GenerateErrorResponse(
       absl::Status status, const p4::v1::PacketOut& packet) {
     p4::v1::StreamMessageResponse response = GenerateErrorResponse(status);
     *response.mutable_error()->mutable_packet_out()->mutable_packet_out() =
         packet;
     return response;
   }
}

P4RtServer::P4RtServer(
    std::unique_ptr<switch_provider::SwitchProviderBase> switch_provider):
  switch_provider_(std::move(switch_provider)){
  LOG(ERROR) << "P4RtServer::P4RtServer calling init";
  controller_manager_ = std::make_shared<SdnControllerManager>(); 
  switch_provider_->AddSdnController(controller_manager_);
}

/*
 * P4RtServer::Write
 * Handles write requests from P4Runtime Controller application
 */
grpc::Status P4RtServer::Write(grpc::ServerContext* context,
                                    const p4::v1::WriteRequest* request,
                                    p4::v1::WriteResponse* response) {
  #ifdef __EXCEPTIONS
    try {
  #endif

      // verify the request comes from the primary connection.
      auto connection_status = controller_manager_->AllowRequest(*request);
      if (!connection_status.ok()) {
        return connection_status;
      }
      auto result = switch_provider_->DoWrite(request);
      return gutil::AbslStatusToGrpcStatus(result);
  #ifdef __EXCEPTIONS
    } catch (const std::exception& e) {
      LOG(FATAL) << "Exception caught in " << __func__ << ", error:" << e.what();
    } catch (...) {
      LOG(FATAL) << "Unknown exception caught in " << __func__;
    }
  #endif
}

/*
 * P4RtServer::Read
 * Handles read requests from P4Runtime Controller application
 */
grpc::Status P4RtServer::Read(
    grpc::ServerContext* context, const p4::v1::ReadRequest* request,
    grpc::ServerWriter<p4::v1::ReadResponse>* response_writer) {
#ifdef __EXCEPTIONS
  try {
#endif
    if (request == nullptr) {
      return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                          "ReadRequest cannot be a nullptr.");
    }   
    if (response_writer == nullptr) {
      return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                          "ReadResponse writer cannot be a nullptr.");
    }   

    auto response_status = switch_provider_->DoRead(request);
    if (!response_status.ok()) {
      LOG(WARNING) << "Read failure: " << response_status.status();
      return grpc::Status(
          grpc::StatusCode::UNKNOWN,
          absl::StrCat("Read failure: ", response_status.status().ToString()));
    }   

    response_writer->Write(response_status.value());
    return grpc::Status::OK;
#ifdef __EXCEPTIONS
  } catch (const std::exception& e) {
    LOG(FATAL) << "Exception caught in " << __func__ << ", error:" << e.what();
  } catch (...) {
    LOG(FATAL) << "Unknown exception caught in " << __func__;
  }
#endif
  
}

/*
 * P4RtServer::StreamChannel
 * Sets up grpc stream channel for bi-directional communication
 * Between the P4Runtime Controller application and this server
 */
grpc::Status P4RtServer::StreamChannel(
    grpc::ServerContext* context,
    grpc::ServerReaderWriter<p4::v1::StreamMessageResponse,
                             p4::v1::StreamMessageRequest>* stream) {
#ifdef __EXCEPTIONS
  try {
#endif
    // We create a unique SDN connection object for every active connection.
    auto sdn_connection = absl::make_unique<SdnConnection>(context, stream);

    // While the connection is active we can receive and send requests.
    p4::v1::StreamMessageRequest request;
    while (stream->Read(&request)) {

      switch (request.update_case()) {
        case p4::v1::StreamMessageRequest::kArbitration: {
          LOG(INFO) << "Received arbitration request: "
                    << request.ShortDebugString();

          auto status = controller_manager_->HandleArbitrationUpdate(
              request.arbitration(), sdn_connection.get());
          if (!status.ok()) {
            LOG(WARNING) << "Failed arbitration request: "
                         << status.error_message();
            controller_manager_->Disconnect(sdn_connection.get());
            return status;
          }
          break;
        }
        case p4::v1::StreamMessageRequest::kPacket: {
          // Returns with an error if the write request was not received from a
          // primary connection
          bool is_primary = controller_manager_
                                ->AllowRequest(sdn_connection->GetRoleName(),
                                               sdn_connection->GetElectionId())
                                .ok();
          if (!is_primary) {
            sdn_connection->SendStreamMessageResponse(GenerateErrorResponse(
                gutil::PermissionDeniedErrorBuilder()
                    << "Cannot process request. Only the primary connection "
                       "can send PacketOuts.",
                request.packet()));
          } else {
              auto status = switch_provider_->SendPacketOut(request.packet());
              if (!status.ok()) {
                // Get the primary streamchannel and write into the stream.
                controller_manager_->SendStreamMessageToPrimary(
                    sdn_connection->GetRoleName(),
                    GenerateErrorResponse(gutil::StatusBuilder(status)
                                              << "Failed to send packet out.",
                                          request.packet()));
              }
            }
            break;
        }
        case p4::v1::StreamMessageRequest::kDigestAck:
        case p4::v1::StreamMessageRequest::kOther:
        default:
          sdn_connection->SendStreamMessageResponse(
                GenerateErrorResponse(gutil::UnimplementedErrorBuilder()
                                      << "Stream update type is not supported."));
          LOG(ERROR) << "Received unhandled stream channel message: "
                       << request.DebugString();
      }
    }//while
    controller_manager_->Disconnect(sdn_connection.get());
    return grpc::Status::OK;
#ifdef __EXCEPTIONS
  } catch (const std::exception& e) {
    LOG(FATAL) << "Exception caught in " << __func__ << ", error:" << e.what();
  } catch (...) {
    LOG(FATAL) << "Unknown exception caught in " << __func__;
  }
#endif
 
}

/*
 * P4RtServer::SetForwardingPipelineConfig
 * Handles P4info.txt pushes from P4Runtime controller application
 */
grpc::Status P4RtServer::SetForwardingPipelineConfig(
    grpc::ServerContext* context,
    const p4::v1::SetForwardingPipelineConfigRequest* request,
    p4::v1::SetForwardingPipelineConfigResponse* response) {
#ifdef __EXCEPTIONS
  try {
#endif
    LOG(INFO)
        << "Received SetForwardingPipelineConfig request from election id: "
        << request->election_id().ShortDebugString();
    auto connection_status = controller_manager_->AllowRequest(*request);
    if (!connection_status.ok()) {
      return connection_status;
    }

    if (request->action() !=
            p4::v1::SetForwardingPipelineConfigRequest::RECONCILE_AND_COMMIT &&
        request->action() !=
            p4::v1::SetForwardingPipelineConfigRequest::VERIFY_AND_COMMIT) {
      return AbslStatusToGrpcStatus(
          gutil::UnimplementedErrorBuilder().LogError()
          << "Only Action RECONCILE_AND_COMMIT or VERIFY_AND_COMMIT is "
             "supported for "
          << "SetForwardingPipelineConfig.");
    }
    auto status = switch_provider_->SetForwardingPipelineConfig(request->config());
    return gutil::AbslStatusToGrpcStatus(status);

#ifdef __EXCEPTIONS
  } catch (const std::exception& e) {
    LOG(FATAL) << "Exception caught in " << __func__ << ", error:" << e.what();
  } catch (...) {
    LOG(FATAL) << "Unknown exception caught in " << __func__;
  }
#endif
  
}

/*
 * P4RtServer::GetForwardingPipelineConfig
 * Returns P4info.txt to P4Runtime controller application
 */
grpc::Status P4RtServer::SetForwardingPipelineConfig(
grpc::Status P4RtServer::GetForwardingPipelineConfig(
    grpc::ServerContext* context,
    const p4::v1::GetForwardingPipelineConfigRequest* request,
    p4::v1::GetForwardingPipelineConfigResponse* response) {
#ifdef __EXCEPTIONS
  try {
#endif
    auto response_status = switch_provider_->GetForwardingPipelineConfig();
    if(response_status.ok()) {
      auto p4info = response_status.value();
      switch (request->response_type())
      {
      case p4::v1::GetForwardingPipelineConfigRequest::COOKIE_ONLY:
        *response->mutable_config()->mutable_cookie() = p4info.cookie();
        break;
      default:
        *response->mutable_config() = p4info  ;
        break;
      }
    }
    return grpc::Status(grpc::StatusCode::OK, "");
#ifdef __EXCEPTIONS
  } catch (const std::exception& e) {
    LOG(FATAL) << "Exception caught in " << __func__ << ", error:" << e.what();
  } catch (...) {
    LOG(FATAL) << "Unknown exception caught in " << __func__;
  }
#endif
}

}//namespace p4rt_server

