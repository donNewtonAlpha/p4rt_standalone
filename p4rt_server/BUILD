
# Copyright 2018 Google LLC
# Copyright 2018-present Open Networking Foundation
# SPDX-License-Identifier: Apache-2.0

load("//:p4rt_server_deps.bzl", "P4RUNTIME_VER")

licenses(["notice"])  # Apache v2
cc_library(
    name = "p4rt_server",
    srcs = ["p4rt_server.cc","sdn_controller_manager.cc"],
    hdrs = ["switch_provider_base.h","p4rt_server.h","sdn_controller_manager.h","macros.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//gutil:status",
        "@com_github_google_glog//:glog",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_public_hdrs",
        "@com_github_p4lang_p4runtime//:p4info_cc_proto",
        "@com_github_p4lang_p4runtime//:p4runtime_cc_grpc",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/container:flat_hash_map",
    ],
    defines = [
        "P4RUNTIME_VER=" + P4RUNTIME_VER,
    ],
)
