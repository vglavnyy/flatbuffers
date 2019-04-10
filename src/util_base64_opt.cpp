#include "flatbuffers/base.h"
#include "flatbuffers/util_base64.h"

#if defined(_MSC_VER)
#  include <stdlib.h>  // _byteswap_ulong
#endif

namespace flatbuffers {

// Change byte order to minimaze bitshifts during encode/decode.
// Only is used for versions with optimization.
static inline uint32_t Base64ByteSwap32(uint32_t v) {
#if FLATBUFFERS_LITTLEENDIAN
#  if defined(__GNUC__) || defined(__clang__)
  return __builtin_bswap32(v);
#  elif defined(_MSC_VER)
  return _byteswap_ulong(v);
#  else
  return (((v >> 0) & 0xFF) << 24) | (((v >> 8) & 0xFF) << 16) |
         (((v >> 16) & 0xFF) << 8) | (((v >> 24) & 0xFF) << 0);
#  endif
#else   // FLATBUFFERS_LITTLEENDIAN
  return v;
#endif  // FLATBUFFERS_LITTLEENDIAN
}

// M=0: Standard base64 encode table (RFC 4648).
// M=1: Mixed table with URL and Filename Safe Alphabet and Standard symbols.
#define B64_ENCODE_TABLE(M)                                                    \
  {                                                                            \
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', \
        'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c',  \
        'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q',  \
        'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4',  \
        '5', '6', '7', '8', '9', (M ? '-' : '+'), (M ? '_' : '/')              \
  }

// M=0: Standard base64 table (RFC 4648).
// M=1: Mixed table with URL and Filename Safe Alphabet and Standard symbols.
// Z - a default value for symbols which are out of rage the base64 alphabet.
// s - a bit-shift parameter to align decoded values, if it needed.
#define B64_DECODE_TABLE(M, Z, s)                                              \
  {                                                                            \
    Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, \
        Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, (62u << s), Z,   \
        (M ? (62u << s) : Z), Z, (63u << s), (52u << s), (53u << s),           \
        (54u << s), (55u << s), (56u << s), (57u << s), (58u << s),            \
        (59u << s), (60u << s), (61u << s), Z, Z, Z, Z, Z, Z, Z, (0u << s),    \
        (1u << s), (2u << s), (3u << s), (4u << s), (5u << s), (6u << s),      \
        (7u << s), (8u << s), (9u << s), (10u << s), (11u << s), (12u << s),   \
        (13u << s), (14u << s), (15u << s), (16u << s), (17u << s),            \
        (18u << s), (19u << s), (20u << s), (21u << s), (22u << s),            \
        (23u << s), (24u << s), (25u << s), Z, Z, Z, Z, (M ? (63u << s) : Z),  \
        Z, (26u << s), (27u << s), (28u << s), (29u << s), (30u << s),         \
        (31u << s), (32u << s), (33u << s), (34u << s), (35u << s),            \
        (36u << s), (37u << s), (38u << s), (39u << s), (40u << s),            \
        (41u << s), (42u << s), (43u << s), (44u << s), (45u << s),            \
        (46u << s), (47u << s), (48u << s), (49u << s), (50u << s),            \
        (51u << s), Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z,   \
        Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z,   \
        Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z,   \
        Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z,   \
        Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z,   \
        Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z       \
  }

// Encode [ubyte] array to base64 string.
bool PrintBase64Vector(const Vector<uint8_t> &vec, const IDLOptions &opts,
                       std::string *_text, const FieldDef *fd) {
  static const char std_encode_table[64] = B64_ENCODE_TABLE(0);
  static const char url_encode_table[64] = B64_ENCODE_TABLE(1);

  const auto src_size = vec.size();
  const auto b64mode = GetBase64Mode(fd);
  if ((0 == src_size) || (kBase64ModeNotSet == b64mode)) return false;

  const auto b64_tbl =
      (b64mode == kBase64ModeStandard) ? std_encode_table : url_encode_table;

  auto src = vec.data();
  auto &text = *_text;
  text += '\"';  // open the string

  // Size of remainder, must be > 0.
  const auto B3rem = (src_size % 3) ? (src_size % 3) : 3;
  // Number of prefect blocks >= 0.
  const auto B3full = (src_size - B3rem) / 3;
  FLATBUFFERS_ASSERT(B3rem > 0);
  FLATBUFFERS_ASSERT((B3full * 3 + B3rem) == src_size);

  // Pre-allocate a memory for the result.
  // Take into account that the remainder is transformed into a complete
  // four-symbol sequence with help of padding.
  const auto dest_size = (B3full * 4) + 4;
  const auto text_base = text.size();
  text.resize(text_base + dest_size);
  auto dst = &(text[text_base]);

  // debug check
  const auto src_stop = src + src_size;
  const auto dst_stop = dst + dest_size;

  for (size_t k = 0; k < B3full; k++) {
    // The remainder is non-zero (B3rem>0) and we can read 4 bytes per round.
    uint32_t v;
    std::memcpy(&v, src, 4);
    v = Base64ByteSwap32(v);
    dst[0] = b64_tbl[0x3f & (v >> (8 + 18))];
    dst[1] = b64_tbl[0x3f & (v >> (8 + 12))];
    dst[2] = b64_tbl[0x3f & (v >> (8 + 6))];
    dst[3] = b64_tbl[0x3f & (v >> (8 + 0))];
    dst += 4;
    src += 3;
  }

  // Process the remainder.
  // Set padding: {a,_,_}=>{'x','y','=','='}, {a,b,_}=>{'x','y','z','='}.
  {
    uint32_t v = 0;
    for (auto k = B3rem * 0; k < B3rem; k++) { v |= src[k] << (3 - k) * 8; }
    dst[0] = b64_tbl[0x3f & (v >> (8 + 18))];
    dst[1] = b64_tbl[0x3f & (v >> (8 + 12))];
    dst[2] = (B3rem < 2) ? '=' : b64_tbl[0x3f & (v >> (8 + 6))];
    dst[3] = (B3rem < 3) ? '=' : b64_tbl[0x3f & (v >> (8 + 0))];
    dst += 4;
    src += B3rem;
  }

  (void)src_stop;
  (void)dst_stop;
  FLATBUFFERS_ASSERT(src_stop == src);
  FLATBUFFERS_ASSERT(dst_stop == dst);

  // Check the base64_cancel_padding and trim the result string.
  if (opts.base64_cancel_padding && (b64mode == kBase64ModeUrlSafe)) {
    if ('=' == text.back()) text.pop_back();
    if ('=' == text.back()) text.pop_back();
  }

  text += '\"';  // close string
  return true;
}

// Decode [ubyte] array from base64 string.
bool ParseBase64Vector(const std::string &text, const FieldDef *fd,
                       uoffset_t *ovalue, FlatBufferBuilder *_builder) {
  // Set an invalid value to 0x1, and use bit0 as error flag.
  static const uint8_t std_decode_table[256] = B64_DECODE_TABLE(0, 1u, 1u);
  static const uint8_t url_decode_table[256] = B64_DECODE_TABLE(1, 1u, 1u);

  const auto b64mode = GetBase64Mode(fd);
  if (kBase64ModeNotSet == b64mode) return false;

  const auto b64_tbl =
      (b64mode == kBase64ModeStandard) ? std_decode_table : url_decode_table;

  auto src = text.c_str();
  auto src_size = text.size();

  // The base64 decoder transforms 4 encoded characters to 3 data bytes.
  // Ignore padding: "abc="->"abc", "ab=="->"ab".
  if (0 != src_size && 0 == (src_size % 4)) {
    // ignore padding: "abc="->"abc", "ab=="->"ab".
    FLATBUFFERS_ASSERT(src_size >= 4);
    if ('=' == text[src_size - 1]) src_size -= 1;
    if ('=' == text[src_size - 1]) src_size -= 1;
  }

  // Make non-zero remander.
  const auto C4rem = (src_size % 4) ? (src_size % 4) : 4;
  // Number of perfect char[4] blocks.
  const auto C4full = (src_size - C4rem) / 4;
  FLATBUFFERS_ASSERT(C4rem > 0);
  FLATBUFFERS_ASSERT((C4full * 4 + C4rem) == src_size);
  const auto dest_size = (src_size * 3) / 4;

  // C4rem is number of remained characters in the last char[4] block.
  // Two encoded symbols are required to decode one data byte.
  // C4rem==1 is forbidden.
  if ((0 == dest_size) || (1 == C4rem)) return false;

  // Create Vector<uint8_t>.
  uint8_t *dst = nullptr;
  *ovalue = _builder->CreateUninitializedVector(dest_size, 1, &dst);

  // debug check
  const auto src_stop = src + src_size;
  const auto dst_stop = dst + dest_size;

  // Unsigned error mask takes only two states: (0)-ok and (1)-fail.
  size_t err_mask = 0;

  // Use (err_mask - 1) = {0,~0} as conditional gate for the loop_cnt.
  for (size_t loop_cnt = C4full; loop_cnt & (err_mask - 1); loop_cnt--) {
    uint32_t a0 = b64_tbl[static_cast<uint8_t>(src[0])];
    uint32_t a1 = b64_tbl[static_cast<uint8_t>(src[1])];
    uint32_t a2 = b64_tbl[static_cast<uint8_t>(src[2])];
    uint32_t a3 = b64_tbl[static_cast<uint8_t>(src[3])];
    // The err_mask will be equal to 1, if an error is detected.
    err_mask = (a0 | a1 | a2 | a3) & 1;
    // Decode by RFC4648 algorithm. Squash 4 symbosl to 3 bytes.
    uint32_t v = (a0 << (8 + 18 - 1)) | (a1 << (8 + 12 - 1)) |
                 (a2 << (8 + 6 - 1)) | (a3 << (8 - 1));
    v = Base64ByteSwap32(v);
    // C4rem > 0, it allows to write 4 bytes per round without violations.
    std::memcpy(dst, &v, 4);  // one write op the unaligned is allowed.
    dst += 3;
    src += 4;
  }

  // Process the remainder.
  if (0 == err_mask) {
    uint32_t a0 = C4rem > 0 ? b64_tbl[static_cast<uint8_t>(src[0])] : 0;
    uint32_t a1 = C4rem > 1 ? b64_tbl[static_cast<uint8_t>(src[1])] : 0;
    uint32_t a2 = C4rem > 2 ? b64_tbl[static_cast<uint8_t>(src[2])] : 0;
    uint32_t a3 = C4rem > 3 ? b64_tbl[static_cast<uint8_t>(src[3])] : 0;
    // The err_mask will be equal to 1, if an error is detected.
    err_mask = (a0 | a1 | a2 | a3) & 1;
    // Decode by RFC4648 algorithm.
    uint32_t v = (a0 << (8 + 18 - 1)) | (a1 << (8 + 12 - 1)) |
                 (a2 << (8 + 6 - 1)) | (a3 << (8 - 1));
    v = Base64ByteSwap32(v);
    if (C4rem > 1) dst[0] = (v >> 0) & 0xFF;
    if (C4rem > 2) dst[1] = (v >> 8) & 0xFF;
    if (C4rem > 3) dst[2] = (v >> 16) & 0xFF;
    dst += C4rem - 1;
    src += C4rem;
  }

  // Ensures.
  if (0 == err_mask) {
    (void)src_stop;
    (void)dst_stop;
    FLATBUFFERS_ASSERT(src_stop == src);
    FLATBUFFERS_ASSERT(dst_stop == dst);
  } else {
    FLATBUFFERS_ASSERT(src_stop > src);
    FLATBUFFERS_ASSERT(dst_stop > dst);
  }

  return (0 == err_mask);
}

}  // namespace flatbuffers
