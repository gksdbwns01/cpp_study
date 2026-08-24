#include <stdio.h>
#include <stdlib.h>
#include <math.h> // 수학 함수 (sqrt, log, cos, round 등) 사용을 위한 헤더 포함
#include <time.h>
#include <sys/random.h> // getrandom 함수를 위한 헤더

// --- LWE (Learning With Errors) 암호 파라미터 정의 ---
#define PARAM_N 4 // 비밀키 차원 (n): LWE 문제의 차원 크기
#define PARAM_M 8 // 행렬 행 개수 (m): 주어지는 LWE 샘플의 개수 (보통 m > n * log(q))
#define PARAM_Q 97 // 모듈러스 (q): 연산이 이루어지는 유한체의 크기 (보통 소수 사용)
#define ERROR_SIGMA 1.5 // 가우스 분포 표준편차 (sigma): 에러 분포의 크기를 결정하는 값

// M_PI가 정의되어 있지 않을 경우(일부 컴파일러)를 대비하여 원주율 상수 정의
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// --- 데이터 구조체 정의 ---

// 공개키: A (m x n 행렬), b (m차원 벡터)
typedef struct {
    int A[PARAM_M][PARAM_N]; // Z_q 상에서 무작위로 추출된 행렬 A
    int b[PARAM_M];          // b = A*s + e (mod q) 형태의 결과값 벡터
} PublicKey;

// 비밀키: s (n차원 벡터)
typedef struct {
    int s[PARAM_N];          // Z_q 상에서 무작위로 추출된 비밀 벡터 s
} SecretKey;

// 암호문: c0 (n차원 벡터), c1 (스칼라 값)
typedef struct {
    int c0[PARAM_N];         // c0 = r^T * A (mod q) 
    int c1;                  // c1 = r^T * b + floor(q/2)*message (mod q)
} Ciphertext;


// --- 유틸리티 및 수학 함수 ---

// getrandom 시스템 콜을 이용한 암호학적으로 안전한 랜덤 바이트 추출 함수
// CSPRNG(안전한 난수 생성기)를 통해 예측 불가능한 난수를 buf에 size만큼 채움
void get_random_bytes(unsigned char *buf, size_t size) {
    size_t fetched = 0; // 지금까지 읽어온 바이트 수
    while (fetched < size) {
        // 커널에서 난수를 읽어옴 (flag 0은 기본 설정)
        ssize_t result = getrandom(buf + fetched, size - fetched, 0);
        if (result < 0) {
            // 난수 생성 실패 시 에러 메시지 출력 후 프로그램 종료
            perror("Error calling getrandom");
            exit(EXIT_FAILURE);
        }
        fetched += result; // 읽어온 바이트 수 갱신
    }
}

// 안전한 난수를 이용한 정수 생성 (0 ~ max-1)
// 암호학적 난수를 가져와서 0부터 max-1 사이의 난수를 반환함
int secure_rand_int(int max) {
    unsigned int val;
    // 4바이트(unsigned int 크기)의 안전한 난수를 채움
    get_random_bytes((unsigned char *)&val, sizeof(val));
    return val % max; // max로 나눈 나머지를 반환하여 범위 제한 (엄밀하게는 modulo bias가 존재할 수 있음)
}

// 양의 나머지 연산: a mod q (0 ~ q-1)
// C언어의 '%' 연산자는 음수에 대해 음수 나머지를 반환하므로, 항상 양수가 나오도록 보정
int mod_q(int a) {
    int r = a % PARAM_Q;
    if (r < 0) {
        r += PARAM_Q; // 음수일 경우 q를 더해 양수로 변환
    }
    return r;
}

// [-q/2, q/2] 범위로 변환하는 centered reduction
// 복호화 과정에서 값과 기준점(0 또는 q/2) 사이의 거리를 계산하기 위해 사용
int centered_mod(int a) {
    int r = mod_q(a);        // 먼저 0 ~ q-1 범위로 맵핑
    if (r > PARAM_Q / 2) {
        r -= PARAM_Q;        // q/2보다 크면 q를 빼서 음수 영역으로 이동
    }
    return r;
}

// Box-Muller 변환을 이용한 이산 가우스 샘플러 (CSPRNG 적용)
// 정규(가우스) 분포를 따르는 난수(오차 e)를 생성하기 위한 함수
int sample_discrete_gaussian(double sigma) {
    unsigned int raw1, raw2;
    // 두 개의 안전한 난수 추출
    get_random_bytes((unsigned char *)&raw1, sizeof(raw1));
    get_random_bytes((unsigned char *)&raw2, sizeof(raw2));

    // 난수를 (0, 1) 범위의 균등 분포 실수 u1, u2로 변환
    double u1 = ((double)(raw1 % 1000000) + 1.0) / 1000002.0;
    double u2 = ((double)(raw2 % 1000000) + 1.0) / 1000002.0;

    // Box-Muller 변환 공식 적용: 표준 정규 분포(평균 0, 분산 1) 상의 값 z0 생성
    double z0 = sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);

    // 표준편차 sigma를 곱하여 스케일링한 후 반올림하여 정수(이산)로 반환
    return (int)round(z0 * sigma);
}


// --- LWE Regev 암호화 알고리즘 핵심 로직 ---

// KeyGen: 공개키 pk = (A, b) 및 비밀키 sk = s를 생성하는 함수
void regev_keygen(PublicKey *pk, SecretKey *sk) {
    // 1. 행렬 A 생성 (Z_q에서 균등분포로 무작위 추출)
    for (int i = 0; i < PARAM_M; i++) {
        for (int j = 0; j < PARAM_N; j++) {
            pk->A[i][j] = secure_rand_int(PARAM_Q);
        }
    }

    // 2. 비밀키 s 생성 (Z_q에서 균등분포로 무작위 추출)
    for (int j = 0; j < PARAM_N; j++) {
        sk->s[j] = secure_rand_int(PARAM_Q);
    }

    // 3. 이산 가우스 오류 벡터 e 생성 및 b = A*s + e (mod q) 계산
    printf("생성된 이산 가우스 오류 e: [");
    for (int i = 0; i < PARAM_M; i++) {
        // 평균 0, 표준편차 ERROR_SIGMA인 가우스 분포에서 에러값 e_i 추출
        int e_i = sample_discrete_gaussian(ERROR_SIGMA);
        printf("%d%s", e_i, (i == PARAM_M - 1) ? "" : ", ");

        int row_sum = 0;
        // 내적 연산: 행렬 A의 i번째 행과 벡터 s의 내적 계산 (A_i * s)
        for (int j = 0; j < PARAM_N; j++) {
            row_sum += pk->A[i][j] * sk->s[j];
        }
        // 내적 결과에 에러 e_i를 더하고 mod q 연산을 수행하여 b_i 저장
        pk->b[i] = mod_q(row_sum + e_i);
    }
    printf("]\n");
}

// Encrypt: 공개키 pk를 사용하여 1비트 메시지(0 또는 1)를 암호문 ct = (c0, c1)로 암호화
void regev_encrypt(Ciphertext *ct, const PublicKey *pk, int message_bit) {
    // 1. 랜덤 binary vector r in {0, 1}^m 선택 (각 원소가 0 또는 1인 벡터)
    int r[PARAM_M];
    for (int i = 0; i < PARAM_M; i++) {
        r[i] = secure_rand_int(2);
    }

    // 2. c0 = r^T * A mod q (1 x n 벡터 계산)
    // r과 A의 선형 결합을 통해 암호문의 첫 번째 부분 생성
    for (int j = 0; j < PARAM_N; j++) {
        int sum = 0;
        for (int i = 0; i < PARAM_M; i++) {
            sum += r[i] * pk->A[i][j];
        }
        ct->c0[j] = mod_q(sum);
    }

    // 3. c1 = r^T * b + floor(q/2)*x mod q
    // r과 b의 선형 결합 결과에 메시지 비트를 더해 암호문의 두 번째 부분 생성
    int r_dot_b = 0;
    for (int i = 0; i < PARAM_M; i++) {
        r_dot_b += r[i] * pk->b[i];
    }
    // 메시지 비트가 1이면 q/2를 더하고, 0이면 0을 더함 (q/2로 스케일링)
    int scale = (PARAM_Q / 2) * (message_bit & 1);
    ct->c1 = mod_q(r_dot_b + scale);
}

// Decrypt: 비밀키 sk를 이용해 암호문 ct를 복호화하여 메시지 x'(0 또는 1) 판별
int regev_decrypt(const Ciphertext *ct, const SecretKey *sk) {
    // 1. 암호문 c0와 비밀키 s의 내적(c0 * s) 계산
    int c0_dot_s = 0;
    for (int j = 0; j < PARAM_N; j++) {
        c0_dot_s += ct->c0[j] * sk->s[j];
    }

    // 2. x_prime = c1 - c0*s (mod q)
    // 수식 상 c1 - c0*s는 에러들의 합 + (q/2)*message 에 근사함
    int x_prime = mod_q(ct->c1 - mod_q(c0_dot_s));

    // 3. 0과의 거리 vs floor(q/2)와의 거리 비교
    // x_prime이 0 근처에 있는지, q/2 근처에 있는지 판별하여 원래 메시지를 유추함
    int dist_0 = abs(centered_mod(x_prime));                      // 0과의 최단 거리
    int dist_1 = abs(centered_mod(x_prime - (PARAM_Q / 2)));      // q/2와의 최단 거리

    // 거리가 더 가까운 쪽을 원래 메시지로 판정
    // 0과 더 가까우면 메시지 0, q/2와 더 가까우면 메시지 1을 반환
    return (dist_0 < dist_1) ? 0 : 1;
}

// --- 메인 함수: 테스트 실행 ---
int main(void) {
    PublicKey pk;
    SecretKey sk;
    Ciphertext ct;

    printf("=== Regev Encryption 가우스분포 표준편차 (sigma=%.2f) ===\n\n", ERROR_SIGMA);

    // 키 생성 과정 테스트
    printf("KeyGen 실행\n");
    regev_keygen(&pk, &sk);
    
    // 생성된 비밀키 s 출력
    printf("Secret Key s: [");
    for (int i = 0; i < PARAM_N; i++) {
        printf("%d%s", sk.s[i], (i == PARAM_N - 1) ? "" : ", "); 
    }
    printf("]\n\n");

    // 0과 1 각각에 대해 암호화 및 복호화 테스트 진행
    for (int msg = 0; msg <= 1; msg++) {
        printf("메시지 bit = %d 테스트\n", msg);

        // 메시지 암호화
        regev_encrypt(&ct, &pk, msg);
        printf("암호화 완료 (c1 = %d)\n", ct.c1); 

        // 복호화 과정의 중간값 확인 (디버깅 목적)
        int c0_dot_s = 0;
        for (int j = 0; j < PARAM_N; j++) {
            c0_dot_s += ct.c0[j] * sk.s[j]; 
        }
        int x_prime = mod_q(ct.c1 - mod_q(c0_dot_s));

        // 복호화 수행 및 결과 판별
        int decrypted = regev_decrypt(&ct, &sk);
        printf("복호화 중간값 x' = %d (기준점: 0 vs q/2=%d)\n", x_prime, PARAM_Q / 2);
        
        // 원본 메시지와 복호화된 메시지를 비교하여 성공/실패 출력
        printf("복호화 판별 결과: %d -> %s\n\n",
               decrypted, (decrypted == msg) ? "성공 (PASS)" : "실패 (FAIL)");
    }

    return 0;
}