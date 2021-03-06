// http://www.iwar.org.uk/comsec/resources/cipher/sha256-384-512.pdf
// https://akkadia.org/drepper/SHA-crypt.txt
//g++ -std=c++11  sha256_code_release/sha256_sse4.o  sha256_code_release/sha256_avx2_rorx2.o sha256_code_release/sha256_avx1.o sha256_code_release/sha256_avx2_rorx8.o sha2.cpp

#include <assert.h>
#include "SHA512.h"


static const SHA512Hash SHA512_INIT = {
  {
    0x6a09e667f3bcc908,
    0xbb67ae8584caa73b,
    0x3c6ef372fe94f82b,
    0xa54ff53a5f1d36f1,
    0x510e527fade682d1,
    0x9b05688c2b3e6c1f,
    0x1f83d9abfb41bd6b,
    0x5be0cd19137e2179
  }
};

// The first thirty-two bits of the fractional parts of the cube roots of the first sixty-four primes.
static const uint64_t K[80] = {
  0x428a2f98d728ae22, 0x7137449123ef65cd, 0xb5c0fbcfec4d3b2f, 0xe9b5dba58189dbbc,
  0x3956c25bf348b538, 0x59f111f1b605d019, 0x923f82a4af194f9b, 0xab1c5ed5da6d8118,
  0xd807aa98a3030242, 0x12835b0145706fbe, 0x243185be4ee4b28c, 0x550c7dc3d5ffb4e2,
  0x72be5d74f27b896f, 0x80deb1fe3b1696b1, 0x9bdc06a725c71235, 0xc19bf174cf692694,
  0xe49b69c19ef14ad2, 0xefbe4786384f25e3, 0x0fc19dc68b8cd5b5, 0x240ca1cc77ac9c65,
  0x2de92c6f592b0275, 0x4a7484aa6ea6e483, 0x5cb0a9dcbd41fbd4, 0x76f988da831153b5,
  0x983e5152ee66dfab, 0xa831c66d2db43210, 0xb00327c898fb213f, 0xbf597fc7beef0ee4,
  0xc6e00bf33da88fc2, 0xd5a79147930aa725, 0x06ca6351e003826f, 0x142929670a0e6e70,
  0x27b70a8546d22ffc, 0x2e1b21385c26c926, 0x4d2c6dfc5ac42aed, 0x53380d139d95b3df,
  0x650a73548baf63de, 0x766a0abb3c77b2a8, 0x81c2c92e47edaee6, 0x92722c851482353b,
  0xa2bfe8a14cf10364, 0xa81a664bbc423001, 0xc24b8b70d0f89791, 0xc76c51a30654be30,
  0xd192e819d6ef5218, 0xd69906245565a910, 0xf40e35855771202a, 0x106aa07032bbd1b8,
  0x19a4c116b8d2d0c8, 0x1e376c085141ab53, 0x2748774cdf8eeb99, 0x34b0bcb5e19b48a8,
  0x391c0cb3c5c95a63, 0x4ed8aa4ae3418acb, 0x5b9cca4f7763e373, 0x682e6ff3d6b2b8a3,
  0x748f82ee5defb2fc, 0x78a5636f43172f60, 0x84c87814a1f0ab72, 0x8cc702081a6439ec,
  0x90befffa23631e28, 0xa4506cebde82bde9, 0xbef9a3f7b2c67915, 0xc67178f2e372532b,
  0xca273eceea26619c, 0xd186b8c721c0c207, 0xeada7dd6cde0eb1e, 0xf57d4f7fee6ed178,
  0x06f067aa72176fba, 0x0a637dc5a2c898a6, 0x113f9804bef90dae, 0x1b710b35131c471b,
  0x28db77f523047d84, 0x32caab7b40c72493, 0x3c9ebe0a15c9bebc, 0x431d67c49c100d4c,
  0x4cc5d4becb3e42b6, 0x597f299cfc657e2a, 0x5fcb6fab3ad6faec, 0x6c44198c4a475817,
};




SHA512Hasher::SHA512Hasher() {
  reset();
  memset_u8(buf, 0, BLOCK_SIZE);
}


void SHA512Hasher::reset() {
  state = SHA512_INIT;
  bsize = 0;
  total = 0;
}


SHA512Hash SHA512Hasher::digest() {
  buf[bsize++] = 0x80;
  // zero out buffer
  if (BLOCK_SIZE - bsize < 2*sizeof(uint64_t)) { // No room
    hashBlock(); //update
  }
  uint64_t size = total*8;
  // TODO unroll this
LOOP_U64:
  for (int i=0; i < sizeof(uint64_t); i++) {
    buf[(BLOCK_SIZE - 1) - i] = (size & 0xff);
    size >>= 8;
  }
  hashBlock(); //update

  return state;
}

void SHA512Hasher::byte_digest(uint8_t buf[64]) {
  digest();
LOOP_DIGEST:
  for (int i=0; i < 8; i++) {
    uint64_t curr = state.hash[i];
LOOP_U64:
    for (int j=0; j < sizeof(uint64_t); j++) {
      buf[sizeof(uint64_t)*(i+1) - 1 - j] = curr & 0xff;
      curr >>= 8;
    }
  }
}

// SHA512ByteHash SHA512Hasher::byte_digest() {
//   digest();
//   SHA512ByteHash ret;
// LOOP_DIGEST:
//   for (int i=0; i < 8; i++) {
//     uint64_t curr = state.hash[i];
// LOOP_U64:
//     for (int j=0; j < sizeof(uint64_t); j++) {
//       ret.hash[sizeof(uint64_t)*(i+1) - 1 - j] = curr & 0xff;
//       curr >>= 8;
//     }
//   }
//   return ret;
// }


void SHA512Hasher::buf_cpy(uint8_t offset, const uint8_t *src, uint8_t len) {
  for (int i=0; i < len; i++) {
    #pragma HLS unroll factor=8
    buf[offset + i] = src[i];
  }
}


void SHA512Hasher::update(const uint8_t *msg, uint8_t len) {
  assert (len <= BLOCK_SIZE);
  uint8_t remain = BLOCK_SIZE - bsize;
  uint8_t tocpy = MIN(remain, len);
  buf_cpy(bsize, msg, tocpy);

  if (tocpy < len || len == remain) { // Not enough room or full
    hashBlock();
    bsize = len - tocpy;
    buf_cpy(0, msg + tocpy, bsize);
  } else { // Enough room
    bsize += len;
  }

  total += len;
}


void SHA512Hasher::hashBlock() {
  uint64_t a = state.hash[0];
  uint64_t b = state.hash[1];
  uint64_t c = state.hash[2];
  uint64_t d = state.hash[3];
  uint64_t e = state.hash[4];
  uint64_t f = state.hash[5];
  uint64_t g = state.hash[6];
  uint64_t h = state.hash[7];

  uint64_t W[16];

  // Do first 16 rounds
LOOP16:
  for (int j=0; j < 16; j++) {
    uint64_t wcurr = read64clear(buf, sizeof(uint64_t)*j);
    W[j] = wcurr;
    uint64_t T1 = h + CSigma1(e) + Ch(e, f, g) + K[j] + wcurr;
    uint64_t T2 = CSigma0(a) + Maj(a, b, c);
    h = g;
    g = f;
    f = e;
    e = d + T1;
    d = c;
    c = b;
    b = a;
    a = T1 + T2;
  }

  // Do last 64 rounds
LOOP64:
  for (int j=16; j < 80; j++) {
    uint64_t wnext = LSigma1(W[14]) + W[9] + LSigma0(W[1]) + W[0];
    // Shift register
LOOP_SHIFT:
    for (int i=0; i < 16-1; i++) {
      W[i] = W[i+1];
    }
    W[15] = wnext;

    uint64_t T1 = h + CSigma1(e) + Ch(e, f, g) + K[j] + wnext;
    uint64_t T2 = CSigma0(a) + Maj(a, b, c);
    h = g;
    g = f;
    f = e;
    e = d + T1;
    d = c;
    c = b;
    b = a;
    a = T1 + T2;
  }

  state.hash[0] += a;
  state.hash[1] += b;
  state.hash[2] += c;
  state.hash[3] += d;
  state.hash[4] += e;
  state.hash[5] += f;
  state.hash[6] += g;
  state.hash[7] += h;
}
