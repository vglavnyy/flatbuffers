#include "flatbuffers/util_base64.h"
#include "flatbuffers/base.h"

namespace flatbuffers {

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
  // Encode tables.
  static const char std_encode_table[64] = B64_ENCODE_TABLE(0);
  static const char url_encode_table[64] = B64_ENCODE_TABLE(1);

  const auto src_size = vec.size();
  const auto b64mode = GetBase64Mode(fd);
  if ((0 == src_size) || (kBase64ModeNotSet == b64mode)) return false;

  const auto b64_tbl =
      (b64mode == kBase64ModeStandard) ? std_encode_table : url_encode_table;
  std::string &text = *_text;
  text += '\"';  // open string

  // The base64 encoder transforms 3 data bytes to 4 encoded characters.
  // Number of complete blocks:
  const auto B3full = src_size / 3;
  // Size of remainder.
  const auto B3rem = src_size % 3;
  FLATBUFFERS_ASSERT((B3full * 3 + B3rem) == src_size);

  const uint8_t *src = vec.data();
  for (auto k = B3full * 0; k < B3full; k++, src += 3) {
    text += b64_tbl[src[0] >> 2];
    text += b64_tbl[(0x3F & (src[0] << 4)) | (src[1] >> 4)];
    text += b64_tbl[(0x3F & (src[1] << 2)) | (src[2] >> 6)];
    text += b64_tbl[0x3F & src[2]];
  }

  auto cancel_pad =
      opts.base64_cancel_padding && (b64mode == kBase64ModeUrlSafe);

  if (B3rem == 1)
  {
    text += b64_tbl[src[0] >> 2];
    text += b64_tbl[(0x3F & (src[0] << 4)) | (0 >> 4)];
    if (false == cancel_pad) {
      text += '=';
      text += '=';
    }
  } else if (B3rem == 2)
  {
    text += b64_tbl[src[0] >> 2];
    text += b64_tbl[(0x3F & (src[0] << 4)) | (src[1] >> 4)];
    text += b64_tbl[(0x3F & (src[1] << 2)) | (0 >> 6)];
    if (false == cancel_pad) { text += '='; }
  }

  text += '\"';  // close string
  return true;
}

// Decode [ubyte] array from base64 string.
bool ParseBase64Vector(const std::string &text, const FieldDef *fd,
                       uoffset_t *ovalue, FlatBufferBuilder *_builder) {

  // Set an invalid value to 0x80, and use bit7 as error flag.
  static const uint8_t std_decode_table[256] = B64_DECODE_TABLE(0, 0x80, 0);
  static const uint8_t url_decode_table[256] = B64_DECODE_TABLE(1, 0x80, 0);

  const auto b64mode = GetBase64Mode(fd);
  if (kBase64ModeNotSet == b64mode) return false;

  const auto b64_tbl =
      (b64mode == kBase64ModeStandard) ? std_decode_table : url_decode_table;

  auto src_size = text.size();
  auto src = text.c_str();

  // The base64 decoder transforms 4 encoded characters to 3 data bytes.
  // Ignore padding: "abc="->"abc", "ab=="->"ab".
  if (0 != src_size && 0 == (src_size % 4)) {
    FLATBUFFERS_ASSERT(src_size >= 4);
    if ('=' == text[src_size - 1]) src_size -= 1;
    if ('=' == text[src_size - 1]) src_size -= 1;
  }

  const auto C4full = src_size / 4;
  const auto C4rem = (src_size % 4);
  FLATBUFFERS_ASSERT((C4full * 4 + C4rem) == src_size);
  const auto dest_size = (src_size * 3) / 4;

  // C4rem is number of remained characters in the last char[4] block.
  // C4rem==1 is forbidden, one data byte encoded by two symbols.
  if ((0 == dest_size) || (1 == C4rem)) return false;

  // Create Vector<uint8_t>.
  uint8_t *dst = nullptr;
  *ovalue = _builder->CreateUninitializedVector(dest_size, 1, &dst);

  for (auto k = C4full * 0; k < C4full; k++) {
    uint8_t a0 = b64_tbl[static_cast<uint8_t>(src[0])];
    uint8_t a1 = b64_tbl[static_cast<uint8_t>(src[1])];
    uint8_t a2 = b64_tbl[static_cast<uint8_t>(src[2])];
    uint8_t a3 = b64_tbl[static_cast<uint8_t>(src[3])];
    // Early return if an invalid symbol in the input.
    if (0x80 & (a0 | a1 | a2 | a3)) return false;
    dst[0] = (a0 << 2) | (a1 >> 4);  // a0[5:0]+a1[5:4]
    dst[1] = (a1 << 4) | (a2 >> 2);  // a1[3:0]+a2[5:2]
    dst[2] = (a2 << 6) | a3;         // a2[1:0]+a3[5:0]
    src += 4;
    dst += 3;
  }

  if (C4rem > 0) {
    FLATBUFFERS_ASSERT(1 < C4rem && C4rem < 4);
    uint8_t a0 = b64_tbl[static_cast<uint8_t>(src[0])];
    uint8_t a1 = (C4rem > 1) ? b64_tbl[static_cast<uint8_t>(src[1])] : 0;
    uint8_t a2 = (C4rem > 2) ? b64_tbl[static_cast<uint8_t>(src[2])] : 0;
    // Early return if an invalid symbol in the input.
    if (0x80 & (a0 | a1 | a2)) return false;
    // Number of data bytes equal to (C4rem-1).
    dst[0] = (a0 << 2) | (a1 >> 4);                 // a0[5:0]+a1[5:4]
    if (C4rem > 2) dst[1] = (a1 << 4) | (a2 >> 2);  // a1[3:0]+a2[5:2]
  }
  return true;
}

}  // namespace flatbuffers
