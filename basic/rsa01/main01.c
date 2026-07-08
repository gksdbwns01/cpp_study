#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
// ---------------------------------------------------------
// [핵심 수학 함수 구현]
// ---------------------------------------------------------

// 1. 모듈러 거듭제곱 (Modular Exponentiation)
// (base^exp) % mod 를 효율적으로 계산하는 Square-and-Multiply 알고리즘
// 암호화와 복호화 과정에서 핵심적으로 사용됩니다.
uint64_t mod_exp(uint64_t base, uint64_t exp, uint64_t mod) {
    uint64_t result = 1;
    base = base % mod;
    while (exp > 0) {
        if (exp % 2 == 1) { // 지수의 홀수 여부 확인
            result = (result * base) % mod;
        }
        exp = exp >> 1;     // 지수를 2로 나눔 (비트 시프트)
        base = (base * base) % mod;
    }
    return result;
}

// 2. 최대공약수 (유클리드 호제법)
// 두 수의 최대공약수(GCD)를 구합니다. e와 phi가 서로소인지 확인할 때 씁니다.
uint64_t gcd(uint64_t a, uint64_t b) {
    while (b != 0) {
        uint64_t temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

// 3. 모듈러 역원 (확장 유클리드 호제법)
// (e * d) ≡ 1 (mod phi) 를 만족하는 개인키 d를 찾습니다.
uint64_t mod_inverse(uint64_t e, uint64_t phi) {
    int64_t t = 0, newt = 1;
    int64_t r = phi, newr = e;

    while (newr != 0) {
        int64_t quotient = r / newr;
        
        int64_t temp_t = t;
        t = newt;
        newt = temp_t - quotient * newt;
        
        int64_t temp_r = r;
        r = newr;
        newr = temp_r - quotient * newr;
    }

    if (r > 1) return 0; // 역원이 존재하지 않음 (서로소가 아님)
    if (t < 0) t = t + phi; // 음수일 경우 양수로 변환
    
    return (uint64_t)t;
}

// ---------------------------------------------------------
// [메인 함수: RSA 키 생성 및 암복호화]
// ---------------------------------------------------------

int main() {
    // 1. 두 개의 소수 p, q 선택
    // (설명: 오버플로우 방지를 위해 16비트 크기의 소수 사용. 결과적으로 모듈러 n은 32비트)
    uint64_t p = 32771;
    uint64_t q = 32749;
    
    // 2. 모듈러 n (키 사이즈) 및 오일러 파이 함수 값 계산
    uint64_t n = p * q;
    uint64_t phi = (p - 1) * (q - 1);
    
    // 3. 공개키 e 선택
    // (설명: 실제 산업 표준 RSA에서 가장 널리 쓰이는 페르마 소수 65537 사용)
    uint64_t e = 65537; 
    
    // e와 phi가 서로소인지 검증
    if (gcd(e, phi) != 1) {
        printf("오류: 선택한 e와 phi가 서로소가 아닙니다.\n");
        return 1;
    }
    
    // 4. 개인키 d 계산
    uint64_t d = mod_inverse(e, phi);
    if (d == 0) {
        printf("오류: 개인키 d를 생성할 수 없습니다.\n");
        return 1;
    }
    
    // 파라미터 출력
    printf("========== [ RSA 파라미터 셋업 ] ==========\n");
    printf("소수 (p, q)     : %" PRIu64 ", %" PRIu64 "\n", p, q);
    printf("모듈러 n (키값) : %" PRIu64 "\n", n);
    printf("오일러 파이(φ)  : %" PRIu64 "\n", phi);
    printf("공개키 (e, n)   : (%" PRIu64 ", %" PRIu64 ")\n", e, n);
    printf("개인키 (d, n)   : (%" PRIu64 ", %" PRIu64 ")\n", d, n);
    printf("===========================================\n\n");
    
    // 5. 암호화 및 복호화 시뮬레이션
    uint64_t message = 828365; // 예시 평문 (반드시 n보다 작아야 함)
    
    printf("▶ 원본 평문(Message) : %" PRIu64 "\n", message);
    
    // 암호화 공식: C = M^e (mod n)
    uint64_t ciphertext = mod_exp(message, e, n);
    printf("▶ 암호문(Ciphertext) : %" PRIu64 "\n", ciphertext);
    
    // 복호화 공식: M = C^d (mod n)
    uint64_t decrypted = mod_exp(ciphertext, d, n);
    printf("▶ 복호화된 평문      : %" PRIu64 "\n", decrypted);
    
    // 검증
    if (message == decrypted) {
        printf("\n[성공] 수학적 RSA 암복호화 사이클이 완벽하게 완료되었습니다!\n");
    } else {
        printf("\n[실패] 복호화 중 데이터 손실 또는 오버플로우가 발생했습니다.\n");
    }

    return 0;
}