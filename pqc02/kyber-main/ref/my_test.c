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
    printf("\n            1. 키 생성 (KEY GENERATION)          ");
    printf("\n========================================\n");
    crypto_kem_keypair(pk, sk);

    printf("\n========================================");
    printf("\n            2. Encaps            ");
    printf("\n========================================\n");
    crypto_kem_enc(ct, ss_b, pk);
    // Alice가 캡슐화를 마치고 만들어낸 최종 암호문 출력
    print_hex_debug("Ciphertext ct (Alice가 생성하여 Bob에게 전송)", ct, KYBER_CIPHERTEXTBYTES);

    // ==========================================
    // [심화 시나리오] 의도적 오류 주입 추가
    // ==========================================
    printf("\n네트워크 전송 중 의도적 오류 주입: 암호문 첫 바이트 변조\n");
    ct[0] ^= 0xFF; // 첫 바이트의 비트를 반전시켜 의도적으로 훼손
    // ==========================================

    printf("\n========================================");
    printf("\n            3. Decaps            ");
    printf("\n========================================\n");
    // Bob이 탈캡슐화를 시작할 때 수신한 암호문 명시
    print_hex_debug("Received Ciphertext ct (Bob이 수신한 암호문)", ct, KYBER_CIPHERTEXTBYTES);
    crypto_kem_dec(ss_a, ct, sk);

    printf("\n========================================");
    printf("\n            최종 결과                ");
    printf("\n========================================\n");
    print_hex_debug("Alice의 공유 비밀키 (ss_b)", ss_b, CRYPTO_BYTES);
    print_hex_debug("Bob의 공유 비밀키 (ss_a)", ss_a, CRYPTO_BYTES);

    // 전체 32바이트(CRYPTO_BYTES)를 완벽하게 비교
    if(memcmp(ss_a, ss_b, CRYPTO_BYTES) == 0) {
        printf("\n=> 성공: 공유 비밀키가 일치\n");
    } else {
        printf("\n=> 실패: 공유 비밀키가 일치하지 않음\n");
    }
    return 0;
}