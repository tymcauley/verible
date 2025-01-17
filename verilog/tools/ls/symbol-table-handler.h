// Copyright 2021 The Verible Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#ifndef VERILOG_TOOLS_LS_SYMBOL_TABLE_HANDLER_H
#define VERILOG_TOOLS_LS_SYMBOL_TABLE_HANDLER_H

#include <filesystem>
#include <memory>
#include <vector>

#include "absl/container/flat_hash_set.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "common/lsp/lsp-protocol.h"
#include "verilog/analysis/symbol_table.h"
#include "verilog/analysis/verilog_project.h"
#include "verilog/tools/ls/lsp-parse-buffer.h"

namespace verilog {

// Converts file:// scheme entries to actual system paths.
// If other scheme is provided, method returns empty string_view.
// TODO (glatosinski) current resolving of LSP URIs is very naive
// and supports only narrow use cases of file:// specifier.
absl::string_view LSPUriToPath(absl::string_view uri);

// Converts filesystem paths to file:// scheme entries.
std::string PathToLSPUri(absl::string_view path);

// Looks for FileList file for SymbolTableHandler
std::string FindFileList(absl::string_view current_dir);

// A class interfacing the SymbolTable with the LSP messages.
// It manages the SymbolTable and its necessary components,
// and provides such information as symbol definitions
// based on LSP requests.
// The provided information is in LSP-friendly format.
class SymbolTableHandler {
 public:
  SymbolTableHandler(){};

  // Sets the project for the symbol table.
  // VerilogProject requires root, include_paths and corpus to
  // create a base of files that may contain definitions for symbols.
  // Once the project's root is set, a new SymbolTable is created.
  void SetProject(const std::shared_ptr<VerilogProject> &project);

  // Returns the current project.
  std::shared_ptr<VerilogProject> mutable_project() { return curr_project_; }

  // Creates a new symbol table given the VerilogProject in setProject
  // method.
  void ResetSymbolTable();

  // Fills the symbol table for a given verilog source file.
  std::vector<absl::Status> BuildSymbolTableFor(const VerilogSourceFile &file);

  // Creates a symbol table for entire project
  std::vector<absl::Status> BuildProjectSymbolTable();

  // Finds the definition for a symbol provided in the DefinitionParams
  // message delivered i.e. in textDocument/definition message.
  // Provides a list of locations with symbol's definitions.
  std::vector<verible::lsp::Location> FindDefinition(
      const verible::lsp::DefinitionParams &params,
      const verilog::BufferTrackerContainer &parsed_buffers);

  void UpdateFileContent(absl::string_view path,
                         const verible::TextStructureView *content);

 private:
  // Scans the symbol table tree to find a given symbol.
  // returns pointer to table node with the symbol on success, else nullptr.
  const SymbolTableNode *ScanSymbolTreeForDefinition(
      const SymbolTableNode *context, absl::string_view symbol);

  // Looks for verible.filelist file down in directory structure and loads data
  // to project.
  // It is meant to be executed once per VerilogProject setup
  bool LoadProjectFileList(absl::string_view current_dir);

  // Path to the filelist file for the project
  std::string filelist_path_;

  // Last timestamp of filelist file - used to check whether SymbolTable should
  // be updated
  absl::optional<std::filesystem::file_time_type> last_filelist_update_;

  // tells that symbol table should be rebuilt due to changes in files
  bool files_dirty_ = true;

  // current VerilogProject for which the symbol table is created
  std::shared_ptr<VerilogProject> curr_project_;
  std::unique_ptr<SymbolTable> symbol_table_;
};

};  // namespace verilog

#endif  // VERILOG_TOOLS_LS_SYMBOL_TABLE_HANDLER_H
