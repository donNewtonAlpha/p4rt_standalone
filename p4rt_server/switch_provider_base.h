/**
 * Copyright 2020 Google LLC
 * Copyright 2020-present Open Networking Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef P4RT_SWITCH_PROVIDER_BASE_
#define P$RT_SWITCH_PROVIDER_BASE_

#include <memory>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "p4/v1/p4runtime.grpc.pb.h"
#include "sdn_controller_manager.h"

/**
 * SwitchProviderBase is a base class that should be sub-classed
 * With switch/application specific implementation of the pure
 * virtual functions below.
 * A unique_ptr of the subclass is passed to the p4rt_server
 * constructor via std::move
 */
namespace switch_provider {
class SwitchProviderBase {
   private:
    std::shared_ptr<p4rt_server::SdnControllerManager> controller_manager_;

   protected:
    /**
     * SwitchProviderBase::SendPacketIn
     * Provided for subclass to send PacketIns to the P4Runtime Controller
     * Application
     */
    void SendStreamMessageResponse(
        absl::optional<uint64_t>& role_id,
        std::shared_ptr<p4::v1::StreamMessageResponse> response) {
        controller_manager_->SendStreamMessageToPrimary(role_id, *response);
    }

   public:
    /**
     * SwitchProviderBase::AddSdnController
     * p4rt_server will pass in a shared_ptr to its SdnControllerManager
     * to allow subclass to send PacketIn to P4Runtime Controller application
     */
    void AddSdnController(
        std::shared_ptr<p4rt_server::SdnControllerManager> controller_manager) {
        controller_manager_ = controller_manager;
    }

    SwitchProviderBase() {}
    virtual ~SwitchProviderBase() {}

    /*
     * pure virtual functions to be implemented by SwitchProvider
     */

    virtual absl::StatusOr<std::vector<absl::Status>> WriteForwardingEntries(
        const p4::v1::WriteRequest* request) = 0;

    virtual absl::StatusOr<std::vector<absl::Status>> ReadForwardingEntries(
        const ::p4::v1::ReadRequest* req,
        grpc::ServerWriter<::p4::v1::ReadResponse>* writer) = 0;

    virtual absl::Status HandleStreamMessageRequest(
        uint64_t node_id, const ::p4::v1::StreamMessageRequest& request) = 0;

    virtual absl::Status VerifyForwardingPipelineConfig(
        uint64_t node_id, const ::p4::v1::ForwardingPipelineConfig& config) = 0;

    virtual absl::Status SaveForwardingPipelineConfig(
        uint64_t node_id, const ::p4::v1::ForwardingPipelineConfig& config) = 0;

    virtual absl::Status CommitForwardingPipelineConfig(uint64_t node_id) = 0;

    virtual absl::Status ReconcileAndCommitForwardingPipelineConfig(
        uint64_t node_id, const ::p4::v1::ForwardingPipelineConfig& config) = 0;

    virtual absl::StatusOr<p4::v1::ForwardingPipelineConfig>
    GetForwardingPipelineConfig(uint64_t node_id) = 0;
};
}  // namespace switch_provider

#endif
