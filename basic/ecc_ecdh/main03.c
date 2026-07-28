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
    // 크기가 0보다 크고, 최상위 블록의 값이 0인 동안 반복하여 size 감소
    while (a->size > 0 && a->data[a->size - 1] == 0) {
        a->size--;
    }
}

// BigInt가 0인지 확인하는 함수
bool BigInt_IsZero(const BigInt* a) {
    // 크기가 0이거나, 크기가 1인데 데이터가 0인 경우 true 반환
    return a->size == 0 || (a->size == 1 && a->data[0] == 0);
}

// 두 BigInt 값을 비교하는 함수 (a > b 이면 1, a < b 이면 -1, a == b 이면 0 반환)
int BigInt_Compare(const BigInt* a, const BigInt* b) {
    if (a->size > b->size) return 1;
    if (a->size < b->size) return -1;
    // 크기가 같다면 최상위 워드부터 아래로 내려가면서 값을 비교
    for (int i = a->size - 1; i >= 0; i--) {
        if (a->data[i] > b->data[i]) return 1;
        if (a->data[i] < b->data[i]) return -1;
    }
    return 0; // 모든 워드가 동일하면 0
}

// 덧셈 연산: res = a + b
void BigInt_Add(BigInt* res, const BigInt* a, const BigInt* b) {
    BigInt temp;
    BigInt_Init(&temp);
    uint64_t carry = 0; // 자리올림수 저장 변수 (오버플로우 방지를 위해 64비트 사용)
    int max_size = (a->size > b->size) ? a->size : b->size; // 둘 중 더 긴 배열의 크기

    // 긴 배열의 끝까지, 혹은 carry가 남아있는 동안 덧셈 수행
    for (int i = 0; i < max_size || carry > 0; i++) {
        if (i >= MAX_WORDS) break; // 최대 크기 초과 시 중단
        uint64_t sum = carry; // 이전 자리에서 올라온 carry부터 더함
        if (i < a->size) sum += a->data[i];
        if (i < b->size) sum += b->data[i];

        temp.data[i] = (uint32_t)(sum & 0xFFFFFFFF); // 하위 32비트만 현재 자리에 저장
        carry = sum >> 32; // 상위 32비트는 다음 자리로 carry 넘김
        temp.size = i + 1; // 사용 크기 갱신
    }
    BigInt_Copy(res, &temp);
}

// 뺄셈 연산: res = a - b
void BigInt_Sub(BigInt* res, const BigInt* a, const BigInt* b) {
    BigInt temp;
    BigInt_Init(&temp);
    int64_t borrow = 0; // 자리내림수 저장 변수

    // a의 크기만큼 반복하며 뺄셈 수행
    for (int i = 0; i < a->size; i++) {
        int64_t diff = (int64_t)a->data[i] - borrow; // a의 현재 자리에서 빌려준 값을 뺌
        if (i < b->size) diff -= b->data[i]; // b의 데이터가 있으면 마저 뺌

        if (diff < 0) { 
            diff += 0x100000000LL; // 음수가 되면 상위 자리에서 빌려옴 (2^32를 더함)
            borrow = 1; // 빌려왔음 표시
        } else {
            borrow = 0; // 빌림이 발생하지 않음
        }
        temp.data[i] = (uint32_t)diff; // 계산된 값을 저장
        temp.size = i + 1;
    }
    BigInt_Trim(&temp); // 앞쪽의 불필요한 0 제거
    BigInt_Copy(res, &temp); // 결과를 반환
}

// 곱셈 연산: res = a * b
void BigInt_Mul(BigInt* res, const BigInt* a, const BigInt* b) {
    BigInt temp;
    BigInt_Init(&temp);
    // 어느 하나라도 0이면 결과는 0
    if (BigInt_IsZero(a) || BigInt_IsZero(b)) {
        BigInt_Copy(res, &temp);
        return;
    }
    // 결과의 최대 크기는 a의 크기와 b의 크기를 합친 것
    temp.size = a->size + b->size;
    if (temp.size > MAX_WORDS) temp.size = MAX_WORDS; // 최대 버퍼 초과 방지

    // 세로 곱셈 방식으로 각 워드를 곱함
    for (int i = 0; i < a->size; i++) {
        uint64_t carry = 0;
        for (int j = 0; j < b->size; j++) {
            if (i + j >= MAX_WORDS) break;
            // a의 i번째 워드 * b의 j번째 워드 + 이전에 저장된 값 + 올림수
            uint64_t prod = (uint64_t)a->data[i] * b->data[j] + temp.data[i + j] + carry;
            temp.data[i + j] = (uint32_t)(prod & 0xFFFFFFFF); // 하위 32비트 저장
            carry = prod >> 32; // 올림수는 다음 자리에 전달
        }
        // 남은 carry가 있으면 다음 자리에 저장
        if (i + b->size < MAX_WORDS) {
            temp.data[i + b->size] = (uint32_t)carry;
        }
    }
    BigInt_Trim(&temp); // 최상위 0 제거
    BigInt_Copy(res, &temp);
}

// 1비트 좌측 시프트 (a = a << 1, 즉 a * 2 효과)
void BigInt_ShiftLeft1(BigInt* a) {
    if (BigInt_IsZero(a)) return;
    uint32_t carry = 0; // 하위 워드에서 넘어온 비트 저장
    for (int i = 0; i < a->size; i++) {
        uint32_t next_carry = a->data[i] >> 31; // 현재 워드의 최상위 비트를 다음 워드로 넘김
        a->data[i] = (a->data[i] << 1) | carry; // 1비트 왼쪽 이동하고, 넘어온 비트 결합
        carry = next_carry;
    }
    // 맨 마지막 워드에서 넘어온 비트가 있다면 배열 크기를 1 늘려 저장
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

    // 0으로 나누려 하거나, 나눠지는 수가 0인 경우 처리
    if (BigInt_IsZero(b) || BigInt_IsZero(a)) {
        if (q) BigInt_Copy(q, &temp_q);
        if (r) BigInt_Copy(r, &temp_r);
        return;
    }
    // a가 b보다 작으면 몫은 0, 나머지는 a가 됨
    if (BigInt_Compare(a, b) < 0) {
        if (q) BigInt_Copy(q, &temp_q);
        if (r) BigInt_Copy(r, a);
        return;
    }

    // a의 실제 유효 비트 길이 계산
    int a_bits = (a->size - 1) * 32;
    uint32_t top = a->data[a->size - 1];
    while (top) { a_bits++; top >>= 1; }

    temp_q.size = (a_bits + 31) / 32; // 몫이 차지할 워드 크기 할당

    // 최상위 비트부터 하나씩 내려오며 몫과 나머지를 계산
    for (int i = a_bits - 1; i >= 0; i--) {
        BigInt_ShiftLeft1(&temp_r); // 나머지를 왼쪽으로 1비트 시프트
        int word_idx = i / 32;
        int bit_idx = i % 32;
        // a의 해당 비트가 1이면 나머지의 최하위 비트에 1을 추가
        if ((a->data[word_idx] >> bit_idx) & 1) {
            temp_r.data[0] |= 1;
            if (temp_r.size == 0) temp_r.size = 1;
        }
        // 나머지가 나누는 수(b)보다 크거나 같으면
        if (BigInt_Compare(&temp_r, b) >= 0) {
            BigInt sub_res;
            BigInt_Sub(&sub_res, &temp_r, b); // 나머지에서 b를 뺌
            BigInt_Copy(&temp_r, &sub_res);
            temp_q.data[word_idx] |= (1U << bit_idx); // 몫의 해당 비트를 1로 설정
        }
    }
    BigInt_Trim(&temp_q);
    BigInt_Trim(&temp_r);

    if (q) BigInt_Copy(q, &temp_q); // 요청 시 몫 반환
    if (r) BigInt_Copy(r, &temp_r); // 요청 시 나머지 반환
}

// 모듈러 역원 연산 (확장 유클리드 알고리즘)
void ModInverse(BigInt* res, const BigInt* e, const BigInt* mod) {
    BigInt t, newt, r, newr, q, temp, prod, next_t;
    int t_sign = 1, newt_sign = 1; // 부호 처리 변수

    BigInt_Init(&t);
    BigInt_Init(&newt); newt.data[0] = 1; newt.size = 1; // newt = 1

    BigInt_Copy(&r, mod); // r = mod
    BigInt_Copy(&newr, e); // newr = e

    while (!BigInt_IsZero(&newr)) {
        BigInt_DivMod(&q, &temp, &r, &newr); // q = r / newr, temp = r % newr
        BigInt_Copy(&r, &newr); // r = newr
        BigInt_Copy(&newr, &temp); // newr = temp

        BigInt_Mul(&prod, &q, &newt); // prod = q * newt

        // t와 prod의 부호에 따른 덧셈/뺄셈 처리
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

    // 결과값이 음수이면 모듈러 값을 더해 양수로 만듦
    if (t_sign == -1) BigInt_Sub(res, mod, &t);
    else BigInt_Copy(res, &t);
}

// ============================================================================
// [2] ECC용 모듈러 연산 추가
// ============================================================================

// 16진수 문자열을 BigInt로 변환
void BigInt_FromHex(BigInt* a, const char* hex) {
    BigInt_Init(a);
    int len = strlen(hex);
    int word_idx = 0;
    // 문자열의 뒤에서부터 8글자(32비트)씩 끊어서 읽음
    for (int i = len; i > 0; i -= 8) {
        int start = i - 8;
        if (start < 0) start = 0;
        char buf[9] = {0};
        strncpy(buf, hex + start, i - start);
        a->data[word_idx++] = strtoul(buf, NULL, 16); // 16진수를 정수로 변환하여 배열에 저장
    }
    a->size = word_idx;
    BigInt_Trim(a); // 사용하지 않는 상위 배열 정리
}

// BigInt 값을 16진수 형태로 화면에 출력
void BigInt_PrintHex(const BigInt* a) {
    if (BigInt_IsZero(a)) {
        printf("00000000"); return;
    }
    for (int i = a->size - 1; i >= 0; i--) {
        if (i == a->size - 1) printf("%X", a->data[i]); // 첫 블록은 앞의 0 생략
        else printf("%08X", a->data[i]); // 중간 블록은 항상 8자리 맞춤
    }
}

// 모듈러 덧셈: res = (a + b) mod m
void ModAdd(BigInt* res, const BigInt* a, const BigInt* b, const BigInt* m) {
    BigInt temp;
    BigInt_Add(&temp, a, b);
    BigInt_DivMod(NULL, res, &temp, m); // 더한 결과를 m으로 나눈 나머지 계산
}

// 모듈러 뺄셈: res = (a - b) mod m
void ModSub(BigInt* res, const BigInt* a, const BigInt* b, const BigInt* m) {
    if (BigInt_Compare(a, b) >= 0) {
        BigInt_Sub(res, a, b); // a가 더 크면 그냥 뺌
    } else {
        BigInt temp;
        BigInt_Sub(&temp, b, a); // b가 더 크면 차이를 구하고
        BigInt_Sub(res, m, &temp); // m에서 그 차이만큼 빼서 양수화시킴 (음수 모듈러 연산)
    }
}

// 모듈러 곱셈: res = (a * b) mod m
void ModMul(BigInt* res, const BigInt* a, const BigInt* b, const BigInt* m) {
    BigInt temp;
    BigInt_Mul(&temp, a, b);
    BigInt_DivMod(NULL, res, &temp, m);
}

// 모듈러 지수승 연산: res = (base ^ exp) mod m (점 복원에 사용)
void ModExp(BigInt* res, const BigInt* base, const BigInt* exp, const BigInt* m) {
    BigInt result;
    BigInt_Init(&result);
    result.data[0] = 1; result.size = 1; // result = 1로 초기화

    if (BigInt_IsZero(exp)) { // 지수가 0이면 결과는 1
        BigInt_Copy(res, &result);
        return;
    }

    BigInt current_base;
    BigInt_Copy(&current_base, base);

    // 지수의 최고차항 비트를 찾음
    int max_bit = exp->size * 32 - 1;
    while(max_bit >= 0) {
        int word_idx = max_bit / 32;
        int bit_idx = max_bit % 32;
        if ((exp->data[word_idx] >> bit_idx) & 1) break; // 1인 비트 발견 시 멈춤
        max_bit--;
    }

    // Square-and-Multiply (제곱-곱) 알고리즘으로 빠르게 지수승 계산
    for (int i = 0; i <= max_bit; i++) {
        int word_idx = i / 32;
        int bit_idx = i % 32;
        if ((exp->data[word_idx] >> bit_idx) & 1) { // 해당 비트가 1이면 곱함
            ModMul(&result, &result, &current_base, m);
        }
        ModMul(&current_base, &current_base, &current_base, m); // 밑수(base)를 계속 제곱
    }
    BigInt_Copy(res, &result);
}


// ============================================================================
// [3] 타원 곡선 수학
// ============================================================================

// 타원 곡선(ECC) 상의 한 점을 표현하는 구조체
typedef struct {
    BigInt x; // X 좌표
    BigInt y; // Y 좌표
    bool is_infinity; // 무한원점 여부 (항등원)
} EC_Point;

// NIST P-256 규격 상수들
BigInt P256_p, P256_a, P256_b, P256_n;
EC_Point P256_G;

// P-256 타원 곡선의 매개변수를 초기화하는 함수
void EC_InitParameters() {
    // p: 소수 모듈러 값
    BigInt_FromHex(&P256_p, "FFFFFFFF00000001000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFF");
    // a, b: 타원 곡선 방정식(y^2 = x^3 + ax + b)의 계수
    BigInt_FromHex(&P256_a, "FFFFFFFF00000001000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFC");
    BigInt_FromHex(&P256_b, "5AC635D8AA3A93E7B3EBBD55769886BC651D06B0CC53B0F63BCE3C3E27D2604B");
    // G: 기준점 (Generator Point)의 X, Y 좌표
    BigInt_FromHex(&P256_G.x, "6B17D1F2E12C4247F8BCE6E563A440F277037D812DEB33A0F4A13945D898C296");
    BigInt_FromHex(&P256_G.y, "4FE342E2FE1A7F9B8EE7EB4A7C0F9E162BCE33576B315ECECBB6406837BF51F5");
    P256_G.is_infinity = false;
    // n: 기준점 G의 위수 (Order) - 개인키의 최대 범위
    BigInt_FromHex(&P256_n, "FFFFFFFF00000000FFFFFFFFFFFFFFFFBCE6FAADA7179E84F3B9CAC2FC632551");
}

// 점 2배 연산 (Point Doubling): R = P + P
void EC_Point_Double(EC_Point* R, const EC_Point* P, const BigInt* a, const BigInt* p) {
    if (P->is_infinity || BigInt_IsZero(&P->y)) { // 점이 무한원이거나 y가 0이면 결과도 무한원점
        R->is_infinity = true; return;
    }
    BigInt x_sq, temp1, temp2, lambda, y_two, y_two_inv;
    EC_Point R_out; R_out.is_infinity = false;

    // 접선의 기울기 계산: lambda = (3 * x^2 + a) / (2 * y) mod p
    ModMul(&x_sq, &P->x, &P->x, p);         // x^2
    ModAdd(&temp1, &x_sq, &x_sq, p);        // 2x^2
    ModAdd(&temp1, &temp1, &x_sq, p);       // 3x^2
    ModAdd(&temp2, &temp1, a, p);           // 3x^2 + a
    ModAdd(&y_two, &P->y, &P->y, p);        // 2y
    ModInverse(&y_two_inv, &y_two, p);      // (2y)^-1
    ModMul(&lambda, &temp2, &y_two_inv, p); // (3x^2 + a) * (2y)^-1

    // 새로운 점의 X 좌표 계산: X_r = lambda^2 - 2*x
    ModMul(&temp1, &lambda, &lambda, p);
    ModAdd(&temp2, &P->x, &P->x, p);
    ModSub(&R_out.x, &temp1, &temp2, p);

    // 새로운 점의 Y 좌표 계산: Y_r = lambda * (x - X_r) - y
    ModSub(&temp1, &P->x, &R_out.x, p);
    ModMul(&temp2, &lambda, &temp1, p);
    ModSub(&R_out.y, &temp2, &P->y, p);

    *R = R_out;
}

// 점 덧셈 연산 (Point Addition): R = P + Q
void EC_Point_Add(EC_Point* R, const EC_Point* P, const EC_Point* Q, const BigInt* a, const BigInt* p) {
    if (P->is_infinity) { *R = *Q; return; } // P가 무한원점이면 Q 반환
    if (Q->is_infinity) { *R = *P; return; } // Q가 무한원점이면 P 반환

    // P와 Q의 X좌표가 같을 경우 처리
    if (BigInt_Compare(&P->x, &Q->x) == 0) {
        if (BigInt_Compare(&P->y, &Q->y) == 0) {
            EC_Point_Double(R, P, a, p); // 같은 점이면 점 2배 연산 수행
        } else {
            R->is_infinity = true; // X좌표는 같고 Y좌표가 다르면 (P + -P)이므로 무한원점
        }
        return;
    }

    BigInt dy, dx, dx_inv, lambda, temp1, temp2;
    EC_Point R_out; R_out.is_infinity = false;

    // 직선의 기울기 계산: lambda = (y2 - y1) / (x2 - x1) mod p
    ModSub(&dy, &Q->y, &P->y, p);
    ModSub(&dx, &Q->x, &P->x, p);
    ModInverse(&dx_inv, &dx, p);
    ModMul(&lambda, &dy, &dx_inv, p);

    // 새로운 점의 X 좌표: X_r = lambda^2 - x1 - x2
    ModMul(&temp1, &lambda, &lambda, p);
    ModSub(&temp2, &temp1, &P->x, p);
    ModSub(&R_out.x, &temp2, &Q->x, p);

    // 새로운 점의 Y 좌표: Y_r = lambda * (x1 - X_r) - y1
    ModSub(&temp1, &P->x, &R_out.x, p);
    ModMul(&temp2, &lambda, &temp1, p);
    ModSub(&R_out.y, &temp2, &P->y, p);

    *R = R_out;
}

// 스칼라 곱셈 (Scalar Multiplication): R = k * P (Double-and-Add 알고리즘)
void EC_Scalar_Mul(EC_Point* R, const EC_Point* P, const BigInt* k, const BigInt* a, const BigInt* p) {
    EC_Point res = {0};
    res.is_infinity = true; // 결과값을 무한원점으로 초기화 (0으로 시작)

    int max_bit = k->size * 32 - 1;
    // 유효한 최상위 비트를 찾음
    while(max_bit >= 0) {
        int word_idx = max_bit / 32;
        int bit_idx = max_bit % 32;
        if ((k->data[word_idx] >> bit_idx) & 1) break;
        max_bit--;
    }

    // 최상위 비트부터 처리하는 좌에서 우로 스칼라 곱
    for (int i = max_bit; i >= 0; i--) {
        EC_Point_Double(&res, &res, a, p); // 2배 (Double)

        int word_idx = i / 32;
        int bit_idx = i % 32;
        if ((k->data[word_idx] >> bit_idx) & 1) { // 현재 비트가 1이면
            EC_Point_Add(&res, &res, P, a, p); // 더함 (Add)
        }
    }
    *R = res;
}

// ----------------------------------------------------------------------------
// 점 압축 및 복원 함수
// ----------------------------------------------------------------------------

// EC Point를 압축하여 Prefix(0x02 또는 0x03) 반환 및 comp_x에 X좌표 저장
uint8_t EC_Point_Compress(const EC_Point* P, BigInt* comp_x) {
    if (P->is_infinity) return 0x00; // 무한원점은 0x00으로 표기

    BigInt_Copy(comp_x, &P->x); // X 좌표는 그대로 복사
    // Y좌표의 최하위 비트(LSB)가 0이면 짝수(0x02), 1이면 홀수(0x03)를 반환
    uint8_t parity = P->y.data[0] & 1;
    return 0x02 + parity;
}

// Prefix와 X좌표로 Y좌표를 복원하여 점을 온전하게 만듦
bool EC_Point_Decompress(EC_Point* P, uint8_t prefix, const BigInt* comp_x, const BigInt* a, const BigInt* b, const BigInt* p) {
    if (prefix == 0x00) { // Prefix가 0x00이면 무한원점
        P->is_infinity = true;
        return true;
    }
    if (prefix != 0x02 && prefix != 0x03) return false; // 잘못된 Prefix 처리

    BigInt_Copy(&P->x, comp_x);
    P->is_infinity = false;

    BigInt x_sq, x_cb, ax, z, y, y_sq;

    // 타원 곡선 방정식 우항(z) 계산: z = x^3 + ax + b mod p
    ModMul(&x_sq, comp_x, comp_x, p);    // x^2
    ModMul(&x_cb, &x_sq, comp_x, p);     // x^3
    ModMul(&ax, a, comp_x, p);           // ax
    ModAdd(&z, &x_cb, &ax, p);           // x^3 + ax
    ModAdd(&z, &z, b, p);                // x^3 + ax + b

    // Y 좌표 구하기: y = z^((p+1)/4) mod p (p가 4로 나누어 3이 남는 경우 적용 가능, P-256이 이에 해당)
    BigInt p_plus_1, four, p_plus_1_over_4, temp_one;
    BigInt_Init(&four); four.data[0] = 4; four.size = 1;
    BigInt_Init(&temp_one); temp_one.data[0] = 1; temp_one.size = 1;
    
    // (p+1)/4 지수 계산
    BigInt_Add(&p_plus_1, p, &temp_one);
    BigInt_DivMod(&p_plus_1_over_4, NULL, &p_plus_1, &four);

    // 모듈러 지수승으로 Y 근 계산
    ModExp(&y, &z, &p_plus_1_over_4, p);

    // 구한 Y가 곡선 위의 올바른 해인지 검증 (y^2가 원래의 z와 같은지 확인)
    ModMul(&y_sq, &y, &y, p);
    if (BigInt_Compare(&y_sq, &z) != 0) {
        return false; // 유효하지 않은 점 (이 X좌표를 갖는 점은 곡선 위에 없음)
    }

    // 복원된 Y의 패리티가 주어진 Prefix와 맞는지 확인
    uint8_t y_parity = y.data[0] & 1;
    uint8_t expected_parity = prefix - 0x02;

    // 기대한 패리티와 다르면 반대편 Y좌표(p - y)가 정답임
    if (y_parity != expected_parity) {
        ModSub(&P->y, p, &y, p); // y = p - y 로 반전
    } else {
        BigInt_Copy(&P->y, &y);
    }

    return true;
}

// ============================================================================
// [4] ECDH 로직 (개인키 생성 및 키 교환)
// ============================================================================

// 안전한 난수 발생기 호출
int GenerateSecureRandom(uint8_t* buffer, size_t length) {
    ssize_t result = getrandom(buffer, length, 0);
    if (result != (ssize_t)length) return -1;
    return 0;
}

// 개인키를 안전하게 생성하는 함수
void EC_GeneratePrivateKey(BigInt* privKey) {
    int bytes = 256 / 8; // 32바이트(256비트)
    int words = 256 / 32; // 8워드
    do {
        BigInt_Init(privKey);
        // 32바이트 난수로 채움
        if (GenerateSecureRandom((uint8_t*)privKey->data, bytes) != 0) {
            fprintf(stderr, "Fatal Error: Failed to generate secure random numbers for private key.\n");
            exit(EXIT_FAILURE);
        }
        privKey->size = words;
        BigInt_Trim(privKey);
    // 개인키는 0이 아니어야 하고, 타원 곡선의 위수(n)보다 작아야 함
    } while (BigInt_IsZero(privKey) || BigInt_Compare(privKey, &P256_n) >= 0);
}

// ============================================================================
// [5] main() 함수
// ============================================================================

int main() {
    EC_InitParameters(); // P-256 타원곡선 매개변수 초기화

    printf("========== [ ECDH 키 교환 및 점 압축 시뮬레이션 ] ==========\n\n");

    BigInt alice_priv, bob_priv;
    EC_Point alice_pub, bob_pub, alice_shared, bob_shared;
    
    // 압축용 변수 선언
    uint8_t bob_pub_prefix;
    BigInt bob_pub_comp_x;
    EC_Point bob_pub_decompressed;

    // Alice 키 쌍 생성 (개인키 뽑고 G를 곱해서 공개키 생성)
    printf("Alice 키 생성 중...\n");
    EC_GeneratePrivateKey(&alice_priv);
    EC_Scalar_Mul(&alice_pub, &P256_G, &alice_priv, &P256_a, &P256_p);

    printf("Alice Private Key: "); BigInt_PrintHex(&alice_priv); printf("\n");
    printf("Alice Public Key (X): "); BigInt_PrintHex(&alice_pub.x); printf("\n");
    printf("Alice Public Key (Y): "); BigInt_PrintHex(&alice_pub.y); printf("\n\n");

    // Bob 키 쌍 생성
    printf("Bob 키 생성 중...\n");
    EC_GeneratePrivateKey(&bob_priv);
    EC_Scalar_Mul(&bob_pub, &P256_G, &bob_priv, &P256_a, &P256_p);

    printf("Bob Private Key: "); BigInt_PrintHex(&bob_priv); printf("\n");
    printf("Bob Public Key (X): "); BigInt_PrintHex(&bob_pub.x); printf("\n");
    printf("Bob Public Key (Y): "); BigInt_PrintHex(&bob_pub.y); printf("\n\n");

    // Bob의 공개키 압축 (전송 용량 감소)
    printf("Bob의 공개키를 압축하여 Alice에게 전송\n");
    bob_pub_prefix = EC_Point_Compress(&bob_pub, &bob_pub_comp_x);
    printf("Bob이 전송하는 압축 데이터: 0x%02X", bob_pub_prefix);
    BigInt_PrintHex(&bob_pub_comp_x); printf("\n\n");

    // Alice가 받은 Bob의 압축된 데이터를 온전한 점(공개키)으로 복원
    printf("Alice가 수신한 압축 데이터 복원 중...\n");
    if (EC_Point_Decompress(&bob_pub_decompressed, bob_pub_prefix, &bob_pub_comp_x, &P256_a, &P256_b, &P256_p)) {
        printf("복원된 Bob Public Key (Y): "); BigInt_PrintHex(&bob_pub_decompressed.y); printf("\n\n");
    } else {
        printf("[에러] 점 복원에 실패했습니다. (유효하지 않은 좌표)\n\n");
        return -1;
    }

    // ECDH 키 교환 시뮬레이션
    printf("ECDH 공유 비밀키(Shared Secret) 계산 중...\n");
    
    // Alice 측 계산: 압축 해제된 Bob의 공개키에 자신의 개인키 곱함 ( S = a * B )
    EC_Scalar_Mul(&alice_shared, &bob_pub_decompressed, &alice_priv, &P256_a, &P256_p);
    
    // Bob 측 계산: Alice의 공개키에 자신의 개인키 곱함 ( S = b * A )
    EC_Scalar_Mul(&bob_shared, &alice_pub, &bob_priv, &P256_a, &P256_p);

    printf("\nAlice가 계산한 Shared Secret (복원된 키 사용):\n");
    printf("    (X) "); BigInt_PrintHex(&alice_shared.x); printf("\n");
    printf("    (Y) "); BigInt_PrintHex(&alice_shared.y); printf("\n\n");

    printf("Bob이 계산한 Shared Secret:\n");
    printf("    (X) "); BigInt_PrintHex(&bob_shared.x); printf("\n");
    printf("    (Y) "); BigInt_PrintHex(&bob_shared.y); printf("\n\n");

    // 검증
    if (BigInt_Compare(&alice_shared.x, &bob_shared.x) == 0 &&
        BigInt_Compare(&alice_shared.y, &bob_shared.y) == 0) {
        printf("[SUCCESS] 압축 및 복원 후에도 양측의 공유 비밀키가 완벽히 일치합니다.\n");
    } else {
        printf("[FAILED] 연산 무결성 검증 실패.\n");
    }

    return 0;
}