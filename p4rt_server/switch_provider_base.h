/*
 * Copyright 2020 Google LLC
 * Copyright 2020-present Open Networking Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SWITCH_PROVIDER_BASE_
#define SWITCH_PROVIDER_BASE_

#include "sdn_controller_manager.h"

#include <memory>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "p4/v1/p4runtime.grpc.pb.h"

/*
 * SwitchProviderBase is a base class that should be sub-classed 
 * With switch/application specific implementation of the pure 
 * virtual functions below.  
 * A unique_ptr of the subclass is passed to the p4rt_server 
 * constructor via std::move 
 */
namespace switch_provider{
  class SwitchProviderBase{
    private:
      std::shared_ptr<p4rt_server::SdnControllerManager> controller_manager_;
    protected:
      /*
       * SwitchProviderBase::SendPacketIn 
       * Provided for subclass to send PacketIns to the P4Runtime Controller Application
       */
      void SendPacketIn (absl::optional<uint64_t>& role_id, std::shared_ptr<p4::v1::StreamMessageResponse> response){
        controller_manager_->SendStreamMessageToPrimary(role_id, *response);
      }
    public:
      /*
       * SwitchProviderBase::AddSdnController
       * p4rt_server will pass in a shared_ptr to its SdnControllerManager
       * to allow subclass to send PacketIn to P4Runtime Controller application
       */ 
      void AddSdnController(std::shared_ptr<p4rt_server::SdnControllerManager> controller_manager){
         controller_manager_=controller_manager;
      }

      SwitchProviderBase(){}
      virtual ~SwitchProviderBase(){}
      
      /*
       * pure virtual functions to be implemented by SwitchProvider
       */
      virtual absl::Status DoWrite(const p4::v1::WriteRequest * request)=0;
      virtual absl::StatusOr<p4::v1::ReadResponse> DoRead(
          const p4::v1::ReadRequest * request)=0;
      virtual absl::Status SendPacketOut(const p4::v1::PacketOut& packet)=0;
      virtual absl::Status SetForwardingPipelineConfig(
          const p4::v1::ForwardingPipelineConfig)=0;
      virtual absl::StatusOr<p4::v1::ForwardingPipelineConfig>  
        GetForwardingPipelineConfig()=0;

  };
}


#endif
