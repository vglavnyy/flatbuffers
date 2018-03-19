/*
Written in 2017-2018 by vglavnyy.

To the extent possible under law, the author(s) have dedicated all
copyright and related and neighboring rights to this software to the
public domain worldwide. This software is distributed without any warranty.

You should have received a copy of the CC0 Public Domain Dedication
along with this software. If not, see
http://creativecommons.org/publicdomain/zero/1.0/
*/

#ifndef UTIL_BASE64FBS_IMPL_H_
#define UTIL_BASE64FBS_IMPL_H_

#ifndef __cplusplus
#include <stddef.h>
#include <stdint.h>
#else
#include <cstddef>
#include <cstdint>
extern "C" {
#endif

// C-interface to a base64 implementation.
// All functions must be thread safe.

enum kBase64Mode {
  // a Field doesn't have base64/base64url attribute.
  kBase64ModeNotSet = 0,

  // Attribute (base64): standard RFC4648 alphabet.
  // Padding is optional for the decoder.
  // Valid input strings:
  //  {"/43+AergFA==", "/43+AergFA"}.
  // The encoder output always with padding if required:
  // [255, 141, 254, 1, 234, 224, 20] => "/43+AergFA==".
  kBase64ModeStandard = 1,

  // Attribute (base64url): RFC4648 with URL and Filename Safe Alphabet.
  // Padding is optional for the decoder.
  // The decoder accepts both alphabets: UrlSafe and Standard.
  // Valid input strings:
  //  {"/43+AergFA==", "/43+AergFA", "_43-AergFA==", "_43-AergFA"}.
  // The encoder output always with padding if required:
  // [255, 141, 254, 1, 234, 224, 20] encoded to "_43-AergFA==".
  kBase64ModeUrlSafe = 2
};

// Decode the input <src> base64 string and save result to the <dst>.
//  <base64_mode> - alphabet {Base64ModeStandard or Base64ModeUrlSafe},
//  <src> - pointer to the base64 string,
//  <src_size> - length of the input string,
//  <dst> - pointer to the output buffer,
//  <dst_size> - length of the buffer,
//  return: the number of written bytes.
//
// On succsess, <error_position> must be equal to <src_size>.
// If the <error_position> less than <src_size>, it should point to a location
// of error symbol inside the string as interval [0, <error_position>].
//
// If <base64_mode> is unsupported, should return zero and zero in
// the <error_position>.
//
// If the <dst_size> is insufficient to save result, should return zero and
// zero in the <error_position>.
//
size_t base64fbs_decode(int base64_mode, const char *src, size_t src_size,
                        uint8_t *dst, size_t dst_size);

// Encode the input byte sequence into the base64 encoded string.
//  <base64_mode> - alphabet {Base64ModeStandard or Base64ModeUrlSafe},
//  <src> - pointer to the binary array,
//  <src_size> - length of the array,
//  <dst> - pointer to the output buffer,
//  <dst_size> - length of the buffer,
//  return: the number of written bytes to the <dst> memory.
//
// If <base64_mode> is unsupported, should return zero.
//
// If the <dst_size> is insufficient to save result, should return zero.
//
size_t base64fbs_encode(int base64_mode, const uint8_t *src, size_t src_size,
                        char *dst, size_t dst_size);

#ifdef __cplusplus
}
#endif

#endif  // UTIL_BASE64FBS_IMPL_H_