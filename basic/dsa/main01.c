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

// BigInt 변수를 0으로 초기화하는 함수
void BigInt_Init(BigInt* a) {
    memset(a->data, 0, sizeof(a->data));
    a->size = 0;
}

// BigInt 변수의 값을 다른 변수로 복사하는 함수
void BigInt_Copy(BigInt* dest, const BigInt* src) {
    memcpy(dest->data, src->data, sizeof(src->data));
    dest->size = src->size;
}

// BigInt의 최상위 데이터 중 불필요한 0을 제거
void BigInt_Trim(BigInt* a) {
    while (a->size > 0 && a->data[a->size - 1] == 0) {
        a->size--;
    }
}

// BigInt가 0인지 확인하는 함수
bool BigInt_IsZero(const BigInt* a) {
    return a->size == 0 || (a->size == 1 && a->data[0] == 0);
}

// 두 BigInt 값을 비교하는 함수 (a > b 이면 1, a < b 이면 -1, a == b 이면 0 반환)
int BigInt_Compare(const BigInt* a, const BigInt* b) {
    if (a->size > b->size) return 1;
    if (a->size < b->size) return -1;
    for (int i = a->size - 1; i >= 0; i--) {
        if (a->data[i] > b->data[i]) return 1;
        if (a->data[i] < b->data[i]) return -1;
    }
    return 0;
}

// 덧셈 연산: res = a + b
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

// 뺄셈 연산: res = a - b
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

// 곱셈 연산: res = a * b
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

// 1비트 좌측 시프트 (a = a << 1, 즉 a * 2 효과)
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

// 나눗셈 및 나머지 연산: q = a / b, r = a % b
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

// 모듈러 역원 연산 (확장 유클리드 알고리즘)
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

// ----------------------------------------------------------------------------
// 점 압축 및 복원 함수
// ----------------------------------------------------------------------------

uint8_t EC_Point_Compress(const EC_Point* P, BigInt* comp_x) {
    if (P->is_infinity) return 0x00;

    BigInt_Copy(comp_x, &P->x);
    uint8_t parity = P->y.data[0] & 1;
    return 0x02 + parity;
}

bool EC_Point_Decompress(EC_Point* P, uint8_t prefix, const BigInt* comp_x, const BigInt* a, const BigInt* b, const BigInt* p) {
    if (prefix == 0x00) {
        P->is_infinity = true;
        return true;
    }
    if (prefix != 0x02 && prefix != 0x03) return false;

    BigInt_Copy(&P->x, comp_x);
    P->is_infinity = false;

    BigInt x_sq, x_cb, ax, z, y, y_sq;

    ModMul(&x_sq, comp_x, comp_x, p);
    ModMul(&x_cb, &x_sq, comp_x, p);
    ModMul(&ax, a, comp_x, p);
    ModAdd(&z, &x_cb, &ax, p);
    ModAdd(&z, &z, b, p);

    BigInt p_plus_1, four, p_plus_1_over_4, temp_one;
    BigInt_Init(&four); four.data[0] = 4; four.size = 1;
    BigInt_Init(&temp_one); temp_one.data[0] = 1; temp_one.size = 1;
    
    BigInt_Add(&p_plus_1, p, &temp_one);
    BigInt_DivMod(&p_plus_1_over_4, NULL, &p_plus_1, &four);

    ModExp(&y, &z, &p_plus_1_over_4, p);

    ModMul(&y_sq, &y, &y, p);
    if (BigInt_Compare(&y_sq, &z) != 0) {
        return false;
    }

    uint8_t y_parity = y.data[0] & 1;
    uint8_t expected_parity = prefix - 0x02;

    if (y_parity != expected_parity) {
        ModSub(&P->y, p, &y, p);
    } else {
        BigInt_Copy(&P->y, &y);
    }

    return true;
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
// [5] ECDSA 서명 및 검증 로직
// ============================================================================

typedef struct {
    BigInt r;
    BigInt s;
} ECDSA_Signature;

void ECDSA_Sign(ECDSA_Signature* sig, const BigInt* msg_hash, const BigInt* priv_key) {
    BigInt k, k_inv, r, s, temp, dr, e_plus_dr;
    EC_Point R;

    do {
        // 1. 임의의 난수 k 생성 (1 <= k < n)
        EC_GeneratePrivateKey(&k);

        // 2. R = k * G
        EC_Scalar_Mul(&R, &P256_G, &k, &P256_a, &P256_p);

        // 3. r = R.x mod n
        BigInt_DivMod(NULL, &r, &R.x, &P256_n);
        
        // r이 0이면 k를 다시 생성
        if (BigInt_IsZero(&r)) continue;

        // 4. k_inv = k^-1 mod n
        ModInverse(&k_inv, &k, &P256_n);

        // 5. s = k^-1 * (msg_hash + r * priv_key) mod n
        ModMul(&dr, &r, priv_key, &P256_n);
        ModAdd(&e_plus_dr, msg_hash, &dr, &P256_n);
        ModMul(&s, &k_inv, &e_plus_dr, &P256_n);

    // s가 0이면 k를 다시 생성
    } while (BigInt_IsZero(&s));

    BigInt_Copy(&sig->r, &r);
    BigInt_Copy(&sig->s, &s);
}

bool ECDSA_Verify(const ECDSA_Signature* sig, const BigInt* msg_hash, const EC_Point* pub_key) {
    // 1. r과 s가 유효한 범위(1 ~ n-1) 내에 있는지 확인
    if (BigInt_IsZero(&sig->r) || BigInt_Compare(&sig->r, &P256_n) >= 0) return false;
    if (BigInt_IsZero(&sig->s) || BigInt_Compare(&sig->s, &P256_n) >= 0) return false;

    BigInt w, u1, u2, P_x_mod_n;
    EC_Point u1G, u2Q, P;

    // 2. w = s^-1 mod n
    ModInverse(&w, &sig->s, &P256_n);

    // 3. u1 = (msg_hash * w) mod n
    ModMul(&u1, msg_hash, &w, &P256_n);

    // 4. u2 = (r * w) mod n
    ModMul(&u2, &sig->r, &w, &P256_n);

    // 5. P = u1 * G + u2 * Q
    EC_Scalar_Mul(&u1G, &P256_G, &u1, &P256_a, &P256_p);
    EC_Scalar_Mul(&u2Q, pub_key, &u2, &P256_a, &P256_p);
    EC_Point_Add(&P, &u1G, &u2Q, &P256_a, &P256_p);

    // 계산된 점이 무한원점이면 유효하지 않은 서명
    if (P.is_infinity) return false;

    // 6. P.x mod n == r 인지 확인
    BigInt_DivMod(NULL, &P_x_mod_n, &P.x, &P256_n);

    return (BigInt_Compare(&P_x_mod_n, &sig->r) == 0);
}

// ============================================================================
// [6] main() 함수
// ============================================================================

int main() {
    EC_InitParameters();

    printf("========== [ ECDSA 서명 및 검증 시뮬레이션 ] ==========\n\n");

    BigInt alice_priv;
    EC_Point alice_pub;
    
    // 1. Alice 키 쌍 생성
    printf("Alice 키 생성 중...\n");
    EC_GeneratePrivateKey(&alice_priv);
    EC_Scalar_Mul(&alice_pub, &P256_G, &alice_priv, &P256_a, &P256_p);

    printf("Alice Private Key: "); BigInt_PrintHex(&alice_priv); printf("\n");
    printf("Alice Public Key (X): "); BigInt_PrintHex(&alice_pub.x); printf("\n");
    printf("Alice Public Key (Y): "); BigInt_PrintHex(&alice_pub.y); printf("\n\n");

    // 2. 전송할 메시지 해시 준비 (SHA-256 해시값이라고 가정)
    BigInt msg_hash;
    BigInt_FromHex(&msg_hash, "BA7816BF8F01CFEA414140DE5DAE2223B00361A396177A9CB410FF61F20015AD");
    printf("Message Hash (가상): "); BigInt_PrintHex(&msg_hash); printf("\n\n");

    // 3. 서명 생성 (Signing)
    ECDSA_Signature signature;
    printf("서명 생성 중...\n");
    ECDSA_Sign(&signature, &msg_hash, &alice_priv);
    
    printf("생성된 서명 (Signature):\n");
    printf("    (r) "); BigInt_PrintHex(&signature.r); printf("\n");
    printf("    (s) "); BigInt_PrintHex(&signature.s); printf("\n\n");

    // 4. 서명 검증 (Verification)
    printf("서명 검증 중...\n");
    bool is_valid = ECDSA_Verify(&signature, &msg_hash, &alice_pub);
    
    if (is_valid) {
        printf("[SUCCESS] 서명이 유효합니다! (Alice가 작성한 메시지가 맞습니다.)\n\n");
    } else {
        printf("[FAILED] 서명 검증에 실패했습니다.\n\n");
    }

    // 5. 서명 위변조 테스트
    printf("--- [조작된 메시지로 검증 시도] ---\n");
    BigInt fake_hash;
    BigInt_FromHex(&fake_hash, "DEADBEEF8F01CFEA414140DE5DAE2223B00361A396177A9CB410FF61F20015AD");
    
    bool is_fake_valid = ECDSA_Verify(&signature, &fake_hash, &alice_pub);
    if (is_fake_valid) {
        printf("[에러] 조작된 메시지가 유효하다고 판별되었습니다!\n");
    } else {
        printf("[SUCCESS] 조작된 메시지의 서명 검증이 정상적으로 실패했습니다.\n");
    }

    return 0;
}