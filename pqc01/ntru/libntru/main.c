#include <stdio.h>    // 표준 입출력 함수 사용을 위한 C 표준 라이브러리 헤더 (printf 등)
#include <string.h>   // 문자열 처리 함수 사용을 위한 헤더 (strlen, memcmp 등)
#include <stdlib.h>   // 일반 유틸리티 함수 사용을 위한 헤더 (메모리 관리 등)
#include "ntru.h"     // NTRU 양자 내성 암호(PQC) 알고리즘의 핵심 라이브러리 헤더
#include "rand.h"     // NTRU 난수 생성 관련 라이브러리 헤더
#include "poly.h"     // NTRU 다항식 연산 관련 라이브러리 헤더

// 16진수 및 데이터 출력 헬퍼 함수
// 데이터를 보기 좋게 16바이트 단위로 줄바꿈하여 16진수로 출력합니다.
void print_hex(const char *label, const uint8_t *data, size_t len) {
    printf("    %s (%zu 바이트):\n    ", label, len); // 라벨과 데이터 길이 출력
    for (size_t i = 0; i < len; i++) {              // 주어진 길이만큼 반복
        printf("%02X ", data[i]);                   // 1바이트씩 16진수(대문자, 2자리)로 포맷팅하여 출력
        if ((i + 1) % 16 == 0) printf("\n    ");    // 16개를 출력할 때마다 줄바꿈을 통해 가독성 확보
    }
    if (len % 16 != 0) printf("\n");                // 데이터가 16의 배수로 안 끝났을 경우 줄바꿈 추가
    printf("\n");                                   // 항목 구분을 위한 여백 줄바꿈
}

// 다항식 계수를 보기 좋게 일부 출력하는 함수 (학습/분석용)
// NTRU의 핵심인 다항식의 계수들을 최대 max_show 개수만큼 화면에 보여줍니다.
void print_poly_sample(const char *name, const NtruIntPoly *poly, int max_show) {
    printf("    %s 계수 (샘플 0 ~ %d):\n    ", name, max_show - 1); // 다항식 이름과 출력할 샘플 범위 출력
    // 최대 max_show 개수 또는 최대 차수 439 중 작은 값까지만 반복하여 출력
    for (int i = 0; i < max_show && i < 439; i++) {
        printf("%d ", poly->coeffs[i]);             // 각 항의 정수 계수 출력
    }
    printf("...\n\n");                              // 나머지 계수는 생략됨을 표시
}

int main() {
    // 프로그램 시작 안내 문구 출력 (한국어 번역)
    printf("========================================\n");
    printf("       NTRU 암호화 테스트 (EES439EP1)\n");
    printf("========================================\n\n");

    // [0] Parameter 설정
    // 보안 강도와 연산 성능을 결정하는 파라미터 세트 'EES439EP1'를 설정
    NtruEncParams params = EES439EP1; 
    
    printf("[파라미터]\n");
    printf("파라미터 셋 : EES439EP1\n");
    printf("N             : %u\n", params.N); // 다항식의 최고 차수 (N값) 출력
    printf("q             : %u\n", params.q); // 계수의 모듈로 연산 기준값 (q값) 출력

    // 난수 생성기 초기화
    NtruRandContext rand_ctx;              // 난수 생성기 상태를 유지할 컨텍스트 구조체 선언
    NtruRandGen rng = NTRU_RNG_DEFAULT;    // 기본 난수 생성기 설정 (주로 OS 차원의 난수 사용)
    
    // 난수 생성기 초기화 시도 및 에러 처리
    if (ntru_rand_init(&rand_ctx, &rng) != 0) {
        printf(" 오류: 난수 생성기 초기화 실패\n");
        return 1;                          // 초기화 실패 시 에러 코드(1)를 반환하고 프로그램 종료
    }

    // [1] Key Generation (키 쌍 생성)
    printf("\n[1] 키 생성\n");
    NtruEncKeyPair kp;                     // 생성된 공개키와 개인키를 함께 보관할 구조체 선언
    
    // 파라미터와 난수를 기반으로 실제 키 쌍을 생성
    if (ntru_gen_key_pair(&params, &kp, &rand_ctx) != 0) {
        printf("오류: 키 생성 실패\n");
        ntru_rand_release(&rand_ctx);      // 실패 시 사용하던 난수 생성기 자원 해제
        return 1;
    }
    printf("키 생성 성공\n\n");

    // 학습/분석용: 생성된 키 내부 구조 확인
    printf("[심층 분석: NTRU 내부 다항식]\n");
    // 생성된 개인키 구조체 내부의 다항식 t (f(x)와 관련된 다항식)의 계수를 10개만 확인
    print_poly_sample("개인키 다항식 f(x)", &kp.priv.t, 10);

    // [2] Plaintext (평문 준비)
    const char *original_msg = "Hello, NTRU!"; // 암호화할 원본 문자열 데이터
    uint8_t *msg = (uint8_t*)original_msg;                     // 암호화 함수 API에 맞게 uint8_t 포인터로 형변환
    uint16_t msg_len = strlen(original_msg);                   // 암호화할 문자열의 길이(바이트 수) 측정
    
    printf("[2] 평문 (Plaintext)\n");
    printf("텍스트 : %s\n", original_msg);
    // 평문 데이터를 16진수 헥스(Hex) 형태로 출력하여 바이트 단위 확인
    print_hex("16진수", msg, msg_len);

    // [3] Encryption (암호화)
    // 현재 파라미터(EES439EP1) 기준으로 암호문이 차지할 길이 계산
    uint16_t enc_len = ntru_enc_len(&params);
    uint8_t ciphertext[enc_len]; // 계산된 길이만큼 암호문을 저장할 바이트 배열 할당

    printf("[3] 암호화\n");
    // 평문을 공개키(&kp.pub)로 암호화 진행. 이때 난수(&rand_ctx)가 개입하여 매번 암호문이 달라짐
    if (ntru_encrypt(msg, msg_len, &kp.pub, &params, &rand_ctx, ciphertext) != 0) {
        printf("오류: 암호화 실패\n");
        ntru_rand_release(&rand_ctx);
        return 1;
    }
    printf("암호화 성공\n");
    printf("암호문 길이 : %u 바이트\n\n", enc_len); 
    
    // 암호문 전체가 길 수 있으므로, 가독성을 위해 최대 64바이트까지만 16진수로 출력
    print_hex("암호문 (16진수)", ciphertext, enc_len > 64 ? 64 : enc_len); 

    // [4] Decryption (복호화)
    // 현재 파라미터 기준으로 허용되는 최대 평문 길이 계산
    uint16_t max_dec_len = ntru_max_msg_len(&params);
    uint8_t decrypted_msg[max_dec_len + 1]; // 복호화된 문자열을 담을 버퍼 (마지막 NULL 문자를 위해 +1)
    uint16_t decrypted_len = 0;             // 성공적으로 복호화된 데이터의 실제 길이가 저장될 변수

    printf("[4] 복호화\n");
    // 암호문을 개인키(&kp)를 사용하여 본래 평문으로 복호화 진행
    if (ntru_decrypt(ciphertext, &kp, &params, decrypted_msg, &decrypted_len) != 0) {
        printf("오류: 복호화 실패\n");
        ntru_rand_release(&rand_ctx);
        return 1;
    }
    decrypted_msg[decrypted_len] = '\0'; // 정상적인 문자열 출력을 위해 끝에 널(NULL) 종단 문자 추가
    printf("복호화 성공\n\n");

    // [5] Result Comparison (최종 결과 비교 확인)
    printf("[5] 결과 확인\n");
    printf("원본 메시지  : %s\n", original_msg);    // 처음에 입력했던 메시지 출력
    printf("복호화 메시지 : %s\n\n", decrypted_msg); // 복호화를 거친 메시지 출력

    // 복호화된 데이터의 길이와 내용(메모리 단위)이 원본과 완벽히 일치하는지 검증
    if (decrypted_len == msg_len && memcmp(msg, decrypted_msg, msg_len) == 0) {
        printf("성공\n");
        printf("원본 메시지 == 복호화된 메시지\n"); // 둘이 완벽히 같으면 성공 출력
    } else {
        printf("실패\n");
        printf("원본 메시지 != 복호화된 메시지\n"); // 다르면 실패 출력
    }

    printf("========================================\n");

    // 메모리 누수 방지를 위해 사용을 마친 난수 생성기 리소스 해제
    ntru_rand_release(&rand_ctx);
    
    return 0; // 프로그램 정상 종료
}