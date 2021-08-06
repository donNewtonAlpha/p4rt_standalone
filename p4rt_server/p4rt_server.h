/*
 * Copyright 2020 Google LLC
 * Copyright 2020-present Open Networking Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef P4RT_SERVER_H_
#define P4RT_SERVER_H_

#include <memory>
#include <thread>

#include "grpcpp/grpcpp.h"
#include "grpcpp/server_context.h"
#include "p4/v1/p4runtime.grpc.pb.h"
#include "sdn_controller_manager.h"
#include "switch_provider_base.h"

namespace p4rt_server {
class P4RtServer final : public p4::v1::P4Runtime::Service {
   private:
    std::unique_ptr<switch_provider::SwitchProviderBase> switch_provider_;
    std::shared_ptr<SdnControllerManager> controller_manager_;

   public:
    P4RtServer(
        std::unique_ptr<switch_provider::SwitchProviderBase> switch_provider);
    ~P4RtServer() = default;

    grpc::Status Write(grpc::ServerContext* context,
                       const p4::v1::WriteRequest* request,
                       p4::v1::WriteResponse* response);

    grpc::Status Read(
        grpc::ServerContext* context, const p4::v1::ReadRequest* request,
        grpc::ServerWriter<p4::v1::ReadResponse>* response_writer);

    grpc::Status StreamChannel(
        grpc::ServerContext* context,
        grpc::ServerReaderWriter<p4::v1::StreamMessageResponse,
                                 p4::v1::StreamMessageRequest>* stream);

    grpc::Status SetForwardingPipelineConfig(
        grpc::ServerContext* context,
        const p4::v1::SetForwardingPipelineConfigRequest* request,
        p4::v1::SetForwardingPipelineConfigResponse* response);

    grpc::Status GetForwardingPipelineConfig(
        grpc::ServerContext* context,
        const p4::v1::GetForwardingPipelineConfigRequest* request,
        p4::v1::GetForwardingPipelineConfigResponse* response);

    grpc::Status Capabilities(grpc::ServerContext* context,
                              const ::p4::v1::CapabilitiesRequest* request,
                              p4::v1::CapabilitiesResponse* response);

    bool SendPacketIn(const absl::optional<uint64_t>& role_id,
                      const p4::v1::StreamMessageResponse& response) {
        return controller_manager_->SendStreamMessageToPrimary(role_id,
                                                               response);
    }
};

}  // namespace p4rt_server

#endif  // ifndef P4RT_SERVER_H_
