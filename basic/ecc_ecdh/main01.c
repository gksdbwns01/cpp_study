#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

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

// 확장 유클리드 호제법 (제공된 코드와 동일)
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
// [2] 유틸리티 및 타원 곡선(ECC)용 모듈러 연산 추가
// ============================================================================

// 16진수 문자열을 BigInt로 변환 (상수 입력용)
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

// 모듈러 덧셈: (a + b) mod m
void ModAdd(BigInt* res, const BigInt* a, const BigInt* b, const BigInt* m) {
    BigInt temp;
    BigInt_Add(&temp, a, b);
    BigInt_DivMod(NULL, res, &temp, m);
}

// 모듈러 뺄셈: (a - b) mod m (음수 방지 처리)
void ModSub(BigInt* res, const BigInt* a, const BigInt* b, const BigInt* m) {
    if (BigInt_Compare(a, b) >= 0) {
        BigInt_Sub(res, a, b); // a가 크면 그냥 빼기
    } else {
        BigInt temp;
        BigInt_Sub(&temp, b, a);   // |a - b|
        BigInt_Sub(res, m, &temp); // m - |a - b|
    }
}

// 모듈러 곱셈: (a * b) mod m
void ModMul(BigInt* res, const BigInt* a, const BigInt* b, const BigInt* m) {
    BigInt temp;
    BigInt_Mul(&temp, a, b);
    BigInt_DivMod(NULL, res, &temp, m);
}

// ============================================================================
// [3] 타원 곡선 수학 (Elliptic Curve Cryptography over GF(p))
// ============================================================================

typedef struct {
    BigInt x;
    BigInt y;
    bool is_infinity; // 무한원점(Point at Infinity) 여부
} EC_Point;

// NIST P-256 파라미터 전역 변수
BigInt P256_p, P256_a, P256_b, P256_n;
EC_Point P256_G;

// 파라미터 초기화
void EC_InitParameters() {
    BigInt_FromHex(&P256_p, "FFFFFFFF00000001000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFF");
    BigInt_FromHex(&P256_a, "FFFFFFFF00000001000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFC"); // a = p - 3
    BigInt_FromHex(&P256_b, "5AC635D8AA3A93E7B3EBBD55769886BC651D06B0CC53B0F63BCE3C3E27D2604B");
    
    BigInt_FromHex(&P256_G.x, "6B17D1F2E12C4247F8BCE6E563A440F277037D812DEB33A0F4A13945D898C296");
    BigInt_FromHex(&P256_G.y, "4FE342E2FE1A7F9B8EE7EB4A7C0F9E162BCE33576B315ECECBB6406837BF51F5");
    P256_G.is_infinity = false;

    // 타원 곡선의 위수 (Order)
    BigInt_FromHex(&P256_n, "FFFFFFFF00000000FFFFFFFFFFFFFFFFBCE6FAADA7179E84F3B9CAC2FC632551");
}

// 점 두 배 연산 (Point Doubling): R = 2 * P
void EC_Point_Double(EC_Point* R, const EC_Point* P, const BigInt* a, const BigInt* p) {
    if (P->is_infinity || BigInt_IsZero(&P->y)) {
        R->is_infinity = true; return;
    }
    
    BigInt x_sq, temp1, temp2, lambda, y_two, y_two_inv;
    EC_Point R_out; R_out.is_infinity = false;
    
    // lambda = (3 * x^2 + a) / (2 * y) mod p
    ModMul(&x_sq, &P->x, &P->x, p);    // x^2
    ModAdd(&temp1, &x_sq, &x_sq, p);   // 2x^2
    ModAdd(&temp1, &temp1, &x_sq, p);  // 3x^2
    ModAdd(&temp2, &temp1, a, p);      // 3x^2 + a
    
    ModAdd(&y_two, &P->y, &P->y, p);   // 2y
    ModInverse(&y_two_inv, &y_two, p); // (2y)^-1
    ModMul(&lambda, &temp2, &y_two_inv, p); // lambda
    
    // x_r = lambda^2 - 2x
    ModMul(&temp1, &lambda, &lambda, p);
    ModAdd(&temp2, &P->x, &P->x, p);
    ModSub(&R_out.x, &temp1, &temp2, p);
    
    // y_r = lambda * (x - x_r) - y
    ModSub(&temp1, &P->x, &R_out.x, p);
    ModMul(&temp2, &lambda, &temp1, p);
    ModSub(&R_out.y, &temp2, &P->y, p);
    
    *R = R_out;
}

// 점 덧셈 연산 (Point Addition): R = P + Q
void EC_Point_Add(EC_Point* R, const EC_Point* P, const EC_Point* Q, const BigInt* a, const BigInt* p) {
    if (P->is_infinity) { *R = *Q; return; }
    if (Q->is_infinity) { *R = *P; return; }
    
    if (BigInt_Compare(&P->x, &Q->x) == 0) {
        if (BigInt_Compare(&P->y, &Q->y) == 0) {
            EC_Point_Double(R, P, a, p); // P == Q 이면 두배 연산 수행
        } else {
            R->is_infinity = true; // P == -Q 이면 무한원점
        }
        return;
    }
    
    BigInt dy, dx, dx_inv, lambda, temp1, temp2;
    EC_Point R_out; R_out.is_infinity = false;
    
    // lambda = (y2 - y1) / (x2 - x1) mod p
    ModSub(&dy, &Q->y, &P->y, p);
    ModSub(&dx, &Q->x, &P->x, p);
    ModInverse(&dx_inv, &dx, p);
    ModMul(&lambda, &dy, &dx_inv, p);
    
    // x_r = lambda^2 - x1 - x2
    ModMul(&temp1, &lambda, &lambda, p);
    ModSub(&temp2, &temp1, &P->x, p);
    ModSub(&R_out.x, &temp2, &Q->x, p);
    
    // y_r = lambda * (x1 - x_r) - y1
    ModSub(&temp1, &P->x, &R_out.x, p);
    ModMul(&temp2, &lambda, &temp1, p);
    ModSub(&R_out.y, &temp2, &P->y, p);
    
    *R = R_out;
}

// 스칼라 곱셈 (Scalar Multiplication): R = k * P (Double-and-Add 알고리즘)
void EC_Scalar_Mul(EC_Point* R, const EC_Point* P, const BigInt* k, const BigInt* a, const BigInt* p) {
    EC_Point res = {0};
    res.is_infinity = true;
    
    // 전체 비트 수 계산
    int max_bit = k->size * 32 - 1;
    while(max_bit >= 0) {
        int word_idx = max_bit / 32;
        int bit_idx = max_bit % 32;
        if ((k->data[word_idx] >> bit_idx) & 1) break;
        max_bit--;
    }
    
    // 최상위 비트(MSB)부터 스캔
    for (int i = max_bit; i >= 0; i--) {
        EC_Point_Double(&res, &res, a, p); // 무조건 2배
        
        int word_idx = i / 32;
        int bit_idx = i % 32;
        if ((k->data[word_idx] >> bit_idx) & 1) { // 해당 비트가 1이면 더하기
            EC_Point_Add(&res, &res, P, a, p);
        }
    }
    *R = res;
}

// ============================================================================
// [4] ECDH 로직 (개인키 생성 및 키 교환)
// ============================================================================

// 타원 곡선의 위수(n) 보다 작은 랜덤 개인키(스칼라) 생성
void EC_GeneratePrivateKey(BigInt* privKey) {
    do {
        BigInt_Init(privKey);
        int words = 256 / 32; // P-256은 256비트
        for (int i = 0; i < words; i++) {
            privKey->data[i] = (rand() << 16) | (rand() & 0xFFFF);
        }
        privKey->size = words;
        BigInt_Trim(privKey);
        // 0보다 크고, n보다 작을 때까지 반복
    } while (BigInt_IsZero(privKey) || BigInt_Compare(privKey, &P256_n) >= 0);
}

// ============================================================================
// [5] main() 함수: ECDH 파이프라인 시뮬레이션
// ============================================================================

int main() {
    srand((unsigned int)time(NULL));
    EC_InitParameters(); // NIST P-256 초기화
    
    printf("========== [ 순수 수학 구현 기반 ECDH 키 교환 시뮬레이션 ] ==========\n\n");
    
    BigInt alice_priv, bob_priv;
    EC_Point alice_pub, bob_pub, alice_shared, bob_shared;
    
    // 1. Alice 키 쌍 생성
    printf("[1] Alice 키 생성 중...\n");
    EC_GeneratePrivateKey(&alice_priv);
    EC_Scalar_Mul(&alice_pub, &P256_G, &alice_priv, &P256_a, &P256_p);
    
    printf("  > Alice Private Key: "); BigInt_PrintHex(&alice_priv); printf("\n");
    printf("  > Alice Public Key (X): "); BigInt_PrintHex(&alice_pub.x); printf("\n\n");
    
    // 2. Bob 키 쌍 생성
    printf("[2] Bob 키 생성 중...\n");
    EC_GeneratePrivateKey(&bob_priv);
    EC_Scalar_Mul(&bob_pub, &P256_G, &bob_priv, &P256_a, &P256_p);
    
    printf("  > Bob Private Key: "); BigInt_PrintHex(&bob_priv); printf("\n");
    printf("  > Bob Public Key (X): "); BigInt_PrintHex(&bob_pub.x); printf("\n\n");
    
    // 3. 키 교환 (서로의 공개키로 공유 비밀키 계산)
    printf("[3] ECDH 공유 비밀키(Shared Secret) 계산 중...\n");
    // Alice 측 계산: Shared = Alice_Priv * Bob_Pub
    EC_Scalar_Mul(&alice_shared, &bob_pub, &alice_priv, &P256_a, &P256_p);
    // Bob 측 계산: Shared = Bob_Priv * Alice_Pub
    EC_Scalar_Mul(&bob_shared, &alice_pub, &bob_priv, &P256_a, &P256_p);
    
    printf("\n  > Alice가 계산한 Shared Secret (X좌표):\n    "); 
    BigInt_PrintHex(&alice_shared.x); printf("\n");
    printf("  > Bob이 계산한 Shared Secret (X좌표):\n    "); 
    BigInt_PrintHex(&bob_shared.x); printf("\n\n");
    
    // 4. 검증
    if (BigInt_Compare(&alice_shared.x, &bob_shared.x) == 0 &&
        BigInt_Compare(&alice_shared.y, &bob_shared.y) == 0) {
        printf("[SUCCESS] 수학적 연산 결과, 두 사람의 공유 비밀키가 완벽히 일치합니다.\n");
    } else {
        printf("[FAILED] 연산 무결성 검증 실패.\n");
    }

    return 0;
}