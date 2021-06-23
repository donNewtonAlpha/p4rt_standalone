# p4rt_standalone
## Purpose
Provide a platform agnostic implementation of P4Runtime Server suitable for embedding in a switch / application stack to enable P4Runtime programability
## Specification 
[P4Runtime v1.3.0](https://p4lang.github.io/p4runtime/spec/v1.3.0/P4Runtime-Spec.html)
## Sample Implementation and Usage 
### Implement Sub-class
```
    class MySwitchProvider final :  public switch_provider::SwitchProviderBase {
      public:
      //INHERITED
          absl::Status DoWrite(const p4::v1::WriteRequest * request);
          absl::StatusOr<p4::v1::ReadResponse> DoRead(
                        const p4::v1::ReadRequest * request);
          absl::Status SendPacketOut(const p4::v1::PacketOut& packet);
          absl::Status SetForwardingPipelineConfig(
                        const p4::v1::ForwardingPipelineConfig);
          absl::StatusOr<p4::v1::ForwardingPipelineConfig>  GetForwardingPipelineConfig();
      //END INHERITED
      //Subclass specific initialization here
      ...
      };
  ```
### Construct Sub-class 

 ```
     auto provider = std::make_unique<MySwitchProvider>(...);
```
### Construct P4RtServer by moving in unique_ptr to Switch Provider sub-class
```  
  p4rt_server::P4RtServer p4runtime_server(std::move(provider));
```
### Wrap P4RtServer with GRPC Service E.G.

```
  // Start a P4 runtime server
  ServerBuilder builder;
  auto server_cred = BuildServerCredentials();
  if (!server_cred.ok()) {
    LOG(ERROR) << "Failed to build server credentials, error "
               << server_cred.status();
    return -1;
  }
  builder.AddListeningPort(kServerAddress, *server_cred);
  builder.RegisterService(&p4runtime_server);

  std::unique_ptr<Server> server(builder.BuildAndStart());
  LOG(INFO) << "Server listening on " << kServerAddress << ".";
  server->Wait();
```
## Build Instructions
### Building Library
```
bazel build //p4rt_server:p4rt_server
```
### Pulling in as dependency for bazel build of switch
```
git_repository(
        name = "p4rt_standalone",
        commit = "$CURRENT_COMMIT_TO_USE",
        remote = "http://github.com/donNewtonAlpha/p4rt_standalone.git",
    )
```
