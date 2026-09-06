#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "params.h"
#include "kem.h"
#include "poly.h"
#include "polyvec.h"
int is_reencap = 0;
uint8_t global_m[KYBER_SYMBYTES];
/* ================================
 * 출력 헬퍼 함수 정의 (헤더 파일 없이 이곳에 배치)
 * ================================ */
void print_hex_debug(const char *name, const uint8_t *data, size_t len) {
    printf("\n[%s]\n", name);
    for(size_t i = 0; i < len; i++) {
        printf("%02X", data[i]);
    }
    printf("\n");
}

void print_poly_short(const char *name, const poly *p) {
    printf("[%s] ", name);
    printf("(");
    for(int i = 0; i < 16 && i < KYBER_N; i++) {
        printf("%d", p->coeffs[i]);
        if(i != 15) printf(", ");
    }
    printf(", ...)\n");
}

void print_polyvec_debug(const char *name, const polyvec *v) {
    printf("\n--- %s ---\n", name);
    for(int i = 0; i < KYBER_K; i++) {
        char label[128];
        snprintf(label, sizeof(label), "%s[%d]", name, i);
        print_poly_short(label, &v->vec[i]);
    }
}

void print_matrix_debug(const char *name, polyvec a[KYBER_K]) {
    printf("\n--- MATRIX %s ---\n", name);
    for(int i = 0; i < KYBER_K; i++) {
        for(int j = 0; j < KYBER_K; j++) {
            char label[128];
            snprintf(label, sizeof(label), "%s[%d][%d]", name, i, j);
            print_poly_short(label, &a[i].vec[j]);
        }
    }
}

/* ================================
 * 메인 실행부
 * ================================ */
int main(void) {
    uint8_t pk[CRYPTO_PUBLICKEYBYTES];
    uint8_t sk[CRYPTO_SECRETKEYBYTES];
    uint8_t ct[CRYPTO_CIPHERTEXTBYTES];
    uint8_t ss_a[CRYPTO_BYTES];
    uint8_t ss_b[CRYPTO_BYTES];

    printf("\n========================================");
    printf("\n            1. KEY GENERATION           ");
    printf("\n========================================\n");
    crypto_kem_keypair(pk, sk);

    printf("\n========================================");
    printf("\n            2. ENCAPSULATION            ");
    printf("\n========================================\n");
    crypto_kem_enc(ct, ss_b, pk);
    // Alice가 캡슐화를 마치고 만들어낸 최종 암호문 출력
    print_hex_debug("Ciphertext ct (Alice가 생성하여 Bob에게 전송)", ct, KYBER_CIPHERTEXTBYTES);

    printf("\n========================================");
    printf("\n            3. DECAPSULATION            ");
    printf("\n========================================\n");
    // Bob이 탈캡슐화를 시작할 때 수신한 암호문 명시
    print_hex_debug("Received Ciphertext ct (Bob이 수신한 암호문)", ct, KYBER_CIPHERTEXTBYTES);
    crypto_kem_dec(ss_a, ct, sk);

    printf("\n========================================");
    printf("\n            FINAL RESULT                ");
    printf("\n========================================\n");
    print_hex_debug("Alice Shared Secret (ss_a)", ss_a, CRYPTO_BYTES);
    print_hex_debug("Bob Shared Secret (ss_b)", ss_b, CRYPTO_BYTES);
    
    // 전체 32바이트(CRYPTO_BYTES)를 완벽하게 비교
    if(memcmp(ss_a, ss_b, CRYPTO_BYTES) == 0) {
        printf("\n=> SUCCESS: Shared secrets match completely!\n");
    } else {
        printf("\n=> FAIL: Shared secrets do not match!\n");
    }
    return 0;
}