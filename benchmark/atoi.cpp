#include "zoo/swar/SWAR.h"
#include "zoo/swar/associative_iteration.h"

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

// Copied from Daniel Lemire's GitHub at
// https://lemire.me/blog/2018/10/03/quickly-parsing-eight-digits/
// https://github.com/lemire/Code-used-on-Daniel-Lemire-s-blog/blob/ddb082981228f7256e9a4dbbf56fd4a335d78e30/2018/10/03/eightchartoi.c#L26C1-L34C2

uint32_t parse_eight_digits_swar(const char *chars) {
  uint64_t val;
  memcpy(&val, chars, 8);
  val = val - 0x3030303030303030;
  uint64_t byte10plus   = ((val        * (1 + (0xa  <<  8))) >>  8) & 0x00FF00FF00FF00FF;
  uint64_t short100plus = ((byte10plus * (1 + (0x64 << 16))) >> 16) & 0x0000FFFF0000FFFF;
  short100plus *= (1 + (10000ULL << 32));
  return short100plus >> 32;
}

// Note: eight digits can represent from 0 to (10^9) - 1, the logarithm base 2
// of 10^9 is slightly less than 30, thus, only 30 bits are needed.
auto lemire_as_zoo_swar(const char *chars) {
    uint64_t bytes;
    memcpy(&bytes, chars, 8);
    auto allCharacterZero = zoo::meta::BitmaskMaker<uint64_t, '0', 8>::value;
    using S8_64 = zoo::swar::SWAR<8, uint64_t>;
    S8_64 convertedToIntegers = S8_64{bytes - allCharacterZero};
    /* the idea is to perform the following multiplication:
     * NOTE: THE BASE OF THE NUMBERS is 256 (2^8), then 65536 (2^16), 2^32
     * convertedToIntegers is IN BASE 256 the number ABCDEFGH
     * BASE256:   A    B    C    D    E    F    G    H *
     * BASE256                                 10    1 =
     *           --------------------------------------
     * BASE256  1*A  1*B  1*C  1*D  1*E  1*F  1*G  1*H +
     * BASE256 10*B 10*C 10*D 10*E 10*F 10*G 10*H    0
     *           --------------------------------------
     * BASE256 A+10B ....................... G+10H   H
     * See that the odd-digits (base256) contain 10*odd + even
     * Then, we can use base(2^16) digits, and base(2^32) to
     * calculate the conversion for the digits in 100s and 10,000s
     */
    auto by11base256 = convertedToIntegers.multiply(256*10 + 1);
    auto bytePairs = zoo::swar::doublePrecision(by11base256).odd;
    static_assert(std::is_same_v<decltype(bytePairs), zoo::swar::SWAR<16, uint64_t>>);
    auto by101base2to16 = bytePairs.multiply(1 + (100 << 16));
    auto byteQuads = zoo::swar::doublePrecision(by101base2to16).odd;
    auto by10001base2to32 = byteQuads.multiply(1 + (10000ull << 32));
    return uint32_t(by10001base2to32.value() >> 32);
}
