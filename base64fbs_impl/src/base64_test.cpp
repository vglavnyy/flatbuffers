/*
Written in 2017-2018 by vglavnyy.

To the extent possible under law, the author(s) have dedicated all
copyright and related and neighboring rights to this software to the
public domain worldwide. This software is distributed without any warranty.

You should have received a copy of the CC0 Public Domain Dedication
along with this software. If not, see
http://creativecommons.org/publicdomain/zero/1.0/
*/

#include "base64fbs/util_base64.h"

//#define COMPILE_BASE64_BENCHMARK

#if defined(COMPILE_BASE64_BENCHMARK)
#include <chrono>
#include <iostream>
#include <vector>
#endif

namespace base64fbs {
  static int Base64PerformanceTest();

// Test of base64 compatibility.
static int Base64CompatibilityTest() {
  // Use internal test for coverage.
  struct _B64T {
    // emulation of flatbuffers::FlatBufferBuilder
    struct fbs_builder_emu {
      std::vector<uint8_t> v;
      flatbuffers::uoffset_t CreateUninitializedVector(size_t len,
                                                       size_t elemsize,
                                                       uint8_t **buf) {
        assert(len > 0);
        assert(elemsize == 1);
        (void)elemsize;
        v.clear();
        v.resize(len);
        *buf = v.data();
        return 0;
      }
    };

    static std::vector<uint8_t> Decode(const std::string &text,
                                       const kBase64Mode b64m,
                                       size_t *const err_pos = nullptr) {
      std::string attr;
      if (kBase64ModeStandard == b64m)
        attr = "base64";
      else if (kBase64ModeUrlSafe == b64m)
        attr = "base64url";

      if (attr.empty()) return std::vector<uint8_t>();

      if (err_pos) *err_pos = 0;
      flatbuffers::Type type(flatbuffers::BASE_TYPE_UCHAR);
      flatbuffers::FieldDef fd;
      fd.attributes.Add(attr, new flatbuffers::Value());
      flatbuffers::uoffset_t ovalue = 0;
      fbs_builder_emu out;

      const size_t rc = ParseBase64Vector(type, text, &fd, &ovalue, &out);
      if (err_pos) *err_pos = rc;
      return (rc == text.size()) ? out.v : std::vector<uint8_t>();
    }

    static std::string Encode(const std::vector<uint8_t> &binary,
                              const kBase64Mode b64m,
                              bool cancel_padding = false) {
      if (binary.empty()) return "";

      std::string attr;
      if (kBase64ModeStandard == b64m)
        attr = "base64";
      else if (kBase64ModeUrlSafe == b64m)
        attr = "base64url";
      if (attr.empty()) return "";

      std::string text;
      flatbuffers::Type type(flatbuffers::BASE_TYPE_UCHAR);
      flatbuffers::IDLOptions opts;
      opts.base64_cancel_padding = cancel_padding;
      flatbuffers::FieldDef fd;
      fd.attributes.Add(attr, new flatbuffers::Value());
      auto done = PrintBase64Vector(binary.data(), binary.size(), type, opts,
                                    &text, &fd);
      if (done) {
        // remove quotes form the text string for test
        text.erase(text.begin());  // open quote
        text.pop_back();           // close quote
      }
      return text;
    }

    static bool DecEncTest(const kBase64Mode dec_mode,
                           const kBase64Mode enc_mode, size_t *const err_pos,
                           const std::string &input,
                           const std::string &expected) {
      auto decoded = Decode(input, dec_mode, err_pos);
      auto encoded = Encode(decoded, enc_mode);
      return expected == encoded;
    }

    static bool EncDecTest(const kBase64Mode enc_mode,
                           const kBase64Mode dec_mode,
                           const std::vector<uint8_t> &input) {
      auto encoded = Encode(input, enc_mode);
      auto decoded = Decode(encoded, dec_mode);
      return input == decoded;
    }
  };

  const size_t B64ModeNum = 2;
  const kBase64Mode BaseSet[B64ModeNum] = { kBase64ModeStandard,
                                            kBase64ModeUrlSafe };
  // clang-format off
    // The padding is optional both for Standard and UrlSafe decoders.
    // Standard decoder can't decode a UrlSafe string.
    const bool expects[B64ModeNum * B64ModeNum] = {
      true,  true, // encoder: Standard
      false, true, // encoder: UrlSafe
    //  |     |_______decoder: UrlSafe
    //  |_____________decoder: Standard
    };
  // clang-format on

  // Auto-generated encode-decode loop:
  for (size_t enc_index = 0; enc_index < B64ModeNum; enc_index++) {
    const auto enc_mode = BaseSet[enc_index];
    for (size_t dec_index = 0; dec_index < B64ModeNum; dec_index++) {
      const auto match_expects = expects[enc_index * B64ModeNum + dec_index];
      const auto dec_mode = BaseSet[dec_index];
      const size_t B = 256;
      const auto N = 2 * B + 1;
      size_t k = 0;
      for (k = 0; k < N; k++) {
        const auto M = (k % (B - 1));
        // Generate a binary data.
        std::vector<uint8_t> bin(M);
        for (size_t j = 0; j < M; j++) {
          bin[j] = static_cast<uint8_t>(((k + j) % B) % 0x100);
        }
        // Check: Binary->Encode->Decode->Compare.
        if (false == _B64T::EncDecTest(enc_mode, dec_mode, bin)) break;
      }
      if ((N == k) != match_expects) return __LINE__;
    }
  }

  // Test of error position detector.
  size_t epos = 0;
  for (size_t mode_ind = 0; mode_ind < B64ModeNum; mode_ind++) {
    const auto mode = BaseSet[mode_ind];
    const std::string ref = "IARXA18BIg==";
    for (size_t k = 0; k < ref.size(); k++) {
      auto t = ref;
      t[k] = '#';
      if (!_B64T::DecEncTest(mode, mode, &epos, t, "")) return __LINE__;
      // the position of error is in half-open range [epos, ref.size())
      if (!(epos <= k && epos < ref.size())) return __LINE__;
    }
  }

  // Input is a standard RFC4648 sequence.
  if (!_B64T::DecEncTest(kBase64ModeStandard, kBase64ModeStandard, &epos,
                         "/43+AergFA==", "/43+AergFA==") ||
      (epos != 12))
    return __LINE__;

  // Input is RFC4648 sequence without padding.
  if (!_B64T::DecEncTest(kBase64ModeStandard, kBase64ModeStandard, &epos,
                         "/43+AergFA", "/43+AergFA==") ||
      (epos != 10))
    return __LINE__;

  // Test with the standard decoder and url-safe encoder.
  if (!_B64T::DecEncTest(kBase64ModeStandard, kBase64ModeUrlSafe, &epos,
                         "/43+AergFA==", "_43-AergFA==") ||
      (epos != 12))
    return __LINE__;

  // Test with url-safe decoder and encoder.
  if (!_B64T::DecEncTest(kBase64ModeUrlSafe, kBase64ModeUrlSafe, &epos,
                         "_43-AergFA==", "_43-AergFA==") ||
      (epos != 12))
    return __LINE__;

  // Test using base64 string without padding symbols.
  if (!_B64T::DecEncTest(kBase64ModeUrlSafe, kBase64ModeUrlSafe, &epos,
                         "_43-AergFA", "_43-AergFA==") ||
      (epos != 10))
    return __LINE__;
  // done
  return 0;
}

int JsonBase64Test() {
  if (base64fbs::Base64CompatibilityTest() != 0) return __LINE__;

  // clang-format off
    const auto base64_schema =
      "table TestBase64 {"
      "data:[ubyte](base64);"
      "urldata:[ubyte](base64url);"
      "}\n"
      "root_type TestBase64;"
      "file_identifier \"B64T\";";
    const auto test_array = "{"
      "data: [247, 208, 63, 251, 129, 52, 179, 220, 24, 249, 42],"
      "urldata: [247, 208, 63, 251, 129, 52, 179, 220, 24, 249, 42]"
      "}";
    // Expected result, with mandatory padding after encoding.
    const auto expected_b64 = "{"
      "data: \"99A/+4E0s9wY+So=\","
      "urldata: \"99A_-4E0s9wY-So=\""
      "}";
    // urldata without padding.
    const auto test_base64 = "{"
      "data: \"99A/+4E0s9wY+So=\","
      "urldata: \"99A_-4E0s9wY-So\""
      "}";
  // clang-format on

  flatbuffers::Parser parser;

  // Use compact form of json.
  parser.opts.indent_step = -1;
  if (parser.Parse(base64_schema) != true) return __LINE__;

  // test_array
  if (parser.Parse(test_array) != true) return __LINE__;
  std::string test_array_b64;
  flatbuffers::GenerateText(parser, parser.builder_.GetBufferPointer(),
                            &test_array_b64);
  if (test_array_b64 != expected_b64) return __LINE__;

  // test_base64
  if (parser.Parse(test_base64) != true) return __LINE__;
  std::string test_base64_b64;
  flatbuffers::GenerateText(parser, parser.builder_.GetBufferPointer(),
                            &test_base64_b64);
  if (test_base64_b64 != expected_b64) return __LINE__;

  // Check that both strings used in test.
  if (test_base64_b64 != test_array_b64) return __LINE__;

  // Cancel padding base64url encoder and check.
  parser.opts.base64_cancel_padding = true;
  std::string wopad_base64_b64;
  flatbuffers::GenerateText(parser, parser.builder_.GetBufferPointer(),
                            &wopad_base64_b64);
  if (wopad_base64_b64 != test_base64) return __LINE__;

  #if defined(COMPILE_BASE64_BENCHMARK)
  if (Base64PerformanceTest()) return __LINE__;
  #endif

  return 0;
}

#if defined(COMPILE_BASE64_BENCHMARK)
static int Base64PerformanceTest() {
  const size_t Nsamp = 4 * 1024 * 1024;
  const size_t Mrep = 128;

  std::vector<uint8_t> bin(Nsamp);
  for (size_t k = 0; k < bin.size(); k++) { bin[k] = (k + 7) % 0x100; }

  const auto b64mode = kBase64ModeStandard;
  // preallocate
  const auto enc_size = (bin.size() / 3) * 4 + 4;
  std::vector<char> enc(enc_size);

  // start encode test
  auto enc_start = std::chrono::steady_clock::now();
  for (size_t rep_cnt = 0; rep_cnt < Mrep; rep_cnt++) {
    size_t enc_rc = base64fbs_encode(b64mode, bin.data(), bin.size(),
                                     enc.data(), enc.size());
    if (enc_rc > enc.size()) return -1;
    enc.resize(enc_rc);
  }
  auto enc_end = std::chrono::steady_clock::now();
  auto enc_time =
      std::chrono::duration_cast<std::chrono::nanoseconds>(enc_end - enc_start)
          .count();

  // start decode test
  auto dec_size = enc.size();
  auto ri = enc.rbegin();
  for (auto k = 2; ri != enc.rend() && k > 0; --k, ++ri) {
    dec_size -= (*ri == '=') ? 1 : 0;
  }
  // encoder transforms 4 symbols to 3 bytes
  dec_size = (dec_size * 3) / 4;
  if (dec_size != Nsamp) return -1;
  std::vector<uint8_t> dec(dec_size);

  auto dec_start = std::chrono::steady_clock::now();
  for (size_t rep_cnt = 0; rep_cnt < Mrep; rep_cnt++) {
    size_t dec_rc = base64fbs_decode(b64mode, enc.data(), enc.size(),
                                     dec.data(), dec.size());
    if (dec_rc != dec_size) return -1;
  }
  auto dec_end = std::chrono::steady_clock::now();
  auto dec_time =
      std::chrono::duration_cast<std::chrono::nanoseconds>(dec_end - dec_start)
          .count();

  auto cpy = dec;
  auto cpy_start = std::chrono::steady_clock::now();
  for (size_t rep_cnt = 0; rep_cnt < Mrep; rep_cnt++) {
    memcpy(&cpy[0], &dec[0], dec.size());
  }
  auto cpy_end = std::chrono::steady_clock::now();
  auto cpy_time =
      std::chrono::duration_cast<std::chrono::nanoseconds>(cpy_end - cpy_start)
          .count();

  auto cpy_bw = (double)(Mrep * Nsamp) / (1024 * 1024) / cpy_time * 1e9;
  auto dec_bw = (double)(Mrep * Nsamp) / (1024 * 1024) / dec_time * 1e9;
  auto enc_bw = (double)(Mrep * Nsamp) / (1024 * 1024) / enc_time * 1e9;
  std::cout << "memcpy performance [MB/s]: " << cpy_bw << std::endl;
  std::cout << "Decode performance [MB/s]: " << dec_bw << std::endl;
  std::cout << "Encode performance [MB/s]: " << enc_bw << std::endl;
  return 0;
}
#endif

}  // namespace base64fbs
