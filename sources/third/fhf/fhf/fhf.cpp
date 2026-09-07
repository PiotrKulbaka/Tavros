/**
 * @file   hp-fp.cpp
 * @author Dennis Sitelew 
 * @date   Mar 19, 2024
 */

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <limits>

#include "fhf.hpp"

// Based on http://www.fox-toolkit.org/ftp/fasthalffloatconversion.pdf
// ("Fast Half Float Conversions") referenced by the Wikipedia article on
// half-precision floating point:
// https://en.wikipedia.org/wiki/Half-precision_floating-point_format

namespace {

////////////////////////////////////////////////////////////////////////////////
/// Lookup Table Helper
////////////////////////////////////////////////////////////////////////////////
// Based on https://joelfilho.com/blog/2020/compile_time_lookup_tables_in_cpp/
template <std::size_t Length, typename Generator>
constexpr auto make_lookup_table(Generator &&f) {
   using content_type = decltype(f(std::size_t{0}));
   std::array<content_type, Length> arr{};

   for (std::size_t i = 0; i < Length; i++) {
      arr[i] = f(i);
   }

   return arr;
}

////////////////////////////////////////////////////////////////////////////////
/// Mantissa Table
////////////////////////////////////////////////////////////////////////////////
// Transform the subnormal representation to a normalized one
constexpr auto convert_mantissa(std::size_t i) -> std::uint32_t {
   std::uint32_t m = i << 13; // Zero pad mantissa bits
   std::uint32_t e = 0;       // Zero exponent

   while (!(m & 0x00800000)) { // While not normalized
      e -= 0x00800000;         // Decrement exponent (1<<23)
      m <<= 1;                 // Shift mantissa
   }

   m &= ~0x00800000; // Clear leading 1 bit
   e += 0x38800000;  // Adjust bias ((127-14)<<23)

   return m | e;
}

constexpr auto mantissa_table = make_lookup_table<2048>([](std::size_t i) -> std::uint32_t {
   if (i == 0) {
      return 0;
   } else if (i >= 1 && i <= 1023) {
      return convert_mantissa(i);
   } else {
      return 0x38000000 + ((i - 1024) << 13);
   }
});

static_assert(mantissa_table[0] == 0);
static_assert(mantissa_table[1] == 0x33800000);
static_assert(mantissa_table[1023] == 0x387fc000);
static_assert(mantissa_table[1025] == 0x38002000);

////////////////////////////////////////////////////////////////////////////////
/// Offset Table
////////////////////////////////////////////////////////////////////////////////
constexpr auto offset_table = make_lookup_table<64>([](std::size_t i) -> std::uint16_t {
   if (i == 0 || i == 32) {
      return 0;
   } else {
      return 1024;
   }
});

static_assert(offset_table[0] == 0);
static_assert(offset_table[1] == 1024);
static_assert(offset_table[32] == 0);

////////////////////////////////////////////////////////////////////////////////
/// Exponent Table
////////////////////////////////////////////////////////////////////////////////
constexpr auto exponent_table = make_lookup_table<64>([](std::size_t i) -> std::uint32_t {
   switch (i) {
      case 0:
         return 0;
      case 32:
         return 0x80000000;
      case 31:
         return 0x47800000;
      case 63:
         return 0xC7800000;
      default:
         break;
   }

   if (i >= 1 && i <= 30) {
      return i << 23;
   }

   return 0x80000000 + ((i - 32) << 23);
});

static_assert(exponent_table[0] == 0);
static_assert(exponent_table[7] == 0x03800000);
static_assert(exponent_table[30] == 0x0F000000);
static_assert(exponent_table[31] == 0x47800000);
static_assert(exponent_table[32] == 0x80000000);
static_assert(exponent_table[33] == 0x80800000);
static_assert(exponent_table[62] == 0x8F000000);
static_assert(exponent_table[63] == 0xC7800000);

////////////////////////////////////////////////////////////////////////////////
/// Base Table
////////////////////////////////////////////////////////////////////////////////
constexpr auto base_table = make_lookup_table<512>([](std::size_t i) -> std::uint16_t {
   const bool low_value = (i < 0x100);
   const int e = static_cast<int>(i) - (low_value ? 127 : 383);

   if (e < -24) { // Very small numbers map to zero
      if (low_value) {
         return 0x0000;
      } else {
         return 0x8000;
      }
   } else if (e < -14) { // Small numbers map to denorms
      const uint16_t base = (0x0400 >> (-e - 14));
      if (low_value) {
         return base;
      } else {
         return base | 0x8000;
      }
   } else if (e <= 15) { // Normal numbers just lose precision
      const uint16_t base = ((e + 15) << 10);
      if (low_value) {
         return base;
      } else {
         return base | 0x8000;
      }
   } else {
      // Large numbers map to infinity.
      // Infinity and NaN's stay Infinity and NaN's.
      if (low_value) {
         return 0x7C00;
      } else {
         return 0xFC00;
      }
   }
});

static_assert(base_table[0] == 0x0000);
static_assert(base_table[102] == 0x0000);
static_assert(base_table[256] == 0x8000);
static_assert(base_table[358] == 0x8000);
static_assert(base_table[103] == 0x0001);
static_assert(base_table[359] == 0x8001);
static_assert(base_table[112] == 0x0200);
static_assert(base_table[368] == 0x8200);
static_assert(base_table[129] == 0x4400);
static_assert(base_table[385] == 0xC400);
static_assert(base_table[144] == 0x7C00);
static_assert(base_table[400] == 0xFC00);
static_assert(base_table[255] == 0x7C00);
static_assert(base_table[511] == 0xFC00);

////////////////////////////////////////////////////////////////////////////////
/// Class: Shift Table
////////////////////////////////////////////////////////////////////////////////
constexpr auto shift_table = make_lookup_table<512>([](std::size_t i) -> std::uint16_t {
   const bool low_value = (i < 0x100);
   const int e = static_cast<int>(i) - (low_value ? 127 : 383);

   if (e < -24) { // Very small numbers map to zero
      return 24;
   } else if (e < -14) { // Small numbers map to denorms
      return -e - 1;
   } else if (e <= 15) { // Normal numbers just lose precision
      return 13;
   } else if (e < 128) { // Large numbers map to Infinity
      return 24;
   } else { // Infinity and NaN's stay Infinity and NaN's
      return 13;
   }
});

static_assert(shift_table[0] == 24);
static_assert(shift_table[102] == 24);
static_assert(shift_table[256] == 24);
static_assert(shift_table[358] == 24);
static_assert(shift_table[103] == 23);
static_assert(shift_table[359] == 23);
static_assert(shift_table[112] == 14);
static_assert(shift_table[368] == 14);
static_assert(shift_table[129] == 13);
static_assert(shift_table[385] == 13);
static_assert(shift_table[144] == 24);
static_assert(shift_table[400] == 24);
static_assert(shift_table[255] == 13);
static_assert(shift_table[511] == 13);

} // namespace

namespace detail {
inline constexpr std::uint16_t pack(float f) {
   static_assert(sizeof(std::uint32_t) == sizeof(float));
   const auto fb = std::bit_cast<std::uint32_t>(f);

   const std::uint32_t exp = (fb >> 23) & 0x1FF;
   const std::uint32_t sig = fb & 0x007FFFFF;

   const std::uint32_t res = base_table[exp] + (sig >> shift_table[exp]);
   return static_cast<std::uint16_t>(res);
}

static_assert(pack(0.0f) == 0x0000);
static_assert(pack(-0.0f) == 0x8000);
static_assert(pack(1.0f) == 0x3C00);
static_assert(pack(-0.5f) == 0xB800);
static_assert(pack(-2.0f) == 0xC000);
static_assert(pack(65504.0f) == 0x7BFF);
static_assert(pack(std::numeric_limits<float>::infinity()) == 0x7C00);
static_assert(pack(-std::numeric_limits<float>::infinity()) == 0xFC00);

////////////////////////////////////////////////////////////////////////////////
/// Unpack
////////////////////////////////////////////////////////////////////////////////
inline constexpr float unpack(std::uint16_t h) {
   static_assert(sizeof(std::uint32_t) == sizeof(float));

   const std::uint32_t exp = (h >> 10) & 0x3F;
   const std::uint32_t sig = h & 0x3FF;

   const std::uint32_t res = mantissa_table[offset_table[exp] + sig] + exponent_table[exp];
   return std::bit_cast<float>(res);
}

static_assert(unpack(0x0000) == 0.0f);
static_assert(unpack(0x8000) == -0.0f);
static_assert(unpack(0x3C00) == 1.0f);
static_assert(unpack(0xB800) == -0.5f);
static_assert(unpack(0xC000) == -2.0f);
static_assert(unpack(0x7BFF) == 65504.0f);
static_assert(unpack(0x7C00) == std::numeric_limits<float>::infinity());
static_assert(unpack(0xFC00) == -std::numeric_limits<float>::infinity());
} // namespace detail

namespace fhf {

std::uint16_t pack(float f) {
   return detail::pack(f);
}

float unpack(std::uint16_t h) {
   return detail::unpack(h);
}

} // namespace fhf

////////////////////////////////////////////////////////////////////////////////
/// C-Interface
////////////////////////////////////////////////////////////////////////////////
uint16_t fhf_pack(float f) {
   return detail::pack(f);
}

float fhf_unpack(uint16_t h) {
   return detail::unpack(h);
}
