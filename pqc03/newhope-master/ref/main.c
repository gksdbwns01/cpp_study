#include <stdio.h>
#include <string.h>
#include "api.h"

int main() {
    unsigned char pk[CRYPTO_PUBLICKEYBYTES];
    unsigned char sk[CRYPTO_SECRETKEYBYTES];
    unsigned char ct[CRYPTO_CIPHERTEXTBYTES];
    unsigned char ss_a[CRYPTO_BYTES]; // Alice의 최종 공유키
    unsigned char ss_b[CRYPTO_BYTES]; // Bob의 최종 공유키

    printf("\n=========================================\n");
    printf("   NewHope1024 테스트\n");
    printf("=========================================\n");

    // 1. Alice: 공개키(pk)와 비밀키(sk) 생성
    crypto_kem_keypair(pk, sk);

    // 2. Bob: 암호문(ct)과 자신의 공유 비밀키(ss_b) 생성
    crypto_kem_enc(ct, ss_b, pk);

    // [옵션] PDF 15페이지의 '의도적 오류 주입'을 테스트하려면 아래 주석을 해제하세요.
    // printf("\n[!] 네트워크 전송 중 암호문 첫 바이트 변조\n");
    // ct[0] = 0xFF; 

    // 3. Alice: 암호문을 받아 복호화하고 공유 비밀키(ss_a) 복원
    crypto_kem_dec(ss_a, ct, sk);

    printf("\n[최종 공유 비밀키 확인]\n");
    printf("Bob의 공유 비밀키   (ss_b): ");
    for(int i = 0; i < CRYPTO_BYTES; i++) {
        printf("%02X", ss_b[i]);
    }
    printf("\n");

    printf("Alice의 공유 비밀키 (ss_a): ");
    for(int i = 0; i < CRYPTO_BYTES; i++) {
        printf("%02X", ss_a[i]);
    }
    printf("\n");
    // 4. 결과 확인 (두 키가 일치하는지 비교)
    printf("\n=========================================\n");
    if(memcmp(ss_a, ss_b, CRYPTO_BYTES) == 0) {
        printf("=> 성공: Alice와 Bob의 공유 비밀키가 일치\n");
    } else {
        printf("=> 실패: 공유 비밀키가 일치하지 않음\n");
    }
    printf("=========================================\n");

    return 0;
}