/*
Written in 2017-2018 by vglavnyy.

To the extent possible under law, the author(s) have dedicated all
copyright and related and neighboring rights to this software to the
public domain worldwide. This software is distributed without any warranty.

You should have received a copy of the CC0 Public Domain Dedication
along with this software. If not, see
http://creativecommons.org/publicdomain/zero/1.0/
*/

#include <assert.h>
#include <cstdint>
#include <cstring>
#if defined(_MSC_VER) && defined(_WIN32)
#include<stdlib.h>
#endif

#include "../include/base64fbs/base64_impl.h"

// define cache-line alignment
#define BASE64_ALIGN_DEF alignas(64)

#if defined(_WIN32) || defined(__LITTLE_ENDIAN__) || \
    (defined(__GNUC__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__))
#define endian_is_little true
#endif

#if defined(__BIG_ENDIAN__) || \
    (defined(__GNUC__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__))
#define endian_is_little false
#endif

static inline uint32_t u32_byte_swap(uint32_t v) {
#if defined(__GNUC__) || defined(__clang__)
  return __builtin_bswap32(v);
#elif defined(_MSC_VER) && defined(_WIN32)
  return _byteswap_ulong(v);
#else
  return (((v >> 0) & 0xFF) << 24) | (((v >> 8) & 0xFF) << 16) |
         (((v >> 16) & 0xFF) << 8) | (((v >> 24) & 0xFF) << 0);
#endif
}

//----------------------------------------------------------------
// generate decode tables
// 1 as error marker
#define XX 1u
// clang-format off
// Standard base64 table (RFC 4648).
#define B64STD_DECODE_TABLE(s) {                                  \
  XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, \
  XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, \
  XX,         XX,         XX,         XX,         XX,         XX,         XX,         XX,         \
  XX,         XX,         XX,         (62u << s), XX,         XX,         XX,         (63u << s), \
  (52u << s), (53u << s), (54u << s), (55u << s), (56u << s), (57u << s), (58u << s), (59u << s), \
  (60u << s), (61u << s), XX,         XX,         XX,         XX,         XX,         XX,         \
  XX,         (0u << s),  (1u << s),  (2u << s),  (3u << s),  (4u << s),  (5u << s),  (6u << s),  \
  (7u << s),  (8u << s),  (9u << s),  (10u << s), (11u << s), (12u << s), (13u << s), (14u << s), \
  (15u << s), (16u << s), (17u << s), (18u << s), (19u << s), (20u << s), (21u << s), (22u << s), \
  (23u << s), (24u << s), (25u << s), XX,         XX,         XX,         XX,         XX,         \
  XX,         (26 << s),  (27 << s),  (28 << s),  (29 << s),  (30u << s), (31u << s), (32u << s), \
  (33u << s), (34u << s), (35u << s), (36u << s), (37u << s), (38u << s), (39u << s), (40u << s), \
  (41u << s), (42u << s), (43u << s), (44u << s), (45u << s), (46u << s), (47u << s), (48u << s), \
  (49u << s), (50u << s), (51u << s), XX,         XX,         XX,         XX,         XX,         \
  XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, \
  XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, \
  XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, \
  XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, \
  XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, \
  XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, \
  XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, \
  XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX  }

// Mixed table with URL and Filename Safe Alphabet and Standard symbols.
#define B64URL_DECODE_TABLE(s) {                                  \
  XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, \
  XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, \
  XX, XX, XX, XX, XX, XX, XX, XX,                                 \
  XX,         XX,         XX,         (62u << s), XX,         (62u << s), XX,         (63u << s), \
  (52u << s), (53u << s), (54u << s), (55u << s), (56u << s), (57u << s), (58u << s), (59u << s), \
  (60u << s), (61u << s), XX,         XX,         XX,         XX,         XX,         XX,         \
  XX,         (0u << s),  (1u << s),  (2u << s),  (3u << s),  (4u << s),  (5u << s),  (6u << s),  \
  (7u << s),  (8u << s),  (9u << s),  (10u << s), (11u << s), (12u << s), (13u << s), (14u << s), \
  (15u << s), (16u << s), (17u << s), (18u << s), (19u << s), (20u << s), (21u << s), (22u << s), \
  (23u << s), (24u << s), (25u << s), XX,         XX,         XX,         XX,         (63u << s), \
  XX,         (26u << s), (27u << s), (28u << s), (29u << s), (30u << s), (31u << s), (32u << s), \
  (33u << s), (34u << s), (35u << s), (36u << s), (37u << s), (38u << s), (39u << s), (40u << s), \
  (41u << s), (42u << s), (43u << s), (44u << s), (45u << s), (46u << s), (47u << s), (48u << s), \
  (49u << s), (50u << s), (51u << s), XX,         XX,         XX,         XX,         XX,         \
  XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, \
  XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, \
  XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, \
  XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, \
  XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, \
  XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, \
  XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, \
  XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX  }
// clang-format on

BASE64_ALIGN_DEF
static const uint8_t std_decode_table[256] = B64STD_DECODE_TABLE(1);
BASE64_ALIGN_DEF
static const uint8_t url_decode_table[256] = B64URL_DECODE_TABLE(1);
#undef XX
// end of generate

BASE64_ALIGN_DEF
static const char std_encode_table[64] = {
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
  'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
  'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
};

BASE64_ALIGN_DEF
static const char url_encode_table[64] = {
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
  'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
  'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '-', '_'
};

//----------------------------------------------------------------
// Base64 Decoder implementaton.
// 1) If condition ((nullptr == dst) && (0 == dst_size)) is satisfied,
// returns the number of bytes, need to save a decoded result.
// 2) Returns the number of bytes written to the destination memory.
// Error condition: (src_size != error_position).
static size_t B64DecodeImpl(const int mode, const char *const src,
                            const size_t src_size, uint8_t *const dst,
                            const size_t dst_size,
                            size_t *const error_position) {
  const auto b64_tbl =
      (mode == kBase64ModeStandard) ? std_decode_table : url_decode_table;
  *error_position = 0;
  if (0 == src_size) return 0;
  // The base64 decoder transforms 4 encoded characters to 3 data bytes.
  // Prepare stage:
  // Number of perfect char[4] blocks.
  size_t C4full = src_size / 4;
  // Number of remained characters in the last char[4] block.
  auto C4rem = src_size % 4;
  // If C4rem==0, then move the last char[4] block to remainder.
  if (C4rem == 0) {
    C4full = C4full - 1;
    C4rem = 4;
  }
  // The C4rem is strictly positive value from: {1,2,3,4}.
  // Build char[4] perfect block for the remainder.
  // If padding is mandatory, preset incomplete characters as invalid symbols
  // ('#'). If it is optional, preset as zero symbols ('A').
  // Now, padding is optional for the decoder.
  char last_enc4[4] = { 'A', 'A', 'A', 'A' };
  // Make the local copy of the remainder.
  memcpy(&last_enc4[0], &src[C4full * 4], C4rem);
  // Size of the last decoded block can be 1,2 or 3.
  auto last_dec_len = C4rem - 1;
  // The input sequence can have padding symbols '=' in the last char[4] block.
  // In this case are allowed only two variants: {x,y,=,=} or {x,y,z,=}.
  // Replace '=' by 'zeros' and recalculate decoded size of the last block.
  if (last_enc4[3] == '=') {
    last_dec_len = 2;
    last_enc4[3] = 'A';
    if (last_enc4[2] == '=') {
      last_dec_len = 1;
      last_enc4[2] = 'A';
    }
  }
  // Size of the decoded sequence if it has no errors.
  const auto decoded_len = (C4full * 3 + last_dec_len);
  // If requested the only the number of required bytes for decoding, return it.
  if ((nullptr == dst) && (0 == dst_size)) return decoded_len;
  // Insufficient memory test.
  if (dst_size < decoded_len) return 0;

  // Main decode loop:
  // At the first iteration decode the last block (remainder).
  uint8_t last_dec3[4];
  auto dst_ = &last_dec3[0];
  const char *src_ = &last_enc4[0];
  // Loop counter: the last block plus all perfect blocks.
  auto loop_cnt = (1 + C4full);
  // Error mask can take only two states: (0)-ok and (1)-fail.
  // Type of <err_mask> must match with unsigned <loop_cnt>.
  size_t err_mask = 0;

  // Use (err_mask - 1) = {0,~0} as conditional gate for the loop_cnt.
  for (size_t k = 0; loop_cnt & (err_mask - 1); loop_cnt--, k++) {
    const uint32_t a0 = b64_tbl[static_cast<uint8_t>(src_[0])];
    const uint32_t a1 = b64_tbl[static_cast<uint8_t>(src_[1])];
    const uint32_t a2 = b64_tbl[static_cast<uint8_t>(src_[2])];
    const uint32_t a3 = b64_tbl[static_cast<uint8_t>(src_[3])];
    // Decode by RFC4648 algorithm.
    uint32_t aa = (a0 << (8 + 18 - 1)) | (a1 << (8 + 12 - 1)) |
                  (a2 << (8 + 6 - 1)) | (a3 << (8 - 1));
    // err_mask is zero if no-errors, or 1 if an error detected
    err_mask = (a0 | a1 | a2 | a3) & 1;
    // C4rem > 0 and we can write 4 bytes per round without violations.
    *(reinterpret_cast<uint32_t *>(dst_)) =
        endian_is_little ? u32_byte_swap(aa) : aa;
    src_ = src + (k * 4);
    dst_ = dst + (k * 3);
  }
  const auto done = (0 == err_mask);
  if (done) {
    // Copy the decoded remainder to the destination.
    memcpy(&dst[C4full * 3], &last_dec3[0], last_dec_len);
    *error_position = src_size;
    return decoded_len;
  } else {
    // if error in the last the last block then loop_cnt == C4full
    assert(loop_cnt <= C4full);
    // If an error detected:
    // Number of blocks processed and written to the [dst] memory:
    const auto indx = C4full - loop_cnt;
    // Reject written data.
    memset(dst, 0, indx * 3);
    // number of successfully written blocks
    const auto pos = (indx ? (indx - 1) : 0);
    // error located inside half-open interval [x, src_size)
    *error_position = 4 * pos;
    // return last successfully written position
    return 3 * pos;
  }
}

// Base64 Encoder implementaton.
// 1) If condition ((nullptr == dst) && (0 == dst_size)) is satisfied,
// returns the number of bytes, need to save a encoded result.
// 2) Returns the number of bytes written to the destination memory.
// Returned Zero indicates nothing was written.
// Error condition: (src_size!=0 && ret==0).
static size_t B64EncodeImpl(const int mode, const uint8_t *const src,
                            const size_t src_size, char *const dst,
                            const size_t dst_size) {
  const auto b64_tbl =
      (mode == kBase64ModeStandard) ? std_encode_table : url_encode_table;
  if (0 == src_size) return 0;
  // The base64 encoder transforms 3 data bytes to 4 encoded characters.
  // Number of complete blocks:
  auto B3full = src_size / 3;
  // Size of remainder:
  auto B3rem = src_size % 3;
  // If B3rem, then move the last complete block to remainder.
  if (B3rem == 0) {
    B3full = B3full - 1;
    B3rem = 3;
  }
  // If the padding is mandatory
  const auto encoded_len = (B3full * 4) + 4;
  // If requested the only number of required bytes for encoding, return it.
  if ((nullptr == dst) && (0 == dst_size)) return encoded_len;
  // Insufficient memory test.
  if (dst_size < encoded_len) return 0;

  // Main encode loop:
  char last_enc4[4];
  uint8_t last_bin3[4] = { 0, 0, 0, 0 };
  memcpy(&last_bin3[0], &src[B3full * 3], B3rem);
  // First, process the complemented remainder.
  const uint8_t *src_ = &last_bin3[0];
  auto dst_ = &last_enc4[0];
  for (size_t k = 0; k < B3full + 1; k++) {
    // The remainder is non-zero (B3rem>0) and we can read 4 bytes per round
    // from the source without memory violation.
    uint32_t v = *(reinterpret_cast<const uint32_t *>(src_));
    v = endian_is_little ? u32_byte_swap(v) : v;
    dst_[0] = b64_tbl[0x3f & (v >> (8 + 18))];
    dst_[1] = b64_tbl[0x3f & (v >> (8 + 12))];
    dst_[2] = b64_tbl[0x3f & (v >> (8 + 6))];
    dst_[3] = b64_tbl[0x3f & (v >> (8 + 0))];
    src_ = src + (k * 3);
    dst_ = dst + (k * 4);
  }
  // Set padding ({a,_,_}=>{'x','y','=','='}, {a,b,_}=>{'x','y','z','='}).
  if (B3rem < 3) last_enc4[3] = '=';
  if (B3rem < 2) last_enc4[2] = '=';
  // Copy remainder.
  memcpy(&dst[B3full * 4], &last_enc4[0], 4);
  return encoded_len;
}

extern "C" size_t base64fbs_decode(int base64_mode, const char *src,
                                   size_t src_size, uint8_t *dst,
                                   size_t dst_size) {
  size_t err_pos = 0;
  size_t rc = 0;
  // If (0==dst_size) && (nullptr==src) then decode_b64 return the required
  // size. Avoid this by checking (dst_size>0).
  if (dst_size > 0) {
    rc = B64DecodeImpl(base64_mode, src, src_size, dst, dst_size, &err_pos);
  }
  return rc;
}

extern "C" size_t base64fbs_encode(int base64_mode, const uint8_t *src,
                                   size_t src_size, char *dst,
                                   size_t dst_size) {
  if (0 == dst_size) return 0;
  return B64EncodeImpl(base64_mode, src, src_size, dst, dst_size);
}
