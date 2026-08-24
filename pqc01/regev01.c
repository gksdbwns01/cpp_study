#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define PARAM_N 4    // 비밀키 차원 (n)
#define PARAM_M 8    // 행렬 행 개수 (m)
#define PARAM_Q 97   // 모듈러스 (q)

// 공개키: A (m x n 행렬), b (m차원 벡터)
typedef struct {
    int A[PARAM_M][PARAM_N];
    int b[PARAM_M];
} PublicKey;

// 비밀키: s (n차원 벡터)
typedef struct {
    int s[PARAM_N];
} SecretKey;

// 암호문: c0 (n차원 벡터), c1 (스칼라 값)
typedef struct {
    int c0[PARAM_N];
    int c1;
} Ciphertext;

// 양의 나머지 연산: a mod q (결과는 0 ~ q-1)
int mod_q(int a) {
    int r = a % PARAM_Q;
    if (r < 0) {
        r += PARAM_Q;
    }
    return r;
}

// [-q/2, q/2] 범위로 변환하는 centered reduction
int centered_mod(int a) {
    int r = mod_q(a);
    if (r > PARAM_Q / 2) {
        r -= PARAM_Q;
    }
    return r;
}

// KeyGen: pk = (A, b), sk = s
void regev_keygen(PublicKey *pk, SecretKey *sk) {
    // 1. 행렬 A 생성 (0 ~ q-1 균등분포)
    for (int i = 0; i < PARAM_M; i++) {
        for (int j = 0; j < PARAM_N; j++) {
            pk->A[i][j] = rand() % PARAM_Q;
        }
    }

    // 2. 비밀키 s 생성 (0 ~ q-1 균등분포)
    for (int j = 0; j < PARAM_N; j++) {
        sk->s[j] = rand() % PARAM_Q;
    }

    // 3. 작은 오류 벡터 e 생성 {-1, 0, 1} 및 b = A*s + e mod q 계산
    for (int i = 0; i < PARAM_M; i++) {
        int e_i = (rand() % 3) - 1; // -1, 0, 1 중 선택
        int row_sum = 0;
        for (int j = 0; j < PARAM_N; j++) {
            row_sum += pk->A[i][j] * sk->s[j];
        }
        pk->b[i] = mod_q(row_sum + e_i);
    }
}

// Encrypt: ct = (c0, c1)
void regev_encrypt(Ciphertext *ct, const PublicKey *pk, int message_bit) {
    // 1. 랜덤 binary vector r in {0, 1}^m 선택
    int r[PARAM_M];
    for (int i = 0; i < PARAM_M; i++) {
        r[i] = rand() % 2;
    }

    // 2. c0 = r^T * A mod q (1 x n 벡터)
    for (int j = 0; j < PARAM_N; j++) {
        int sum = 0;
        for (int i = 0; i < PARAM_M; i++) {
            sum += r[i] * pk->A[i][j];
        }
        ct->c0[j] = mod_q(sum);
    }

    // 3. c1 = r^T * b + floor(q/2)*x mod q
    int r_dot_b = 0;
    for (int i = 0; i < PARAM_M; i++) {
        r_dot_b += r[i] * pk->b[i];
    }
    int scale = (PARAM_Q / 2) * (message_bit & 1);
    ct->c1 = mod_q(r_dot_b + scale);
}

// Decrypt: x' = c1 - c0*s mod q -> 0 또는 1 판별
int regev_decrypt(const Ciphertext *ct, const SecretKey *sk) {
    // 1. c0 * s 계산
    int c0_dot_s = 0;
    for (int j = 0; j < PARAM_N; j++) {
        c0_dot_s += ct->c0[j] * sk->s[j];
    }

    // 2. x_prime = c1 - c0*s (mod q)
    int x_prime = mod_q(ct->c1 - mod_q(c0_dot_s));

    // 3. 0과의 거리 vs floor(q/2)와의 거리 비교
    int dist_0 = abs(centered_mod(x_prime));
    int dist_1 = abs(centered_mod(x_prime - (PARAM_Q / 2)));

    return (dist_0 < dist_1) ? 0 : 1;
}

int main(void) {
    srand((unsigned int)time(NULL));

    PublicKey pk;
    SecretKey sk;
    Ciphertext ct;

    printf("=== Regev Encryption Demo (q=%d, n=%d, m=%d) ===\n\n", PARAM_Q, PARAM_N, PARAM_M);

    // 1. 키 생성
    regev_keygen(&pk, &sk);
    printf("[1] KeyGen 완료\n");
    printf("  - Secret Key s: [");
    for (int i = 0; i < PARAM_N; i++) {
        printf("%d%s", sk.s[i], (i == PARAM_N - 1) ? "" : ", ");
    }
    printf("]\n\n");

    // 2. 비트 0과 1 각각 테스트
    for (int msg = 0; msg <= 1; msg++) {
        printf("[2] 메시지 bit = %d 테스트\n", msg);

        regev_encrypt(&ct, &pk, msg);
        printf("  - 암호화 완료 (c1 = %d)\n", ct.c1);

        // x_prime 내부 값 확인용 계산
        int c0_dot_s = 0;
        for (int j = 0; j < PARAM_N; j++) {
            c0_dot_s += ct.c0[j] * sk.s[j];
        }
        int x_prime = mod_q(ct.c1 - mod_q(c0_dot_s));

        int decrypted = regev_decrypt(&ct, &sk);
        printf("  - 복호화 중간값 x' = %d (기준점: 0 vs q/2=%d)\n", x_prime, PARAM_Q / 2);
        printf("  - 복호화 판별 결과: %d -> %s\n\n",
               decrypted, (decrypted == msg) ? "성공 (PASS)" : "실패 (FAIL)");
    }

    return 0;
}