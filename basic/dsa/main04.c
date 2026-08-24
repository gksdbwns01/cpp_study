#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <sys/random.h>
#include <openssl/sha.h> // OpenSSL 해시 라이브러리 추가

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

void BigInt_FromBytes(BigInt* a, const uint8_t* bytes, size_t len) {
    BigInt_Init(a);
    int word_idx = 0;
    
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

// ECDSA 서명 및 검증 로직
typedef struct {
    BigInt r; // 랜덤하게 생성된 타원곡선 점 R의 x좌표 값 (mod n)
    BigInt s; // 개인키와 해시값, 그리고 r값을 이용해 계산된 검증용 값
} ECDSA_Signature;


// ECDSA_Sign 함수 (코어 서명 로직)
// 해시된 메시지(msg_hash)와 개인키(priv_key)를 이용해 서명(r, s)을 생성
void ECDSA_Sign(ECDSA_Signature* sig, const BigInt* msg_hash, const BigInt* priv_key) {
    BigInt k, k_inv, r, s, dr, e_plus_dr;
    EC_Point R;

    // r이나 s가 0이 나오면 극히 희박한 확률로 서명이 실패한 것이므로 다시 시도
    do {
        // 임시 개인키(Nonce) k 생성 (1부터 n-1 사이의 안전한 난수)
        EC_GeneratePrivateKey(&k);
        
        // 임시 공개키 R 계산 (R = k * G)
        // 생성점 G를 k번 더해서 타원곡선 위의 점 R을 구함
        EC_Scalar_Mul(&R, &P256_G, &k, &P256_a, &P256_p);
        
        // 서명의 첫 번째 값 'r' 계산 (r = R의 x좌표 mod n)
        // 타원곡선의 위수(Order)인 P256_n으로 나눈 나머지를 구함
        BigInt_DivMod(NULL, &r, &R.x, &P256_n);
        
        // r이 0이면 서명에 사용할 수 없으므로 k부터 다시 생성
        if (BigInt_IsZero(&r)) continue;

        // 서명의 두 번째 값 's' 계산. s = k^-1 * (msg_hash + r * priv_key) mod n
        
        // k의 모듈러 역원(k^-1 mod n)을 구함
        ModInverse(&k_inv, &k, &P256_n);
        
        // dr = r * priv_key (mod n) 계산
        ModMul(&dr, &r, priv_key, &P256_n);
        
        // e_plus_dr = msg_hash + (r * priv_key) (mod n) 계산
        ModAdd(&e_plus_dr, msg_hash, &dr, &P256_n);
        
        // s = k_inv * e_plus_dr (mod n) 계산 완료
        ModMul(&s, &k_inv, &e_plus_dr, &P256_n);

    // s가 0이 나오면 서명 규칙 위반이므로 k부터 다시 시도
    } while (BigInt_IsZero(&s));

    // 계산된 r과 s를 서명 구조체에 복사하여 반환
    BigInt_Copy(&sig->r, &r);
    BigInt_Copy(&sig->s, &s);
}


// ECDSA_Verify 함수 (코어 검증 로직)
// 해시된 메시지, 서명(r, s), 그리고 서명자의 공개키(pub_key)를 이용해 서명을 검증
bool ECDSA_Verify(const ECDSA_Signature* sig, const BigInt* msg_hash, const EC_Point* pub_key) {
    // 기본 유효성 검사
    // r과 s는 0보다 커야 하고, 타원곡선 위수(n)보다 작아야 함 (1 <= r, s < n)
    if (BigInt_IsZero(&sig->r) || BigInt_Compare(&sig->r, &P256_n) >= 0) return false;
    if (BigInt_IsZero(&sig->s) || BigInt_Compare(&sig->s, &P256_n) >= 0) return false;

    BigInt w, u1, u2, P_x_mod_n;
    EC_Point u1G, u2Q, P;

    // s의 모듈러 역원(w) 계산. w = s^-1 mod n
    ModInverse(&w, &sig->s, &P256_n);
    
    // u1 계산. u1 = (msg_hash * w) mod n
    ModMul(&u1, msg_hash, &w, &P256_n);
    
    // u2 계산. u2 = (r * w) mod n
    ModMul(&u2, &sig->r, &w, &P256_n);

    // 타원곡선 점 P 복원. P = (u1 * G) + (u2 * pub_key)
    
    // u1 * G 계산 (생성점에 u1을 스칼라 곱)
    EC_Scalar_Mul(&u1G, &P256_G, &u1, &P256_a, &P256_p);
    
    // u2 * pub_key 계산 (공개키에 u2를 스칼라 곱)
    EC_Scalar_Mul(&u2Q, pub_key, &u2, &P256_a, &P256_p);
    
    // 두 점을 더해 P를 구함 (P = u1G + u2Q)
    EC_Point_Add(&P, &u1G, &u2Q, &P256_a, &P256_p);

    // 계산된 점 P가 무한원점이면 잘못된 서명
    if (P.is_infinity) return false;

    // P의 x좌표를 mod n 한 값 구하기
    BigInt_DivMod(NULL, &P_x_mod_n, &P.x, &P256_n);

    // 복원된 x좌표(P_x_mod_n)가 서명의 r 값과 일치하는지 확인
    // 일치하면 true(유효한 서명), 다르면 false(위조된 서명) 반환
    return (BigInt_Compare(&P_x_mod_n, &sig->r) == 0);
}


// ECDSA_Sign_Message 함수 (래퍼 함수)
// 일반 문자열(message)을 받아 직접 SHA-256 해시 후 서명하는 편의용 함수
void ECDSA_Sign_Message(ECDSA_Signature* sig, const char* message, const BigInt* priv_key) {
    // SHA256_DIGEST_LENGTH는 OpenSSL에 정의된 상수(32)
    uint8_t hash[SHA256_DIGEST_LENGTH]; 
    BigInt msg_hash;

    // OpenSSL 라이브러리를 이용해 문자열을 SHA-256으로 해시
    // 결과는 32바이트 길이의 hash 배열에 저장
    SHA256((const unsigned char*)message, strlen(message), hash);

    // 32바이트 해시 배열(uint8_t 배열)을 우리가 만든 BigInt 구조체로 변환
    BigInt_FromBytes(&msg_hash, hash, SHA256_DIGEST_LENGTH);

    // 변환된 BigInt 해시값과 개인키를 코어 서명 함수로 넘겨 실제 서명을 수행
    ECDSA_Sign(sig, &msg_hash, priv_key);
}

// ECDSA_Verify_Message 함수 (래퍼 함수)
// 일반 문자열(message)의 서명 유효성을 곧바로 검증하는 편의용 함수
bool ECDSA_Verify_Message(const ECDSA_Signature* sig, const char* message, const EC_Point* pub_key) {
    uint8_t hash[SHA256_DIGEST_LENGTH];
    BigInt msg_hash;

    // 검증할 문자열 원본을 OpenSSL로 다시 SHA-256 해시
    SHA256((const unsigned char*)message, strlen(message), hash);

    // 해시 결과(32바이트 배열)를 BigInt 구조체로 변환
    BigInt_FromBytes(&msg_hash, hash, SHA256_DIGEST_LENGTH);

    // 변환된 BigInt 해시값, 넘겨받은 서명(sig), 공개키를 코어 검증 함수로 넘겨 검사
    // 검증 결과(참/거짓)를 그대로 반환
    return ECDSA_Verify(sig, &msg_hash, pub_key);
}

int main() {
    EC_InitParameters();

    printf("=================================================================\n");
    printf("                  [ ECDSA 서명 및 검증 (SHA256) ]                  \n");
    printf("=================================================================\n\n");

    BigInt alice_priv;
    EC_Point alice_pub;
    
    // Alice 키 쌍 생성
    EC_GeneratePrivateKey(&alice_priv);
    EC_Scalar_Mul(&alice_pub, &P256_G, &alice_priv, &P256_a, &P256_p);

    printf("Alice 키 쌍 생성 완료\n");
    printf("  - Private Key : "); BigInt_PrintHex(&alice_priv); printf("\n");
    printf("  - Public Key X: "); BigInt_PrintHex(&alice_pub.x); printf("\n");
    printf("  - Public Key Y: "); BigInt_PrintHex(&alice_pub.y); printf("\n\n");

    // 전송할 원본 메시지 준비
    const char* original_message = "ECDSA 원본 메시지";
    
    // 서명 생성
    ECDSA_Signature signature;
    ECDSA_Sign_Message(&signature, original_message, &alice_priv);
    
    printf("서명 생성 완료\n");
    printf("  - 원본 메시지 : \"%s\"\n", original_message);
    printf("  - Signature r : "); BigInt_PrintHex(&signature.r); printf("\n");
    printf("  - Signature s : "); BigInt_PrintHex(&signature.s); printf("\n\n");

    // 서명 검증 상세 과정
    printf("메인 함수 내부 검증 과정\n");
    
    uint8_t hash[SHA256_DIGEST_LENGTH];
    SHA256((const unsigned char*)original_message, strlen(original_message), hash);
    
    BigInt msg_hash;
    BigInt_FromBytes(&msg_hash, hash, SHA256_DIGEST_LENGTH);
    printf("  - Message Hash: "); BigInt_PrintHex(&msg_hash); printf("\n");

    BigInt w, u1, u2, P_x_mod_n;
    EC_Point u1G, u2Q, P;

    ModInverse(&w, &signature.s, &P256_n);
    printf("  - w (s^-1)    : "); BigInt_PrintHex(&w); printf("\n");

    ModMul(&u1, &msg_hash, &w, &P256_n);
    printf("  - u1 (hash*w) : "); BigInt_PrintHex(&u1); printf("\n");

    ModMul(&u2, &signature.r, &w, &P256_n);
    printf("  - u2 (r*w)    : "); BigInt_PrintHex(&u2); printf("\n\n");

    EC_Scalar_Mul(&u1G, &P256_G, &u1, &P256_a, &P256_p);
    EC_Scalar_Mul(&u2Q, &alice_pub, &u2, &P256_a, &P256_p);
    EC_Point_Add(&P, &u1G, &u2Q, &P256_a, &P256_p);
    
    BigInt_DivMod(NULL, &P_x_mod_n, &P.x, &P256_n);
    
    printf("  [최종 검증 대조]\n");
    printf("  => 복원된 P의 x좌표: "); BigInt_PrintHex(&P_x_mod_n); printf("\n");
    printf("  => 원본 서명의 r 값: "); BigInt_PrintHex(&signature.r); printf("\n");
    
    if (BigInt_Compare(&P_x_mod_n, &signature.r) == 0) {
        printf("  [SUCCESS] 서명이 유효합니다! (Alice가 작성한 메시지)\n\n");
    } else {
        printf("  [FAILED] 서명 검증에 실패했습니다.\n\n");
    }

    // 서명 위변조 테스트
    const char* fake_message = "해커가 변조한 가짜 메시지";
    printf("-----------------------------------------------------------------\n");
    printf("서명 위변조 테스트 (조작된 메시지로 시도)\n");
    printf("  - 조작된 메시지 : \"%s\"\n", fake_message);
    
    uint8_t fake_hash_bytes[SHA256_DIGEST_LENGTH];
    SHA256((const unsigned char*)fake_message, strlen(fake_message), fake_hash_bytes);
    BigInt fake_hash;
    BigInt_FromBytes(&fake_hash, fake_hash_bytes, SHA256_DIGEST_LENGTH);
    printf("  - Fake Hash     : "); BigInt_PrintHex(&fake_hash); printf("\n");
    
    ModMul(&u1, &fake_hash, &w, &P256_n); 
    printf("  - 새로운 u1     : "); BigInt_PrintHex(&u1); printf("\n\n");

    EC_Scalar_Mul(&u1G, &P256_G, &u1, &P256_a, &P256_p);
    EC_Point_Add(&P, &u1G, &u2Q, &P256_a, &P256_p);
    BigInt_DivMod(NULL, &P_x_mod_n, &P.x, &P256_n);
    
    printf("  [위변조 검증 대조]\n");
    printf("  => 복원된 P의 x좌표: "); BigInt_PrintHex(&P_x_mod_n); printf("\n");
    printf("  => 원본 서명의 r 값: "); BigInt_PrintHex(&signature.r); printf("\n");
    
    if (BigInt_Compare(&P_x_mod_n, &signature.r) == 0) {
        printf("  [ERROR] 조작된 메시지가 유효하다고 판별되었습니다!\n");
    } else {
        printf("  [SUCCESS] 위변조 감지! 복원된 x값이 원본 r과 달라 검증에 실패했습니다.\n");
    }

    // 키(Key) 위변조 및 불일치 상세 과정 테스트
    printf("\n=================================================================\n");
    printf("            [ 추가 키 불일치 방어 테스트 (중간 과정 포함) ]            \n");
    printf("=================================================================\n\n");

    // Bob의 키 쌍 생성 (타인)
    BigInt bob_priv;
    EC_Point bob_pub;
    EC_GeneratePrivateKey(&bob_priv);
    EC_Scalar_Mul(&bob_pub, &P256_G, &bob_priv, &P256_a, &P256_p);
    
    printf("Bob 키 쌍 생성 완료 (타인)\n");
    printf("  - Bob Public Key X: "); BigInt_PrintHex(&bob_pub.x); printf("\n\n");

    // ----------------------------------------------------------------------------
    // 테스트 A: Alice의 서명을 Bob의 공개키로 검증 시도
    // ----------------------------------------------------------------------------
    printf("-----------------------------------------------------------------\n");
    printf("테스트 A: 올바른 서명을 '다른 사람(Bob)의 공개키'로 검증 시도\n");
    
    // msg_hash, w, u1, u2는 Alice의 서명(signature)과 원본 메시지를 기반으로 하므로 기존 값을 그대로 씀
    // 타원곡선 점을 더할 때 alice_pub가 아닌 bob_pub를 사용
    EC_Point u2Q_bob, P_testA;
    BigInt P_x_mod_n_testA;

    EC_Scalar_Mul(&u2Q_bob, &bob_pub, &u2, &P256_a, &P256_p); // Bob의 공개키 사용
    EC_Point_Add(&P_testA, &u1G, &u2Q_bob, &P256_a, &P256_p);
    BigInt_DivMod(NULL, &P_x_mod_n_testA, &P_testA.x, &P256_n);

    printf("  [공개키 불일치 검증 대조]\n");
    printf("  => Bob의 공개키로 복원된 P의 x좌표: "); BigInt_PrintHex(&P_x_mod_n_testA); printf("\n");
    printf("  => 원본 서명의 r 값               : "); BigInt_PrintHex(&signature.r); printf("\n");

    if (BigInt_Compare(&P_x_mod_n_testA, &signature.r) == 0) {
        printf("  [ERROR] 잘못된 공개키인데 검증이 통과되었습니다!\n\n");
    } else {
        printf("  [SUCCESS] 복원된 x값이 원본 r과 다릅니다. (잘못된 공개키 방어 성공)\n\n");
    }

    // ----------------------------------------------------------------------------
    // 테스트 B: Bob이 서명한 가짜 서명을 Alice의 공개키로 검증 시도
    // ----------------------------------------------------------------------------
    printf("-----------------------------------------------------------------\n");
    printf("테스트 B: 다른 사람(Bob)이 생성한 서명을 'Alice의 공개키'로 검증 시도\n");

    // Bob이 자신의 개인키로 원본 메시지에 서명
    ECDSA_Signature bob_signature;
    ECDSA_Sign_Message(&bob_signature, original_message, &bob_priv);
    
    printf("  - Bob이 생성한 Signature r : "); BigInt_PrintHex(&bob_signature.r); printf("\n");
    printf("  - Bob이 생성한 Signature s : "); BigInt_PrintHex(&bob_signature.s); printf("\n\n");

    // Bob의 서명값을 바탕으로 새로운 w, u1, u2를 계산
    BigInt w_bob, u1_bob, u2_bob, P_x_mod_n_testB;
    EC_Point u1G_bob, u2Q_alice, P_testB;

    ModInverse(&w_bob, &bob_signature.s, &P256_n);
    printf("  - 새로운 w (Bob s^-1): "); BigInt_PrintHex(&w_bob); printf("\n");

    ModMul(&u1_bob, &msg_hash, &w_bob, &P256_n);
    printf("  - 새로운 u1          : "); BigInt_PrintHex(&u1_bob); printf("\n");

    ModMul(&u2_bob, &bob_signature.r, &w_bob, &P256_n);
    printf("  - 새로운 u2          : "); BigInt_PrintHex(&u2_bob); printf("\n\n");

    // Alice의 공개키를 사용하여 검증을 시도
    EC_Scalar_Mul(&u1G_bob, &P256_G, &u1_bob, &P256_a, &P256_p);
    EC_Scalar_Mul(&u2Q_alice, &alice_pub, &u2_bob, &P256_a, &P256_p); // Alice 공개키 적용
    EC_Point_Add(&P_testB, &u1G_bob, &u2Q_alice, &P256_a, &P256_p);
    
    BigInt_DivMod(NULL, &P_x_mod_n_testB, &P_testB.x, &P256_n);

    printf("  [서명자 위조 검증 대조]\n");
    printf("  => Alice공개키로 복원된 P의 x좌표: "); BigInt_PrintHex(&P_x_mod_n_testB); printf("\n");
    printf("  => Bob 서명의 r 값               : "); BigInt_PrintHex(&bob_signature.r); printf("\n");

    if (BigInt_Compare(&P_x_mod_n_testB, &bob_signature.r) == 0) {
        printf("  [ERROR] 타인의 서명이 유효하다고 판별되었습니다!\n");
    } else {
        printf("  [SUCCESS] 복원된 x값이 Bob의 r과 다릅니다. (위조 서명 방어 성공)\n");
    }
    printf("=================================================================\n");
    return 0;
}