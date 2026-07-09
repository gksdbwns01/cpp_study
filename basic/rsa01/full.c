#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

// ============================================================================
// [1] bigint.h & bigint.c (Big Integer 직접 구현)
// ============================================================================

// [개선] 256 워드 = 8192 비트 지원 (최대 4096비트 RSA의 곱셈 버퍼용으로 충분히 확장)
// 32비트(4바이트) 정수 256개를 배열로 묶어 최대 8192비트 크기의 정수를 표현합니다.
#define MAX_WORDS 256 

// 큰 정수를 표현하기 위한 구조체 정의
typedef struct {
    uint32_t data[MAX_WORDS]; // 실제 데이터를 저장하는 32비트 정수 배열 (Little-endian 방식: data[0]이 최하위 비트)
    int size;                 // 현재 배열에서 실제로 사용 중인 32비트 워드의 개수
} BigInt;

// BigInt 변수를 0으로 초기화하는 함수
void BigInt_Init(BigInt* a) {
    memset(a->data, 0, sizeof(a->data)); // 데이터 배열의 모든 바이트를 0으로 덮어씀
    a->size = 0;                         // 사용 중인 워드 개수를 0으로 설정
}

// src의 값을 dest로 복사하는 함수 (깊은 복사)
void BigInt_Copy(BigInt* dest, const BigInt* src) {
    memcpy(dest->data, src->data, sizeof(src->data)); // src의 데이터 배열을 dest로 복사
    dest->size = src->size;                           // 사용 중인 워드의 개수도 동일하게 복사
}

// 불필요한 상위 0(leading zeros)을 제거하여 실제 데이터 크기를 맞추는 함수
void BigInt_Trim(BigInt* a) {
    // 사이즈가 0보다 크고, 최상위 워드가 0인 동안 반복
    while (a->size > 0 && a->data[a->size - 1] == 0) {
        a->size--; // 사이즈를 1씩 줄임
    }
}

// BigInt의 값이 0인지 확인하는 함수
bool BigInt_IsZero(const BigInt* a) {
    // 사이즈가 0이거나, 사이즈가 1인데 첫 번째 워드가 0이면 true 반환
    return a->size == 0 || (a->size == 1 && a->data[0] == 0);
}

// 두 BigInt의 크기를 비교하는 함수 (a > b 이면 1, a < b 이면 -1, 같으면 0 반환)
int BigInt_Compare(const BigInt* a, const BigInt* b) {
    if (a->size > b->size) return 1;  // a의 워드 개수가 더 많으면 a가 큼
    if (a->size < b->size) return -1; // b의 워드 개수가 더 많으면 b가 큼
    
    // 워드 개수가 같다면 최상위 워드부터 최하위 워드 방향으로 값 비교
    for (int i = a->size - 1; i >= 0; i--) {
        if (a->data[i] > b->data[i]) return 1;  // a의 현재 워드가 크면 a가 큼
        if (a->data[i] < b->data[i]) return -1; // b의 현재 워드가 크면 b가 큼
    }
    return 0; // 모든 워드가 같으면 두 수는 같음
}

// 두 BigInt를 더하는 함수 (res = a + b)
void BigInt_Add(BigInt* res, const BigInt* a, const BigInt* b) {
    BigInt temp;
    BigInt_Init(&temp); // 결과를 임시 저장할 변수 초기화
    uint64_t carry = 0; // 덧셈 시 발생하는 올림수(Carry)를 저장할 64비트 변수
    int max_size = (a->size > b->size) ? a->size : b->size; // 두 수 중 더 큰 사이즈를 구함
    
    // 최대 사이즈만큼 반복, 혹은 처리할 올림수가 남아있다면 계속 반복
    for (int i = 0; i < max_size || carry > 0; i++) {
        if (i >= MAX_WORDS) break; // 안전 장치: 할당된 최대 버퍼를 넘어가면 오버플로우 방지를 위해 종료
        
        uint64_t sum = carry; // 이전 자리에서 올라온 올림수를 기본값으로 설정
        if (i < a->size) sum += a->data[i]; // a의 현재 자릿수 값을 더함
        if (i < b->size) sum += b->data[i]; // b의 현재 자릿수 값을 더함
        
        temp.data[i] = (uint32_t)(sum & 0xFFFFFFFF); // 하위 32비트만 결과 배열에 저장
        carry = sum >> 32;                           // 상위 32비트는 다음 자리로 넘길 올림수로 갱신
        temp.size = i + 1;                           // 현재까지 처리된 자릿수로 사이즈 갱신
    }
    BigInt_Copy(res, &temp); // 계산 완료된 임시 결과를 res에 복사 (a나 b와 res가 동일한 포인터일 경우 대비)
}

// 두 BigInt를 빼는 함수 (res = a - b, a >= b를 가정)
void BigInt_Sub(BigInt* res, const BigInt* a, const BigInt* b) {
    BigInt temp;
    BigInt_Init(&temp);
    int64_t borrow = 0; // 뺄셈 시 발생하는 빌림수(Borrow)를 저장할 64비트 변수
    
    // 빼지는 수(a)의 크기만큼 반복
    for (int i = 0; i < a->size; i++) {
        int64_t diff = (int64_t)a->data[i] - borrow; // 현재 자릿수에서 빌림수를 먼저 뺌
        if (i < b->size) diff -= b->data[i];         // 빼는 수(b)의 값이 존재하면 그 값도 뺌
        
        if (diff < 0) { // 뺀 결과가 음수라면 (윗자리에서 빌려와야 함)
            diff += 0x100000000LL; // 2^32를 더해 양수로 보정 (32비트 최댓값 빌려옴)
            borrow = 1;            // 다음 뺄셈을 위해 빌림수를 1로 설정
        } else {
            borrow = 0; // 빌려올 필요가 없으면 빌림수 0
        }
        temp.data[i] = (uint32_t)diff; // 계산된 결과를 배열에 저장
        temp.size = i + 1;             // 임시 사이즈 갱신
    }
    BigInt_Trim(&temp);      // 뺄셈 후 상위에 남은 0들을 제거
    BigInt_Copy(res, &temp); // 결과를 최종 변수에 복사
}

// 두 BigInt를 곱하는 함수 (res = a * b) O(N^2) 학교 식 곱셈 방식
void BigInt_Mul(BigInt* res, const BigInt* a, const BigInt* b) {
    BigInt temp;
    BigInt_Init(&temp);
    
    // 어느 하나라도 0이면 결과는 0이므로 즉시 반환
    if (BigInt_IsZero(a) || BigInt_IsZero(b)) {
        BigInt_Copy(res, &temp);
        return;
    }
    
    temp.size = a->size + b->size; // 두 수를 곱한 결과의 최대 자릿수는 a자릿수 + b자릿수
    if (temp.size > MAX_WORDS) temp.size = MAX_WORDS; // 최대 버퍼를 초과하지 않도록 오버플로우 방지
    
    // a의 각 자릿수를 순회
    for (int i = 0; i < a->size; i++) {
        uint64_t carry = 0; // 곱셈 올림수 초기화
        // b의 각 자릿수를 순회
        for (int j = 0; j < b->size; j++) {
            if (i + j >= MAX_WORDS) break; // 최대 자릿수 초과 시 반복 중단
            
            // a의 자릿수 * b의 자릿수 + 기존 임시 배열에 있던 값 + 이전 올림수
            uint64_t prod = (uint64_t)a->data[i] * b->data[j] + temp.data[i + j] + carry;
            
            temp.data[i + j] = (uint32_t)(prod & 0xFFFFFFFF); // 하위 32비트를 현재 자리에 저장
            carry = prod >> 32;                               // 상위 32비트는 다음 자리로 넘김
        }
        // 곱셈이 끝나고도 올림수가 남아있고 공간이 있다면 저장
        if (i + b->size < MAX_WORDS) {
            temp.data[i + b->size] = (uint32_t)carry;
        }
    }
    BigInt_Trim(&temp);      // 곱셈 결과에서 불필요한 0 제거
    BigInt_Copy(res, &temp); // 결과 복사
}

// BigInt를 1비트 왼쪽으로 시프트하는 함수 (a = a << 1) / 곱하기 2와 동일한 효과
void BigInt_ShiftLeft1(BigInt* a) {
    if (BigInt_IsZero(a)) return; // 0이면 처리할 필요 없음
    uint32_t carry = 0;           // 다음 워드로 넘어갈 비트 값을 저장할 변수
    
    // 최하위 워드부터 최상위 워드까지 반복
    for (int i = 0; i < a->size; i++) {
        uint32_t next_carry = a->data[i] >> 31; // 현재 워드의 최상위 비트를 추출하여 보관 (다음 워드의 최하위로 이동)
        a->data[i] = (a->data[i] << 1) | carry; // 왼쪽으로 1비트 밀고 이전 워드에서 넘어온 비트를 맨 뒤에 붙임
        carry = next_carry;                     // 현재 워드에서 추출한 비트를 carry로 갱신
    }
    // 모든 워드를 처리한 후 캐리가 남았고 배열 공간이 있다면
    if (carry > 0 && a->size < MAX_WORDS) {
        a->data[a->size] = carry; // 새로운 워드를 추가하여 캐리값 저장
        a->size++;                // 사이즈 1 증가
    }
}

// BigInt 나눗셈 및 나머지 연산 함수 (a / b -> 몫은 q, 나머지는 r에 저장)
// q나 r 인자에 NULL을 전달하면 해당 값은 저장하지 않음. 이진 Long Division 방식 사용.
void BigInt_DivMod(BigInt* q, BigInt* r, const BigInt* a, const BigInt* b) {
    BigInt temp_q, temp_r;
    BigInt_Init(&temp_q); // 몫을 저장할 임시 변수
    BigInt_Init(&temp_r); // 나머지를 저장할 임시 변수
    
    // 나누는 수(b)가 0이거나, 나누어지는 수(a)가 0이면 처리 종료
    if (BigInt_IsZero(b) || BigInt_IsZero(a)) {
        if (q) BigInt_Copy(q, &temp_q); // 0 리턴
        if (r) BigInt_Copy(r, &temp_r); // 0 리턴
        return;
    }
    
    // 나누어지는 수(a)가 나누는 수(b)보다 작다면 (a < b)
    if (BigInt_Compare(a, b) < 0) {
        if (q) BigInt_Copy(q, &temp_q); // 몫은 0
        if (r) BigInt_Copy(r, a);       // 나머지는 a 자기 자신
        return;
    }
    
    // a의 유효한 총 비트 수를 계산
    int a_bits = (a->size - 1) * 32;          // 최상위 워드를 제외한 나머지 워드들의 비트 수 합
    uint32_t top = a->data[a->size - 1];      // 최상위 워드
    while (top) { a_bits++; top >>= 1; }      // 최상위 워드의 실제 비트 길이를 측정하여 더함
    
    temp_q.size = (a_bits + 31) / 32;         // 몫을 담을 변수의 사이즈를 예측하여 설정 (최대 a 사이즈와 동일)
    
    // a의 최상위 비트부터 하나씩 내려오면서 나눗셈 진행 (Shift-Subtract 알고리즘)
    for (int i = a_bits - 1; i >= 0; i--) {
        BigInt_ShiftLeft1(&temp_r); // 나머지(임시)를 왼쪽으로 1비트 시프트
        
        // a의 i번째 비트를 추출할 위치 계산
        int word_idx = i / 32;
        int bit_idx = i % 32;
        
        // a의 i번째 비트가 1이라면
        if ((a->data[word_idx] >> bit_idx) & 1) {
            temp_r.data[0] |= 1;                  // temp_r의 최하위 비트에 1을 추가
            if (temp_r.size == 0) temp_r.size = 1;// temp_r 사이즈 보정
        }
        
        // temp_r이 나누는 수 b보다 크거나 같으면 뺄셈이 가능
        if (BigInt_Compare(&temp_r, b) >= 0) {
            BigInt sub_res;
            BigInt_Sub(&sub_res, &temp_r, b); // temp_r에서 b를 뺌
            BigInt_Copy(&temp_r, &sub_res);   // 뺀 결과를 다시 temp_r에 저장
            temp_q.data[word_idx] |= (1U << bit_idx); // 몫의 현재 비트 위치를 1로 설정
        }
    }
    BigInt_Trim(&temp_q); // 몫과 나머지의 불필요한 0 제거
    BigInt_Trim(&temp_r);
    
    // 결과 포인터가 유효하면 임시 변수의 값을 최종 복사
    if (q) BigInt_Copy(q, &temp_q);
    if (r) BigInt_Copy(r, &temp_r);
}

// 큰 수 a를 32비트 작은 정수 m으로 나눈 나머지를 구하는 최적화 함수 (소수 판별용)
uint32_t BigInt_Mod_Small(const BigInt* a, uint32_t m) {
    uint64_t rem = 0; // 나머지를 보관할 변수
    // 최상위 워드부터 내려오면서 계산
    for (int i = a->size - 1; i >= 0; i--) {
        // 이전 나머지 값을 상위 32비트로 시프트하고 현재 워드를 붙여서 새 값을 만듦
        rem = (rem << 32) | a->data[i];
        rem %= m; // 작은 수 m으로 나머지 연산 수행
    }
    return (uint32_t)rem; // 최종 나머지를 32비트로 캐스팅하여 반환
}


// ============================================================================
// [2] rsa_math.c (모듈러 연산 및 확장 유클리드)
// ============================================================================

// 모듈러 거듭제곱 함수 (res = (base ^ exp) % mod)
// Exponentiation by squaring (Square-and-Multiply) 방식 사용하여 효율적으로 계산
void ModExp(BigInt* res, const BigInt* base, const BigInt* exp, const BigInt* mod) {
    BigInt b, e, temp, dummy_q;
    BigInt_Copy(&b, base); // 밑(base) 변수 초기화
    BigInt_Copy(&e, exp);  // 지수(exp) 변수 초기화
    
    BigInt_Init(res);
    res->data[0] = 1; res->size = 1; // 결과값을 1로 초기화 (곱셈의 항등원)
    
    // 지수가 0이 될 때까지 반복
    while (!BigInt_IsZero(&e)) {
        // 지수의 최하위 비트가 1이면 (홀수이면)
        if (e.data[0] & 1) {
            BigInt_Mul(&temp, res, &b);            // 현재까지의 결과에 밑(b)을 곱함
            BigInt_DivMod(&dummy_q, res, &temp, mod); // 모듈러(mod) 연산 수행하여 결과값 제한
        }
        BigInt_Mul(&temp, &b, &b);             // 밑(b)을 스스로 곱하여 제곱함 (b = b^2)
        BigInt_DivMod(&dummy_q, &b, &temp, mod);  // 밑(b)에 대해 모듈러(mod) 연산 수행
        
        // 지수(e)를 오른쪽으로 1비트 시프트 (e = e / 2)
        for (int i = 0; i < e.size; i++) {
            // 상위 워드에서 넘어올 비트 계산 (최상위 워드는 0이 들어옴)
            uint32_t carry = (i == e.size - 1) ? 0 : (e.data[i + 1] & 1) << 31;
            e.data[i] = (e.data[i] >> 1) | carry; // 오른쪽 시프트 후 상위에서 넘어온 비트 삽입
        }
        BigInt_Trim(&e); // 불필요한 0 제거
    }
}

// 모듈러 역원 함수 (res = e^-1 mod phi)
// 확장 유클리드 알고리즘(Extended Euclidean Algorithm)을 사용하여 계산
void ModInverse(BigInt* res, const BigInt* e, const BigInt* phi) {
    BigInt t, newt, r, newr, q, temp, prod, next_t;
    int t_sign = 1, newt_sign = 1; // 부호를 관리하기 위한 변수 (BigInt가 부호 없는 정수만 처리하므로)
    
    BigInt_Init(&t);                        // 초기값 t = 0
    BigInt_Init(&newt); newt.data[0] = 1; newt.size = 1; // 초기값 newt = 1
    
    BigInt_Copy(&r, phi);                   // 초기값 r = 오일러 파이값(phi)
    BigInt_Copy(&newr, e);                  // 초기값 newr = 공개키(e)
    
    // newr이 0이 될 때까지 유클리드 알고리즘 반복
    while (!BigInt_IsZero(&newr)) {
        // r을 newr로 나누어 몫(q)과 나머지(temp)를 구함
        BigInt_DivMod(&q, &temp, &r, &newr);
        BigInt_Copy(&r, &newr);   // r에 이전 newr(나누는 수) 대입
        BigInt_Copy(&newr, &temp); // newr에 새로 구한 나머지 대입
        
        BigInt_Mul(&prod, &q, &newt); // prod = q * newt
        
        // next_t = t - q * newt 연산을 수행 (부호를 고려하여 덧셈/뺄셈 결정)
        if (t_sign == newt_sign) {
            // 두 값의 부호가 같으면 뺄셈 수행
            if (BigInt_Compare(&t, &prod) >= 0) {
                BigInt_Sub(&next_t, &t, &prod); // t >= prod 이면 일반 뺄셈, 부호 변경 없음
            } else {
                BigInt_Sub(&next_t, &prod, &t); // t < prod 이면 순서 바꿔 빼고 부호 반전
                t_sign = -t_sign;
            }
        } else {
            // 부호가 다르면 뺄셈이 덧셈으로 변함
            BigInt_Add(&next_t, &t, &prod);
        }
        
        // 상태 갱신: t <- newt, newt <- next_t
        BigInt_Copy(&t, &newt);
        t_sign = newt_sign;
        BigInt_Copy(&newt, &next_t);
        // 다음 newt의 부호 결정 규칙
        newt_sign = t_sign == newt_sign ? -newt_sign : t_sign;
    }
    
    // 최종 역원이 음수라면 양수로 변환 (res = phi - t)
    if (t_sign == -1) BigInt_Sub(res, phi, &t);
    else BigInt_Copy(res, &t); // 양수면 그대로 복사
}


// ============================================================================
// [3] random.h & random.c (난수 및 소수 판별)
// ============================================================================

// 지정된 비트 길이를 가진 무작위 BigInt 생성 함수
void Generate_Random_BigInt(BigInt* a, int bit_length) {
    BigInt_Init(a);
    if (bit_length <= 0) return;
    
    // 필요한 32비트 워드 개수 올림하여 계산
    int words = (bit_length + 31) / 32; 
    if (words > MAX_WORDS) words = MAX_WORDS; // 최대 버퍼 초과 방지
    
    // 워드 개수만큼 난수 생성하여 채워넣음
    for (int i = 0; i < words; i++) {
        uint32_t r1 = rand() & 0xFF; // C의 rand()는 15비트 또는 31비트 난수이므로
        uint32_t r2 = rand() & 0xFF; // 8비트씩 쪼개어 안전하게 조합하여 32비트를 꽉 채움
        uint32_t r3 = rand() & 0xFF;
        uint32_t r4 = rand() & 0xFF;
        a->data[i] = (r1 << 24) | (r2 << 16) | (r3 << 8) | r4;
    }
    a->size = words;
    
    // 생성된 값이 정확한 bit_length 길이를 갖도록 최상위 비트 처리
    int rem = bit_length % 32;
    if (rem != 0) {
        uint32_t mask = (1U << rem) - 1;         // 남는 비트 수만큼 1을 채운 마스크 생성
        a->data[words - 1] &= mask;              // 상위의 불필요한 비트를 0으로 클리어
        a->data[words - 1] |= (1U << (rem - 1)); // 지정된 길이의 최상위 비트를 무조건 1로 세팅 (길이 보장)
    } else {
        // 나머지가 0(정확히 32배수)인 경우 최상위 워드의 31번째 비트를 1로 세팅
        a->data[words - 1] |= (1U << 31);
    }
    
    a->data[0] |= 1; // 최하위 비트를 1로 세팅하여 홀수로 강제 (짝수는 2를 제외하고 소수가 될 수 없으므로)
    BigInt_Trim(a);  // 사이즈 최종 보정
}

// 밀러-라빈 (Miller-Rabin) 소수 판별법
// 소수일 확률이 높은 수인지 k번 검사. (true면 확률적 소수, false면 확실한 합성수)
bool MillerRabin(const BigInt* n, int k) {
    // 0, 1은 소수가 아님
    if (BigInt_IsZero(n) || (n->size == 1 && n->data[0] <= 1)) return false;
    // 짝수인 경우 2일때만 소수고 나머지는 합성수
    if ((n->data[0] & 1) == 0) return (n->size == 1 && n->data[0] == 2); 
    
    BigInt n_minus_1, d, one;
    BigInt_Init(&one); one.data[0] = 1; one.size = 1;
    BigInt_Sub(&n_minus_1, n, &one); // n - 1 계산
    
    BigInt_Copy(&d, &n_minus_1);
    int s = 0;
    // n - 1 을 d * 2^s 형태로 분해 (d가 홀수가 될 때까지 2로 계속 나눔)
    while ((d.data[0] & 1) == 0 && !BigInt_IsZero(&d)) {
        // d를 오른쪽으로 1비트 시프트 (d = d / 2)
        for (int i = 0; i < d.size; i++) {
            uint32_t carry = (i == d.size - 1) ? 0 : (d.data[i + 1] & 1) << 31;
            d.data[i] = (d.data[i] >> 1) | carry;
        }
        BigInt_Trim(&d);
        s++; // 2로 나눈 횟수 증가
    }
    
    BigInt a, x, temp, dummy;
    // k번 만큼 베이스를 바꿔가며 테스트 반복
    for (int i = 0; i < k; i++) {
        BigInt_Init(&a);
        
        // 2 ~ n-2 사이의 무작위 베이스(a) 생성 준비
        int bits_n = (n->size - 1) * 32;
        uint32_t top = n->data[n->size - 1];
        while (top) { bits_n++; top >>= 1; }
        
        Generate_Random_BigInt(&a, bits_n - 1); // n보다 한 비트 작은 길이로 무작위 수(a) 생성
        
        // 생성된 a가 0이나 1이면 밀러라빈 테스트에 무의미하므로 2 이상으로 보정
        if (BigInt_IsZero(&a) || (a.size == 1 && a.data[0] <= 1)) {
            a.data[0] = 2 + (rand() % 100);
            a.size = 1;
        }
        
        // x = a^d mod n 계산
        ModExp(&x, &a, &d, n);
        
        // x == 1 이거나 x == n-1 이면 이 베이스(a)에 대해서는 소수일 가능성 통과
        if (x.size == 1 && x.data[0] == 1) continue;
        if (BigInt_Compare(&x, &n_minus_1) == 0) continue;
        
        bool composite = true; // 일단 합성수로 가정
        // s-1 번 동안 x를 제곱해가며 검사
        for (int j = 0; j < s - 1; j++) {
            BigInt_Mul(&temp, &x, &x);            // x = x^2
            BigInt_DivMod(&dummy, &x, &temp, n);  // x = (x^2) % n
            
            // 제곱한 값이 1이면 중간에 n-1이 나오지 않은 것이므로 합성수 판정 확정
            if (x.size == 1 && x.data[0] == 1) return false; 
            // 제곱한 값이 n-1이 나오면 해당 베이스에 대해 통과, 반복 중단
            if (BigInt_Compare(&x, &n_minus_1) == 0) {
                composite = false;
                break;
            }
        }
        // 위 루프에서 소수 가능성을 찾지 못했다면 합성수 반환
        if (composite) return false;
    }
    // k번의 테스트를 모두 통과했다면 (매우 높은 확률로) 소수로 판정
    return true;
}

// 지정된 비트 길이의 큰 소수를 생성하는 함수
void Generate_Prime(BigInt* p, int bit_length) {
    // 밀러라빈 테스트 전, 연산 속도 향상을 위해 작은 소수들로 먼저 걸러내기 위한 배열
    const uint32_t small_primes[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97};
    int num_primes = sizeof(small_primes) / sizeof(small_primes[0]);
    
    // 소수를 찾을 때까지 무한 반복
    while (1) {
        Generate_Random_BigInt(p, bit_length); // 해당 비트 길이의 무작위 홀수 생성
        
        bool divisible = false;
        // 작은 소수들로 나누어 떨어지는지 빠르게 검사 (Trial Division)
        for (int i = 0; i < num_primes; i++) {
            if (BigInt_Mod_Small(p, small_primes[i]) == 0) {
                divisible = true; // 나누어 떨어지면 합성수
                break;
            }
        }
        // 작은 소수로 나누어 떨어지면, 복잡한 검사 없이 바로 새 난수 생성으로 넘어감
        if (divisible) continue; 
        
        // RSA 공개키(e=65537) 생성 시 문제가 발생하지 않도록, p를 65537로 나눈 나머지가 1이면 배제
        if (BigInt_Mod_Small(p, 65537) == 1) continue;
        
        // 위 기초 검사를 통과한 수에 대해 밀러라빈 테스트 5회 진행 (안전성을 위해 횟수 증가 가능)
        if (MillerRabin(p, 5)) { 
            break; // 밀러라빈 테스트를 통과하면 소수를 찾은 것이므로 반복 종료
        }
    }
}


// ============================================================================
// [4] rsa.h & rsa.c (RSA 키 생성 및 암복호화)
// ============================================================================

// RSA 키 묶음을 저장하기 위한 구조체 정의
typedef struct {
    BigInt p, q, n, e, d, phi; // 일반 RSA 요소: 소수 p, q / 모듈러스 n / 공개키 e / 개인키 d / 오일러파이 phi
    BigInt dp, dq, qInv;       // CRT 최적화 복호화를 위한 요소 (dp = d mod (p-1), dq = d mod (q-1), qInv = q^-1 mod p)
} RSA_Key;

// 전체 키 비트수(total_bits)를 입력받아 RSA 키 쌍을 생성하는 함수
void RSA_GenerateKey(RSA_Key* key, int total_bits) {
    int prime_bits = total_bits / 2; // p와 q는 각각 전체 키 크기의 절반이어야 함 (예: 2048비트 RSA면 1024비트 소수 생성)
    BigInt dummy_q;
    
    // 소수 p 생성
    printf("  [p 탐색] "); Generate_Prime(&key->p, prime_bits); printf(" 완료\n");
    
    // 소수 q 생성 (p와 완전히 똑같은 값이 나올 확률은 극히 희박하나, 예외 처리로 재탐색 구조 적용)
    do {
        printf("  [q 탐색] "); Generate_Prime(&key->q, prime_bits); 
        if (BigInt_Compare(&key->p, &key->q) == 0) {
            printf(" (p와 중복 발생, 재탐색 진행)\n");
        } else {
            printf(" 완료\n");
        }
    } while (BigInt_Compare(&key->p, &key->q) == 0);
    
    // CRT 복호화 연산의 편의성(뺄셈 시 음수 방지)을 위해 p가 항상 q보다 크도록 교환(Swap)
    if (BigInt_Compare(&key->p, &key->q) < 0) {
        BigInt temp;
        BigInt_Copy(&temp, &key->p);
        BigInt_Copy(&key->p, &key->q);
        BigInt_Copy(&key->q, &temp);
    }
    
    // 모듈러스 N 생성 (n = p * q)
    BigInt_Mul(&key->n, &key->p, &key->q);
    
    // 오일러 파이 함수 생성: phi(n) = (p-1) * (q-1)
    BigInt p_minus_1, q_minus_1, one;
    BigInt_Init(&one); one.data[0] = 1; one.size = 1;
    
    BigInt_Sub(&p_minus_1, &key->p, &one); // p - 1
    BigInt_Sub(&q_minus_1, &key->q, &one); // q - 1
    BigInt_Mul(&key->phi, &p_minus_1, &q_minus_1); // phi = (p-1)(q-1)
    
    // 공개키 e 설정 (가장 널리 쓰이는 소수 65537 사용)
    BigInt_Init(&key->e); key->e.data[0] = 65537; key->e.size = 1;
    
    // 개인키 d 생성 (d = e^-1 mod phi)
    ModInverse(&key->d, &key->e, &key->phi);
    
    // CRT(중국인의 나머지 정리) 기반 고속 복호화에 필요한 파라미터 계산
    // dp = d % (p - 1)
    BigInt_DivMod(&dummy_q, &key->dp, &key->d, &p_minus_1);
    // dq = d % (q - 1)
    BigInt_DivMod(&dummy_q, &key->dq, &key->d, &q_minus_1);
    // qInv = q^-1 mod p
    ModInverse(&key->qInv, &key->q, &key->p);
}

// RSA 암호화 함수 (Ciphertext = M^e mod N)
void RSA_Encrypt(BigInt* C, const BigInt* M, const RSA_Key* key) {
    ModExp(C, M, &key->e, &key->n); // 평문(M)을 e번 곱하고 n으로 나눈 나머지를 계산
}

// CRT(중국인의 나머지 정리)를 활용한 초고속 RSA 복호화 함수
void RSA_Decrypt_CRT(BigInt* M, const BigInt* C, const RSA_Key* key) {
    BigInt m1, m2, h, temp, dummy_q;
    
    // 1. 작은 지수와 모듈러스를 이용해 각각 복호화
    // m1 = C^dp mod p
    ModExp(&m1, C, &key->dp, &key->p);
    // m2 = C^dq mod q
    ModExp(&m2, C, &key->dq, &key->q);
    
    // 2. Garner의 알고리즘을 이용해 두 결과 결합 (M = m2 + h * q)
    BigInt diff;
    // (m1 - m2) 연산 시 m1이 m2보다 작아 음수가 되는 것을 방지하기 위해 p를 더해줌
    if (BigInt_Compare(&m1, &m2) < 0) {
        BigInt m1_plus_p;
        BigInt_Add(&m1_plus_p, &m1, &key->p); // m1 + p
        BigInt_Sub(&diff, &m1_plus_p, &m2);   // (m1 + p) - m2
    } else {
        BigInt_Sub(&diff, &m1, &m2);          // m1 - m2
    }
    
    // h = (qInv * (m1 - m2)) mod p 계산
    BigInt_Mul(&temp, &key->qInv, &diff);     // temp = qInv * diff
    BigInt_DivMod(&dummy_q, &h, &temp, &key->p); // h = temp % p
    
    // 최종 평문 복원: M = m2 + (h * q)
    BigInt_Mul(&temp, &h, &key->q);           // temp = h * q
    BigInt_Add(M, &m2, &temp);                // M = m2 + temp
}


// ============================================================================
// [5] main.c (전체 로직 검증 및 실행 파이프라인)
// ============================================================================

// BigInt의 값을 16진수 문자열 형태로 콘솔에 출력하는 함수
void BigInt_Print(const BigInt* a) {
    if (BigInt_IsZero(a)) {
        printf("00000000");
        return;
    }
    // 최상위 워드부터 16진수 8자리 대문자로 패딩하여 연속 출력
    for (int i = a->size - 1; i >= 0; i--) {
        printf("%08X", a->data[i]);
    }
}

int main() {
    // 난수 발생기 시드 설정 (현재 시간을 기반으로 매번 다른 난수가 생성되도록 함)
    srand((unsigned int)time(NULL));
    
    RSA_Key key; // 키 보관 구조체 선언
    BigInt plaintext, ciphertext, decrypted; // 평문, 암호문, 복호문 변수 선언
    
    printf("========== [ 순수 C 구현 RSA-CRT 시뮬레이션 (최적화 완료) ] ==========\n\n");
    
    // 키 생성 테스트
    int key_size = 2048; // 전체 N의 비트 수 (p, q는 각각 1024비트로 생성됨)
    printf("%d비트 RSA 키 생성 시작...\n", key_size);
    RSA_GenerateKey(&key, key_size); // 소수 탐색 및 키 쌍 생성 함수 호출
    printf("\n키 생성 완료!\n\n");
    
    // 생성된 키 내용 출력 (디버깅 목적)
    printf("- p :\n"); BigInt_Print(&key.p); printf("\n\n");
    printf("- q :\n"); BigInt_Print(&key.q); printf("\n\n");
    printf("- 오일러 파이(phi) :\n"); BigInt_Print(&key.phi); printf("\n\n");
    printf("- 모듈러스(n) :\n"); BigInt_Print(&key.n); printf("\n\n");
    printf("- 공개키(e) :\n"); BigInt_Print(&key.e); printf("\n\n");
    printf("- 개인키(d) :\n"); BigInt_Print(&key.d); printf("\n\n\n");
    
    // 테스트할 평문 초기화 및 설정
    BigInt_Init(&plaintext);
    // 평문 값으로 'HELLO'를 아스키 16진수(0x48454C4C, 0x4F)로 설정
    // Little-endian 구조에 맞춰 data[1]에 앞글자, data[0]에 뒷글자 삽입
    plaintext.data[0] = 0x48454C4C; 
    plaintext.data[1] = 0x0000004F; 
    plaintext.size = 2; // 사용 중인 워드 수 설정
    
    // 원본 평문 출력
    printf("원본 평문 (M) :\n"); BigInt_Print(&plaintext); printf("\n");
    
    // 평문을 공개키로 암호화 진행 (C = M^e mod N)
    RSA_Encrypt(&ciphertext, &plaintext, &key);
    printf("암호화 완료 (C) :\n"); BigInt_Print(&ciphertext); printf("\n");
    
    // 암호문을 개인키(CRT 방식)로 복호화 진행 (M = C^d mod N 의 최적화 연산)
    RSA_Decrypt_CRT(&decrypted, &ciphertext, &key);
    printf("복호화 완료 (M) :\n"); BigInt_Print(&decrypted); printf("\n\n");
    
    // 최초에 설정한 평문과 복호화된 평문이 동일한지 검증
    if (BigInt_Compare(&plaintext, &decrypted) == 0) {
        printf("[SUCCESS] CRT 기반 수학적 RSA 암복호화가 완벽히 일치합니다.\n");
    } else {
        printf("[FAILED] 연산 무결성 검증 실패.\n");
    }

    return 0; // 프로그램 정상 종료
}