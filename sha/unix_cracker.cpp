#include <stdio.h>
#include <assert.h>
//#include <stdlib.h> for crypt()
#include "SHA512.h"
#include "unix_cracker.h"

static const uint8_t P[] = {
  42, 21,  0,  1, 43, 22, 23,  2, 44,
  45, 24,  3,  4, 46, 25, 26,  5, 47,
  48, 27,  6,  7, 49, 28, 29,  8, 50,
  51, 30,  9, 10, 52, 31, 32, 11, 53,
  54, 33, 12, 13, 55, 34, 35, 14, 56,
  57, 36, 15, 16, 58, 37, 38, 17, 59,
  60, 39, 18, 19, 61, 40, 41, 20, 62,
  63
};

static const char b64t[65] =
"./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

static void update_hack(SHA512Hasher &hasher, const uint8_t *buf, uint8_t len) {
  #pragma HLS inline off
  uint8_t stupid_vivado[SHA512Hasher::BLOCK_SIZE];
  memcpy_u8(stupid_vivado, buf, len);
  hasher.update(stupid_vivado, len);
}

static inline void runIters(SHA512Hasher &hasher,
                                      int slen, int pwlen,
                                      uint8_t A[SHA512Hasher::HASH_SIZE],
                                      uint8_t DS[SHA512Hasher::HASH_SIZE],
                                      uint8_t DP[SHA512Hasher::HASH_SIZE],
                                      int nrounds=5000) {

  uint8_t twos = 0;
  uint8_t threes = 0;
  uint8_t sevens = 0;
  for (int i=0; i < nrounds; i++) {
    hasher.reset();

    if (twos) {
      hasher.update(DP, pwlen);
    } else {
      hasher.update(A, SHA512Hasher::HASH_SIZE);
    }

    if (threes) {
      hasher.update(DS, slen);
    }

    if (sevens) {
      hasher.update(DP, pwlen);
    }

    if (twos == 0) {
      hasher.update(DP, pwlen);
    } else {
      hasher.update(A, SHA512Hasher::HASH_SIZE);
    }
    hasher.byte_digest(A);

    twos = twos ? 0 : 1;
    threes = threes == 2 ? 0 : threes + 1;
    sevens = sevens == 6 ? 0 : sevens + 1;
  }
}

void calc(char hash[86], const uint8_t pwd[MAX_PWD_LEN], const uint8_t pwlen, const uint8_t salt[MAX_SALT_LEN], const uint8_t slen, int nrounds) {
  assert(pwlen <= 64);
  assert(slen <= 64);
  // Compute B
  SHA512Hasher hasher;
  update_hack(hasher, pwd, pwlen);
  update_hack(hasher, salt, slen);
  update_hack(hasher, pwd, pwlen);
  uint8_t B[SHA512Hasher::HASH_SIZE];
  hasher.byte_digest(B);

  hasher.reset();

  // Compute A
  update_hack(hasher, pwd, pwlen);
  update_hack(hasher, salt, slen);
  update_hack(hasher, B, pwlen);

  uint8_t curr = pwlen;
  for (int i=0; i < 8; i++) {
    if (curr) {
      if (curr & 1) {
        update_hack(hasher, B, sizeof(B));
      } else {
        update_hack(hasher, pwd, pwlen);
      }
    }
    curr >>= 1;
  }
  uint8_t A[SHA512Hasher::HASH_SIZE];
  hasher.byte_digest(A);

  hasher.reset();

  // Compute DP
  for (int i=0; i < pwlen; i++) {
    update_hack(hasher, pwd, pwlen);
  }
  uint8_t DP[SHA512Hasher::HASH_SIZE];
  hasher.byte_digest(DP);

  hasher.reset();

  // Compute DS
  for (int i=0; i < 16 + A[0]; i++) {
    update_hack(hasher, salt, slen);
  }
  uint8_t DS[SHA512Hasher::HASH_SIZE];
  hasher.byte_digest(DS);


  // Note P is the first N bytes of DP
  // We reuse A for C
  runIters(hasher, slen, pwlen, A, DS, DP, nrounds);

  // TODO: unroll this
  for (int i=0; i < 21; i++) {
    uint32_t C = A[P[3*i]] | (A[P[3*i + 1]] << 8) | (A[P[3*i + 2]] << 16);
    for (int j=0; j < 4; j++) {
      hash[4*i + j] = b64t[(C >> (6*j)) & 0x3f];
    }
  }
  // Handle last byte
  uint8_t C = A[P[63]];
  hash[84] = b64t[C & 0x3f];
  hash[85] = b64t[C >> 6];

}
