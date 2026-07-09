#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

// ============================================================================
// [1] bigint.h & bigint.c (Big Integer 직접 구현)
// ============================================================================

// 256 워드 = 8192 비트 지원 (최대 4096비트 RSA의 곱셈 버퍼용 확장)
// 32비트(4바이트) 정수 256개를 배열로 묶어 최대 8192비트 정수 표현
#define MAX_WORDS 256 

// 큰 정수를 표현 구조체 정의
typedef struct {
    uint32_t data[MAX_WORDS]; // 데이터를 저장하는 32비트 정수 배열
    int size; // 사용 중인 32비트 워드의 개수
} BigInt;

// BigInt 변수를 0으로 초기화
void BigInt_Init(BigInt* a) {
    memset(a->data, 0, sizeof(a->data));
    a->size = 0;
}

// src의 값을 dest로 복사
void BigInt_Copy(BigInt* dest, const BigInt* src) {
    memcpy(dest->data, src->data, sizeof(src->data));
    dest->size = src->size;
}

// 상위 0을 제거. 데이터 크기를 맞추는 함수
void BigInt_Trim(BigInt* a) {
    while (a->size > 0 && a->data[a->size - 1] == 0) {
        a->size--;
    }
}

// BigInt의 값이 0인지 확인
bool BigInt_IsZero(const BigInt* a) {
    // 사이즈가 0이거나 사이즈가 1인데 첫 번째 워드가 0이면 true 반환
    return a->size == 0 || (a->size == 1 && a->data[0] == 0);
}

// 두 BigInt의 크기 비교
int BigInt_Compare(const BigInt* a, const BigInt* b) {
    if (a->size > b->size) return 1;
    if (a->size < b->size) return -1;
    for (int i = a->size - 1; i >= 0; i--) { // 워드 개수가 같다면 최상위 워드부터 비교
        if (a->data[i] > b->data[i]) return 1;
        if (a->data[i] < b->data[i]) return -1;
    }
    return 0;
}

// 두 BigInt를 더함
void BigInt_Add(BigInt* res, const BigInt* a, const BigInt* b) {
    BigInt temp;
    BigInt_Init(&temp);
    uint64_t carry = 0; // 덧셈 시 발생하는 올림수 저장
    int max_size = (a->size > b->size) ? a->size : b->size; // 두 수 중 큰 사이즈를 기준으로 반복
    
    for (int i = 0; i < max_size || carry > 0; i++) {
        if (i >= MAX_WORDS) break; // 오버플로우 방지
        uint64_t sum = carry;
        if (i < a->size) sum += a->data[i];
        if (i < b->size) sum += b->data[i];
        
        temp.data[i] = (uint32_t)(sum & 0xFFFFFFFF);
        carry = sum >> 32; // 상위 32비트는 다음 자리로 넘길 올림수로 갱신
        temp.size = i + 1; // 현재까지 처리된 자릿수로 사이즈 갱신
    }
    BigInt_Copy(res, &temp); // 계산 완료된 임시 결과를 res에 복사
}

// 두 BigInt를 뺌
void BigInt_Sub(BigInt* res, const BigInt* a, const BigInt* b) {
    BigInt temp;
    BigInt_Init(&temp);
    int64_t borrow = 0; // 뺄셈 시 발생하는 빌림수 저장
    
    for (int i = 0; i < a->size; i++) { // 빼지는 수(a)의 크기만큼 반복
        int64_t diff = (int64_t)a->data[i] - borrow;
        if (i < b->size) diff -= b->data[i];
        
        if (diff < 0) { // 뺀 결과가 음수라면 윗자리에서 빌림
            diff += 0x100000000LL; // 2^32를 더해 양수로 보정
            borrow = 1;
        } else {
            borrow = 0;
        }
        temp.data[i] = (uint32_t)diff; // 뺄셈 후 상위에 남은 0들을 제거
        temp.size = i + 1; // 임시 사이즈 갱신
    }
    BigInt_Trim(&temp); // 뺄셈 후 상위에 남은 0들을 제거
    BigInt_Copy(res, &temp); // 결과를 최종 변수에 복사
}

// 두 BigInt를 곱함
void BigInt_Mul(BigInt* res, const BigInt* a, const BigInt* b) {
    BigInt temp;
    BigInt_Init(&temp);
    if (BigInt_IsZero(a) || BigInt_IsZero(b)) {
        BigInt_Copy(res, &temp);
        return;
    }
    
    temp.size = a->size + b->size; // 곱한 결과의 최대 자릿수는 a자릿수 + b자릿수
    if (temp.size > MAX_WORDS) temp.size = MAX_WORDS; // 오버플로우 방지
    
    for (int i = 0; i < a->size; i++) { // a의 각 자릿수를 순회
        uint64_t carry = 0;
        for (int j = 0; j < b->size; j++) { // b의 각 자릿수를 순회
            if (i + j >= MAX_WORDS) break;
            uint64_t prod = (uint64_t)a->data[i] * b->data[j] + temp.data[i + j] + carry;
            temp.data[i + j] = (uint32_t)(prod & 0xFFFFFFFF); // 하위 32비트를 현재 자리에 저장
            carry = prod >> 32; // 상위 32비트는 다음 자리로 넘김
        }
        if (i + b->size < MAX_WORDS) { // 곱셈이 끝나고도 올림수가 남아있고 공간이 있다면 저장
            temp.data[i + b->size] = (uint32_t)carry;
        }
    }
    BigInt_Trim(&temp);
    BigInt_Copy(res, &temp);
}

// BigInt를 1비트 왼쪽으로 시프트
void BigInt_ShiftLeft1(BigInt* a) {
    if (BigInt_IsZero(a)) return;
    uint32_t carry = 0; // 다음 워드로 넘어갈 비트값 저장
    for (int i = 0; i < a->size; i++) {
        uint32_t next_carry = a->data[i] >> 31; // 최상위 비트를 추출하여 보관
        a->data[i] = (a->data[i] << 1) | carry;
        carry = next_carry; // carry 갱신
    }
    if (carry > 0 && a->size < MAX_WORDS) { // 캐리가 남았고 배열 공간이 있다면 캐리값 저장
        a->data[a->size] = carry;
        a->size++;
    }
}

// BigInt 나눗셈, 나머지 연산 (몫 q, 나머지 r)
void BigInt_DivMod(BigInt* q, BigInt* r, const BigInt* a, const BigInt* b) {
    BigInt temp_q, temp_r;
    BigInt_Init(&temp_q); // 몫 임시 변수
    BigInt_Init(&temp_r); // 나머지 임시 변수
    
    if (BigInt_IsZero(b) || BigInt_IsZero(a)) {
        if (q) BigInt_Copy(q, &temp_q);
        if (r) BigInt_Copy(r, &temp_r);
        return;
    }
    
    if (BigInt_Compare(a, b) < 0) { // 나누어지는 수가 나누는 수보다 작다면
        if (q) BigInt_Copy(q, &temp_q); // 몫은 0
        if (r) BigInt_Copy(r, a); // 나머지는 a 그대로
        return;
    }
    
    // a의 총 비트 수를 계산
    int a_bits = (a->size - 1) * 32; // 최상위 워드를 제외한 나머지 워드들의 비트 수 합
    uint32_t top = a->data[a->size - 1]; // 최상위 워드
    while (top) { a_bits++; top >>= 1; } // 최상위 워드의 실제 비트 길이 더함
    
    temp_q.size = (a_bits + 31) / 32; // 몫을 담을 변수의 사이즈
    
    // a의 최상위 비트부터 내려오면서 나눗셈 진행 (Shift-Subtract 알고리즘)
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

// 큰 수 a를 32비트 작은 정수 m으로 나눈 나머지를 구하는 최적화 함수 (소수 판별용)
uint32_t BigInt_Mod_Small(const BigInt* a, uint32_t m) {
    uint64_t rem = 0; // 나머지를 보관할 변수
    for (int i = a->size - 1; i >= 0; i--) {
        rem = (rem << 32) | a->data[i]; // 이전 나머지 값을 상위 32비트로 시프트하고 현재 워드를 붙여서 새 값을 만듦
        rem %= m;
    }
    return (uint32_t)rem; // 최종 나머지를 32비트로 캐스팅하여 반환
}


// ============================================================================
// [2] rsa_math.c (모듈러 연산 및 확장 유클리드)
// ============================================================================

void ModExp(BigInt* res, const BigInt* base, const BigInt* exp, const BigInt* mod) {
    BigInt b, e, temp, dummy_q;
    BigInt_Copy(&b, base);
    BigInt_Copy(&e, exp);
    
    BigInt_Init(res);
    res->data[0] = 1; res->size = 1;
    
    while (!BigInt_IsZero(&e)) {
        if (e.data[0] & 1) {
            BigInt_Mul(&temp, res, &b);
            BigInt_DivMod(&dummy_q, res, &temp, mod);
        }
        BigInt_Mul(&temp, &b, &b);
        BigInt_DivMod(&dummy_q, &b, &temp, mod);
        
        for (int i = 0; i < e.size; i++) {
            uint32_t carry = (i == e.size - 1) ? 0 : (e.data[i + 1] & 1) << 31;
            e.data[i] = (e.data[i] >> 1) | carry;
        }
        BigInt_Trim(&e);
    }
}

void ModInverse(BigInt* res, const BigInt* e, const BigInt* phi) {
    BigInt t, newt, r, newr, q, temp, prod, next_t;
    int t_sign = 1, newt_sign = 1;
    
    BigInt_Init(&t);
    BigInt_Init(&newt); newt.data[0] = 1; newt.size = 1;
    
    BigInt_Copy(&r, phi);
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
    
    if (t_sign == -1) BigInt_Sub(res, phi, &t);
    else BigInt_Copy(res, &t);
}


// ============================================================================
// [3] random.h & random.c (난수 및 소수 판별)
// ============================================================================

void Generate_Random_BigInt(BigInt* a, int bit_length) {
    BigInt_Init(a);
    if (bit_length <= 0) return;
    
    int words = (bit_length + 31) / 32; // [개선] 32 배수가 아닌 비트수도 올림으로 계산
    if (words > MAX_WORDS) words = MAX_WORDS;
    
    for (int i = 0; i < words; i++) {
        uint32_t r1 = rand() & 0xFF;
        uint32_t r2 = rand() & 0xFF;
        uint32_t r3 = rand() & 0xFF;
        uint32_t r4 = rand() & 0xFF;
        a->data[i] = (r1 << 24) | (r2 << 16) | (r3 << 8) | r4;
    }
    a->size = words;
    
    // [개선] 지정된 비트 길이를 정확히 맞추기 위한 비트 마스킹
    int rem = bit_length % 32;
    if (rem != 0) {
        uint32_t mask = (1U << rem) - 1;
        a->data[words - 1] &= mask;              // 상위 불필요한 비트 클리어
        a->data[words - 1] |= (1U << (rem - 1)); // 최상위 비트를 1로 강제
    } else {
        a->data[words - 1] |= (1U << 31);
    }
    
    a->data[0] |= 1; // 홀수 보장
    BigInt_Trim(a);
}

bool MillerRabin(const BigInt* n, int k) {
    if (BigInt_IsZero(n) || (n->size == 1 && n->data[0] <= 1)) return false;
    if ((n->data[0] & 1) == 0) return (n->size == 1 && n->data[0] == 2); 
    
    BigInt n_minus_1, d, one;
    BigInt_Init(&one); one.data[0] = 1; one.size = 1;
    BigInt_Sub(&n_minus_1, n, &one);
    
    BigInt_Copy(&d, &n_minus_1);
    int s = 0;
    while ((d.data[0] & 1) == 0 && !BigInt_IsZero(&d)) {
        for (int i = 0; i < d.size; i++) {
            uint32_t carry = (i == d.size - 1) ? 0 : (d.data[i + 1] & 1) << 31;
            d.data[i] = (d.data[i] >> 1) | carry;
        }
        BigInt_Trim(&d);
        s++;
    }
    
    BigInt a, x, temp, dummy;
    for (int i = 0; i < k; i++) {
        BigInt_Init(&a);
        
        // [개선] 2 ~ n-2 사이의 무작위 베이스(a) 생성
        int bits_n = (n->size - 1) * 32;
        uint32_t top = n->data[n->size - 1];
        while (top) { bits_n++; top >>= 1; }
        
        Generate_Random_BigInt(&a, bits_n - 1); // n보다 한 비트 작은 랜덤수 생성
        
        // 생성된 a가 1 이하인 경우 안전하게 2 이상으로 보정
        if (BigInt_IsZero(&a) || (a.size == 1 && a.data[0] <= 1)) {
            a.data[0] = 2 + (rand() % 100);
            a.size = 1;
        }
        
        ModExp(&x, &a, &d, n);
        
        if (x.size == 1 && x.data[0] == 1) continue;
        if (BigInt_Compare(&x, &n_minus_1) == 0) continue;
        
        bool composite = true;
        for (int j = 0; j < s - 1; j++) {
            BigInt_Mul(&temp, &x, &x);
            BigInt_DivMod(&dummy, &x, &temp, n);
            
            if (x.size == 1 && x.data[0] == 1) return false; 
            if (BigInt_Compare(&x, &n_minus_1) == 0) {
                composite = false;
                break;
            }
        }
        if (composite) return false;
    }
    return true;
}

void Generate_Prime(BigInt* p, int bit_length) {
    const uint32_t small_primes[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97};
    int num_primes = sizeof(small_primes) / sizeof(small_primes[0]);
    
    while (1) {
        Generate_Random_BigInt(p, bit_length);
        
        bool divisible = false;
        for (int i = 0; i < num_primes; i++) {
            if (BigInt_Mod_Small(p, small_primes[i]) == 0) {
                divisible = true;
                break;
            }
        }
        if (divisible) continue; 
        
        if (BigInt_Mod_Small(p, 65537) == 1) continue;
        
        if (MillerRabin(p, 5)) { // 정확도를 위해 k값을 늘려도 좋습니다.
            break;
        }
    }
}


// ============================================================================
// [4] rsa.h & rsa.c (RSA 키 생성 및 암복호화)
// ============================================================================

typedef struct {
    BigInt p, q, n, e, d, phi;
    BigInt dp, dq, qInv; 
} RSA_Key;

void RSA_GenerateKey(RSA_Key* key, int total_bits) {
    int prime_bits = total_bits / 2;
    BigInt dummy_q;
    
    printf("  [p 탐색] "); Generate_Prime(&key->p, prime_bits); printf(" 완료\n");
    
    // [개선] p와 q가 동일하게 생성될 경우 재탐색
    do {
        printf("  [q 탐색] "); Generate_Prime(&key->q, prime_bits); 
        if (BigInt_Compare(&key->p, &key->q) == 0) {
            printf(" (p와 중복 발생, 재탐색 진행)\n");
        } else {
            printf(" 완료\n");
        }
    } while (BigInt_Compare(&key->p, &key->q) == 0);
    
    if (BigInt_Compare(&key->p, &key->q) < 0) {
        BigInt temp;
        BigInt_Copy(&temp, &key->p);
        BigInt_Copy(&key->p, &key->q);
        BigInt_Copy(&key->q, &temp);
    }
    
    BigInt_Mul(&key->n, &key->p, &key->q);
    
    BigInt p_minus_1, q_minus_1, one;
    BigInt_Init(&one); one.data[0] = 1; one.size = 1;
    
    BigInt_Sub(&p_minus_1, &key->p, &one);
    BigInt_Sub(&q_minus_1, &key->q, &one);
    BigInt_Mul(&key->phi, &p_minus_1, &q_minus_1);
    
    BigInt_Init(&key->e); key->e.data[0] = 65537; key->e.size = 1;
    ModInverse(&key->d, &key->e, &key->phi);
    
    BigInt_DivMod(&dummy_q, &key->dp, &key->d, &p_minus_1);
    BigInt_DivMod(&dummy_q, &key->dq, &key->d, &q_minus_1);
    ModInverse(&key->qInv, &key->q, &key->p);
}

void RSA_Encrypt(BigInt* C, const BigInt* M, const RSA_Key* key) {
    ModExp(C, M, &key->e, &key->n);
}

void RSA_Decrypt_CRT(BigInt* M, const BigInt* C, const RSA_Key* key) {
    BigInt m1, m2, h, temp, dummy_q;
    
    ModExp(&m1, C, &key->dp, &key->p);
    ModExp(&m2, C, &key->dq, &key->q);
    
    BigInt diff;
    if (BigInt_Compare(&m1, &m2) < 0) {
        BigInt m1_plus_p;
        BigInt_Add(&m1_plus_p, &m1, &key->p);
        BigInt_Sub(&diff, &m1_plus_p, &m2);
    } else {
        BigInt_Sub(&diff, &m1, &m2);
    }
    
    BigInt_Mul(&temp, &key->qInv, &diff);
    BigInt_DivMod(&dummy_q, &h, &temp, &key->p);
    
    BigInt_Mul(&temp, &h, &key->q);
    BigInt_Add(M, &m2, &temp);
}


// ============================================================================
// [5] main.c (전체 로직 검증 및 실행 파이프라인)
// ============================================================================

void BigInt_Print(const BigInt* a) {
    if (BigInt_IsZero(a)) {
        printf("00000000");
        return;
    }
    for (int i = a->size - 1; i >= 0; i--) {
        printf("%08X", a->data[i]);
    }
}

int main() {
    srand((unsigned int)time(NULL));
    
    RSA_Key key;
    BigInt plaintext, ciphertext, decrypted;
    
    printf("========== [ 순수 C 구현 RSA-CRT 시뮬레이션 (최적화 완료) ] ==========\n\n");
    
    int key_size = 2048; // 1024비트로 변경 시 연산 시간에 주의하세요.
    printf("%d비트 RSA 키 생성 시작...\n", key_size);
    RSA_GenerateKey(&key, key_size);
    printf("\n키 생성 완료!\n\n");
    
    printf("- p :\n"); BigInt_Print(&key.p); printf("\n\n");
    printf("- q :\n"); BigInt_Print(&key.q); printf("\n\n");
    printf("- 오일러 파이(phi) :\n"); BigInt_Print(&key.phi); printf("\n\n");
    printf("- 모듈러스(n) :\n"); BigInt_Print(&key.n); printf("\n\n");
    printf("- 공개키(e) :\n"); BigInt_Print(&key.e); printf("\n\n");
    printf("- 개인키(d) :\n"); BigInt_Print(&key.d); printf("\n\n\n");
    
    BigInt_Init(&plaintext);
    plaintext.data[0] = 0x48454C4C; 
    plaintext.data[1] = 0x0000004F; 
    plaintext.size = 2; // HELLO를 32비트 단위로 저장
    
    printf("원본 평문 (M) :\n"); BigInt_Print(&plaintext); printf("\n");
    
    RSA_Encrypt(&ciphertext, &plaintext, &key);
    printf("암호화 완료 (C) :\n"); BigInt_Print(&ciphertext); printf("\n");
    
    RSA_Decrypt_CRT(&decrypted, &ciphertext, &key);
    printf("복호화 완료 (M) :\n"); BigInt_Print(&decrypted); printf("\n\n");
    
    if (BigInt_Compare(&plaintext, &decrypted) == 0) {
        printf("[SUCCESS] CRT 기반 수학적 RSA 암복호화가 완벽히 일치합니다.\n");
    } else {
        printf("[FAILED] 연산 무결성 검증 실패.\n");
    }

    return 0;
}