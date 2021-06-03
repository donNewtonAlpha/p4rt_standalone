workspace(name = "com_github_p4lang_p4rt")

load("p4rt_deps.bzl", "p4rt_deps")
p4rt_deps()

# -- Load Rules Foreign CC -----------------------------------------------------

load("@rules_foreign_cc//foreign_cc:repositories.bzl", "rules_foreign_cc_dependencies")

rules_foreign_cc_dependencies()

# -- Load Protobuf -------------------------------------------------------------

load("@com_google_protobuf//:protobuf_deps.bzl", "protobuf_deps")

protobuf_deps()

load("@rules_proto//proto:repositories.bzl", "rules_proto_dependencies", "rules_proto_toolchains")

rules_proto_dependencies()

rules_proto_toolchains()

# -- Load P4Runtime ------------------------------------------------------------

load("@com_github_p4lang_p4runtime//:p4runtime_deps.bzl", "p4runtime_deps")

p4runtime_deps()
load("@com_google_googleapis//:repository_rules.bzl", "switched_rules_by_language")

switched_rules_by_language(
    name = "com_google_googleapis_imports",
    cc = True,
    grpc = True,
)

load("@com_github_grpc_grpc//bazel:grpc_deps.bzl", "grpc_deps")

grpc_deps()

load("@com_github_grpc_grpc//bazel:grpc_extra_deps.bzl", "grpc_extra_deps")

grpc_extra_deps()


