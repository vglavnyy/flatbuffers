/*
Written in 2017-2018 by vglavnyy.

To the extent possible under law, the author(s) have dedicated all
copyright and related and neighboring rights to this software to the
public domain worldwide. This software is distributed without any warranty.

You should have received a copy of the CC0 Public Domain Dedication
along with this software. If not, see
http://creativecommons.org/publicdomain/zero/1.0/
*/

#ifndef UTIL_BASE64FBS_H_
#define UTIL_BASE64FBS_H_

#include "base64fbs/base64_impl.h"
#include "flatbuffers/idl.h"

namespace base64fbs {
// [BASE64]{https://tools.ietf.org/html/rfc4648}
//
// Flattbuffers type declaration:
// table TestBase64 {
//    data:[ubyte](base64);
//    urldata:[ubyte](base64url);
// }
//
// json-data:
// {
//    data: "99A/+4E0s9wY+So=",
//    urldata: "99A_-4E0s9wY-So="
// }
//
// Field attribute (base64): accepts standard RFC4648 alphabet for [ubyte].
// Padding is optional for the decoder.
// Valid input strings: {"/43+AergFA==", "/43+AergFA"}.
// The encoder output always with padding if required:
// Vector [255, 141, 254, 1, 234, 224, 20] => "/43+AergFA==".
//
// Field attribute (base64url): RFC4648 with URL and Filename Safe Alphabet for
// [ubye]. Padding is optional for the decoder.
// The decoder accepts both alphabets: UrlSafe and Standard.
// Valid input strings:
//    {"/43+AergFA==", "/43+AergFA", "_43-AergFA==", "_43-AergFA"}.
// The encoder output always with padding if required:
// Vector [255, 141, 254, 1, 234, 224, 20] encoded to "_43-AergFA==".
//
// Print extension of (base64url) attribute:
// If IDLOptions::base64_cancel_padding is true, then the encoder omits
// a padding for the output sequence.
// Vector [255, 141, 254, 1, 234, 224, 20] encoded to "_43-AergFA".
//

// Helper for checking the FieldDef attributes.
static inline int GetFieldBase64Mode(const flatbuffers::FieldDef *fd) {
  if (fd && fd->attributes.Lookup("base64"))
    return kBase64ModeStandard;
  else if (fd && fd->attributes.Lookup("base64url"))
    return kBase64ModeUrlSafe;
  else
    return kBase64ModeNotSet;
}

// Returns the number of processed symbols in the text.
// On succsess, the return value is equal to text.size().
template<class FBB>
inline size_t ParseBase64Vector(flatbuffers::Type type, const std::string &text,
                                const flatbuffers::FieldDef *fd,
                                flatbuffers::uoffset_t *ovalue, FBB *_builder) {
  // Only UCHAR supported.
  if ((type.base_type != flatbuffers::BASE_TYPE_UCHAR) || text.empty())
    return 0;
  // Leave if the string doesn't have the base64 attribute.
  const auto b64mode = GetFieldBase64Mode(fd);
  if (!b64mode) return 0;
  // Calculate the number of required bytes.
  // It must be precise, created vector can't be resized or rolled back.
  auto dec_size = text.size();
  // Encoder transforms 4 symbols to 3 bytes.
  // Even unpadded base64 strig can't be less than two symbols.
  if (dec_size < 2) return 0;
  if ('=' == text[dec_size - 1]) {
    dec_size -= ('=' == text[dec_size - 2]) ? 2 : 1;
  }
  dec_size = (dec_size * 3) / 4;
  if (dec_size < 1) return 0;
  // Allocate memory.
  uint8_t *dst = nullptr;
  *ovalue = _builder->CreateUninitializedVector(dec_size, 1, &dst);
  // Decode the string to the memory.
  const auto written =
      base64fbs_decode(b64mode, text.c_str(), text.size(), dst, dec_size);
  assert(written <= dec_size);
  // Transform to the input space.
  if (written == dec_size) {
    return text.size();
  } else {
    size_t decode_pos = (written / 3) * 4;
    return (decode_pos == text.size()) ? 0 : decode_pos;
  }
}

// Print a Vector as a base64 encoded string.
// On succsess, appends quoted base64 string  to the _text and returs true.
template<typename T>
inline bool PrintBase64Vector(const T *vdata, size_t vsize,
                              flatbuffers::Type type,
                              const flatbuffers::IDLOptions &opts,
                              std::string *_text,
                              const flatbuffers::FieldDef *fd) {
  (void)vdata;
  (void)vsize;
  (void)type;
  (void)opts;
  (void)_text;
  (void)fd;
  return false;
}

template<>
inline bool PrintBase64Vector<uint8_t>(const uint8_t *vdata, const size_t vsize,
                                       flatbuffers::Type type,
                                       const flatbuffers::IDLOptions &opts,
                                       std::string *_text,
                                       const flatbuffers::FieldDef *fd) {
  (void)type;

  // An empty vector can't be present as true base64 string.
  if (0 == vsize) return false;

  const auto b64mode = GetFieldBase64Mode(fd);
  const auto b64pad_cancel = opts.base64_cancel_padding && (b64mode == 2);
  if (!b64mode) return false;

  std::string &text = *_text;
  text += '\"';
  const auto text_pos = text.size();
  // std::string is a contiguous memory container since C++11.
  // data() + i == &operator[](i) for every i in [0, size()].	(since C++11)
  // http://en.cppreference.com/w/cpp/string/basic_string/data
  // http://en.cppreference.com/w/cpp/string/basic_string/operator_at
  // https://stackoverflow.com/questions/39200665/directly-write-into-char-buffer-of-stdstring
  // Allocate memory for a result.
  // Encoder transforms 3 bytes to 4 symbols with round up.
  const auto req_size = (vsize / 3) * 4 + 4;
  text.append(req_size, '#');
  // Return zero if required b64mode is unsupported.
  const auto written =
      base64fbs_encode(b64mode, vdata, vsize, &text[text_pos], req_size);
  assert(written <= req_size);
  // Shrink to real size if initial req_size overestimated.
  text.resize(text_pos + written);
  if (b64pad_cancel && written) {
    // Remove padding symbols (up to 2), if it are present.
    if ('=' == text.back()) text.pop_back();
    if ('=' == text.back()) text.pop_back();
  }
  text += '\"';
  // Roll-back open/close quotes and return.
  if (0 == written) {
    assert(text_pos > 0);
    text.resize(text_pos - 1);
    return false;
  }
  return true;
}

// On succsess, returns zero.
int JsonBase64Test();

}  // namespace base64fbs

#endif  // UTIL_BASE64FBS_H_
