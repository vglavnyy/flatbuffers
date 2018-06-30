#include "flatbuffers/util_base64.h"
#include "flatbuffers/base.h"

#if defined(_MSC_VER)
#  include <stdlib.h>  // _byteswap_ulong
#endif

namespace flatbuffers {

// Change byte order to minimaze bitshifts during encode/decode.
// Only is used for versions with optimization.
static inline uint32_t Base64ByteSwap_u32(uint32_t v) {
#  if FLATBUFFERS_LITTLEENDIAN
#    if defined(__GNUC__) || defined(__clang__)
  return __builtin_bswap32(v);
#    elif defined(_MSC_VER)
  return _byteswap_ulong(v);
#    else
  return (((v >> 0) & 0xFF) << 24) | (((v >> 8) & 0xFF) << 16) |
         (((v >> 16) & 0xFF) << 8) | (((v >> 24) & 0xFF) << 0);
#    endif
#  else   // FLATBUFFERS_LITTLEENDIAN
  return v;
#  endif  // FLATBUFFERS_LITTLEENDIAN
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
  // First:
  // The base64 encoder transforms 3 data bytes to 4 encoded characters.
  // Byte by byte reading is too costly, it is better to read by uint32 or
  // uint64 per time. Lets to split the input array to a perfect part (% 3 == 0)
  // and to a remainder of %3. If the remainder is non zero, can read the
  // perfect part by uint32 access to the array's memory.
  // Second:
  // Size of the result string computable before encoding.
  // A memory for the result preallocated before start.
  // Encoded symbols are written with direct access by a raw pointer.

  static const char std_encode_table[64] = B64_ENCODE_TABLE(0);
  static const char url_encode_table[64] = B64_ENCODE_TABLE(1);

  const auto src_size = vec.size();
  const auto b64mode = GetBase64Mode(fd);
  if ((0 == src_size) || (kBase64ModeNotSet == b64mode)) return false;

  const auto b64_tbl =
      (b64mode == kBase64ModeStandard) ? std_encode_table : url_encode_table;

  const auto src = vec.data();
  std::string &text = *_text;
  text += '\"';  // open string

  // Size of remainder, must be > 0:
  const auto B3rem = (src_size % 3) ? (src_size % 3) : 3;
  // Number of complete blocks >= 0.
  const auto B3full = (src_size - B3rem) / 3;
  FLATBUFFERS_ASSERT(B3rem > 0);
  FLATBUFFERS_ASSERT((B3full * 3 + B3rem) == src_size);

  // Preallocate destanation memory.
  const auto text_base = text.size();
  text.resize(text_base + (B3full * 4) + 4);
  char *const dst = &(text[text_base]);

  // Make complete remainder.
  char last_enc4[4];
  uint8_t last_bin3[4] = { 0, 0, 0, 0 };
  for (auto k = B3rem * 0; k < B3rem; k++) last_bin3[k] = src[B3full * 3 + k];
  // First, process the remainder.
  const uint8_t *src_ = &last_bin3[0];
  auto dst_ = &last_enc4[0];

  for (size_t k = 0; k < (1 + B3full); k++) {
    // The remainder is non-zero (B3rem>0) and we can read 4 bytes per round
    // from the source without memory violation.
    uint32_t v = *(reinterpret_cast<const uint32_t *>(src_));
    v = Base64ByteSwap_u32(v);
    dst_[0] = b64_tbl[0x3f & (v >> (8 + 18))];
    dst_[1] = b64_tbl[0x3f & (v >> (8 + 12))];
    dst_[2] = b64_tbl[0x3f & (v >> (8 + 6))];
    dst_[3] = b64_tbl[0x3f & (v >> (8 + 0))];
    src_ = src + (k * 3);
    dst_ = dst + (k * 4);
  }
  // Set padding ({a,_,_}=>{'x','y','=','='}, {a,b,_}=>{'x','y','z','='}).
  dst[B3full * 4 + 0] = last_enc4[0];
  dst[B3full * 4 + 1] = last_enc4[1];
  dst[B3full * 4 + 2] = (B3rem < 2) ? '=' : last_enc4[2];
  dst[B3full * 4 + 3] = (B3rem < 3) ? '=' : last_enc4[3];

  // Check the base64_cancel_padding.
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

  auto src_size = text.size();
  auto src = text.c_str();

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
  // C4rem==1 is forbidden, one data byte encoded by two symbols.
  if ((0 == dest_size) || (1 == C4rem)) return false;

  // Create Vector<uint8_t>.
  uint8_t *dst = nullptr;
  *ovalue = _builder->CreateUninitializedVector(dest_size, 1, &dst);

  // Base64 decode.

  // Build char[4] block for the remainder.
  char last_enc4[4] = { 'A', 'A', 'A', 'A' };
  // Make the local copy of the remainder.
  for (size_t k = 0; k < C4rem; k++) { last_enc4[k] = src[C4full * 4 + k]; }
  // Size of the last decoded block can be 1,2 or 3.
  const auto rem_dec_len = C4rem ? (C4rem - 1) : 0;
  FLATBUFFERS_ASSERT((C4full * 3 + rem_dec_len) == dest_size);

  // Main decode loop:
  // At the first iteration decode the last block (remainder).
  uint8_t last_dec3[4];
  auto _dst = &last_dec3[0];
  const char *_src = &last_enc4[0];
  // Loop counter: the last block plus all perfect blocks.
  auto loop_cnt = (1 + C4full);
  // Error mask takes only two states: (0)-ok and (1)-fail.
  // Type of <err_mask> must match with unsigned <loop_cnt>.
  size_t err_mask = loop_cnt * 0;

  // Use (err_mask - 1) = {0,~0} as conditional gate for the loop_cnt.
  for (size_t k = 0; loop_cnt & (err_mask - 1); loop_cnt--, k++) {
    const uint32_t a0 = b64_tbl[static_cast<uint8_t>(_src[0])];
    const uint32_t a1 = b64_tbl[static_cast<uint8_t>(_src[1])];
    const uint32_t a2 = b64_tbl[static_cast<uint8_t>(_src[2])];
    const uint32_t a3 = b64_tbl[static_cast<uint8_t>(_src[3])];
    // The err_mask will be equal to 1, if an error is detected.
    err_mask = (a0 | a1 | a2 | a3) & 1;
    // Decode by RFC4648 algorithm.
    uint32_t aa = (a0 << (8 + 18 - 1)) | (a1 << (8 + 12 - 1)) |
                  (a2 << (8 + 6 - 1)) | (a3 << (8 - 1));
    // C4rem > 0, it allows to write 4 bytes per round without violations.
    *(reinterpret_cast<uint32_t *>(_dst)) = Base64ByteSwap_u32(aa);
    _src = src + (k * 4);
    _dst = dst + (k * 3);
  }
  if (false == err_mask) {
    // Copy decoded remainder.
    for (size_t k = 0; k < rem_dec_len; k++) {
      dst[C4full * 3 + k] = last_dec3[k];
    }
  }
  return (false == err_mask);
}

}  // namespace flatbuffers
