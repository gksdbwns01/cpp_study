#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

// ============================================================================
// [1] bigint.h & bigint.c (Big Integer 직접 구현)
// ============================================================================

#define MAX_WORDS 64 // 64 * 32bit = 2048bit 지원

typedef struct {
    uint32_t data[MAX_WORDS];
    int size; // 사용 중인 32비트 워드의 개수
} BigInt;

// 초기화 및 복사
void BigInt_Init(BigInt* a) {
    memset(a->data, 0, sizeof(a->data));
    a->size = 0;
}

void BigInt_Copy(BigInt* dest, const BigInt* src) {
    memcpy(dest->data, src->data, sizeof(src->data));
    dest->size = src->size;
}

// 상위 워드의 불필요한 0 제거 (연산 속도 최적화)
void BigInt_Trim(BigInt* a) {
    while (a->size > 0 && a->data[a->size - 1] == 0) {
        a->size--;
    }
}

// 0 확인
bool BigInt_IsZero(const BigInt* a) {
    return a->size == 0 || (a->size == 1 && a->data[0] == 0);
}

// 비교 연산자 (a > b: 1, a == b: 0, a < b: -1)
int BigInt_Compare(const BigInt* a, const BigInt* b) {
    if (a->size > b->size) return 1;
    if (a->size < b->size) return -1;
    for (int i = a->size - 1; i >= 0; i--) {
        if (a->data[i] > b->data[i]) return 1;
        if (a->data[i] < b->data[i]) return -1;
    }
    return 0;
}

// 덧셈 (res = a + b)
void BigInt_Add(BigInt* res, const BigInt* a, const BigInt* b) {
    uint64_t carry = 0;
    int max_size = (a->size > b->size) ? a->size : b->size;
    BigInt_Init(res);
    
    for (int i = 0; i < max_size || carry > 0; i++) {
        uint64_t sum = carry;
        if (i < a->size) sum += a->data[i];
        if (i < b->size) sum += b->data[i];
        
        res->data[i] = (uint32_t)(sum & 0xFFFFFFFF);
        carry = sum >> 32;
        res->size = i + 1;
    }
}

// 뺄셈 (res = a - b, a >= b 가정)
void BigInt_Sub(BigInt* res, const BigInt* a, const BigInt* b) {
    int64_t borrow = 0;
    BigInt_Init(res);
    
    for (int i = 0; i < a->size; i++) {
        int64_t diff = (int64_t)a->data[i] - borrow;
        if (i < b->size) diff -= b->data[i];
        
        if (diff < 0) {
            diff += 0x100000000LL;
            borrow = 1;
        } else {
            borrow = 0;
        }
        res->data[i] = (uint32_t)diff;
        res->size = i + 1;
    }
    BigInt_Trim(res);
}

// 곱셈 (res = a * b)
void BigInt_Mul(BigInt* res, const BigInt* a, const BigInt* b) {
    BigInt_Init(res);
    if (BigInt_IsZero(a) || BigInt_IsZero(b)) return;
    
    res->size = a->size + b->size;
    for (int i = 0; i < a->size; i++) {
        uint64_t carry = 0;
        for (int j = 0; j < b->size; j++) {
            uint64_t prod = (uint64_t)a->data[i] * b->data[j] + res->data[i + j] + carry;
            res->data[i + j] = (uint32_t)(prod & 0xFFFFFFFF);
            carry = prod >> 32;
        }
        res->data[i + b->size] = (uint32_t)carry;
    }
    BigInt_Trim(res);
}

// 비트 시프트 연산 (Left)
void BigInt_ShiftLeft1(BigInt* a) {
    if (BigInt_IsZero(a)) return;
    uint32_t carry = 0;
    for (int i = 0; i < a->size; i++) {
        uint32_t next_carry = a->data[i] >> 31;
        a->data[i] = (a->data[i] << 1) | carry;
        carry = next_carry;
    }
    if (carry > 0) {
        a->data[a->size] = carry;
        a->size++;
    }
}

// 나눗셈 및 나머지 연산 (q = a / b, r = a % b) - Shift-and-Subtract 방식
void BigInt_DivMod(BigInt* q, BigInt* r, const BigInt* a, const BigInt* b) {
    BigInt_Init(q);
    BigInt_Init(r);
    if (BigInt_IsZero(b)) return; // 0으로 나누기 방지
    
    if (BigInt_Compare(a, b) < 0) {
        if (r) BigInt_Copy(r, a);
        return;
    }
    
    // a의 총 비트 수 계산
    int a_bits = (a->size - 1) * 32;
    uint32_t top = a->data[a->size - 1];
    while (top) { a_bits++; top >>= 1; }
    
    q->size = (a_bits + 31) / 32;
    
    for (int i = a_bits - 1; i >= 0; i--) {
        BigInt_ShiftLeft1(r);
        
        // r의 최하위 비트에 a의 i번째 비트 삽입
        int word_idx = i / 32;
        int bit_idx = i % 32;
        if ((a->data[word_idx] >> bit_idx) & 1) {
            r->data[0] |= 1;
            if (r->size == 0) r->size = 1;
        }
        
        if (BigInt_Compare(r, b) >= 0) {
            BigInt temp;
            BigInt_Sub(&temp, r, b);
            BigInt_Copy(r, &temp);
            q->data[word_idx] |= (1 << bit_idx);
        }
    }
    BigInt_Trim(q);
    BigInt_Trim(r);
}


// ============================================================================
// [2] rsa_math.c (모듈러 연산 및 확장 유클리드)
// ============================================================================

// 모듈러 거듭제곱 (res = base^exp mod n)
void ModExp(BigInt* res, const BigInt* base, const BigInt* exp, const BigInt* mod) {
    BigInt b, e, temp;
    BigInt_Copy(&b, base);
    BigInt_Copy(&e, exp);
    
    BigInt_Init(res);
    res->data[0] = 1;
    res->size = 1;
    
    BigInt dummy_q;
    
    while (!BigInt_IsZero(&e)) {
        if (e.data[0] & 1) {
            BigInt_Mul(&temp, res, &b);
            BigInt_DivMod(&dummy_q, res, &temp, mod);
        }
        BigInt_Mul(&temp, &b, &b);
        BigInt_DivMod(&dummy_q, &b, &temp, mod);
        
        // e >>= 1 (1비트 Right Shift)
        for (int i = 0; i < e.size; i++) {
            uint32_t carry = (i == e.size - 1) ? 0 : (e.data[i + 1] & 1) << 31;
            e.data[i] = (e.data[i] >> 1) | carry;
        }
        BigInt_Trim(&e);
    }
}

// 확장 유클리드 호제법 (d = e^-1 mod phi 계산)
// 부호 관리를 위해 절대값 구조체와 별도의 부호 플래그를 사용합니다.
void ModInverse(BigInt* res, const BigInt* e, const BigInt* phi) {
    BigInt t, newt, r, newr, q, temp, prod, next_t;
    int t_sign = 1, newt_sign = 1; // 1: 양수, -1: 음수
    
    BigInt_Init(&t);
    BigInt_Init(&newt); newt.data[0] = 1; newt.size = 1;
    
    BigInt_Copy(&r, phi);
    BigInt_Copy(&newr, e);
    
    BigInt dummy;
    
    while (!BigInt_IsZero(&newr)) {
        // q = r / newr, temp = r % newr
        BigInt_DivMod(&q, &temp, &r, &newr);
        BigInt_Copy(&r, &newr);
        BigInt_Copy(&newr, &temp);
        
        // prod = q * newt
        BigInt_Mul(&prod, &q, &newt);
        
        // next_t = t - (prod * newt_sign) 
        // 부호가 같은지 다른지에 따라 덧셈/뺄셈 결정
        if (t_sign == newt_sign) {
            if (BigInt_Compare(&t, &prod) >= 0) {
                BigInt_Sub(&next_t, &t, &prod);
                // t_sign 그대로
            } else {
                BigInt_Sub(&next_t, &prod, &t);
                t_sign = -t_sign;
            }
        } else {
            BigInt_Add(&next_t, &t, &prod);
            // t_sign 그대로
        }
        
        BigInt_Copy(&t, &newt);
        t_sign = newt_sign;
        
        BigInt_Copy(&newt, &next_t);
        newt_sign = t_sign == newt_sign ? -newt_sign : t_sign; // 갱신된 부호
    }
    
    // 결과 처리 (d가 음수이면 phi를 더해 양수로 변환)
    if (t_sign == -1) {
        BigInt_Sub(res, phi, &t);
    } else {
        BigInt_Copy(res, &t);
    }
}


// ============================================================================
// [3] random.h & random.c (난수 및 소수 판별)
// ============================================================================

// 난수 생성기 (지정된 비트 길이의 홀수 생성)
void Generate_Random_BigInt(BigInt* a, int bit_length) {
    BigInt_Init(a);
    int words = bit_length / 32;
    for (int i = 0; i < words; i++) {
        a->data[i] = ((uint32_t)rand() << 16) ^ (uint32_t)rand();
    }
    a->size = words;
    a->data[words - 1] |= (1 << 31); // 최상위 비트를 1로 설정하여 길이 보장
    a->data[0] |= 1;                 // 홀수로 설정
}

// Miller-Rabin 소수 판별
bool MillerRabin(const BigInt* n, int k) {
    if (BigInt_IsZero(n) || (n->size == 1 && n->data[0] == 1)) return false;
    if ((n->data[0] & 1) == 0) return false; // 짝수
    
    // n-1 계산
    BigInt n_minus_1, d;
    BigInt_Init(&n_minus_1); n_minus_1.data[0] = 1; n_minus_1.size = 1;
    BigInt_Sub(&n_minus_1, n, &n_minus_1);
    
    // d * 2^s = n - 1
    BigInt_Copy(&d, &n_minus_1);
    int s = 0;
    while ((d.data[0] & 1) == 0) {
        // d >>= 1
        for (int i = 0; i < d.size; i++) {
            uint32_t carry = (i == d.size - 1) ? 0 : (d.data[i + 1] & 1) << 31;
            d.data[i] = (d.data[i] >> 1) | carry;
        }
        BigInt_Trim(&d);
        s++;
    }
    
    // k번 검사
    BigInt a, x, temp, dummy;
    BigInt two; BigInt_Init(&two); two.data[0] = 2; two.size = 1;
    
    for (int i = 0; i < k; i++) {
        // 임의의 a 생성 (간단히 작은 수로 테스트)
        BigInt_Init(&a);
        a.data[0] = 2 + (rand() % 100); 
        a.size = 1;
        
        ModExp(&x, &a, &d, n);
        
        if (x.size == 1 && x.data[0] == 1) continue;
        if (BigInt_Compare(&x, &n_minus_1) == 0) continue;
        
        bool composite = true;
        for (int j = 0; j < s - 1; j++) {
            BigInt_Mul(&temp, &x, &x);
            BigInt_DivMod(&dummy, &x, &temp, n);
            
            if (x.size == 1 && x.data[0] == 1) return false; // 합성수
            if (BigInt_Compare(&x, &n_minus_1) == 0) {
                composite = false;
                break;
            }
        }
        if (composite) return false;
    }
    return true;
}

// 최종 소수 생성
void Generate_Prime(BigInt* p, int bit_length) {
    while (1) {
        Generate_Random_BigInt(p, bit_length);
        // 오류 확률 2^-80 이하를 위해 약 40번의 루프
        if (MillerRabin(p, 40)) {
            break;
        }
    }
}


// ============================================================================
// [4] rsa.h & rsa.c (RSA 키 생성 및 암복호화)
// ============================================================================

typedef struct {
    BigInt p, q, n, e, d;
    BigInt dp, dq, qInv; // CRT(중국인의 나머지 정리) 파라미터
} RSA_Key;

void RSA_GenerateKey(RSA_Key* key, int total_bits) {
    int prime_bits = total_bits / 2;
    BigInt dummy_q, dummy_r;
    
    // 1. 소수 p, q 생성
    Generate_Prime(&key->p, prime_bits);
    Generate_Prime(&key->q, prime_bits);
    
    // p가 q보다 항상 크도록 강제 설정 (CRT 계산에서 음수 방지)
    if (BigInt_Compare(&key->p, &key->q) < 0) {
        BigInt temp;
        BigInt_Copy(&temp, &key->p);
        BigInt_Copy(&key->p, &key->q);
        BigInt_Copy(&key->q, &temp);
    }
    
    // 2. n = p * q
    BigInt_Mul(&key->n, &key->p, &key->q);
    
    // 3. phi = (p-1)*(q-1)
    BigInt p_minus_1, q_minus_1, phi, one;
    BigInt_Init(&one); one.data[0] = 1; one.size = 1;
    
    BigInt_Sub(&p_minus_1, &key->p, &one);
    BigInt_Sub(&q_minus_1, &key->q, &one);
    BigInt_Mul(&phi, &p_minus_1, &q_minus_1);
    
    // 4. e = 65537
    BigInt_Init(&key->e);
    key->e.data[0] = 65537;
    key->e.size = 1;
    
    // 5. d = e^-1 mod phi
    ModInverse(&key->d, &key->e, &phi);
    
    // 6. CRT 파라미터 계산
    // dp = d mod (p-1)
    BigInt_DivMod(&dummy_q, &key->dp, &key->d, &p_minus_1);
    // dq = d mod (q-1)
    BigInt_DivMod(&dummy_q, &key->dq, &key->d, &q_minus_1);
    // qInv = q^-1 mod p
    ModInverse(&key->qInv, &key->q, &key->p);
}

// 암호화 (C = M^e mod n)
void RSA_Encrypt(BigInt* C, const BigInt* M, const RSA_Key* key) {
    // 실제 표준에서는 여기서 M에 OAEP 패딩을 적용해야 합니다.
    ModExp(C, M, &key->e, &key->n);
}

// 복호화 (CRT 적용)
void RSA_Decrypt_CRT(BigInt* M, const BigInt* C, const RSA_Key* key) {
    BigInt m1, m2, h, temp, dummy_q;
    
    // m1 = C^dp mod p
    ModExp(&m1, C, &key->dp, &key->p);
    // m2 = C^dq mod q
    ModExp(&m2, C, &key->dq, &key->q);
    
    // m1 < m2 일 경우를 대비해 p를 더한 뒤 뺌 (m1 - m2 mod p)
    BigInt diff;
    if (BigInt_Compare(&m1, &m2) < 0) {
        BigInt m1_plus_p;
        BigInt_Add(&m1_plus_p, &m1, &key->p);
        BigInt_Sub(&diff, &m1_plus_p, &m2);
    } else {
        BigInt_Sub(&diff, &m1, &m2);
    }
    
    // h = (qInv * diff) mod p
    BigInt_Mul(&temp, &key->qInv, &diff);
    BigInt_DivMod(&dummy_q, &h, &temp, &key->p);
    
    // M = m2 + (h * q)
    BigInt_Mul(&temp, &h, &key->q);
    BigInt_Add(M, &m2, &temp);
}


// ============================================================================
// [5] main.c (전체 로직 검증 및 실행 파이프라인)
// ============================================================================

int main() {
    srand((unsigned int)time(NULL));
    
    RSA_Key key;
    BigInt plaintext, ciphertext, decrypted;
    
    printf("========== [ 순수 C 구현 RSA-CRT 시뮬레이션 ] ==========\n\n");
    
    // 1. 키 생성
    // 시뮬레이션 속도를 위해 512비트(소수 256비트)로 설정했습니다.
    // 2048로 변경 시 수십 초 이상 소요될 수 있습니다.
    int key_size = 512; 
    printf("[*] %d비트 RSA 키를 Miller-Rabin으로 생성 중입니다...\n", key_size);
    RSA_GenerateKey(&key, key_size);
    printf("[+] 키 생성 완료!\n\n");
    
    printf("- p (상위 워드)   : %08X...\n", key.p.data[key.p.size - 1]);
    printf("- q (상위 워드)   : %08X...\n", key.q.data[key.q.size - 1]);
    printf("- n (모듈러)      : %08X... (Size: %d words)\n\n", key.n.data[key.n.size - 1], key.n.size);
    
    // 2. 평문 설정 (예시: "HELLO"의 아스키 모의값)
    BigInt_Init(&plaintext);
    plaintext.data[0] = 0x48454C4C; // HELL
    plaintext.data[1] = 0x0000004F; // O
    plaintext.size = 2;
    
    printf("[*] 원본 평문 (M) : %08X %08X\n", plaintext.data[1], plaintext.data[0]);
    
    // 3. 암호화
    RSA_Encrypt(&ciphertext, &plaintext, &key);
    printf("[+] 암호화 완료 (C) : %08X...\n", ciphertext.data[ciphertext.size - 1]);
    
    // 4. 복호화 (CRT)
    RSA_Decrypt_CRT(&decrypted, &ciphertext, &key);
    printf("[+] 복호화 완료 (M) : %08X %08X\n\n", decrypted.data[1], decrypted.data[0]);
    
    // 5. 무결성 검증
    if (BigInt_Compare(&plaintext, &decrypted) == 0) {
        printf("[SUCCESS] CRT 기반 수학적 RSA 암복호화가 완벽히 일치합니다.\n");
    } else {
        printf("[FAILED] 데이터 손실 발생. 연산 검증이 필요합니다.\n");
    }

    return 0;
}