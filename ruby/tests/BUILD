load(
    "//upb/cmake:build_defs.bzl",
    "staleness_test",
)

package(default_applicable_licenses = ["//:license"])

genrule(
    name = "ruby_ffi_bindings_golden",
    testonly = True,
    srcs = [
        "//upb:ruby-upb.h",
        "///ruby:ext/google/protobuf_c/shared_convert.h",
        "///ruby:ext/google/protobuf_c/glue.c",
    ],
    outs = ["generated/golden-ruby_ffi_bindings.txt"],
    cmd = """
      $(location :ruby_ffi_bindings) \
      $(location //upb:ruby-upb.h) \
      $(location ///ruby:ext/google/protobuf_c/shared_convert.h) \
      $(location ///ruby:ext/google/protobuf_c/glue.c) \
       > $@
    """,
    tools = [":ruby_ffi_bindings"],
)

staleness_test(
    name = "ruby_ffi_bindings-staleness_test",
    outs = ["golden-ruby_ffi_bindings.txt"],
    generated_pattern = "generated/%s",
    target_files = ["golden-ruby_ffi_bindings.txt"],
)

cc_binary(
    name = "ruby_ffi_bindings",
    testonly = True,
    srcs = ["ruby-ffi-bindings.cpp"],
    visibility = ["//src/google/protobuf:__subpackages__"],
    deps = [
        "//base",
        "//file/base:path",
        "//third_party/llvm/llvm-project/clang:ast",
        "//third_party/llvm/llvm-project/clang:basic",
        "//third_party/llvm/llvm-project/clang:frontend",
        "//third_party/llvm/llvm-project/clang:serialization",
        "//third_party/llvm/llvm-project/clang:tooling",
        "//third_party/llvm/llvm-project/llvm:Support",
        "@abseil-cpp//absl/log:absl_log",
        "@abseil-cpp//absl/log:check",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
    ],
)
