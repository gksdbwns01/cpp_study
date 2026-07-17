#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

// ============================================================================
// [1] BigInt 기본 구현
// ============================================================================

#define MAX_WORDS 256  // BigInt가 저장할 수 있는 최대 32비트 워드 개수 (256 * 32 = 8192비트까지 표현 가능)

typedef struct {
    uint32_t data[MAX_WORDS]; // 리틀 엔디안 방식으로 저장되는 워드 배열 (data[0]이 최하위 32비트)
    int size;                 // 실제로 사용 중인 워드 개수 (0이면 값이 0)
} BigInt;

// BigInt를 0으로 초기화하는 함수
void BigInt_Init(BigInt* a) {
    memset(a->data, 0, sizeof(a->data)); // data 배열 전체를 0으로 클리어
    a->size = 0;                          // 사용 중인 워드 수를 0으로 설정
}

// src의 내용을 dest로 복사하는 함수 (깊은 복사)
void BigInt_Copy(BigInt* dest, const BigInt* src) {
    memcpy(dest->data, src->data, sizeof(src->data)); // 전체 data 배열을 통째로 복사
    dest->size = src->size;                            // size 필드도 복사
}

// 상위 워드 중 불필요한 0을 제거해서 size를 정규화하는 함수 (예: [5,3,0,0] -> size=2)
void BigInt_Trim(BigInt* a) {
    while (a->size > 0 && a->data[a->size - 1] == 0) { // 최상위 워드가 0인 동안 반복
        a->size--;                                       // size를 줄여서 최상위 유효 워드까지만 남김
    }
}

// BigInt가 0인지 검사하는 함수
bool BigInt_IsZero(const BigInt* a) {
    return a->size == 0 || (a->size == 1 && a->data[0] == 0); // size가 0이거나, size가 1인데 그 값이 0이면 0으로 간주
}

// a와 b의 크기를 비교하는 함수 (a>b: 1, a<b: -1, a==b: 0)
int BigInt_Compare(const BigInt* a, const BigInt* b) {
    if (a->size > b->size) return 1;   // 워드 개수가 많으면 값이 더 큼
    if (a->size < b->size) return -1;  // 워드 개수가 적으면 값이 더 작음
    for (int i = a->size - 1; i >= 0; i--) { // 워드 개수가 같으면 최상위 워드부터 비교
        if (a->data[i] > b->data[i]) return 1;
        if (a->data[i] < b->data[i]) return -1;
    }
    return 0; // 모든 워드가 같으면 두 수는 동일
}

// res = a + b 를 계산하는 덧셈 함수
void BigInt_Add(BigInt* res, const BigInt* a, const BigInt* b) {
    BigInt temp;             // 결과를 임시로 담을 지역 변수 (res가 a나 b와 같은 포인터여도 안전하게 처리하기 위함)
    BigInt_Init(&temp);      // temp를 0으로 초기화
    uint64_t carry = 0;      // 자리올림(캐리) 값을 64비트로 저장 (32비트 덧셈의 오버플로우를 담기 위해)
    int max_size = (a->size > b->size) ? a->size : b->size; // a, b 중 더 긴 쪽의 워드 수

    for (int i = 0; i < max_size || carry > 0; i++) { // 두 수 중 긴 쪽 길이만큼, 그리고 캐리가 남아있는 동안 반복
        if (i >= MAX_WORDS) break;                       // 버퍼 오버플로우 방지: 최대 워드 수를 넘으면 중단
        uint64_t sum = carry;                            // 이번 자리의 합을 캐리로 시작
        if (i < a->size) sum += a->data[i];              // a의 해당 워드가 존재하면 더함
        if (i < b->size) sum += b->data[i];              // b의 해당 워드가 존재하면 더함

        temp.data[i] = (uint32_t)(sum & 0xFFFFFFFF);     // 하위 32비트만 이번 자리 결과로 저장
        carry = sum >> 32;                                // 상위 비트는 다음 자리로 넘길 캐리가 됨
        temp.size = i + 1;                                // 지금까지 채운 워드 수를 기록
    }
    BigInt_Copy(res, &temp); // 계산이 끝난 temp를 실제 결과(res)로 복사
}

// res = a - b 를 계산하는 뺄셈 함수 (a >= b 라고 가정, 음수 처리 없음)
void BigInt_Sub(BigInt* res, const BigInt* a, const BigInt* b) {
    BigInt temp;              // 결과 임시 저장용
    BigInt_Init(&temp);
    int64_t borrow = 0;       // 빌림(borrow) 값

    for (int i = 0; i < a->size; i++) { // a의 워드 수만큼 반복 (a가 더 크거나 같다고 가정하므로 a 기준)
        int64_t diff = (int64_t)a->data[i] - borrow; // 이전 자리에서 빌려온 값을 먼저 뺌
        if (i < b->size) diff -= b->data[i];          // b의 해당 워드가 존재하면 추가로 뺌

        if (diff < 0) {               // 결과가 음수면 다음 자리에서 빌려와야 함
            diff += 0x100000000LL;    // 2^32를 더해서 양수로 보정 (빌림 처리)
            borrow = 1;               // 다음 자리에 빌림이 발생했음을 표시
        } else {
            borrow = 0;               // 빌림이 필요 없으면 0으로 리셋
        }
        temp.data[i] = (uint32_t)diff; // 보정된 하위 32비트를 결과로 저장
        temp.size = i + 1;              // 현재까지 채운 워드 수 갱신
    }
    BigInt_Trim(&temp);       // 상위의 불필요한 0 워드를 제거
    BigInt_Copy(res, &temp);  // 최종 결과를 res로 복사
}

// res = a * b 를 계산하는 곱셈 함수 (Long Multiplication, O(n*m))
void BigInt_Mul(BigInt* res, const BigInt* a, const BigInt* b) {
    BigInt temp;
    BigInt_Init(&temp);
    if (BigInt_IsZero(a) || BigInt_IsZero(b)) { // 둘 중 하나라도 0이면 결과는 무조건 0
        BigInt_Copy(res, &temp);
        return;
    }
    temp.size = a->size + b->size;              // 곱셈 결과의 최대 가능한 워드 수 (a자릿수 + b자릿수)
    if (temp.size > MAX_WORDS) temp.size = MAX_WORDS; // 버퍼 크기를 넘지 않도록 클램프

    for (int i = 0; i < a->size; i++) {         // a의 각 워드에 대해
        uint64_t carry = 0;                      // 이번 a워드를 곱할 때 발생하는 캐리
        for (int j = 0; j < b->size; j++) {      // b의 각 워드와 곱함
            if (i + j >= MAX_WORDS) break;         // 결과 인덱스가 버퍼를 넘으면 중단
            // a[i]*b[j] + 기존에 누적된 temp[i+j] + 이전 캐리를 모두 더함
            uint64_t prod = (uint64_t)a->data[i] * b->data[j] + temp.data[i + j] + carry;
            temp.data[i + j] = (uint32_t)(prod & 0xFFFFFFFF); // 하위 32비트를 해당 자리에 저장
            carry = prod >> 32;                                // 상위 비트는 캐리로 이월
        }
        if (i + b->size < MAX_WORDS) {           // 마지막 남은 캐리를 그 다음 자리에 반영
            temp.data[i + b->size] = (uint32_t)carry;
        }
    }
    BigInt_Trim(&temp);       // 불필요한 상위 0 워드 제거
    BigInt_Copy(res, &temp);  // 결과를 res로 복사
}

// a를 왼쪽으로 1비트 시프트하는 함수 (a = a * 2), in-place 연산
void BigInt_ShiftLeft1(BigInt* a) {
    if (BigInt_IsZero(a)) return; // 0이면 시프트해도 그대로 0이므로 바로 리턴
    uint32_t carry = 0;            // 이전 워드에서 넘어온 최상위 비트(캐리)
    for (int i = 0; i < a->size; i++) {
        uint32_t next_carry = a->data[i] >> 31;      // 현재 워드의 최상위 비트를 다음 캐리로 저장
        a->data[i] = (a->data[i] << 1) | carry;       // 현재 워드를 1비트 시프트하고 이전 캐리를 최하위 비트로 채움
        carry = next_carry;                            // 캐리 갱신
    }
    if (carry > 0 && a->size < MAX_WORDS) { // 최상위에서 넘친 비트가 있으면 새로운 워드를 추가
        a->data[a->size] = carry;
        a->size++;
    }
}

// q = a / b, r = a % b 를 계산하는 나눗셈 함수 (비트 단위 시프트-뺄셈 방식, 매우 느림: O(bits^2))
void BigInt_DivMod(BigInt* q, BigInt* r, const BigInt* a, const BigInt* b) {
    BigInt temp_q, temp_r;    // 몫과 나머지를 담을 임시 변수
    BigInt_Init(&temp_q);
    BigInt_Init(&temp_r);

    if (BigInt_IsZero(b) || BigInt_IsZero(a)) { // 0으로 나누거나 a가 0이면 몫=0, 나머지=0
        if (q) BigInt_Copy(q, &temp_q);
        if (r) BigInt_Copy(r, &temp_r);
        return;
    }
    if (BigInt_Compare(a, b) < 0) { // a가 b보다 작으면 몫은 0, 나머지는 a 그대로
        if (q) BigInt_Copy(q, &temp_q);
        if (r) BigInt_Copy(r, a);
        return;
    }

    // a의 전체 비트 길이(a_bits)를 계산
    int a_bits = (a->size - 1) * 32;      // 최상위 워드를 제외한 나머지 워드들의 비트 수
    uint32_t top = a->data[a->size - 1];  // 최상위 워드
    while (top) { a_bits++; top >>= 1; }  // 최상위 워드 안에서 유효 비트 수를 세어 더함

    temp_q.size = (a_bits + 31) / 32; // 몫이 저장될 최대 워드 수를 미리 설정 (뒤에서 필요한 비트만 세팅됨)

    // a의 최상위 비트부터 하위 비트까지 한 비트씩 내려가며 긴 나눗셈(long division) 수행
    for (int i = a_bits - 1; i >= 0; i--) {
        BigInt_ShiftLeft1(&temp_r); // 나머지를 왼쪽으로 1비트 시프트 (다음 비트를 받아들일 자리 마련)
        int word_idx = i / 32;       // a에서 i번째 비트가 속한 워드 인덱스
        int bit_idx = i % 32;        // 해당 워드 내에서의 비트 위치
        if ((a->data[word_idx] >> bit_idx) & 1) { // a의 i번째 비트가 1이면
            temp_r.data[0] |= 1;                      // temp_r의 최하위 비트에 그 비트를 끌어옴
            if (temp_r.size == 0) temp_r.size = 1;    // temp_r이 비어있었다면 size를 1로 보정
        }
        if (BigInt_Compare(&temp_r, b) >= 0) { // 현재까지의 부분 나머지가 b 이상이면 뺄 수 있음
            BigInt sub_res;
            BigInt_Sub(&sub_res, &temp_r, b);  // temp_r -= b
            BigInt_Copy(&temp_r, &sub_res);
            temp_q.data[word_idx] |= (1U << bit_idx); // 몫의 해당 비트를 1로 세팅
        }
    }
    BigInt_Trim(&temp_q); // 몫의 불필요한 상위 0 제거
    BigInt_Trim(&temp_r); // 나머지의 불필요한 상위 0 제거

    if (q) BigInt_Copy(q, &temp_q); // 호출자가 몫을 요청했으면 복사
    if (r) BigInt_Copy(r, &temp_r); // 호출자가 나머지를 요청했으면 복사
}

// 확장 유클리드 호제법 (제공된 코드와 동일)
// res = e^(-1) mod mod 를 계산 (e의 mod에 대한 모듈러 역원)
void ModInverse(BigInt* res, const BigInt* e, const BigInt* mod) {
    BigInt t, newt, r, newr, q, temp, prod, next_t; // 확장 유클리드 알고리즘에서 쓰이는 계수들
    int t_sign = 1, newt_sign = 1; // t와 newt는 음수가 될 수 있으므로 부호를 별도 변수로 관리

    BigInt_Init(&t);                          // t = 0 으로 시작
    BigInt_Init(&newt); newt.data[0] = 1; newt.size = 1; // newt = 1 로 시작

    BigInt_Copy(&r, mod);   // r = mod (초기 나머지 r0)
    BigInt_Copy(&newr, e);  // newr = e (초기 나머지 r1)

    while (!BigInt_IsZero(&newr)) { // newr이 0이 될 때까지 유클리드 호제법 반복
        BigInt_DivMod(&q, &temp, &r, &newr); // q = r / newr, temp = r % newr
        BigInt_Copy(&r, &newr);                // r <- newr (한 단계 전진)
        BigInt_Copy(&newr, &temp);             // newr <- temp(나머지)

        BigInt_Mul(&prod, &q, &newt); // prod = q * newt (t를 갱신하기 위한 곱)

        // t_next = t - q*newt 를 부호를 고려하여 계산 (모두 절댓값으로 저장하고 부호는 따로 추적)
        if (t_sign == newt_sign) { // t와 newt(및 prod)의 부호가 같은 경우: 뺄셈으로 처리
            if (BigInt_Compare(&t, &prod) >= 0) {
                BigInt_Sub(&next_t, &t, &prod); // |t| >= |prod| 이면 그대로 빼고 부호는 t_sign 유지
            } else {
                BigInt_Sub(&next_t, &prod, &t); // |t| < |prod| 이면 반대로 빼고 부호를 뒤집음
                t_sign = -t_sign;
            }
        } else {
            BigInt_Add(&next_t, &t, &prod); // 부호가 다르면 절댓값끼리 더함 (실제로는 부호가 있는 뺄셈의 결과)
        }

        BigInt_Copy(&t, &newt);  // t <- newt (한 단계 전진)
        t_sign = newt_sign;
        BigInt_Copy(&newt, &next_t); // newt <- 방금 계산한 next_t
        newt_sign = t_sign == newt_sign ? -newt_sign : t_sign; // newt의 다음 부호를 결정 (주의: 다소 관례적이지 않은 부호 갱신 로직)
    }

    if (t_sign == -1) BigInt_Sub(res, mod, &t); // 최종 t가 음수이면 mod를 더해 양수 범위로 보정 (res = mod - |t|)
    else BigInt_Copy(res, &t);                   // 양수이면 그대로 결과로 사용
}

// ============================================================================
// [2] 유틸리티 및 타원 곡선(ECC)용 모듈러 연산 추가
// ============================================================================

// 16진수 문자열을 BigInt로 변환 (상수 입력용)
void BigInt_FromHex(BigInt* a, const char* hex) {
    BigInt_Init(a);            // 결과를 0으로 초기화
    int len = strlen(hex);     // 입력 16진수 문자열의 길이
    int word_idx = 0;          // 채워 넣을 워드 인덱스 (하위 워드부터 채움)
    for (int i = len; i > 0; i -= 8) { // 문자열 끝에서부터 8자(=32비트)씩 잘라서 처리
        int start = i - 8;               // 이번에 자를 부분의 시작 위치
        if (start < 0) start = 0;        // 맨 앞부분이 8자 미만일 수 있으므로 0으로 클램프
        char buf[9] = {0};                // 최대 8자 + NULL 종료를 담을 임시 버퍼
        strncpy(buf, hex + start, i - start); // 해당 구간의 문자를 buf로 복사
        a->data[word_idx++] = strtoul(buf, NULL, 16); // 16진수 문자열을 정수로 변환해 워드에 저장
    }
    a->size = word_idx;  // 채워진 워드 개수를 size로 설정
    BigInt_Trim(a);       // 상위의 불필요한 0 워드 제거
}

// BigInt를 16진수 문자열 형태로 출력하는 함수
void BigInt_PrintHex(const BigInt* a) {
    if (BigInt_IsZero(a)) {         // 값이 0이면 그냥 "00000000" 출력하고 종료
        printf("00000000"); return;
    }
    for (int i = a->size - 1; i >= 0; i--) { // 최상위 워드부터 출력 (사람이 읽는 순서)
        if (i == a->size - 1) printf("%X", a->data[i]);  // 최상위 워드는 앞에 불필요한 0을 붙이지 않음
        else printf("%08X", a->data[i]);                  // 나머지 워드는 8자리로 맞춰 0-padding 출력
    }
}

// 모듈러 덧셈: (a + b) mod m
void ModAdd(BigInt* res, const BigInt* a, const BigInt* b, const BigInt* m) {
    BigInt temp;
    BigInt_Add(&temp, a, b);          // 먼저 a + b를 그냥 계산 (오버플로우 가능성 있는 상태)
    BigInt_DivMod(NULL, res, &temp, m); // temp를 m으로 나눈 나머지만 필요하므로 몫은 버리고(NULL) 나머지를 res에 저장
}

// 모듈러 뺄셈: (a - b) mod m (음수 방지 처리)
void ModSub(BigInt* res, const BigInt* a, const BigInt* b, const BigInt* m) {
    if (BigInt_Compare(a, b) >= 0) {
        BigInt_Sub(res, a, b); // a가 b보다 크거나 같으면 그냥 빼도 결과가 음수가 되지 않음
    } else {
        BigInt temp;
        BigInt_Sub(&temp, b, a);   // a < b 인 경우 먼저 |a - b| = b - a 를 계산
        BigInt_Sub(res, m, &temp); // 그 다음 m - |a-b| 를 계산해서 모듈러 범위 안의 양수로 보정
    }
}

// 모듈러 곱셈: (a * b) mod m
void ModMul(BigInt* res, const BigInt* a, const BigInt* b, const BigInt* m) {
    BigInt temp;
    BigInt_Mul(&temp, a, b);          // 먼저 a * b를 그냥 계산 (자릿수가 늘어날 수 있음)
    BigInt_DivMod(NULL, res, &temp, m); // m으로 나눈 나머지만 필요하므로 몫은 버림
}

// ============================================================================
// [3] 타원 곡선 수학 (Elliptic Curve Cryptography over GF(p))
// ============================================================================

typedef struct {
    BigInt x;             // 점의 x좌표
    BigInt y;             // 점의 y좌표
    bool is_infinity;     // 무한원점(Point at Infinity) 여부 - 타원곡선의 항등원 역할
} EC_Point;

// NIST P-256 파라미터 전역 변수
BigInt P256_p, P256_a, P256_b, P256_n; // p: 소수 모듈러스, a,b: 곡선 계수(y^2 = x^3+ax+b), n: 곡선의 위수(order)
EC_Point P256_G;                         // 기준점(Generator point)

// 파라미터 초기화
void EC_InitParameters() {
    // NIST P-256 표준 곡선의 소수 모듈러스 p
    BigInt_FromHex(&P256_p, "FFFFFFFF00000001000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFF");
    BigInt_FromHex(&P256_a, "FFFFFFFF00000001000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFC"); // a = p - 3 (P-256 곡선 계수)
    BigInt_FromHex(&P256_b, "5AC635D8AA3A93E7B3EBBD55769886BC651D06B0CC53B0F63BCE3C3E27D2604B"); // 곡선 계수 b

    // 기준점 G의 x, y 좌표 (표준에 정의된 값)
    BigInt_FromHex(&P256_G.x, "6B17D1F2E12C4247F8BCE6E563A440F277037D812DEB33A0F4A13945D898C296");
    BigInt_FromHex(&P256_G.y, "4FE342E2FE1A7F9B8EE7EB4A7C0F9E162BCE33576B315ECECBB6406837BF51F5");
    P256_G.is_infinity = false; // 기준점은 무한원점이 아님

    // 타원 곡선의 위수 (Order) - G를 몇 번 더해야 무한원점으로 돌아오는지에 대한 값
    BigInt_FromHex(&P256_n, "FFFFFFFF00000000FFFFFFFFFFFFFFFFBCE6FAADA7179E84F3B9CAC2FC632551");
}

// 점 두 배 연산 (Point Doubling): R = 2 * P
void EC_Point_Double(EC_Point* R, const EC_Point* P, const BigInt* a, const BigInt* p) {
    if (P->is_infinity || BigInt_IsZero(&P->y)) { // P가 무한원점이거나 y=0(자기 자신의 역원)이면 2P는 무한원점
        R->is_infinity = true; return;
    }

    BigInt x_sq, temp1, temp2, lambda, y_two, y_two_inv; // 계산 과정에서 쓰이는 임시 변수들
    EC_Point R_out; R_out.is_infinity = false;             // 결과를 담을 임시 점 (무한원점 아님으로 초기화)

    // lambda = (3 * x^2 + a) / (2 * y) mod p  -- 접선의 기울기 계산
    ModMul(&x_sq, &P->x, &P->x, p);    // x^2 mod p
    ModAdd(&temp1, &x_sq, &x_sq, p);   // 2x^2 mod p
    ModAdd(&temp1, &temp1, &x_sq, p);  // 3x^2 mod p (2x^2 + x^2)
    ModAdd(&temp2, &temp1, a, p);      // 3x^2 + a mod p (분자)

    ModAdd(&y_two, &P->y, &P->y, p);   // 2y mod p (분모)
    ModInverse(&y_two_inv, &y_two, p); // (2y)^-1 mod p (분모의 모듈러 역원, 즉 나눗셈을 곱셈으로 변환)
    ModMul(&lambda, &temp2, &y_two_inv, p); // lambda = 분자 * 분모의 역원

    // x_r = lambda^2 - 2x
    ModMul(&temp1, &lambda, &lambda, p); // lambda^2
    ModAdd(&temp2, &P->x, &P->x, p);      // 2x
    ModSub(&R_out.x, &temp1, &temp2, p); // lambda^2 - 2x = 새로운 x좌표

    // y_r = lambda * (x - x_r) - y
    ModSub(&temp1, &P->x, &R_out.x, p);   // x - x_r
    ModMul(&temp2, &lambda, &temp1, p);   // lambda * (x - x_r)
    ModSub(&R_out.y, &temp2, &P->y, p);  // 위 결과 - y = 새로운 y좌표

    *R = R_out; // 계산된 결과를 R에 반영 (R이 P나 res와 같은 포인터여도 안전하도록 임시 변수 사용)
}

// 점 덧셈 연산 (Point Addition): R = P + Q
void EC_Point_Add(EC_Point* R, const EC_Point* P, const EC_Point* Q, const BigInt* a, const BigInt* p) {
    if (P->is_infinity) { *R = *Q; return; } // P가 무한원점(항등원)이면 결과는 그냥 Q
    if (Q->is_infinity) { *R = *P; return; } // Q가 무한원점이면 결과는 그냥 P

    if (BigInt_Compare(&P->x, &Q->x) == 0) { // x좌표가 같은 경우 (같은 점이거나 서로 역원인 경우)
        if (BigInt_Compare(&P->y, &Q->y) == 0) {
            EC_Point_Double(R, P, a, p); // P == Q 이면 점 두배(덧셈 공식이 아닌 접선 공식) 연산 수행
        } else {
            R->is_infinity = true; // P == -Q (같은 x, 다른 y)이면 두 점의 합은 무한원점
        }
        return;
    }

    BigInt dy, dx, dx_inv, lambda, temp1, temp2; // 계산용 임시 변수
    EC_Point R_out; R_out.is_infinity = false;

    // lambda = (y2 - y1) / (x2 - x1) mod p  -- 두 점을 잇는 직선의 기울기
    ModSub(&dy, &Q->y, &P->y, p);       // y2 - y1
    ModSub(&dx, &Q->x, &P->x, p);       // x2 - x1
    ModInverse(&dx_inv, &dx, p);        // (x2-x1)^-1 mod p
    ModMul(&lambda, &dy, &dx_inv, p);   // lambda = dy * dx_inv

    // x_r = lambda^2 - x1 - x2
    ModMul(&temp1, &lambda, &lambda, p); // lambda^2
    ModSub(&temp2, &temp1, &P->x, p);     // lambda^2 - x1
    ModSub(&R_out.x, &temp2, &Q->x, p);  // (lambda^2 - x1) - x2 = 새로운 x좌표

    // y_r = lambda * (x1 - x_r) - y1
    ModSub(&temp1, &P->x, &R_out.x, p);   // x1 - x_r
    ModMul(&temp2, &lambda, &temp1, p);   // lambda * (x1 - x_r)
    ModSub(&R_out.y, &temp2, &P->y, p);  // 위 결과 - y1 = 새로운 y좌표

    *R = R_out; // 최종 결과를 R에 반영
}

// 스칼라 곱셈 (Scalar Multiplication): R = k * P (Double-and-Add 알고리즘)
void EC_Scalar_Mul(EC_Point* R, const EC_Point* P, const BigInt* k, const BigInt* a, const BigInt* p) {
    EC_Point res = {0};       // 결과 누산 변수, 모든 필드를 0으로 초기화
    res.is_infinity = true;  // 처음에는 항등원(무한원점)에서 시작 (곱셈에서 1과 유사한 역할)

    // k의 최상위 유효 비트(MSB) 위치를 찾음
    int max_bit = k->size * 32 - 1; // 이론상 최상위 비트 위치 후보에서 시작
    while(max_bit >= 0) {
        int word_idx = max_bit / 32; // 해당 비트가 속한 워드
        int bit_idx = max_bit % 32;  // 워드 내 비트 위치
        if ((k->data[word_idx] >> bit_idx) & 1) break; // 실제로 1인 최상위 비트를 찾으면 멈춤
        max_bit--; // 아니면 한 비트 아래로 내려가며 탐색
    }

    // 최상위 비트(MSB)부터 최하위 비트까지 스캔하며 Double-and-Add 수행
    for (int i = max_bit; i >= 0; i--) {
        EC_Point_Double(&res, &res, a, p); // 매 비트마다 무조건 결과를 2배로 함 (res = 2*res)

        int word_idx = i / 32;
        int bit_idx = i % 32;
        if ((k->data[word_idx] >> bit_idx) & 1) { // k의 i번째 비트가 1이면
            EC_Point_Add(&res, &res, P, a, p);       // 원래의 점 P를 더함 (res = res + P)
        }
    }
    *R = res; // 최종 결과를 R에 반영
}

// ============================================================================
// [4] ECDH 로직 (개인키 생성 및 키 교환)
// ============================================================================

// 타원 곡선의 위수(n) 보다 작은 랜덤 개인키(스칼라) 생성
void EC_GeneratePrivateKey(BigInt* privKey) {
    do {
        BigInt_Init(privKey);        // 매 시도마다 privKey를 0으로 초기화
        int words = 256 / 32;         // P-256은 256비트 키이므로 8개의 32비트 워드가 필요
        for (int i = 0; i < words; i++) {
            // rand()는 보통 15~31비트 정도의 난수만 제공하므로, 두 번 호출해 32비트를 채움
            // 주의: rand()는 암호학적으로 안전한 난수 생성기가 아니므로 실제 서비스에는 부적합
            privKey->data[i] = (rand() << 16) | (rand() & 0xFFFF);
        }
        privKey->size = words;   // 8개 워드를 모두 채웠다고 표시
        BigInt_Trim(privKey);     // 최상위 워드가 우연히 0이면 정리
        // 0보다 크고, n(곡선의 위수)보다 작을 때까지 반복 (유효한 개인키 범위를 만족할 때까지 재시도)
    } while (BigInt_IsZero(privKey) || BigInt_Compare(privKey, &P256_n) >= 0);
}

// ============================================================================
// [5] main() 함수: ECDH 파이프라인 시뮬레이션
// ============================================================================

int main() {
    srand((unsigned int)time(NULL)); // 현재 시각을 시드로 난수 생성기 초기화 (테스트/데모용, 암호학적으로는 부적절)
    EC_InitParameters();               // NIST P-256 표준 파라미터(p, a, b, G, n) 초기화

    printf("========== [ 순수 수학 구현 기반 ECDH 키 교환 시뮬레이션 ] ==========\n\n");

    BigInt alice_priv, bob_priv;                                   // Alice와 Bob의 개인키
    EC_Point alice_pub, bob_pub, alice_shared, bob_shared;         // 공개키와 각자 계산한 공유 비밀

    // 1. Alice 키 쌍 생성
    printf("[1] Alice 키 생성 중...\n");
    EC_GeneratePrivateKey(&alice_priv);                       // Alice의 개인키(랜덤 스칼라) 생성
    EC_Scalar_Mul(&alice_pub, &P256_G, &alice_priv, &P256_a, &P256_p); // Alice 공개키 = 개인키 * G

    printf("  > Alice Private Key: "); BigInt_PrintHex(&alice_priv); printf("\n");
    printf("  > Alice Public Key (X): "); BigInt_PrintHex(&alice_pub.x); printf("\n\n");

    // 2. Bob 키 쌍 생성
    printf("[2] Bob 키 생성 중...\n");
    EC_GeneratePrivateKey(&bob_priv);                          // Bob의 개인키 생성
    EC_Scalar_Mul(&bob_pub, &P256_G, &bob_priv, &P256_a, &P256_p);   // Bob 공개키 = 개인키 * G

    printf("  > Bob Private Key: "); BigInt_PrintHex(&bob_priv); printf("\n");
    printf("  > Bob Public Key (X): "); BigInt_PrintHex(&bob_pub.x); printf("\n\n");

    // 3. 키 교환 (서로의 공개키로 공유 비밀키 계산)
    printf("[3] ECDH 공유 비밀키(Shared Secret) 계산 중...\n");
    // Alice 측 계산: Shared = Alice_Priv * Bob_Pub
    EC_Scalar_Mul(&alice_shared, &bob_pub, &alice_priv, &P256_a, &P256_p);
    // Bob 측 계산: Shared = Bob_Priv * Alice_Pub
    EC_Scalar_Mul(&bob_shared, &alice_pub, &bob_priv, &P256_a, &P256_p);
    // ECDH의 핵심 원리: (Alice_Priv * Bob_Priv) * G == (Bob_Priv * Alice_Priv) * G 이므로 양쪽 결과가 같아야 함

    printf("\n  > Alice가 계산한 Shared Secret (X좌표):\n    ");
    BigInt_PrintHex(&alice_shared.x); printf("\n");
    printf("  > Bob이 계산한 Shared Secret (X좌표):\n    ");
    BigInt_PrintHex(&bob_shared.x); printf("\n\n");

    // 4. 검증
    if (BigInt_Compare(&alice_shared.x, &bob_shared.x) == 0 &&
        BigInt_Compare(&alice_shared.y, &bob_shared.y) == 0) {
        printf("[SUCCESS] 수학적 연산 결과, 두 사람의 공유 비밀키가 완벽히 일치합니다.\n"); // x, y좌표가 모두 같으면 키 교환 성공
    } else {
        printf("[FAILED] 연산 무결성 검증 실패.\n"); // 좌표가 다르면 어딘가 버그가 있다는 뜻
    }

    return 0;
}