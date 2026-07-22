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
// [2] 유틸리티 및 타원 곡선(ECC)용 모듈러 연산 추가
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

// [추가] 모듈러 거듭제곱: (base^exp) mod m (점 복원 시 y좌표 계산용)
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
// [3] 타원 곡선 수학 (Elliptic Curve Cryptography over GF(p))
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
// [추가] 점 압축 및 복원 함수
// ----------------------------------------------------------------------------

// EC Point를 압축하여 Prefix(0x02 또는 0x03) 반환 및 comp_x에 X좌표 저장
uint8_t EC_Point_Compress(const EC_Point* P, BigInt* comp_x) {
    if (P->is_infinity) return 0x00; // 무한원점은 0x00으로 표기
    
    BigInt_Copy(comp_x, &P->x);
    // Y좌표의 최하위 비트(LSB)가 0이면 짝수(0x02), 1이면 홀수(0x03)
    uint8_t parity = P->y.data[0] & 1;
    return 0x02 + parity; 
}

// Prefix와 X좌표로 Y좌표를 복원하여 점을 온전하게 만듦
bool EC_Point_Decompress(EC_Point* P, uint8_t prefix, const BigInt* comp_x, const BigInt* a, const BigInt* b, const BigInt* p) {
    if (prefix == 0x00) {
        P->is_infinity = true;
        return true;
    }
    if (prefix != 0x02 && prefix != 0x03) return false; // 잘못된 Prefix

    BigInt_Copy(&P->x, comp_x);
    P->is_infinity = false;

    BigInt x_sq, x_cb, ax, z, y, y_sq;
    
    // z = x^3 + ax + b mod p (타원 곡선 방정식 우항)
    ModMul(&x_sq, comp_x, comp_x, p);    // x^2
    ModMul(&x_cb, &x_sq, comp_x, p);     // x^3
    ModMul(&ax, a, comp_x, p);           // ax
    ModAdd(&z, &x_cb, &ax, p);           // x^3 + ax
    ModAdd(&z, &z, b, p);                // x^3 + ax + b

    // p_plus_1_over_4 = (p + 1) / 4 계산
    BigInt p_plus_1, four, p_plus_1_over_4, temp_one;
    BigInt_Init(&four); four.data[0] = 4; four.size = 1;
    BigInt_Init(&temp_one); temp_one.data[0] = 1; temp_one.size = 1;
    BigInt_Add(&p_plus_1, p, &temp_one);
    BigInt_DivMod(&p_plus_1_over_4, NULL, &p_plus_1, &four);

    // NIST P-256과 같이 p ≡ 3 (mod 4)인 경우: y = z^((p+1)/4) mod p
    ModExp(&y, &z, &p_plus_1_over_4, p);

    // 구한 y가 곡선 위의 올바른 해인지 검증 (z가 제곱잉여인지 확인)
    ModMul(&y_sq, &y, &y, p);
    if (BigInt_Compare(&y_sq, &z) != 0) {
        return false; // 유효하지 않은 점 (곡선 위에 없음)
    }

    // Prefix 패리티와 복원된 Y의 패리티 비교
    uint8_t y_parity = y.data[0] & 1;
    uint8_t expected_parity = prefix - 0x02;

    if (y_parity != expected_parity) {
        ModSub(&P->y, p, &y, p); // 패리티가 다르면 y = p - y 로 반전
    } else {
        BigInt_Copy(&P->y, &y);
    }

    return true;
}

// ============================================================================
// [4] ECDH 로직 (개인키 생성 및 키 교환)
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
// [5] main() 함수: ECDH 파이프라인 및 압축 시뮬레이션
// ============================================================================

int main() {
    EC_InitParameters();

    printf("========== [ ECDH 키 교환 및 점 압축 시뮬레이션 ] ==========\n\n");

    BigInt alice_priv, bob_priv;
    EC_Point alice_pub, bob_pub, alice_shared, bob_shared;
    
    // 압축용 변수
    uint8_t bob_pub_prefix;
    BigInt bob_pub_comp_x;
    EC_Point bob_pub_decompressed;

    // 1. Alice 키 쌍 생성
    printf("[1] Alice 키 생성 중...\n");
    EC_GeneratePrivateKey(&alice_priv);
    EC_Scalar_Mul(&alice_pub, &P256_G, &alice_priv, &P256_a, &P256_p);

    printf("  > Alice Private Key: "); BigInt_PrintHex(&alice_priv); printf("\n");
    printf("  > Alice Public Key (X): "); BigInt_PrintHex(&alice_pub.x); printf("\n");
    printf("  > Alice Public Key (Y): "); BigInt_PrintHex(&alice_pub.y); printf("\n\n");

    // 2. Bob 키 쌍 생성
    printf("[2] Bob 키 생성 중...\n");
    EC_GeneratePrivateKey(&bob_priv);
    EC_Scalar_Mul(&bob_pub, &P256_G, &bob_priv, &P256_a, &P256_p);

    printf("  > Bob Private Key: "); BigInt_PrintHex(&bob_priv); printf("\n");
    printf("  > Bob Public Key (X): "); BigInt_PrintHex(&bob_pub.x); printf("\n");
    printf("  > Bob Public Key (Y): "); BigInt_PrintHex(&bob_pub.y); printf("\n\n");

    // ---------------------------------------------------------
    // [추가된 구간] Bob의 공개키 압축 및 복원 (통신 시뮬레이션)
    // ---------------------------------------------------------
    printf("[3] Bob의 공개키를 압축하여 Alice에게 전송 시뮬레이션\n");
    
    // Bob이 자신의 공개키 압축
    bob_pub_prefix = EC_Point_Compress(&bob_pub, &bob_pub_comp_x);
    printf("  > Bob이 전송하는 압축 데이터:\n");
    printf("    Prefix: 0x%02X\n", bob_pub_prefix);
    printf("    X좌표 : "); BigInt_PrintHex(&bob_pub_comp_x); printf("\n\n");

    // Alice가 받은 데이터를 복원 (Decompression)
    printf("  > Alice가 수신한 압축 데이터 복원 중...\n");
    if (EC_Point_Decompress(&bob_pub_decompressed, bob_pub_prefix, &bob_pub_comp_x, &P256_a, &P256_b, &P256_p)) {
        printf("  > 복원된 Bob Public Key (Y): "); BigInt_PrintHex(&bob_pub_decompressed.y); printf("\n\n");
    } else {
        printf("  > [에러] 점 복원에 실패했습니다. (유효하지 않은 좌표)\n\n");
        return -1;
    }

    // 4. 키 교환 (서로의 공개키로 공유 비밀키 계산)
    printf("[4] ECDH 공유 비밀키(Shared Secret) 계산 중...\n");
    // Alice 측 계산: 압축 해제된 Bob의 키를 사용
    EC_Scalar_Mul(&alice_shared, &bob_pub_decompressed, &alice_priv, &P256_a, &P256_p);
    
    // Bob 측 계산
    EC_Scalar_Mul(&bob_shared, &alice_pub, &bob_priv, &P256_a, &P256_p);

    printf("\n  > Alice가 계산한 Shared Secret (복원된 키 사용):\n");
    printf("    (X) "); BigInt_PrintHex(&alice_shared.x); printf("\n");
    printf("    (Y) "); BigInt_PrintHex(&alice_shared.y); printf("\n\n");

    printf("  > Bob이 계산한 Shared Secret:\n");
    printf("    (X) "); BigInt_PrintHex(&bob_shared.x); printf("\n");
    printf("    (Y) "); BigInt_PrintHex(&bob_shared.y); printf("\n\n");

    // 5. 검증
    if (BigInt_Compare(&alice_shared.x, &bob_shared.x) == 0 &&
        BigInt_Compare(&alice_shared.y, &bob_shared.y) == 0) {
        printf("[SUCCESS] 압축 및 복원 후에도 양측의 공유 비밀키가 완벽히 일치합니다.\n");
    } else {
        printf("[FAILED] 연산 무결성 검증 실패.\n");
    }

    return 0;
}