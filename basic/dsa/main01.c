#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <sys/random.h>

// ============================================================================
// [1] BigInt 기본 구현
// ============================================================================

#define MAX_WORDS 256

typedef struct {
    uint32_t data[MAX_WORDS];
    int size;
} BigInt;

void BigInt_Init(BigInt* a) {
    memset(a->data, 0, sizeof(a->data));
    a->size = 0;
}

void BigInt_Copy(BigInt* dest, const BigInt* src) {
    memcpy(dest->data, src->data, sizeof(src->data));
    dest->size = src->size;
}

void BigInt_Trim(BigInt* a) {
    while (a->size > 0 && a->data[a->size - 1] == 0) {
        a->size--;
    }
}

bool BigInt_IsZero(const BigInt* a) {
    return a->size == 0 || (a->size == 1 && a->data[0] == 0);
}

int BigInt_Compare(const BigInt* a, const BigInt* b) {
    if (a->size > b->size) return 1;
    if (a->size < b->size) return -1;
    for (int i = a->size - 1; i >= 0; i--) {
        if (a->data[i] > b->data[i]) return 1;
        if (a->data[i] < b->data[i]) return -1;
    }
    return 0;
}

void BigInt_Add(BigInt* res, const BigInt* a, const BigInt* b) {
    BigInt temp;
    BigInt_Init(&temp);
    uint64_t carry = 0;
    int max_size = (a->size > b->size) ? a->size : b->size;

    for (int i = 0; i < max_size || carry > 0; i++) {
        if (i >= MAX_WORDS) break;
        uint64_t sum = carry;
        if (i < a->size) sum += a->data[i];
        if (i < b->size) sum += b->data[i];

        temp.data[i] = (uint32_t)(sum & 0xFFFFFFFF);
        carry = sum >> 32;
        temp.size = i + 1;
    }
    BigInt_Copy(res, &temp);
}

void BigInt_Sub(BigInt* res, const BigInt* a, const BigInt* b) {
    BigInt temp;
    BigInt_Init(&temp);
    int64_t borrow = 0;

    for (int i = 0; i < a->size; i++) {
        int64_t diff = (int64_t)a->data[i] - borrow;
        if (i < b->size) diff -= b->data[i];

        if (diff < 0) { 
            diff += 0x100000000LL;
            borrow = 1;
        } else {
            borrow = 0;
        }
        temp.data[i] = (uint32_t)diff;
        temp.size = i + 1;
    }
    BigInt_Trim(&temp);
    BigInt_Copy(res, &temp);
}

void BigInt_Mul(BigInt* res, const BigInt* a, const BigInt* b) {
    BigInt temp;
    BigInt_Init(&temp);
    if (BigInt_IsZero(a) || BigInt_IsZero(b)) {
        BigInt_Copy(res, &temp);
        return;
    }
    temp.size = a->size + b->size;
    if (temp.size > MAX_WORDS) temp.size = MAX_WORDS;

    for (int i = 0; i < a->size; i++) {
        uint64_t carry = 0;
        for (int j = 0; j < b->size; j++) {
            if (i + j >= MAX_WORDS) break;
            uint64_t prod = (uint64_t)a->data[i] * b->data[j] + temp.data[i + j] + carry;
            temp.data[i + j] = (uint32_t)(prod & 0xFFFFFFFF);
            carry = prod >> 32;
        }
        if (i + b->size < MAX_WORDS) {
            temp.data[i + b->size] = (uint32_t)carry;
        }
    }
    BigInt_Trim(&temp);
    BigInt_Copy(res, &temp);
}

void BigInt_ShiftLeft1(BigInt* a) {
    if (BigInt_IsZero(a)) return;
    uint32_t carry = 0;
    for (int i = 0; i < a->size; i++) {
        uint32_t next_carry = a->data[i] >> 31;
        a->data[i] = (a->data[i] << 1) | carry;
        carry = next_carry;
    }
    if (carry > 0 && a->size < MAX_WORDS) {
        a->data[a->size] = carry;
        a->size++;
    }
}

void BigInt_DivMod(BigInt* q, BigInt* r, const BigInt* a, const BigInt* b) {
    BigInt temp_q, temp_r;
    BigInt_Init(&temp_q);
    BigInt_Init(&temp_r);

    if (BigInt_IsZero(b) || BigInt_IsZero(a)) {
        if (q) BigInt_Copy(q, &temp_q);
        if (r) BigInt_Copy(r, &temp_r);
        return;
    }
    if (BigInt_Compare(a, b) < 0) {
        if (q) BigInt_Copy(q, &temp_q);
        if (r) BigInt_Copy(r, a);
        return;
    }

    int a_bits = (a->size - 1) * 32;
    uint32_t top = a->data[a->size - 1];
    while (top) { a_bits++; top >>= 1; }

    temp_q.size = (a_bits + 31) / 32;

    for (int i = a_bits - 1; i >= 0; i--) {
        BigInt_ShiftLeft1(&temp_r);
        int word_idx = i / 32;
        int bit_idx = i % 32;
        if ((a->data[word_idx] >> bit_idx) & 1) {
            temp_r.data[0] |= 1;
            if (temp_r.size == 0) temp_r.size = 1;
        }
        if (BigInt_Compare(&temp_r, b) >= 0) {
            BigInt sub_res;
            BigInt_Sub(&sub_res, &temp_r, b);
            BigInt_Copy(&temp_r, &sub_res);
            temp_q.data[word_idx] |= (1U << bit_idx);
        }
    }
    BigInt_Trim(&temp_q);
    BigInt_Trim(&temp_r);

    if (q) BigInt_Copy(q, &temp_q);
    if (r) BigInt_Copy(r, &temp_r);
}

void ModInverse(BigInt* res, const BigInt* e, const BigInt* mod) {
    BigInt t, newt, r, newr, q, temp, prod, next_t;
    int t_sign = 1, newt_sign = 1;

    BigInt_Init(&t);
    BigInt_Init(&newt); newt.data[0] = 1; newt.size = 1;

    BigInt_Copy(&r, mod);
    BigInt_Copy(&newr, e);

    while (!BigInt_IsZero(&newr)) {
        BigInt_DivMod(&q, &temp, &r, &newr);
        BigInt_Copy(&r, &newr);
        BigInt_Copy(&newr, &temp);

        BigInt_Mul(&prod, &q, &newt);

        if (t_sign == newt_sign) {
            if (BigInt_Compare(&t, &prod) >= 0) {
                BigInt_Sub(&next_t, &t, &prod);
            } else {
                BigInt_Sub(&next_t, &prod, &t);
                t_sign = -t_sign;
            }
        } else {
            BigInt_Add(&next_t, &t, &prod);
        }

        BigInt_Copy(&t, &newt);
        t_sign = newt_sign;
        BigInt_Copy(&newt, &next_t);
        newt_sign = t_sign == newt_sign ? -newt_sign : t_sign;
    }

    if (t_sign == -1) BigInt_Sub(res, mod, &t);
    else BigInt_Copy(res, &t);
}

// ============================================================================
// [2] ECC용 모듈러 연산 추가
// ============================================================================

void BigInt_FromHex(BigInt* a, const char* hex) {
    BigInt_Init(a);
    int len = strlen(hex);
    int word_idx = 0;
    for (int i = len; i > 0; i -= 8) {
        int start = i - 8;
        if (start < 0) start = 0;
        char buf[9] = {0};
        strncpy(buf, hex + start, i - start);
        a->data[word_idx++] = strtoul(buf, NULL, 16);
    }
    a->size = word_idx;
    BigInt_Trim(a);
}

// (추가) 해시 결과(32바이트 배열)를 BigInt로 변환하는 함수
void BigInt_FromBytes(BigInt* a, const uint8_t* bytes, size_t len) {
    BigInt_Init(a);
    int word_idx = 0;
    
    // 바이트 배열을 뒤에서부터(Little-endian 구조체에 맞게) 읽어옵니다.
    for (int i = (int)len; i > 0; i -= 4) {
        uint32_t val = 0;
        int bytes_to_read = (i >= 4) ? 4 : i;
        int start = i - bytes_to_read;
        
        for (int j = 0; j < bytes_to_read; j++) {
            val = (val << 8) | bytes[start + j];
        }
        a->data[word_idx++] = val;
    }
    a->size = word_idx;
    BigInt_Trim(a);
}

void BigInt_PrintHex(const BigInt* a) {
    if (BigInt_IsZero(a)) {
        printf("00000000"); return;
    }
    for (int i = a->size - 1; i >= 0; i--) {
        if (i == a->size - 1) printf("%X", a->data[i]);
        else printf("%08X", a->data[i]);
    }
}

void ModAdd(BigInt* res, const BigInt* a, const BigInt* b, const BigInt* m) {
    BigInt temp;
    BigInt_Add(&temp, a, b);
    BigInt_DivMod(NULL, res, &temp, m);
}

void ModSub(BigInt* res, const BigInt* a, const BigInt* b, const BigInt* m) {
    if (BigInt_Compare(a, b) >= 0) {
        BigInt_Sub(res, a, b);
    } else {
        BigInt temp;
        BigInt_Sub(&temp, b, a);
        BigInt_Sub(res, m, &temp);
    }
}

void ModMul(BigInt* res, const BigInt* a, const BigInt* b, const BigInt* m) {
    BigInt temp;
    BigInt_Mul(&temp, a, b);
    BigInt_DivMod(NULL, res, &temp, m);
}

void ModExp(BigInt* res, const BigInt* base, const BigInt* exp, const BigInt* m) {
    BigInt result;
    BigInt_Init(&result);
    result.data[0] = 1; result.size = 1;

    if (BigInt_IsZero(exp)) {
        BigInt_Copy(res, &result);
        return;
    }

    BigInt current_base;
    BigInt_Copy(&current_base, base);

    int max_bit = exp->size * 32 - 1;
    while(max_bit >= 0) {
        int word_idx = max_bit / 32;
        int bit_idx = max_bit % 32;
        if ((exp->data[word_idx] >> bit_idx) & 1) break;
        max_bit--;
    }

    for (int i = 0; i <= max_bit; i++) {
        int word_idx = i / 32;
        int bit_idx = i % 32;
        if ((exp->data[word_idx] >> bit_idx) & 1) {
            ModMul(&result, &result, &current_base, m);
        }
        ModMul(&current_base, &current_base, &current_base, m);
    }
    BigInt_Copy(res, &result);
}

// ============================================================================
// [3] 타원 곡선 수학
// ============================================================================

typedef struct {
    BigInt x;
    BigInt y;
    bool is_infinity;
} EC_Point;

BigInt P256_p, P256_a, P256_b, P256_n;
EC_Point P256_G;

void EC_InitParameters() {
    BigInt_FromHex(&P256_p, "FFFFFFFF00000001000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFF");
    BigInt_FromHex(&P256_a, "FFFFFFFF00000001000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFC");
    BigInt_FromHex(&P256_b, "5AC635D8AA3A93E7B3EBBD55769886BC651D06B0CC53B0F63BCE3C3E27D2604B");
    BigInt_FromHex(&P256_G.x, "6B17D1F2E12C4247F8BCE6E563A440F277037D812DEB33A0F4A13945D898C296");
    BigInt_FromHex(&P256_G.y, "4FE342E2FE1A7F9B8EE7EB4A7C0F9E162BCE33576B315ECECBB6406837BF51F5");
    P256_G.is_infinity = false;
    BigInt_FromHex(&P256_n, "FFFFFFFF00000000FFFFFFFFFFFFFFFFBCE6FAADA7179E84F3B9CAC2FC632551");
}

void EC_Point_Double(EC_Point* R, const EC_Point* P, const BigInt* a, const BigInt* p) {
    if (P->is_infinity || BigInt_IsZero(&P->y)) {
        R->is_infinity = true; return;
    }
    BigInt x_sq, temp1, temp2, lambda, y_two, y_two_inv;
    EC_Point R_out; R_out.is_infinity = false;

    ModMul(&x_sq, &P->x, &P->x, p);
    ModAdd(&temp1, &x_sq, &x_sq, p);
    ModAdd(&temp1, &temp1, &x_sq, p);
    ModAdd(&temp2, &temp1, a, p);
    ModAdd(&y_two, &P->y, &P->y, p);
    ModInverse(&y_two_inv, &y_two, p);
    ModMul(&lambda, &temp2, &y_two_inv, p);

    ModMul(&temp1, &lambda, &lambda, p);
    ModAdd(&temp2, &P->x, &P->x, p);
    ModSub(&R_out.x, &temp1, &temp2, p);

    ModSub(&temp1, &P->x, &R_out.x, p);
    ModMul(&temp2, &lambda, &temp1, p);
    ModSub(&R_out.y, &temp2, &P->y, p);

    *R = R_out;
}

void EC_Point_Add(EC_Point* R, const EC_Point* P, const EC_Point* Q, const BigInt* a, const BigInt* p) {
    if (P->is_infinity) { *R = *Q; return; }
    if (Q->is_infinity) { *R = *P; return; }

    if (BigInt_Compare(&P->x, &Q->x) == 0) {
        if (BigInt_Compare(&P->y, &Q->y) == 0) {
            EC_Point_Double(R, P, a, p);
        } else {
            R->is_infinity = true;
        }
        return;
    }

    BigInt dy, dx, dx_inv, lambda, temp1, temp2;
    EC_Point R_out; R_out.is_infinity = false;

    ModSub(&dy, &Q->y, &P->y, p);
    ModSub(&dx, &Q->x, &P->x, p);
    ModInverse(&dx_inv, &dx, p);
    ModMul(&lambda, &dy, &dx_inv, p);

    ModMul(&temp1, &lambda, &lambda, p);
    ModSub(&temp2, &temp1, &P->x, p);
    ModSub(&R_out.x, &temp2, &Q->x, p);

    ModSub(&temp1, &P->x, &R_out.x, p);
    ModMul(&temp2, &lambda, &temp1, p);
    ModSub(&R_out.y, &temp2, &P->y, p);

    *R = R_out;
}

void EC_Scalar_Mul(EC_Point* R, const EC_Point* P, const BigInt* k, const BigInt* a, const BigInt* p) {
    EC_Point res = {0};
    res.is_infinity = true;

    int max_bit = k->size * 32 - 1;
    while(max_bit >= 0) {
        int word_idx = max_bit / 32;
        int bit_idx = max_bit % 32;
        if ((k->data[word_idx] >> bit_idx) & 1) break;
        max_bit--;
    }

    for (int i = max_bit; i >= 0; i--) {
        EC_Point_Double(&res, &res, a, p);

        int word_idx = i / 32;
        int bit_idx = i % 32;
        if ((k->data[word_idx] >> bit_idx) & 1) {
            EC_Point_Add(&res, &res, P, a, p);
        }
    }
    *R = res;
}

// ============================================================================
// [4] 난수 생성 및 키 로직
// ============================================================================

int GenerateSecureRandom(uint8_t* buffer, size_t length) {
    ssize_t result = getrandom(buffer, length, 0);
    if (result != (ssize_t)length) return -1;
    return 0;
}

void EC_GeneratePrivateKey(BigInt* privKey) {
    int bytes = 256 / 8;
    int words = 256 / 32;
    do {
        BigInt_Init(privKey);
        if (GenerateSecureRandom((uint8_t*)privKey->data, bytes) != 0) {
            fprintf(stderr, "Fatal Error: Failed to generate secure random numbers for private key.\n");
            exit(EXIT_FAILURE);
        }
        privKey->size = words;
        BigInt_Trim(privKey);
    } while (BigInt_IsZero(privKey) || BigInt_Compare(privKey, &P256_n) >= 0);
}

// ============================================================================
// [4.5] SHA-256 해시 함수 구현 (추가됨)
// ============================================================================
#define ROTRIGHT(a,b) (((a) >> (b)) | ((a) << (32-(b))))
#define CH(x,y,z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTRIGHT(x,2) ^ ROTRIGHT(x,13) ^ ROTRIGHT(x,22))
#define EP1(x) (ROTRIGHT(x,6) ^ ROTRIGHT(x,11) ^ ROTRIGHT(x,25))
#define SIG0(x) (ROTRIGHT(x,7) ^ ROTRIGHT(x,18) ^ ((x) >> 3))
#define SIG1(x) (ROTRIGHT(x,17) ^ ROTRIGHT(x,19) ^ ((x) >> 10))

typedef struct {
    uint8_t data[64];
    uint32_t datalen;
    unsigned long long bitlen;
    uint32_t state[8];
} SHA256_CTX;

static const uint32_t k[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

void sha256_transform(SHA256_CTX *ctx, const uint8_t data[]) {
    uint32_t a, b, c, d, e, f, g, h, i, j, t1, t2, m[64];
    for (i = 0, j = 0; i < 16; ++i, j += 4)
        m[i] = (data[j] << 24) | (data[j + 1] << 16) | (data[j + 2] << 8) | (data[j + 3]);
    for ( ; i < 64; ++i)
        m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];

    a = ctx->state[0]; b = ctx->state[1]; c = ctx->state[2]; d = ctx->state[3];
    e = ctx->state[4]; f = ctx->state[5]; g = ctx->state[6]; h = ctx->state[7];

    for (i = 0; i < 64; ++i) {
        t1 = h + EP1(e) + CH(e,f,g) + k[i] + m[i];
        t2 = EP0(a) + MAJ(a,b,c);
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }

    ctx->state[0] += a; ctx->state[1] += b; ctx->state[2] += c; ctx->state[3] += d;
    ctx->state[4] += e; ctx->state[5] += f; ctx->state[6] += g; ctx->state[7] += h;
}

void sha256_init(SHA256_CTX *ctx) {
    ctx->datalen = 0;
    ctx->bitlen = 0;
    ctx->state[0] = 0x6a09e667; ctx->state[1] = 0xbb67ae85;
    ctx->state[2] = 0x3c6ef372; ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f; ctx->state[5] = 0x9b05688c;
    ctx->state[6] = 0x1f83d9ab; ctx->state[7] = 0x5be0cd19;
}

void sha256_update(SHA256_CTX *ctx, const uint8_t data[], size_t len) {
    for (size_t i = 0; i < len; ++i) {
        ctx->data[ctx->datalen] = data[i];
        ctx->datalen++;
        if (ctx->datalen == 64) {
            sha256_transform(ctx, ctx->data);
            ctx->bitlen += 512;
            ctx->datalen = 0;
        }
    }
}

void sha256_final(SHA256_CTX *ctx, uint8_t hash[]) {
    uint32_t i = ctx->datalen;
    if (ctx->datalen < 56) {
        ctx->data[i++] = 0x80;
        while (i < 56) ctx->data[i++] = 0x00;
    } else {
        ctx->data[i++] = 0x80;
        while (i < 64) ctx->data[i++] = 0x00;
        sha256_transform(ctx, ctx->data);
        memset(ctx->data, 0, 56);
    }
    ctx->bitlen += ctx->datalen * 8;
    ctx->data[63] = ctx->bitlen; ctx->data[62] = ctx->bitlen >> 8;
    ctx->data[61] = ctx->bitlen >> 16; ctx->data[60] = ctx->bitlen >> 24;
    ctx->data[59] = ctx->bitlen >> 32; ctx->data[58] = ctx->bitlen >> 40;
    ctx->data[57] = ctx->bitlen >> 48; ctx->data[56] = ctx->bitlen >> 56;
    sha256_transform(ctx, ctx->data);
    for (i = 0; i < 4; ++i) {
        hash[i]      = (ctx->state[0] >> (24 - i * 8)) & 0x000000ff;
        hash[i + 4]  = (ctx->state[1] >> (24 - i * 8)) & 0x000000ff;
        hash[i + 8]  = (ctx->state[2] >> (24 - i * 8)) & 0x000000ff;
        hash[i + 12] = (ctx->state[3] >> (24 - i * 8)) & 0x000000ff;
        hash[i + 16] = (ctx->state[4] >> (24 - i * 8)) & 0x000000ff;
        hash[i + 20] = (ctx->state[5] >> (24 - i * 8)) & 0x000000ff;
        hash[i + 24] = (ctx->state[6] >> (24 - i * 8)) & 0x000000ff;
        hash[i + 28] = (ctx->state[7] >> (24 - i * 8)) & 0x000000ff;
    }
}

// ============================================================================
// [5] ECDSA 서명 및 검증 로직
// ============================================================================

typedef struct {
    BigInt r;
    BigInt s;
} ECDSA_Signature;

// (원래 있던 내부 서명 로직 - 해시값이 주어졌을 때)
void ECDSA_Sign(ECDSA_Signature* sig, const BigInt* msg_hash, const BigInt* priv_key) {
    BigInt k, k_inv, r, s, temp, dr, e_plus_dr;
    EC_Point R;

    do {
        EC_GeneratePrivateKey(&k);
        EC_Scalar_Mul(&R, &P256_G, &k, &P256_a, &P256_p);
        BigInt_DivMod(NULL, &r, &R.x, &P256_n);
        
        if (BigInt_IsZero(&r)) continue;

        ModInverse(&k_inv, &k, &P256_n);
        ModMul(&dr, &r, priv_key, &P256_n);
        ModAdd(&e_plus_dr, msg_hash, &dr, &P256_n);
        ModMul(&s, &k_inv, &e_plus_dr, &P256_n);

    } while (BigInt_IsZero(&s));

    BigInt_Copy(&sig->r, &r);
    BigInt_Copy(&sig->s, &s);
}

// (원래 있던 내부 검증 로직)
bool ECDSA_Verify(const ECDSA_Signature* sig, const BigInt* msg_hash, const EC_Point* pub_key) {
    if (BigInt_IsZero(&sig->r) || BigInt_Compare(&sig->r, &P256_n) >= 0) return false;
    if (BigInt_IsZero(&sig->s) || BigInt_Compare(&sig->s, &P256_n) >= 0) return false;

    BigInt w, u1, u2, P_x_mod_n;
    EC_Point u1G, u2Q, P;

    ModInverse(&w, &sig->s, &P256_n);
    ModMul(&u1, msg_hash, &w, &P256_n);
    ModMul(&u2, &sig->r, &w, &P256_n);

    EC_Scalar_Mul(&u1G, &P256_G, &u1, &P256_a, &P256_p);
    EC_Scalar_Mul(&u2Q, pub_key, &u2, &P256_a, &P256_p);
    EC_Point_Add(&P, &u1G, &u2Q, &P256_a, &P256_p);

    if (P.is_infinity) return false;

    BigInt_DivMod(NULL, &P_x_mod_n, &P.x, &P256_n);

    return (BigInt_Compare(&P_x_mod_n, &sig->r) == 0);
}

// ----------------------------------------------------------------------------
// [추가] 실제 메시지(문자열/파일)를 해시하여 서명/검증하는 Wrapper 함수
// ----------------------------------------------------------------------------
void ECDSA_Sign_Message(ECDSA_Signature* sig, const char* message, const BigInt* priv_key) {
    SHA256_CTX ctx;
    uint8_t hash[32];
    BigInt msg_hash;

    // 1. 메시지 해시
    sha256_init(&ctx);
    sha256_update(&ctx, (const uint8_t*)message, strlen(message));
    sha256_final(&ctx, hash);

    // 2. 해시 결과(32바이트)를 BigInt로 변환
    BigInt_FromBytes(&msg_hash, hash, 32);

    // 3. 서명 진행
    ECDSA_Sign(sig, &msg_hash, priv_key);
}

bool ECDSA_Verify_Message(const ECDSA_Signature* sig, const char* message, const EC_Point* pub_key) {
    SHA256_CTX ctx;
    uint8_t hash[32];
    BigInt msg_hash;

    // 1. 메시지 해시
    sha256_init(&ctx);
    sha256_update(&ctx, (const uint8_t*)message, strlen(message));
    sha256_final(&ctx, hash);

    // 2. 해시 결과(32바이트)를 BigInt로 변환
    BigInt_FromBytes(&msg_hash, hash, 32);

    // 3. 검증 진행
    return ECDSA_Verify(sig, &msg_hash, pub_key);
}

// ============================================================================
// [6] main() 함수
// ============================================================================

int main() {
    EC_InitParameters();

    printf("========== [ 실제 ECDSA 서명 및 검증 시뮬레이션 ] ==========\n\n");

    BigInt alice_priv;
    EC_Point alice_pub;
    
    // 1. Alice 키 쌍 생성
    printf("Alice 키 생성 중...\n");
    EC_GeneratePrivateKey(&alice_priv);
    EC_Scalar_Mul(&alice_pub, &P256_G, &alice_priv, &P256_a, &P256_p);

    printf("Alice Private Key: "); BigInt_PrintHex(&alice_priv); printf("\n");
    printf("Alice Public Key (X): "); BigInt_PrintHex(&alice_pub.x); printf("\n");
    printf("Alice Public Key (Y): "); BigInt_PrintHex(&alice_pub.y); printf("\n\n");

    // 2. 전송할 원본 메시지 준비
    const char* original_message = "이것은 ECDSA로 보호되는 아주 중요한 원본 메시지입니다.";
    printf("원본 메시지: \"%s\"\n\n", original_message);

    // 3. 서명 생성 (메시지 원본을 직접 서명)
    ECDSA_Signature signature;
    printf("서명 생성 중...\n");
    ECDSA_Sign_Message(&signature, original_message, &alice_priv);
    
    printf("생성된 서명 (Signature):\n");
    printf("    (r) "); BigInt_PrintHex(&signature.r); printf("\n");
    printf("    (s) "); BigInt_PrintHex(&signature.s); printf("\n\n");

    // 4. 서명 검증 (Verification)
    printf("서명 검증 중...\n");
    bool is_valid = ECDSA_Verify_Message(&signature, original_message, &alice_pub);
    
    if (is_valid) {
        printf("[SUCCESS] 서명이 유효합니다! (Alice가 작성한 메시지가 맞습니다.)\n\n");
    } else {
        printf("[FAILED] 서명 검증에 실패했습니다.\n\n");
    }

    // 5. 서명 위변조 테스트
    const char* fake_message = "이것은 해커가 변조한 가짜 메시지입니다.";
    printf("--- [조작된 메시지로 검증 시도] ---\n");
    printf("조작된 메시지: \"%s\"\n", fake_message);
    
    bool is_fake_valid = ECDSA_Verify_Message(&signature, fake_message, &alice_pub);
    if (is_fake_valid) {
        printf("[에러] 조작된 메시지가 유효하다고 판별되었습니다!\n");
    } else {
        printf("[SUCCESS] 조작된 메시지의 서명 검증이 정상적으로 실패했습니다.\n");
    }

    return 0;
}