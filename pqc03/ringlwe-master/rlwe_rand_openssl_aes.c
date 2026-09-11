#include <sys/random.h>
#include <stdint.h>
#include <stdlib.h>

/* 
 * 1. 상태 변수 제거
 * 원래 OpenSSL AES 컨텍스트를 초기화하던 매크로입니다.
 * 리눅스 getrandom()은 커널 레벨에서 상태를 관리하므로 앱 단의 상태 변수가 필요 없습니다.
 * 따라서 에러가 나지 않도록 빈 매크로로 둡니다.
 */
#define RANDOM_VARS 

/* 
 * 2. 난수 생성 함수 (OpenSSL 대체)
 */
static inline uint64_t randomplease_linux(void) {
    uint64_t out = 0;
    // 리눅스 커널의 CSPRNG에서 직접 8바이트(64비트)를 가져옴
    if (getrandom(&out, sizeof(out), 0) < 0) {
        exit(1);
    }
    return out;
}

/* 
 * 3. 매크로 덮어쓰기
 * rlwe_kex.c에서 호출하는 매크로들이 기존의 복잡한 인자 대신 
 * 새로 만든 randomplease_linux()를 호출하도록 연결합니다.
 */
#define RANDOM8  ((uint8_t) randomplease_linux())
#define RANDOM32 ((uint32_t) randomplease_linux())
#define RANDOM64 ((uint64_t) randomplease_linux())