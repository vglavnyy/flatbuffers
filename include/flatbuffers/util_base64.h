#ifndef FLATBUFFERS_UTIL_BASE64_H_
#define FLATBUFFERS_UTIL_BASE64_H_

#include "flatbuffers/idl.h"

namespace flatbuffers {
// [BASE64]{https://tools.ietf.org/html/rfc4648}
//
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
// Field with attribute [ubyte](base64): accepts standard RFC4648 alphabet.
// The padding is optional for the decoder.
// Valid input strings: {"/43+AergFA==", "/43+AergFA"}.
// The encoder output will have a padding if it is required:
// [255, 141, 254, 1, 234, 224, 20] encoded to "/43+AergFA==".
//
// Field with attribute [ubyte](base64url): RFC4648 with URL Safe Alphabet.
// The padding is optional for the decoder.
// The decoder accepts both alphabets: UrlSafe and Standard.
// Valid input strings:
//    {"/43+AergFA==", "/43+AergFA", "_43-AergFA==", "_43-AergFA"}.
// The encoder output will have a padding if it is required:
// [255, 141, 254, 1, 234, 224, 20] encoded to "_43-AergFA==".
//

enum Base64Mode {
  // A field doesn't have base64/base64url attribute.
  Base64ModeNotSet = 0,

  // Attribute (base64): standard RFC4648 alphabet.
  // The padding is optional for the decoder.
  // Valid input strings:
  // {"/43+AergFA==", "/43+AergFA"}.
  Base64ModeStandard = 1,

  // Attribute (base64url): RFC4648 with URL and Filename Safe Alphabet.
  // The padding is optional for the decoder.
  // The decoder accepts both alphabets: UrlSafe and Standard.
  // Valid input strings:
  // {"/43+AergFA==", "/43+AergFA", "_43-AergFA==", "_43-AergFA"}.
  Base64ModeUrlSafe = 2
};

// Map a field attribute to kBase64Mode enum.
inline Base64Mode GetBase64Mode(const FieldDef *fd) {
  auto is_u8vec = fd && (BASE_TYPE_UCHAR == fd->value.type.element) &&
                  ((BASE_TYPE_VECTOR == fd->value.type.base_type) ||
                   (BASE_TYPE_ARRAY == fd->value.type.base_type));
  if (is_u8vec && fd->attributes.Lookup("base64url")) return Base64ModeUrlSafe;
  if (is_u8vec && fd->attributes.Lookup("base64")) return Base64ModeStandard;
  return Base64ModeNotSet;
}

// Check that field is a base64 field.
inline bool IsBase64(const FieldDef *fd) {
  return GetBase64Mode(fd) != Base64ModeNotSet;
}

// Decode [ubyte] array from base64 string.
bool ParseBase64Vector(Base64Mode b64mode, const std::string &text,
                       FlatBufferBuilder *_builder, uoffset_t *_ofs);

// Print a Vector as quoted base64 encoded string.
bool PrintBase64Vector(Base64Mode b64mode, const uint8_t *src, size_t src_size,
                       std::string *_text);

}  // namespace flatbuffers

#endif  // FLATBUFFERS_UTIL_BASE64_H_
