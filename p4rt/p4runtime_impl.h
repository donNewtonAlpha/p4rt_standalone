#ifndef P4RUNTIME_IMPL_H_
#define P4RUNTIME_IMPL_H_

#include <memory>
#include "switch_provider_base.h"
#include "sdn_controller_manager.h"
#include "grpcpp/grpcpp.h"
#include "grpcpp/server_context.h"
#include "p4/v1/p4runtime.grpc.pb.h"

namespace p4rt{ 
  class P4RuntimeImpl final : public p4::v1::P4Runtime::Service{
    public: 
      P4RuntimeImpl(std::unique_ptr<switch_provider::SwitchProviderBase> switch_provider);
      ~P4RuntimeImpl() = default;
      
      grpc::Status Write(grpc::ServerContext* context,
                                    const p4::v1::WriteRequest* request,
                                    p4::v1::WriteResponse* response);
      
      grpc::Status Read( grpc::ServerContext* context, 
          const p4::v1::ReadRequest* request,
          grpc::ServerWriter<p4::v1::ReadResponse>* response_writer);
      
      grpc::Status StreamChannel( grpc::ServerContext* context,
            grpc::ServerReaderWriter<p4::v1::StreamMessageResponse,
            p4::v1::StreamMessageRequest>* stream); 

      grpc::Status SetForwardingPipelineConfig(grpc::ServerContext* context,
            const p4::v1::SetForwardingPipelineConfigRequest* request,
            p4::v1::SetForwardingPipelineConfigResponse* response);

      grpc::Status GetForwardingPipelineConfig( grpc::ServerContext* context,
          const p4::v1::GetForwardingPipelineConfigRequest* request,
          p4::v1::GetForwardingPipelineConfigResponse* response);

    private:
      std::unique_ptr<switch_provider::SwitchProviderBase> switch_provider_;
      std::unique_ptr<SdnControllerManager> controller_manager_;
  };

}

#endif //ifndef P4RUNTIME_IMPL_H_
