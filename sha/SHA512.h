
#pragma once

#include <stdint.h>

#include <string.h>
#include "helpers.h"

struct SHA512Hash {
  uint64_t hash[8];

  bool operator==(const SHA512Hash& rhs)
  {
      return !memcmp(hash, rhs.hash, sizeof(uint64_t[8]));
  }
};

struct SHA512ByteHash {
  uint8_t hash[64];
};


class SHA512Hasher {
public:
  SHA512Hasher();
  void reset();
  // len <= 128
  SHA512Hash digest();
  SHA512ByteHash byte_digest();

  template <int MAX_LEN>
  void update(const void *msgp, uint8_t len) {
    assert (len <= MAX_LEN);
    uint8_t *msg = (uint8_t*)msgp;
    uint8_t remain = BLOCK_SIZE - bsize;
    uint8_t tocpy = MIN(remain, len);
    memcpy_u8<MAX_LEN>(buf+bsize, msg, tocpy);

    if (tocpy < len || len == remain) { // Not enough room or full
      hashBlock();
      bsize = len - tocpy;
      memcpy_u8<MAX_LEN>(buf, msg + tocpy, bsize);
    } else { // Enough room
      bsize += len;
    }

    total += len;
  }

  static const uint8_t HASH_SIZE = 64;

private:
  static const uint8_t BLOCK_SIZE = 128;
  SHA512Hash state;
  uint8_t buf[BLOCK_SIZE]; // TODO: This should be partitioned in chunks of 8
  uint8_t bsize;
  uint64_t total;

  void hashBlock();
  // Rotate right n
  static inline uint64_t Sn(uint64_t x, int n) { return (x >> n) | (x << (64 - n)); }
  static inline uint64_t Rn(uint64_t x, int n) { return x >> n; }

  static inline uint64_t Ch(uint64_t x, uint64_t y, uint64_t z) { return (x & y) ^ (~x & z); }
  static inline uint64_t Maj(uint64_t x, uint64_t y, uint64_t z) { return (x & y) ^ (x & z) ^ (y & z); }
  static inline uint64_t CSigma0(uint64_t x) { return Sn(x, 28)^Sn(x, 34)^Sn(x, 39); }
  static inline uint64_t CSigma1(uint64_t x) { return Sn(x, 14)^Sn(x, 18)^Sn(x, 41); }
  static inline uint64_t LSigma0(uint64_t x) { return Sn(x, 1)^Sn(x, 8)^Rn(x, 7); }
  static inline uint64_t LSigma1(uint64_t x) { return Sn(x, 19)^Sn(x, 61)^Rn(x, 6); }

};
