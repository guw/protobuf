/*
Analyzes the contents of the files named on the command line and prints out
the function signatures of the functions of interest, which are the subset of
functions defined in ruby-upb.h that are called directly from Ruby via FFI.

NOTE: Assumes to run on ruby-upb.h without a preprocessor, which means that
attributes like UPB_API are not expanded, so this cannot guard against
functions that are not exported. This is assumed to be a non-issue since
unexported functions won't be callable and thus unit test should fail.
TODO: Align output format with attach_function_finder.rb

Example usage:

`bazel run //third_party/protobuf/github/ruby/lib/google:ruby_ffi_bindings -- \
  $PWD/third_party/protobuf/github/ruby/ext/google/protobuf_c/shared_convert.h
  $PWD/third_party/protobuf/github/ruby/ext/google/protobuf_c/glue.c`

Will emit the following output to stdout:
```
upb_JsonEncode [const upb_Message *, const upb_MessageDef *, const upb_DefPool
*, int, char *, int, upb_Status *] int Arena_create [] upb_Arena *
FileDescriptorProto_parse [const char *, int, upb_Arena *]
google_protobuf_FileDescriptorProto * EnumDescriptor_serialized_options [const
upb_EnumDef *, int *, upb_Arena *] char * FileDescriptor_serialized_options
[const upb_FileDef *, int *, upb_Arena *] char * Descriptor_serialized_options
[const upb_MessageDef *, int *, upb_Arena *] char *
OneOfDescriptor_serialized_options [const upb_OneofDef *, int *, upb_Arena *]
char * FieldDescriptor_serialized_options [const upb_FieldDef *, int *,
upb_Arena *] char * ServiceDescriptor_serialized_options [const upb_ServiceDef
*, int *, upb_Arena *] char * MethodDescriptor_serialized_options [const
upb_MethodDef *, int *, upb_Arena *] char *
[...]
```

Will also emit the following output to stderr that can be ignored (see TODO):
```
In file included from shared_convert.h:15:
./ruby-upb.h:44:10: fatal error: 'stdbool.h' file not found
   44 | #include <stdbool.h>
      |          ^~~~~~~~~~~
1 error generated.
In file included from glue.c:12:
./ruby-upb.h:44:10: fatal error: 'stdbool.h' file not found
   44 | #include <stdbool.h>
      |          ^~~~~~~~~~~
1 error generated.
```

# Intended to be used in conjunction with the output of
# `ruby/attach_function_finder.rb`. When working, the outputs should match.

*/

#include <algorithm>
#include <fstream>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#ifndef PROTO2_OPENSOURCE
#include "base/init_google.h"
#endif  // !PROTO2_OPENSOURCE
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"
#include "file/base/path.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"
#include "third_party/absl/log/absl_log.h"
#include "third_party/absl/status/status.h"
#include "third_party/absl/status/statusor.h"
#include "third_party/absl/strings/str_cat.h"
#include "third_party/absl/strings/str_join.h"
#include "third_party/llvm/llvm-project/clang/include/clang/Serialization/PCHContainerOperations.h"

struct RubyFFI {
  std::set<std::string> FunctionsOfInterest() const {
    return {
        // go/keep-sorted start
        "Arena_create",
        "Descriptor_serialized_options",
        "Descriptor_serialized_to_proto",
        "EnumDescriptor_serialized_options",
        "EnumDescriptor_serialized_to_proto",
        "FieldDescriptor_serialized_options",
        "FieldDescriptor_serialized_to_proto",
        "FileDescriptorProto_parse",
        "FileDescriptor_serialized_options",
        "FileDescriptor_serialized_to_proto",
        "MethodDescriptor_serialized_options",
        "MethodDescriptor_serialized_to_proto",
        "OneOfDescriptor_serialized_options",
        "OneOfDescriptor_serialized_to_proto",
        "ServiceDescriptor_serialized_options",
        "ServiceDescriptor_serialized_to_proto",
        "shared_Msgval_GetHash",
        "shared_Msgval_IsEqual",
        "upb_Arena_Free",
        "upb_Arena_Fuse",
        "upb_Arena_Malloc",
        "upb_Array_Append",
        "upb_Array_Freeze",
        "upb_Array_Get",
        "upb_Array_IsFrozen",
        "upb_Array_New",
        "upb_Array_Resize",
        "upb_Array_Set",
        "upb_Array_Size",
        "upb_Decode",
        "upb_DefPool_AddFile",
        "upb_DefPool_ExtensionRegistry",
        "upb_DefPool_FindEnumByName",
        "upb_DefPool_FindExtensionByName",
        "upb_DefPool_FindFileByName",
        "upb_DefPool_FindMessageByName",
        "upb_DefPool_FindServiceByName",
        "upb_DefPool_Free",
        "upb_DefPool_New",
        "upb_Encode",
        "upb_EnumDef_File",
        "upb_EnumDef_FindValueByNameWithSize",
        "upb_EnumDef_FindValueByNumber",
        "upb_EnumDef_FullName",
        "upb_EnumDef_Value",
        "upb_EnumDef_ValueCount",
        "upb_EnumValueDef_Name",
        "upb_EnumValueDef_Number",
        "upb_FieldDef_CType",
        "upb_FieldDef_ContainingType",
        "upb_FieldDef_Default",
        "upb_FieldDef_EnumSubDef",
        "upb_FieldDef_File",
        "upb_FieldDef_HasPresence",
        "upb_FieldDef_IsMap",
        "upb_FieldDef_IsPacked",
        "upb_FieldDef_IsRepeated",
        "upb_FieldDef_IsSubMessage",
        "upb_FieldDef_JsonName",
        "upb_FieldDef_Label",
        "upb_FieldDef_MessageSubDef",
        "upb_FieldDef_Name",
        "upb_FieldDef_Number",
        "upb_FieldDef_RealContainingOneof",
        "upb_FieldDef_Type",
        "upb_FileDef_Name",
        "upb_FileDef_Pool",
        "upb_JsonDecodeDetectingNonconformance",
        "upb_JsonEncode",
        "upb_MapIterator_Done",
        "upb_MapIterator_Key",
        "upb_MapIterator_Next",
        "upb_MapIterator_Value",
        "upb_Map_Clear",
        "upb_Map_Delete",
        "upb_Map_Freeze",
        "upb_Map_Get",
        "upb_Map_IsFrozen",
        "upb_Map_New",
        "upb_Map_Set",
        "upb_Map_Size",
        "upb_MessageDef_Field",
        "upb_MessageDef_FieldCount",
        "upb_MessageDef_File",
        "upb_MessageDef_FindByNameWithSize",
        "upb_MessageDef_FindFieldByNameWithSize",
        "upb_MessageDef_FindFieldByNumber",
        "upb_MessageDef_FindOneofByNameWithSize",
        "upb_MessageDef_FullName",
        "upb_MessageDef_MiniTable",
        "upb_MessageDef_Oneof",
        "upb_MessageDef_OneofCount",
        "upb_MessageDef_WellKnownType",
        "upb_Message_ClearFieldByDef",
        "upb_Message_DiscardUnknown",
        "upb_Message_Freeze",
        "upb_Message_GetFieldByDef",
        "upb_Message_HasFieldByDef",
        "upb_Message_IsFrozen",
        "upb_Message_Mutable",
        "upb_Message_New",
        "upb_Message_Next",
        "upb_Message_SetFieldByDef",
        "upb_Message_WhichOneofByDef",
        "upb_MethodDef_ClientStreaming",
        "upb_MethodDef_InputType",
        "upb_MethodDef_Name",
        "upb_MethodDef_OutputType",
        "upb_MethodDef_ServerStreaming",
        "upb_MethodDef_Service",
        "upb_OneofDef_ContainingType",
        "upb_OneofDef_Field",
        "upb_OneofDef_FieldCount",
        "upb_OneofDef_Name",
        "upb_ServiceDef_File",
        "upb_ServiceDef_FullName",
        "upb_ServiceDef_Method",
        "upb_ServiceDef_MethodCount",
        "upb_Status_Clear",
        "upb_Status_ErrorMessage",
        // go/keep-sorted end
    };
  }
};

absl::StatusOr<std::string> readFileIntoString(const std::string &filename) {
  std::ifstream file(filename);
  if (file.is_open()) {
    std::string contents((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    file.close();
    return contents;
  } else {
    return absl::UnavailableError(
        absl::StrCat("Failed to open file ", filename));
  }
}

class FindNamedClassVisitor
    : public clang::RecursiveASTVisitor<FindNamedClassVisitor> {
 public:
  explicit FindNamedClassVisitor(
      clang::ASTContext *Context,
      std::map<std::string, std::string> &found_functions)
      : context_(Context), found_functions_(found_functions) {}

  bool VisitFunctionDecl(clang::FunctionDecl *Declaration) {
    clang::FullSourceLoc FullLocation =
        context_->getFullLoc(Declaration->getBeginLoc());
    if (FullLocation.isValid()) {
      if (functions_of_interest_.contains(Declaration->getNameAsString())) {
        std::vector<std::string> param_types(Declaration->getNumParams());
        for (uint i = 0; i < Declaration->getNumParams(); ++i) {
          param_types[i] =
              Declaration->getParamDecl(i)->getType().getAsString();
        }
        found_functions_[Declaration->getNameAsString()] =
            absl::StrCat(Declaration->getNameAsString(), " [",
                         absl::StrJoin(param_types, ", "), "] ",
                         Declaration->getReturnType().getAsString());
      }
    }
    return true;
  }

 private:
  clang::ASTContext *context_;
  std::map<std::string, std::string> &found_functions_;
  std::set<std::string> functions_of_interest_ =
      RubyFFI().FunctionsOfInterest();
};

class FindNamedClassConsumer : public clang::ASTConsumer {
 public:
  explicit FindNamedClassConsumer(
      clang::ASTContext *Context,
      std::map<std::string, std::string> &found_functions)
      : visitor_(Context, found_functions) {}

  void HandleTranslationUnit(clang::ASTContext &Context) override {
    visitor_.TraverseDecl(Context.getTranslationUnitDecl());
  }

 private:
  FindNamedClassVisitor visitor_;
};

class FindNamedClassAction : public clang::ASTFrontendAction {
 public:
  explicit FindNamedClassAction(
      std::map<std::string, std::string> &found_functions)
      : found_functions_(found_functions) {}
  bool PrepareToExecuteAction(clang::CompilerInstance &CI) override {
    // Enable language features to prevent spurious warnings about undefined
    // types e.g. `bool` and `wchar_t`.
    CI.getLangOpts().Bool = 1;
    CI.getLangOpts().WChar = 1;
    CI.getLangOpts().GNUMode = 1;
    CI.getLangOpts().CPlusPlus17 = 1;
    return true;
  }
  std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
      clang::CompilerInstance &Compiler, llvm::StringRef InFile) override {
    // Suppress include not found errors e.g. for <stdbool.h>
    Compiler.getPreprocessor().SetSuppressIncludeNotFoundError(true);
    return std::make_unique<FindNamedClassConsumer>(&Compiler.getASTContext(),
                                                    found_functions_);
  }

 private:
  std::map<std::string, std::string> &found_functions_;
};

std::string Usage(char **argv) {
  return absl::StrCat("Usage: ", file::Basename(argv[0]),
                      " /path/to/ruby-upb.h <file1> [file2] ...");
}

int main(int argc, char **argv) {
#ifndef PROTO2_OPENSOURCE
  InitGoogle(argv[0], &argc, &argv, true);
#endif  // !PROTO2_OPENSOURCE
  if (argc <= 2) {
    ABSL_LOG(FATAL) << "Insufficient arguments. " << Usage(argv);
  }
  if (!std::string(argv[1]).ends_with("/ruby-upb.h")) {
    ABSL_LOG(FATAL) << "First argument must be path to ruby-upb.h. "
                    << Usage(argv);
  }

  absl::StatusOr<std::string> contents = readFileIntoString(argv[1]);
  if (!contents.ok()) {
    ABSL_LOG(FATAL) << contents.status();
  }
  std::string ruby_upb_h =
      // Add #define and typedef statements to prevent warnings about undefined
      // types and functions e.g. `size_t` and `va_start`.
      absl::StrCat("typedef unsigned long size_t;\n",
                   "typedef __builtin_va_list __gnuc_va_list;\n",
                   "typedef signed long ptrdiff_t;\n",
                   "#define va_start(foo, bar) /* ignored */\n",
                   "#define va_end(foo) /* ignored */\n", "\n#define NULL 0u\n",
                   contents.value());
  std::map<std::string, std::string> found_functions;

  for (int i = 2; i < argc; ++i) {
    contents = readFileIntoString(argv[i]);
    if (!contents.ok()) {
      ABSL_LOG(FATAL) << contents.status();
    }
    clang::tooling::runToolOnCodeWithArgs(
        std::make_unique<FindNamedClassAction>(found_functions),
        contents.value(), std::vector<std::string>(),
        file::Basename(argv[i]).data(), "clang-tool",
        std::make_shared<clang::PCHContainerOperations>(),
        {
            std::make_pair("ruby-upb.h", ruby_upb_h),
        });
  }

  std::set<std::string> difference;
  std::set<std::string> functions_of_interest = RubyFFI().FunctionsOfInterest();
  std::set<std::string> found_functions_keys;
  for (const auto &element : found_functions) {
    found_functions_keys.insert(element.first);
  }
  std::set_difference(functions_of_interest.begin(),
                      functions_of_interest.end(), found_functions_keys.begin(),
                      found_functions_keys.end(),
                      std::inserter(difference, difference.begin()));
  if (!difference.empty()) {
    ABSL_LOG(FATAL) << "Found " << found_functions.size() << " of the expected "
                    << functions_of_interest.size()
                    << " functions of interest; missing: \n"
                    << absl::StrJoin(difference, "\n");
  }
  for (const auto &pair : found_functions) {
    llvm::outs() << pair.second << "\n";
  }
}
