load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def p4rt_server_deps():
    """Sets up 3rd party workspaces needed to build PINS infrastructure."""
    if not native.existing_rule("com_github_grpc_grpc"):
        http_archive(
            name = "com_github_grpc_grpc",
            # Move to newer commit to avoid the use of com_github_google_re2.
            url = "https://github.com/grpc/grpc/archive/565520443bdbda0b8ac28337a4904f3f20276305.zip",
            strip_prefix = "grpc-565520443bdbda0b8ac28337a4904f3f20276305",
            sha256 = "7206cc8e4511620fe70da7234bc98d6a4cd2eb226a7914f68fdbd991ffe38d34",
        )
    if not native.existing_rule("com_google_absl"):
        http_archive(
            name = "com_google_absl",
            url = "https://github.com/abseil/abseil-cpp/archive/refs/tags/20210324.rc1.tar.gz",
            strip_prefix = "abseil-cpp-20210324.rc1",
            sha256 = "7e0cf185ddd0459e8e55a9c51a548e859d98c0d7533de374bf038e4c7434f682",
        )
    if not native.existing_rule("com_google_googletest"):
        http_archive(
            name = "com_google_googletest",
            urls = ["https://github.com/google/googletest/archive/release-1.10.0.tar.gz"],
            strip_prefix = "googletest-release-1.10.0",
            sha256 = "9dc9157a9a1551ec7a7e43daea9a694a0bb5fb8bec81235d8a1e6ef64c716dcb",
        )
    if not native.existing_rule("com_google_protobuf"):
        http_archive(
            name = "com_google_protobuf",
            url = "https://github.com/protocolbuffers/protobuf/releases/download/v3.14.0/protobuf-all-3.14.0.tar.gz",
            strip_prefix = "protobuf-3.14.0",
            sha256 = "6dd0f6b20094910fbb7f1f7908688df01af2d4f6c5c21331b9f636048674aebf",
        )
    if not native.existing_rule("com_googlesource_code_re2"):
        http_archive(
            name = "com_googlesource_code_re2",
            # Newest commit on "absl" branch as of 2021-03-25.
            url = "https://github.com/google/re2/archive/72f110e82ccf3a9ae1c9418bfb447c3ba1cf95c2.zip",
            strip_prefix = "re2-72f110e82ccf3a9ae1c9418bfb447c3ba1cf95c2",
            sha256 = "146bf2e8796317843106a90543356c1baa4b48236a572e39971b839172f6270e",
        )
    if not native.existing_rule("com_google_googleapis"):
        http_archive(
            name = "com_google_googleapis",
            url = "https://github.com/googleapis/googleapis/archive/f405c718d60484124808adb7fb5963974d654bb4.zip",
            strip_prefix = "googleapis-f405c718d60484124808adb7fb5963974d654bb4",
            sha256 = "406b64643eede84ce3e0821a1d01f66eaf6254e79cb9c4f53be9054551935e79",
        )
    if not native.existing_rule("com_github_google_glog"):
        http_archive(
            name = "com_github_google_glog",
            url = "https://github.com/google/glog/archive/v0.4.0.tar.gz",
            strip_prefix = "glog-0.4.0",
            sha256 = "f28359aeba12f30d73d9e4711ef356dc842886968112162bc73002645139c39c",
        )

    # Needed to make glog happy.
    if not native.existing_rule("com_github_gflags_gflags"):
        http_archive(
            name = "com_github_gflags_gflags",
            url = "https://github.com/gflags/gflags/archive/v2.2.2.tar.gz",
            strip_prefix = "gflags-2.2.2",
            sha256 = "34af2f15cf7367513b352bdcd2493ab14ce43692d2dcd9dfc499492966c64dcf",
        )
    if not native.existing_rule("com_github_gnmi"):
        http_archive(
            name = "com_github_gnmi",
            # Newest commit on master on 2020-09-10.
            # TODO: Upstream changes from this private repo to official gnmi repo.
            url = "https://github.com/vamsipunati/gnmi/archive/ffcc8725208e76d19981e71d22006b779819ae0a.zip",
            strip_prefix = "gnmi-ffcc8725208e76d19981e71d22006b779819ae0a",
            sha256 = "6e16b99d0ccb1d46703ccdc659d1e59e958df24fe82c8f07d451f3ffe042da5b",
        )
    if not native.existing_rule("com_github_gnoi"):
        http_archive(
            name = "com_github_gnoi",
            # Newest commit on master on 2021-05-20.
            url = "https://github.com/openconfig/gnoi/archive/c4a8573f7f0070ce3cef0a8127c7541c2bc71e05.zip",
            strip_prefix = "gnoi-c4a8573f7f0070ce3cef0a8127c7541c2bc71e05",
            sha256 = "8e3d3fc6a8f28dc2acb4eafbeb9fda7522d28ae9dee3e4edf788efb9a65b39c9",
        )
    if not native.existing_rule("com_github_p4lang_p4c"):
        http_archive(
            name = "com_github_p4lang_p4c",
            # Newest commit on master on 2020-09-10.
            url = "https://github.com/p4lang/p4c/archive/557b77f8c41fc5ee1158710eda1073d84f5acf53.zip",
            strip_prefix = "p4c-557b77f8c41fc5ee1158710eda1073d84f5acf53",
            sha256 = "c0b39fe2a3f394a87ff8fd055df4b5f1d19790bfa27a6cff68f484380746363e",
        )
    if not native.existing_rule("com_github_p4lang_p4runtime"):
        # We frequently need bleeding-edge, unreleased version of P4Runtime, so we use a commit
        # rather than a release.
        http_archive(
            name = "com_github_p4lang_p4runtime",
            # 0332e is the newest commit on master as of 2021-04-29.
            urls = ["https://github.com/p4lang/p4runtime/archive/0332e999c24c6bb6795181ad407cbb759bfa827d.zip"],
            strip_prefix = "p4runtime-0332e999c24c6bb6795181ad407cbb759bfa827d/proto",
            sha256 = "50a2251a5250bcfba3037fb7712e232b3581684a7cbd983840fc97363243921f",
        )
    if not native.existing_rule("com_github_p4lang_p4_constraints"):
        http_archive(
            name = "com_github_p4lang_p4_constraints",
            urls = ["https://github.com/p4lang/p4-constraints/archive/refs/tags/v1.0.0.tar.gz"],
            strip_prefix = "p4-constraints-1.0.0",
            sha256 = "1693e65336059e760a6179b80850560a63df347e08d559a8d7be4cd579ccbebe",
        )
    if not native.existing_rule("com_github_nlohmann_json"):
        http_archive(
            name = "com_github_nlohmann_json",
            # JSON for Modern C++
            url = "https://github.com/nlohmann/json/archive/v3.7.3.zip",
            strip_prefix = "json-3.7.3",
            sha256 = "e109cd4a9d1d463a62f0a81d7c6719ecd780a52fb80a22b901ed5b6fe43fb45b",
            build_file_content = """cc_library(name="json",
                                               visibility=["//visibility:public"],
                                               hdrs=["single_include/nlohmann/json.hpp"]
                                              )""",
        )
    if not native.existing_rule("com_jsoncpp"):
        http_archive(
            name = "com_jsoncpp",
            url = "https://github.com/open-source-parsers/jsoncpp/archive/1.9.4.zip",
            strip_prefix = "jsoncpp-1.9.4",
            build_file = "@//:bazel/BUILD.jsoncpp.bazel",
            sha256 = "6da6cdc026fe042599d9fce7b06ff2c128e8dd6b8b751fca91eb022bce310880",
        )
    if not native.existing_rule("com_github_ivmai_cudd"):
        http_archive(
            name = "com_github_ivmai_cudd",
            build_file = "@//:bazel/BUILD.cudd.bazel",
            strip_prefix = "cudd-cudd-3.0.0",
            sha256 = "5fe145041c594689e6e7cf4cd623d5f2b7c36261708be8c9a72aed72cf67acce",
            urls = ["https://github.com/ivmai/cudd/archive/cudd-3.0.0.tar.gz"],
        )
    if not native.existing_rule("com_gnu_gmp"):
        http_archive(
            name = "com_gnu_gmp",
            url = "https://gmplib.org/download/gmp/gmp-6.1.2.tar.xz",
            strip_prefix = "gmp-6.1.2",
            sha256 = "87b565e89a9a684fe4ebeeddb8399dce2599f9c9049854ca8c0dfbdea0e21912",
            build_file = "@//:bazel/BUILD.gmp.bazel",
        )
    if not native.existing_rule("com_github_z3prover_z3"):
        http_archive(
            name = "com_github_z3prover_z3",
            url = "https://github.com/Z3Prover/z3/archive/z3-4.8.7.tar.gz",
            strip_prefix = "z3-z3-4.8.7",
            sha256 = "8c1c49a1eccf5d8b952dadadba3552b0eac67482b8a29eaad62aa7343a0732c3",
            build_file = "@//:bazel/BUILD.z3.bazel",
        )
    if not native.existing_rule("rules_foreign_cc"):
        http_archive(
            name = "rules_foreign_cc",
            sha256 = "d54742ffbdc6924f222d2179f0e10e911c5c659c4ae74158e9fe827aad862ac6",
            strip_prefix = "rules_foreign_cc-0.2.0",
            url = "https://github.com/bazelbuild/rules_foreign_cc/archive/0.2.0.tar.gz",
        )
    if not native.existing_rule("rules_proto"):
        http_archive(
            name = "rules_proto",
            urls = [
                "https://mirror.bazel.build/github.com/bazelbuild/rules_proto/archive/97d8af4dc474595af3900dd85cb3a29ad28cc313.tar.gz",
                "https://github.com/bazelbuild/rules_proto/archive/97d8af4dc474595af3900dd85cb3a29ad28cc313.tar.gz",
            ],
            strip_prefix = "rules_proto-97d8af4dc474595af3900dd85cb3a29ad28cc313",
            sha256 = "602e7161d9195e50246177e7c55b2f39950a9cf7366f74ed5f22fd45750cd208",
        )
