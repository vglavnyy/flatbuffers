/*
 * Copyright 2014 Google Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "flatbuffers/idl.h"
#include "monster_generated.h"  // Already includes "flatbuffers/flatbuffers.h".
// namespace embedded_data {
// #include "monster_bfbs.opendb"        // Embedded bfbs-schema.
// #include "monsterdata_json_wire.txt"  // json-message
// }  // namespace embedded_data

int sample() {
  // // Inizialize parser by deserializing bfbs schema.
  // flatbuffers::Parser parser;
  // if (false == parser.Deserialize(embedded_data::monster_bfbs,
  //                                 embedded_data::monster_bfbs_len))
  //   return -1;

  // if (false == parser.Parse(embedded_data::monsterdata_json_wire, nullptr))
  //   return -2;

  return 0;
}

int main(int /*argc*/, const char * /*argv*/ []) {
  auto ret = sample();
  printf("sample_compact finished with code %d.\n", ret);
  return ret;
}

namespace flatbuffers {

// Simplest stubs of the platform-dependent code.
// For details see util.h and util_io.cpp.
// Stubs aren't inline to prevent mixing of util.cpp and this file.
// This file should be included only once to make linker happy.

LoadFileFunction SetLoadFileFunction(LoadFileFunction) { return nullptr; }
FileExistsFunction SetFileExistsFunction(FileExistsFunction) { return nullptr; }
bool FileExists(const char *) { return false; }
bool LoadFile(const char *, bool, std::string *) { return false; }
bool SaveFile(const char *, const char *, size_t, bool) { return false; }
bool SaveFile(const char *, const std::string &, bool) { return false; }
bool DirExists(const char *) { return false; }
void EnsureDirExists(const std::string &) {}
std::string AbsolutePath(const std::string &filepath) { return "stub"; }
}  // namespace flatbuffers
