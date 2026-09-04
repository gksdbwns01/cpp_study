#include <stdio.h>
#include <stdint.h>
#include "kem.h"

// 바이트 배열을 16진수로 출력하는 헬퍼 함수
void print_hex(const char *label, const uint8_t *data, size_t len) {
    printf("%s: ", label);
    for (size_t i = 0; i < len; i++) {
        printf("%02X", data[i]);
    }
    printf("\n\n");
}

int main(void) {
    // 카이버 알고리즘에 필요한 배열 변수 선언
    uint8_t pk[CRYPTO_PUBLICKEYBYTES];
    uint8_t sk[CRYPTO_SECRETKEYBYTES];
    uint8_t ct[CRYPTO_CIPHERTEXTBYTES];
    uint8_t ss_a[CRYPTO_BYTES]; // Alice가 복원할 공유 비밀키
    uint8_t ss_b[CRYPTO_BYTES]; // Bob이 생성할 공유 비밀키

    printf("=== 1. 키 생성 (Key Generation - Alice) ===\n");
    // 공개키(pk)와 비밀키(sk) 쌍을 생성
    crypto_kem_keypair(pk, sk);
    // 출력량이 너무 많아지지 않도록 배열의 앞 32바이트만 출력합니다.
    print_hex("공개키 (pk, 첫 32바이트)", pk, 32);
    print_hex("비밀키 (sk, 첫 32바이트)", sk, 32);

    printf("=== 2. 캡슐화 (Encapsulation - Bob) ===\n");
    // Bob이 Alice의 공개키(pk)를 이용해 암호문(ct)과 공유 비밀키(ss_b)를 생성
    crypto_kem_enc(ct, ss_b, pk);
    print_hex("생성된 암호문 (ct, 첫 32바이트)", ct, 32);
    print_hex("Bob이 생성한 공유 비밀키 (ss_b, 32바이트)", ss_b, CRYPTO_BYTES);

    printf("=== 3. 탈캡슐화 (Decapsulation - Alice) ===\n");
    // Alice가 자신의 비밀키(sk)를 이용해 암호문(ct)에서 공유 비밀키(ss_a)를 복원
    crypto_kem_dec(ss_a, ct, sk);
    print_hex("Alice가 복원한 공유 비밀키 (ss_a, 32바이트)", ss_a, CRYPTO_BYTES);

    return 0;
}