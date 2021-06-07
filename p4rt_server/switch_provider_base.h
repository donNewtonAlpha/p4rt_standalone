#ifndef SWITCH_PROVIDER_BASE_
#define SWITCH_PROVIDER_BASE_

#include <memory>


#include "p4/v1/p4runtime.grpc.pb.h"
#include "absl/status/statusor.h"
#include "absl/status/status.h"

#include "channel.h"

namespace switch_provider{
  class SwitchProviderBase{
    protected:
      Channel<std::shared_ptr<p4::v1::PacketIn>>  * chan_;
    public:
      SwitchProviderBase();
      
      void AddChannel(Channel<std::shared_ptr<p4::v1::PacketIn>> * chan){
        chan_=chan;
      }
      void SendPacketIn (std::shared_ptr<p4::v1::PacketIn> packet_in){
        chan_->put(packet_in);
      }

     
      virtual absl::Status DoWrite(const p4::v1::WriteRequest * request);
      virtual absl::StatusOr<p4::v1::ReadResponse> DoRead(
          const p4::v1::ReadRequest * request);
      virtual absl::Status SendPacketOut(const p4::v1::PacketOut& packet);
      virtual absl::Status SetForwardingPipelineConfig(
          const p4::v1::ForwardingPipelineConfig);
      virtual absl::StatusOr<p4::v1::ForwardingPipelineConfig>  GetForwardingPipelineConfig();

  };
}


#endif
