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
// Print extension of (base64url) attribute:
// If IDLOptions::base64_cancel_padding is true, the encoder will remove
// the padding in the output sequence.
// [255, 141, 254, 1, 234, 224, 20] encoded to "_43-AergFA".
//

enum kBase64Mode {
  // A field doesn't have base64/base64url attribute.
  kBase64ModeNotSet = 0,

  // Attribute (base64): standard RFC4648 alphabet.
  // The padding is optional for the decoder.
  // Valid input strings:
  // {"/43+AergFA==", "/43+AergFA"}.
  kBase64ModeStandard = 1,

  // Attribute (base64url): RFC4648 with URL and Filename Safe Alphabet.
  // The padding is optional for the decoder.
  // The decoder accepts both alphabets: UrlSafe and Standard.
  // Valid input strings:
  // {"/43+AergFA==", "/43+AergFA", "_43-AergFA==", "_43-AergFA"}.
  kBase64ModeUrlSafe = 2
};

// Map a field attribute to kBase64Mode enum.
inline kBase64Mode GetBase64Mode(const FieldDef *fd) {
  FLATBUFFERS_ASSERT(fd);
  if (fd->attributes.Lookup("base64url"))
    return kBase64ModeUrlSafe;
  else if (fd->attributes.Lookup("base64"))
    return kBase64ModeStandard;
  else
    return kBase64ModeNotSet;
}

// Check that field is a base64 field.
inline bool IsBase64(const FieldDef *fd) {
  return fd && (BASE_TYPE_VECTOR == fd->value.type.base_type) &&
         (BASE_TYPE_UCHAR == fd->value.type.element) &&
         (kBase64ModeNotSet != GetBase64Mode(fd));
}

// Decode [ubyte] array from base64 string.
bool ParseBase64Vector(const std::string &text, const FieldDef *fd,
                       uoffset_t *ovalue, FlatBufferBuilder *_builder);

// Print a Vector as a base64 encoded string.
// On succsess, appends quoted base64 string to the _text and returs true.
template<typename T>
inline bool PrintBase64Vector(const Vector<T> &v, const IDLOptions &opts,
                              std::string *_text, const FieldDef *fd) {
  (void)v;
  (void)opts;
  (void)_text;
  (void)fd;
  return false;
}

// Overload for Vector<uint8_t>.
bool PrintBase64Vector(const Vector<uint8_t> &vec, const IDLOptions &opts,
                       std::string *_text, const FieldDef *fd);

}  // namespace flatbuffers

#endif  // FLATBUFFERS_UTIL_BASE64_H_
