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

#ifdef _MSC_VER
#  include <crtdbg.h>
#endif

#include "flatbuffers/base.h"
#include "flatbuffers/util.h"

namespace flatbuffers {

// We internally store paths in posix format ('/'). Paths supplied
// by the user should go through PosixPath to ensure correct behavior
// on Windows when paths are string-compared.

static const char kPathSeparatorWindows = '\\';
static const char *PathSeparatorSet = "\\/";  // Intentionally no ':'

std::string StripExtension(const std::string &filepath) {
  size_t i = filepath.find_last_of('.');
  return i != std::string::npos ? filepath.substr(0, i) : filepath;
}

std::string GetExtension(const std::string &filepath) {
  size_t i = filepath.find_last_of('.');
  return i != std::string::npos ? filepath.substr(i + 1) : "";
}

std::string StripPath(const std::string &filepath) {
  size_t i = filepath.find_last_of(PathSeparatorSet);
  return i != std::string::npos ? filepath.substr(i + 1) : filepath;
}

std::string StripFileName(const std::string &filepath) {
  size_t i = filepath.find_last_of(PathSeparatorSet);
  return i != std::string::npos ? filepath.substr(0, i) : "";
}

std::string ConCatPathFileName(const std::string &path,
                               const std::string &filename) {
  std::string filepath = path;
  if (filepath.length()) {
    char &filepath_last_character = string_back(filepath);
    if (filepath_last_character == kPathSeparatorWindows) {
      filepath_last_character = kPathSeparator;
    } else if (filepath_last_character != kPathSeparator) {
      filepath += kPathSeparator;
    }
  }
  filepath += filename;
  // Ignore './' at the start of filepath.
  if (filepath[0] == '.' && filepath[1] == kPathSeparator) {
    filepath.erase(0, 2);
  }
  return filepath;
}

std::string PosixPath(const char *path) {
  std::string p = path;
  std::replace(p.begin(), p.end(), '\\', '/');
  return p;
}

void SetupDefaultCRTReportMode() {
  // clang-format off

  #ifdef _MSC_VER
    // By default, send all reports to STDOUT to prevent CI hangs.
    // Enable assert report box [Abort|Retry|Ignore] if a debugger is present.
    const int dbg_mode = (_CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG) |
                         (IsDebuggerPresent() ? _CRTDBG_MODE_WNDW : 0);
    (void)dbg_mode; // release mode fix
    // CrtDebug reports to _CRT_WARN channel.
    _CrtSetReportMode(_CRT_WARN, dbg_mode);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
    // The assert from <assert.h> reports to _CRT_ERROR channel
    _CrtSetReportMode(_CRT_ERROR, dbg_mode);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
    // Internal CRT assert channel?
    _CrtSetReportMode(_CRT_ASSERT, dbg_mode);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);
  #endif

  // clang-format on
}

// Use locale independent functions {strtod_l, strtof_l, strtoll_l, strtoull_l}.
#if defined(FLATBUFFERS_LOCALE_INDEPENDENT) && \
    (FLATBUFFERS_LOCALE_INDEPENDENT > 0)

// clang-format off
// Allocate locale instance at startup of application.
ClassicLocale ClassicLocale::instance_;

#ifdef _MSC_VER
  ClassicLocale::ClassicLocale()
    : locale_(_create_locale(LC_ALL, "C")) {}
  ClassicLocale::~ClassicLocale() { _free_locale(locale_); }
#else
  ClassicLocale::ClassicLocale()
    : locale_(newlocale(LC_ALL, "C", nullptr)) {}
  ClassicLocale::~ClassicLocale() { freelocale(locale_); }
#endif
// clang-format on

#endif  // !FLATBUFFERS_LOCALE_INDEPENDENT

}  // namespace flatbuffers
