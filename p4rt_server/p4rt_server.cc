/*
 * Copyright 2020 Google LLC
 * Copyright 2020-present Open Networking Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "p4rt_server.h"

#include <memory>

#include "absl/status/status.h"
#include "glog/logging.h"
#include "grpcpp/grpcpp.h"
#include "grpcpp/server_context.h"
#include "gutil/status.h"
#include "macros.h"
#include "p4/v1/p4runtime.grpc.pb.h"
#include "sdn_controller_manager.h"

namespace p4rt_server {
namespace {
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
    absl::Status status, const p4::v1::PacketOut &packet) {
    p4::v1::StreamMessageResponse response = GenerateErrorResponse(status);
    *response.mutable_error()->mutable_packet_out()->mutable_packet_out() =
        packet;
    return response;
}

grpc::Status StatusOrToGrpcStatus(
    absl::StatusOr<std::vector<absl::Status>> results) {
    if (results.ok()) {
        return grpc::Status::OK;
    } else {
        auto statusVec = results.value();
        grpc::Status returned_status;
        std::string returned_message = "Errors:\n";
        for (auto &result : statusVec) {
            auto error_message =
                absl::StrCat("\tCode:", (int)result.code(),
                             "\tMessage: ", result.message(), "\n");
        }
        return returned_status;
    }
}

}  // namespace

P4RtServer::P4RtServer(
    std::unique_ptr<switch_provider::SwitchProviderBase> switch_provider)
    : switch_provider_(std::move(switch_provider)) {
    LOG(ERROR) << "P4RtServer::P4RtServer calling init";
    controller_manager_ = std::make_shared<SdnControllerManager>();
    switch_provider_->AddSdnController(controller_manager_);
}

/*
 * P4RtServer::Write
 * Handles write requests from P4Runtime Controller application
 */
grpc::Status P4RtServer::Write(grpc::ServerContext *context,
                               const p4::v1::WriteRequest *request,
                               p4::v1::WriteResponse *response) {
    // verify the request comes from the primary connection.
    auto connection_status = controller_manager_->AllowRequest(*request);
    if (!connection_status.ok()) {
        return connection_status;
    }
    if (!request->updates_size()) return ::grpc::Status::OK;  // Nothing to do.

    // device_id is nothing but the node_id specified in the config for the
    // node.
    uint64_t node_id = request->device_id();
    if (node_id == 0) {
        return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT,
                              "device_id can not be 0 or null.");
    }

    auto result = switch_provider_->WriteForwardingEntries(request);
    return StatusOrToGrpcStatus(result);
}

/*
 * P4RtServer::Read
 * Handles read requests from P4Runtime Controller application
 */
grpc::Status P4RtServer::Read(
    grpc::ServerContext *context, const p4::v1::ReadRequest *request,
    grpc::ServerWriter<p4::v1::ReadResponse> *response_writer) {
    if (request == nullptr) {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                            "ReadRequest cannot be a nullptr.");
    }
    if (response_writer == nullptr) {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                            "ReadResponse writer cannot be a nullptr.");
    }
    if (!request->entities_size()) return ::grpc::Status::OK;
    if (request->device_id() == 0) {
        auto status = grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                                   "Deivce ID cannot be 0");
        return status;
    }

    auto response_status =
        switch_provider_->ReadForwardingEntries(request, response_writer);
    return StatusOrToGrpcStatus(response_status);
}

/*
 * P4RtServer::StreamChannel
 * Sets up grpc stream channel for bi-directional communication
 * Between the P4Runtime Controller application and this server
 */
grpc::Status P4RtServer::StreamChannel(
    grpc::ServerContext *context,
    grpc::ServerReaderWriter<p4::v1::StreamMessageResponse,
                             p4::v1::StreamMessageRequest> *stream) {
    // We create a unique SDN connection object for every active connection.
    auto sdn_connection = absl::make_unique<SdnConnection>(context, stream);

    // While the connection is active we can receive and send requests.

    p4::v1::StreamMessageRequest request;
    while (stream->Read(&request)) {
        uint64_t node_id;
        switch (request.update_case()) {
            case p4::v1::StreamMessageRequest::kArbitration: {
                LOG(INFO) << "Received arbitration request: "
                          << request.ShortDebugString();
                if (request.arbitration().device_id() == 0) {
                    return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT,
                                          "Invalid node (aka device) ID.");
                } else if (node_id == 0) {
                    node_id = request.arbitration()
                                  .device_id();  // will be available for
                                                 // subsequent requests
                } else if (node_id != request.arbitration().device_id()) {
                    std::stringstream ss;
                    ss << "Node (aka device) ID for this stream has changed. "
                          "Was "
                       << node_id << ", now is "
                       << request.arbitration().device_id() << ".";
                    return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT,
                                          ss.str());
                }
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
            case p4::v1::StreamMessageRequest::kDigestAck:
            case p4::v1::StreamMessageRequest::kOther:
            case p4::v1::StreamMessageRequest::kPacket: {
                // Returns with an error if the write request was not
                // received from a primary connection
                bool is_primary =
                    controller_manager_
                        ->AllowRequest(sdn_connection->GetRoleId(),
                                       sdn_connection->GetElectionId())
                        .ok();
                if (!is_primary) {
                    sdn_connection->SendStreamMessageResponse(
                        GenerateErrorResponse(
                            gutil::PermissionDeniedErrorBuilder()
                                << "Cannot process request. Only the "
                                   "primary connection "
                                   "can send PacketOuts.",
                            request.packet()));
                } else {
                    auto status = switch_provider_->HandleStreamMessageRequest(
                        node_id, request);
                    if (!status.ok()) {
                        // Get the primary streamchannel and write into the
                        // stream.
                        controller_manager_->SendStreamMessageToPrimary(
                            sdn_connection->GetRoleId(),
                            GenerateErrorResponse(
                                gutil::StatusBuilder(status)
                                    << "Failed to send packet out.",
                                request.packet()));
                    }
                }
            } break;
            default:
                break;  // nothing to do here
        }
    }  // while
    controller_manager_->Disconnect(sdn_connection.get());
    return grpc::Status::OK;
}

/*
 * P4RtServer::SetForwardingPipelineConfig
 * Handles P4info.txt pushes from P4Runtime controller application
 */
grpc::Status P4RtServer::SetForwardingPipelineConfig(
    grpc::ServerContext *context,
    const p4::v1::SetForwardingPipelineConfigRequest *request,
    p4::v1::SetForwardingPipelineConfigResponse *response) {
    LOG(INFO)
        << "Received SetForwardingPipelineConfig request from election id: "
        << request->election_id().ShortDebugString();
    // device_id is nothing but the node_id specified in the config for the
    // node.
    uint64_t node_id = request->device_id();
    if (node_id == 0) {
        return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT,
                              "Invalid device ID.");
    }
    auto connection_status = controller_manager_->AllowRequest(*request);
    if (!connection_status.ok()) {
        return connection_status;
    }
    absl::Status status;
    switch (request->action()) {
        case ::p4::v1::SetForwardingPipelineConfigRequest::UNSPECIFIED:
            return grpc::Status(grpc::StatusCode::UNKNOWN,
                                "Action is Unspecified");
        case ::p4::v1::SetForwardingPipelineConfigRequest::VERIFY:
            status = switch_provider_->VerifyForwardingPipelineConfig(
                node_id, request->config());
            break;
        case ::p4::v1::SetForwardingPipelineConfigRequest::VERIFY_AND_SAVE:
            status = switch_provider_->VerifyForwardingPipelineConfig(
                node_id, request->config());
            if (status.ok()) {
                status = switch_provider_->SaveForwardingPipelineConfig(
                    node_id, request->config());
            }
            break;
        case ::p4::v1::SetForwardingPipelineConfigRequest::VERIFY_AND_COMMIT:
            status = switch_provider_->VerifyForwardingPipelineConfig(
                node_id, request->config());
            if (status.ok()) {
                status =
                    switch_provider_->CommitForwardingPipelineConfig(node_id);
            }
            break;
        case ::p4::v1::SetForwardingPipelineConfigRequest::COMMIT:
            status = switch_provider_->CommitForwardingPipelineConfig(node_id);
            break;
        case ::p4::v1::SetForwardingPipelineConfigRequest::RECONCILE_AND_COMMIT:
            status =
                switch_provider_->ReconcileAndCommitForwardingPipelineConfig(
                    node_id, request->config());
            break;
        default:
            return grpc::Status(grpc::StatusCode::UNIMPLEMENTED,
                                "Invalid Action Passed in");
    }
    return gutil::AbslStatusToGrpcStatus(status);
}

/*
 * P4RtServer::GetForwardingPipelineConfig
 * Returns P4info.txt to P4Runtime controller application
 */
grpc::Status P4RtServer::GetForwardingPipelineConfig(
    grpc::ServerContext *context,
    const p4::v1::GetForwardingPipelineConfigRequest *request,
    p4::v1::GetForwardingPipelineConfigResponse *response) {
    auto response_status =
        switch_provider_->GetForwardingPipelineConfig(request->device_id());
    if (response_status.ok()) {
        auto p4info = response_status.value();
        switch (request->response_type()) {
            case p4::v1::GetForwardingPipelineConfigRequest::ALL: {
                *response->mutable_config() = p4info;
                break;
            }
            case p4::v1::GetForwardingPipelineConfigRequest::COOKIE_ONLY: {
                *response->mutable_config()->mutable_cookie() = p4info.cookie();
                break;
            }
            case p4::v1::GetForwardingPipelineConfigRequest::
                P4INFO_AND_COOKIE: {
                *response->mutable_config()->mutable_p4info() = p4info.p4info();
                *response->mutable_config()->mutable_cookie() = p4info.cookie();
                break;
            }
            case p4::v1::GetForwardingPipelineConfigRequest::
                DEVICE_CONFIG_AND_COOKIE: {
                *response->mutable_config()->mutable_p4_device_config() =
                    p4info.p4_device_config();
                *response->mutable_config()->mutable_cookie() = p4info.cookie();
                break;
            }
            default:
                return ::grpc::Status(
                    ::grpc::StatusCode::INVALID_ARGUMENT,
                    absl::StrCat("Invalid action passed for node ",
                                 request->device_id(), "."));
        }
    } else {
        return gutil::AbslStatusToGrpcStatus(response_status.status());
    }
    return grpc::Status::OK;
}
grpc::Status P4RtServer::Capabilities(
    ::grpc::ServerContext *context,
    const ::p4::v1::CapabilitiesRequest *request,
    ::p4::v1::CapabilitiesResponse *response) {
    response->set_p4runtime_api_version(STRINGIFY(P4RUNTIME_VER));
    return ::grpc::Status::OK;
}

}  // namespace p4rt_server
