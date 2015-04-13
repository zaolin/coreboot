#ifndef SECURITY_NACL_H
#define SECURITY_NACL_H

#include <stdint.h>

#define FOR(i,n) for (i = 0;i < n;++i)

typedef long long i64;
typedef i64 gf[16];

static const u8
  _0[16],
  _9[32] = {9};
static const gf
  gf0,
  gf1 = {1},
  _121665 = {0xDB41,1},
  D = {0x78a3, 0x1359, 0x4dca, 0x75eb, 0xd8ab, 0x4141, 0x0a4d, 0x0070, 0xe898, 0x7779, 0x4079, 0x8cc7, 0xfe73, 0x2b6f, 0x6cee, 0x5203},
  D2 = {0xf159, 0x26b2, 0x9b94, 0xebd6, 0xb156, 0x8283, 0x149a, 0x00e0, 0xd130, 0xeef3, 0x80f2, 0x198e, 0xfce7, 0x56df, 0xd9dc, 0x2406},
  X = {0xd51a, 0x8f25, 0x2d60, 0xc956, 0xa7b2, 0x9525, 0xc760, 0x692c, 0xdc5c, 0xfdd6, 0xe231, 0xc0a4, 0x53fe, 0xcd6e, 0x36d3, 0x2169},
  Y = {0x6658, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666},
  I = {0xa0b0, 0x4a0e, 0x1b27, 0xc4ee, 0xe478, 0xad2f, 0x1806, 0x2f43, 0xd7a7, 0x3dfb, 0x0099, 0x2b4d, 0xdf0b, 0x4fc1, 0x2480, 0x2b83};

static const u32 minusp[17] = {
  5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 252
} ;

static const u64 K[80] =
{
  0x428a2f98d728ae22ULL, 0x7137449123ef65cdULL, 0xb5c0fbcfec4d3b2fULL, 0xe9b5dba58189dbbcULL,
  0x3956c25bf348b538ULL, 0x59f111f1b605d019ULL, 0x923f82a4af194f9bULL, 0xab1c5ed5da6d8118ULL,
  0xd807aa98a3030242ULL, 0x12835b0145706fbeULL, 0x243185be4ee4b28cULL, 0x550c7dc3d5ffb4e2ULL,
  0x72be5d74f27b896fULL, 0x80deb1fe3b1696b1ULL, 0x9bdc06a725c71235ULL, 0xc19bf174cf692694ULL,
  0xe49b69c19ef14ad2ULL, 0xefbe4786384f25e3ULL, 0x0fc19dc68b8cd5b5ULL, 0x240ca1cc77ac9c65ULL,
  0x2de92c6f592b0275ULL, 0x4a7484aa6ea6e483ULL, 0x5cb0a9dcbd41fbd4ULL, 0x76f988da831153b5ULL,
  0x983e5152ee66dfabULL, 0xa831c66d2db43210ULL, 0xb00327c898fb213fULL, 0xbf597fc7beef0ee4ULL,
  0xc6e00bf33da88fc2ULL, 0xd5a79147930aa725ULL, 0x06ca6351e003826fULL, 0x142929670a0e6e70ULL,
  0x27b70a8546d22ffcULL, 0x2e1b21385c26c926ULL, 0x4d2c6dfc5ac42aedULL, 0x53380d139d95b3dfULL,
  0x650a73548baf63deULL, 0x766a0abb3c77b2a8ULL, 0x81c2c92e47edaee6ULL, 0x92722c851482353bULL,
  0xa2bfe8a14cf10364ULL, 0xa81a664bbc423001ULL, 0xc24b8b70d0f89791ULL, 0xc76c51a30654be30ULL,
  0xd192e819d6ef5218ULL, 0xd69906245565a910ULL, 0xf40e35855771202aULL, 0x106aa07032bbd1b8ULL,
  0x19a4c116b8d2d0c8ULL, 0x1e376c085141ab53ULL, 0x2748774cdf8eeb99ULL, 0x34b0bcb5e19b48a8ULL,
  0x391c0cb3c5c95a63ULL, 0x4ed8aa4ae3418acbULL, 0x5b9cca4f7763e373ULL, 0x682e6ff3d6b2b8a3ULL,
  0x748f82ee5defb2fcULL, 0x78a5636f43172f60ULL, 0x84c87814a1f0ab72ULL, 0x8cc702081a6439ecULL,
  0x90befffa23631e28ULL, 0xa4506cebde82bde9ULL, 0xbef9a3f7b2c67915ULL, 0xc67178f2e372532bULL,
  0xca273eceea26619cULL, 0xd186b8c721c0c207ULL, 0xeada7dd6cde0eb1eULL, 0xf57d4f7fee6ed178ULL,
  0x06f067aa72176fbaULL, 0x0a637dc5a2c898a6ULL, 0x113f9804bef90daeULL, 0x1b710b35131c471bULL,
  0x28db77f523047d84ULL, 0x32caab7b40c72493ULL, 0x3c9ebe0a15c9bebcULL, 0x431d67c49c100d4cULL,
  0x4cc5d4becb3e42b6ULL, 0x597f299cfc657e2aULL, 0x5fcb6fab3ad6faecULL, 0x6c44198c4a475817ULL
};

static const u8 iv[64] = {
  0x6a,0x09,0xe6,0x67,0xf3,0xbc,0xc9,0x08,
  0xbb,0x67,0xae,0x85,0x84,0xca,0xa7,0x3b,
  0x3c,0x6e,0xf3,0x72,0xfe,0x94,0xf8,0x2b,
  0xa5,0x4f,0xf5,0x3a,0x5f,0x1d,0x36,0xf1,
  0x51,0x0e,0x52,0x7f,0xad,0xe6,0x82,0xd1,
  0x9b,0x05,0x68,0x8c,0x2b,0x3e,0x6c,0x1f,
  0x1f,0x83,0xd9,0xab,0xfb,0x41,0xbd,0x6b,
  0x5b,0xe0,0xcd,0x19,0x13,0x7e,0x21,0x79
};

static const u64 L[32] = {0xed, 0xd3, 0xf5, 0x5c, 0x1a, 0x63, 0x12, 0x58, 0xd6, 0x9c, 0xf7, 0xa2, 0xde, 0xf9, 0xde, 0x14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x10};

/*
 * Cryptographic Primitives
 */

extern u64 R(u64 x,int c);
extern u64 Ch(u64 x,u64 y,u64 z);
extern u64 Maj(u64 x,u64 y,u64 z);
extern u64 Sigma0(u64 x);
extern u64 Sigma1(u64 x);
extern u64 sigma0(u64 x);
extern u64 sigma1(u64 x);

extern u64 dl64(const u8 *x);
extern void ts64(u8 *x,u64 u);

extern int unpackneg(gf r[4],const u8 p[32]);
extern void reduce(u8 *r);
extern void scalarbase(gf p[4],const u8 *s);
extern void scalarmult(gf p[4],gf q[4],const u8 *s);
extern void pack(u8 *r,gf p[4]);
extern void add(gf p[4],gf q[4]);

/*
 * Cryptographic Functions
 */

#define crypto_auth_PRIMITIVE "hmacsha512256"
#define crypto_auth crypto_auth_hmacsha512256
#define crypto_auth_verify crypto_auth_hmacsha512256_verify
#define crypto_auth_BYTES crypto_auth_hmacsha512256_BYTES
#define crypto_auth_KEYBYTES crypto_auth_hmacsha512256_KEYBYTES
#define crypto_auth_IMPLEMENTATION crypto_auth_hmacsha512256_IMPLEMENTATION
#define crypto_auth_VERSION crypto_auth_hmacsha512256_VERSION
#define crypto_auth_hmacsha512256_tweet_BYTES 32
#define crypto_auth_hmacsha512256_tweet_KEYBYTES 32
extern int crypto_auth_hmacsha512256_tweet(unsigned char *,const unsigned char *,unsigned long long,const unsigned char *);
extern int crypto_auth_hmacsha512256_tweet_verify(const unsigned char *,const unsigned char *,unsigned long long,const unsigned char *);
#define crypto_auth_hmacsha512256_tweet_VERSION "-"
#define crypto_auth_hmacsha512256 crypto_auth_hmacsha512256_tweet
#define crypto_auth_hmacsha512256_verify crypto_auth_hmacsha512256_tweet_verify
#define crypto_auth_hmacsha512256_BYTES crypto_auth_hmacsha512256_tweet_BYTES
#define crypto_auth_hmacsha512256_KEYBYTES crypto_auth_hmacsha512256_tweet_KEYBYTES
#define crypto_auth_hmacsha512256_VERSION crypto_auth_hmacsha512256_tweet_VERSION
#define crypto_auth_hmacsha512256_IMPLEMENTATION "crypto_auth/hmacsha512256/tweet"
#define crypto_hashblocks_PRIMITIVE "sha512"
#define crypto_hashblocks crypto_hashblocks_sha512
#define crypto_hashblocks_STATEBYTES crypto_hashblocks_sha512_STATEBYTES
#define crypto_hashblocks_BLOCKBYTES crypto_hashblocks_sha512_BLOCKBYTES
#define crypto_hashblocks_IMPLEMENTATION crypto_hashblocks_sha512_IMPLEMENTATION
#define crypto_hashblocks_VERSION crypto_hashblocks_sha512_VERSION
#define crypto_hashblocks_sha512_tweet_STATEBYTES 64
#define crypto_hashblocks_sha512_tweet_BLOCKBYTES 128
extern int crypto_hashblocks_sha512_tweet(unsigned char *,const unsigned char *,unsigned long long);
#define crypto_hashblocks_sha512_tweet_VERSION "-"
#define crypto_hashblocks_sha512 crypto_hashblocks_sha512_tweet
#define crypto_hashblocks_sha512_STATEBYTES crypto_hashblocks_sha512_tweet_STATEBYTES
#define crypto_hashblocks_sha512_BLOCKBYTES crypto_hashblocks_sha512_tweet_BLOCKBYTES
#define crypto_hashblocks_sha512_VERSION crypto_hashblocks_sha512_tweet_VERSION
#define crypto_hashblocks_sha512_IMPLEMENTATION "crypto_hashblocks/sha512/tweet"
#define crypto_hashblocks_sha256_tweet_STATEBYTES 32
#define crypto_hashblocks_sha256_tweet_BLOCKBYTES 64
extern int crypto_hashblocks_sha256_tweet(unsigned char *,const unsigned char *,unsigned long long);
#define crypto_hashblocks_sha256_tweet_VERSION "-"
#define crypto_hashblocks_sha256 crypto_hashblocks_sha256_tweet
#define crypto_hashblocks_sha256_STATEBYTES crypto_hashblocks_sha256_tweet_STATEBYTES
#define crypto_hashblocks_sha256_BLOCKBYTES crypto_hashblocks_sha256_tweet_BLOCKBYTES
#define crypto_hashblocks_sha256_VERSION crypto_hashblocks_sha256_tweet_VERSION
#define crypto_hashblocks_sha256_IMPLEMENTATION "crypto_hashblocks/sha256/tweet"
#define crypto_hash_PRIMITIVE "sha512"
#define crypto_hash crypto_hash_sha512
#define crypto_hash_BYTES crypto_hash_sha512_BYTES
#define crypto_hash_IMPLEMENTATION crypto_hash_sha512_IMPLEMENTATION
#define crypto_hash_VERSION crypto_hash_sha512_VERSION
#define crypto_hash_sha512_tweet_BYTES 64
extern int crypto_hash_sha512_tweet(unsigned char *,const unsigned char *,unsigned long long);
#define crypto_hash_sha512_tweet_VERSION "-"
#define crypto_hash_sha512 crypto_hash_sha512_tweet
#define crypto_hash_sha512_BYTES crypto_hash_sha512_tweet_BYTES
#define crypto_hash_sha512_VERSION crypto_hash_sha512_tweet_VERSION
#define crypto_hash_sha512_IMPLEMENTATION "crypto_hash/sha512/tweet"
#define crypto_hash_sha256_tweet_BYTES 32
extern int crypto_hash_sha256_tweet(unsigned char *,const unsigned char *,unsigned long long);
#define crypto_hash_sha256_tweet_VERSION "-"
#define crypto_hash_sha256 crypto_hash_sha256_tweet
#define crypto_hash_sha256_BYTES crypto_hash_sha256_tweet_BYTES
#define crypto_hash_sha256_VERSION crypto_hash_sha256_tweet_VERSION
#define crypto_hash_sha256_IMPLEMENTATION "crypto_hash/sha256/tweet"
#define crypto_scalarmult_PRIMITIVE "curve25519"
#define crypto_scalarmult crypto_scalarmult_curve25519
#define crypto_scalarmult_base crypto_scalarmult_curve25519_base
#define crypto_scalarmult_BYTES crypto_scalarmult_curve25519_BYTES
#define crypto_scalarmult_SCALARBYTES crypto_scalarmult_curve25519_SCALARBYTES
#define crypto_scalarmult_IMPLEMENTATION crypto_scalarmult_curve25519_IMPLEMENTATION
#define crypto_scalarmult_VERSION crypto_scalarmult_curve25519_VERSION
#define crypto_scalarmult_curve25519_tweet_BYTES 32
#define crypto_scalarmult_curve25519_tweet_SCALARBYTES 32
extern int crypto_scalarmult_curve25519_tweet(unsigned char *,const unsigned char *,const unsigned char *);
extern int crypto_scalarmult_curve25519_tweet_base(unsigned char *,const unsigned char *);
#define crypto_scalarmult_curve25519_tweet_VERSION "-"
#define crypto_scalarmult_curve25519 crypto_scalarmult_curve25519_tweet
#define crypto_scalarmult_curve25519_base crypto_scalarmult_curve25519_tweet_base
#define crypto_scalarmult_curve25519_BYTES crypto_scalarmult_curve25519_tweet_BYTES
#define crypto_scalarmult_curve25519_SCALARBYTES crypto_scalarmult_curve25519_tweet_SCALARBYTES
#define crypto_scalarmult_curve25519_VERSION crypto_scalarmult_curve25519_tweet_VERSION
#define crypto_scalarmult_curve25519_IMPLEMENTATION "crypto_scalarmult/curve25519/tweet"
#define crypto_sign_PRIMITIVE "ed25519"
#define crypto_sign_open crypto_sign_ed25519_open
#define crypto_sign_BYTES crypto_sign_ed25519_BYTES
#define crypto_sign_PUBLICKEYBYTES crypto_sign_ed25519_PUBLICKEYBYTES
#define crypto_sign_SECRETKEYBYTES crypto_sign_ed25519_SECRETKEYBYTES
#define crypto_sign_IMPLEMENTATION crypto_sign_ed25519_IMPLEMENTATION
#define crypto_sign_VERSION crypto_sign_ed25519_VERSION
#define crypto_sign_ed25519_tweet_BYTES 64
#define crypto_sign_ed25519_tweet_PUBLICKEYBYTES 32
#define crypto_sign_ed25519_tweet_SECRETKEYBYTES 64
extern int crypto_sign_ed25519_tweet_open(unsigned char *,unsigned long long *,const unsigned char *,unsigned long long,const unsigned char *);
#define crypto_sign_ed25519_tweet_VERSION "-"
#define crypto_sign_ed25519_open crypto_sign_ed25519_tweet_open
#define crypto_sign_ed25519_BYTES crypto_sign_ed25519_tweet_BYTES
#define crypto_sign_ed25519_PUBLICKEYBYTES crypto_sign_ed25519_tweet_PUBLICKEYBYTES
#define crypto_sign_ed25519_SECRETKEYBYTES crypto_sign_ed25519_tweet_SECRETKEYBYTES
#define crypto_sign_ed25519_VERSION crypto_sign_ed25519_tweet_VERSION
#define crypto_sign_ed25519_IMPLEMENTATION "crypto_sign/ed25519/tweet"
#define crypto_verify_PRIMITIVE "16"
#define crypto_verify crypto_verify_16
#define crypto_verify_BYTES crypto_verify_16_BYTES
#define crypto_verify_IMPLEMENTATION crypto_verify_16_IMPLEMENTATION
#define crypto_verify_VERSION crypto_verify_16_VERSION
#define crypto_verify_16_tweet_BYTES 16
extern int crypto_verify_16_tweet(const unsigned char *,const unsigned char *);
#define crypto_verify_16_tweet_VERSION "-"
#define crypto_verify_16 crypto_verify_16_tweet
#define crypto_verify_16_BYTES crypto_verify_16_tweet_BYTES
#define crypto_verify_16_VERSION crypto_verify_16_tweet_VERSION
#define crypto_verify_16_IMPLEMENTATION "crypto_verify/16/tweet"
#define crypto_verify_32_tweet_BYTES 32
extern int crypto_verify_32_tweet(const unsigned char *,const unsigned char *);
#define crypto_verify_32_tweet_VERSION "-"
#define crypto_verify_32 crypto_verify_32_tweet
#define crypto_verify_32_BYTES crypto_verify_32_tweet_BYTES
#define crypto_verify_32_VERSION crypto_verify_32_tweet_VERSION
#define crypto_verify_32_IMPLEMENTATION "crypto_verify/32/tweet"
#define crypto_verify_64_tweet_BYTES 64
extern int crypto_verify_64_tweet(const unsigned char *,const unsigned char *);
#define crypto_verify_64_tweet_VERSION "-"
#define crypto_verify_64 crypto_verify_64_tweet
#define crypto_verify_64_BYTES crypto_verify_64_tweet_BYTES
#define crypto_verify_64_VERSION crypto_verify_64_tweet_VERSION
#define crypto_verify_64_IMPLEMENTATION "crypto_verify/64/tweet"

#endif