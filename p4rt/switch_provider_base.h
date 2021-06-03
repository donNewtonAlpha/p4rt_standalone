#ifndef SWITCH_PROVIDER_BASE_
#define SWITCH_PROVIDER_BASE_

#include "p4/v1/p4runtime.grpc.pb.h"
#include "absl/status/statusor.h"
#include "absl/status/status.h"

namespace switch_provider{
  class SwitchProviderBase{
    public:
      SwitchProviderBase();
      virtual absl::Status DoWrite(const p4::v1::WriteRequest * request);
      virtual absl::StatusOr<p4::v1::ReadResponse> DoRead(
          const p4::v1::ReadRequest * request);
      virtual absl::Status SendPacketOut(const p4::v1::PacketOut& packet);
      virtual absl::Status SetForwardingPipelineConfig(
          const p4::config::v1::P4Info& p4info);
      virtual absl::StatusOr<p4::v1::ForwardingPipelineConfig>  GetForwardingPipelineConfig();
    private:
  };
}


#endif
