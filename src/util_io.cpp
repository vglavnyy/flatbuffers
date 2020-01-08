/*
 * Copyright 2016 Google Inc. All rights reserved.
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

// This file contains implementation of platform-specific routines like file-io.

// clang-format off
// Dont't remove `format off`, it prevent reordering of win-includes.
#define _POSIX_C_SOURCE 200112L // For stat from stat/stat.h and fseeko() (POSIX extensions).
#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  ifdef _MSC_VER
#    include <crtdbg.h>
#  endif
#  include <windows.h>  // Must be included before <direct.h>
#  include <direct.h>
#  include <winbase.h>
#  undef interface  // This is also important because of reasons
#else
#  define _XOPEN_SOURCE 600 // For PATH_MAX from limits.h (SUSv2 extension) 
#  include <limits.h>
#endif
// clang-format on

#include "flatbuffers/base.h"
#include "flatbuffers/util.h"

#include <fstream>
#include <sstream>
#include <sys/stat.h>

namespace flatbuffers {

// Platform dependent code.

static bool FileExistsRaw(const char *name) {
  std::ifstream ifs(name);
  return ifs.good();
}

static bool LoadFileRaw(const char *name, bool binary, std::string *buf) {
  if (DirExists(name)) return false;
  std::ifstream ifs(name, binary ? std::ifstream::binary : std::ifstream::in);
  if (!ifs.is_open()) return false;
  if (binary) {
    // The fastest way to read a file into a string.
    ifs.seekg(0, std::ios::end);
    auto size = ifs.tellg();
    (*buf).resize(static_cast<size_t>(size));
    ifs.seekg(0, std::ios::beg);
    ifs.read(&(*buf)[0], (*buf).size());
  } else {
    // This is slower, but works correctly on all platforms for text files.
    std::ostringstream oss;
    oss << ifs.rdbuf();
    *buf = oss.str();
  }
  return !ifs.bad();
}

static LoadFileFunction g_load_file_function = LoadFileRaw;
static FileExistsFunction g_file_exists_function = FileExistsRaw;

bool LoadFile(const char *name, bool binary, std::string *buf) {
  FLATBUFFERS_ASSERT(g_load_file_function);
  return g_load_file_function(name, binary, buf);
}

bool FileExists(const char *name) {
  FLATBUFFERS_ASSERT(g_file_exists_function);
  return g_file_exists_function(name);
}

bool DirExists(const char *name) {
  // clang-format off

  #ifdef _WIN32
    #define flatbuffers_stat _stat
    #define FLATBUFFERS_S_IFDIR _S_IFDIR
  #else
    #define flatbuffers_stat stat
    #define FLATBUFFERS_S_IFDIR S_IFDIR
  #endif
  // clang-format on
  struct flatbuffers_stat file_info;
  if (flatbuffers_stat(name, &file_info) != 0) return false;
  return (file_info.st_mode & FLATBUFFERS_S_IFDIR) != 0;
}

LoadFileFunction SetLoadFileFunction(LoadFileFunction load_file_function) {
  LoadFileFunction previous_function = g_load_file_function;
  g_load_file_function = load_file_function ? load_file_function : LoadFileRaw;
  return previous_function;
}

FileExistsFunction SetFileExistsFunction(
    FileExistsFunction file_exists_function) {
  FileExistsFunction previous_function = g_file_exists_function;
  g_file_exists_function =
      file_exists_function ? file_exists_function : FileExistsRaw;
  return previous_function;
}

bool SaveFile(const char *name, const char *buf, size_t len, bool binary) {
  std::ofstream ofs(name, binary ? std::ofstream::binary : std::ofstream::out);
  if (!ofs.is_open()) return false;
  ofs.write(buf, len);
  return !ofs.bad();
}

bool SaveFile(const char *name, const std::string &buf, bool binary) {
  return SaveFile(name, buf.c_str(), buf.size(), binary);
}

void EnsureDirExists(const std::string &filepath) {
  // clang-format off
  auto parent = StripFileName(filepath);
  if (parent.length()) EnsureDirExists(parent);
  #ifdef _WIN32
    (void)_mkdir(filepath.c_str());
  #else
    mkdir(filepath.c_str(), S_IRWXU|S_IRGRP|S_IXGRP);
  #endif
  // clang-format on
}

std::string AbsolutePath(const std::string &filepath) {
  // clang-format off
  if (filepath.empty()) return "";

  #ifdef FLATBUFFERS_NO_ABSOLUTE_PATH_RESOLUTION
    return filepath;
  #else
    #ifdef _WIN32
      char abs_path[MAX_PATH];
      return GetFullPathNameA(filepath.c_str(), MAX_PATH, abs_path, nullptr)
    #else
      char abs_path[PATH_MAX];
      return realpath(filepath.c_str(), abs_path)
    #endif
      ? abs_path
      : filepath;
  #endif // FLATBUFFERS_NO_ABSOLUTE_PATH_RESOLUTION
  // clang-format on
}

}  // namespace flatbuffers
