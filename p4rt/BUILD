cc_library(
    name = "p4runtime_impl",
    srcs = ["p4runtime_impl.cc","sdn_controller_manager.cc"],
    hdrs = ["switch_provider_base.h","p4runtime_impl.h","sdn_controller_manager.h"],
    deps = [
        "//gutil:status",
        "@com_github_google_glog//:glog",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_public_hdrs",
        "@com_github_p4lang_p4runtime//:p4info_cc_proto",
        "@com_github_p4lang_p4runtime//:p4runtime_cc_grpc",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/status",
    ]
)
