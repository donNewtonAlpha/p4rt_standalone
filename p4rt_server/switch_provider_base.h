#ifndef SWITCH_PROVIDER_BASE_
#define SWITCH_PROVIDER_BASE_

#include <memory>


#include "p4/v1/p4runtime.grpc.pb.h"
#include "absl/status/statusor.h"
#include "absl/status/status.h"
#include "sdn_controller_manager.h"


namespace switch_provider{
  class SwitchProviderBase{
    protected:
      std::shared_ptr<p4rt_server::SdnControllerManager> controller_manager_;
      void SendPacketIn (std::string role_name, std::shared_ptr<p4::v1::StreamMessageResponse> response){
        controller_manager_->SendStreamMessageToPrimary(role_name, *response);
      }
    public:
      void AddSdnController(std::shared_ptr<p4rt_server::SdnControllerManager> controller_manager){
         controller_manager_=controller_manager;
      }

      SwitchProviderBase(){}

      virtual ~SwitchProviderBase(){}
      
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
