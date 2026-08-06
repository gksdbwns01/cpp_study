#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
// AES. 외부 라이브러리 없이 AES의 수학적 과정 구현

// AES-128 기본 설정 (상수 정의)
enum {
    Nb = 4,   // 블록 크기
    Nk = 4,   // 키 크기
    Nr = 10   // 라운드 수
};

#define AES_BLOCK_SIZE 16
#define AES_ROUND_KEY_SIZE (Nb * (Nr + 1) * 4) // 라운드 키 전체 크기

// 전역 상태 변수 및 상수 배열
static uint8_t sbox[256];
static uint8_t rsbox[256];
static bool is_aes_initialized = false; // 1. 초기화 의존성 체크용 플래그

// Rcon: Key Expansion 과정에서 각 라운드마다 사용되는 라운드 상수
static const uint8_t Rcon[11] = {
    0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36
}; // 배열 인덱스를 맞추기 위해 맨 앞(0x8d)은 더미 값

// 갈루아 필드 GF(2^8) 수학 연산 유틸리티

// 입력값에 다항식 x(0x02)를 곱하는 연산
static uint8_t xtime(uint8_t x) {
    return (x << 1) ^ (((x >> 7) & 1) * 0x1b);
}

// GF(2^8)의 다항식 모듈러 곱셈
static uint8_t MultiplyGF(uint8_t x, uint8_t y) {
    uint8_t res = 0;
    uint8_t temp = x;
    for (int i = 0; i < 8; i++) {
        if (y & 1) res ^= temp; // y의 현재 비트가 1이면 결과에 더함(XOR)
        temp = xtime(temp); // 다음 자리수를 위해 x에 'x'를 곱함
        y >>= 1; // y의 다음 비트를 확인하기 위해 우측 시프트
    }
    return res;
}

// 다항식 곱셈 분리 (확장 유클리드 알고리즘에서의 수학적 엄밀성 확보. 기약 다항식 모듈러 없이 곱할 때 사용)
static uint32_t PolynomialMultiply(uint32_t x, uint32_t y) {
    uint32_t res = 0;
    while (y > 0) {
        if (y & 1) res ^= x;
        x <<= 1;
        y >>= 1;
    }
    return res;
}

// 다항식의 차수 반환. 최고차항의 위치를 찾음
static int GetPolyDegree(uint32_t poly) {
    int degree = -1;
    while (poly > 0) {
        degree++;
        poly >>= 1;
    }
    return degree;
}

// 확장 유클리드 알고리즘. 역원 계산
static uint8_t GF_Inverse(uint8_t a) {
    if (a == 0) return 0; // 0의 역원은 0으로 지정

    uint32_t r0 = 0x11B; // 기약 다항식
    uint32_t r1 = a;     
    uint32_t t0 = 0;     
    uint32_t t1 = 1;     

    while (r1 != 1) {
        uint32_t q = 0; // 몫
        uint32_t rem = r0; // 나머지
        int deg_r1 = GetPolyDegree(r1);

        // 다항식 나눗셈 수행
        while (1) {
            int deg_rem = GetPolyDegree(rem);
            if (deg_rem < deg_r1) break;

            int shift = deg_rem - deg_r1;
            q ^= (1 << shift);       
            rem ^= (r1 << shift);    
        }

        // 역원 갱신(XOR)
        uint32_t t2 = t0 ^ PolynomialMultiply(q, t1);

        r0 = r1;
        r1 = rem;
        t0 = t1;
        t1 = t2;
    }
    return (uint8_t)t1;
}

// 아핀 변환(Affine Transformation)
static uint8_t AES_AffineTransformation(const uint8_t x) {
    uint8_t res = 0;
    for (int i = 0; i < 8; i++) {
        uint8_t bit = (x >> i) & 1;
        bit ^= (x >> ((i + 4) % 8)) & 1;
        bit ^= (x >> ((i + 5) % 8)) & 1;
        bit ^= (x >> ((i + 6) % 8)) & 1;
        bit ^= (x >> ((i + 7) % 8)) & 1;
        bit ^= (0x63 >> i) & 1; // 0x63 상수 벡터를 더함 (XOR)

        res |= (bit << i);
    }
    return res;
}

// AES 초기화 및 키 스케줄링 (Key Expansion)

// S-box 생성 의존성 해결을 위한 명시적 초기화 함수
void AES_Init() {
    if (is_aes_initialized) return; // 이미 초기화되었다면 스킵

    for (int i = 0; i < 256; i++) {
        uint8_t inv = GF_Inverse((uint8_t)i);
        sbox[i] = AES_AffineTransformation(inv);
        rsbox[sbox[i]] = i;
    }
    is_aes_initialized = true;
}

// Word 단위로 S-box 치환을 수행 (Key Expansion에 사용)
static void AES_SubWord(uint8_t* word) {
    word[0] = sbox[word[0]];
    word[1] = sbox[word[1]];
    word[2] = sbox[word[2]];
    word[3] = sbox[word[3]];
}

// Word 단위를 왼쪽으로 1바이트 순환 시프트 (Key Expansion에 사용)
static void AES_RotWord(uint8_t* word) {
    uint8_t temp = word[0];
    word[0] = word[1];
    word[1] = word[2];
    word[2] = word[3];
    word[3] = temp;
}

// 16바이트 원본 키를 입력받아 확장된 라운드 키 생성
void AES_KeyExpansion(const uint8_t* key, uint8_t* RoundKey) {
    if (!is_aes_initialized) {
        AES_Init(); // S-box가 없으면 강제 초기화
    }

    uint32_t i = 0;
    // 첫 번째 라운드 키는 원본 키(16바이트) 그대로 사용
    while (i < Nk) {
        RoundKey[i * 4 + 0] = key[i * 4 + 0];
        RoundKey[i * 4 + 1] = key[i * 4 + 1];
        RoundKey[i * 4 + 2] = key[i * 4 + 2];
        RoundKey[i * 4 + 3] = key[i * 4 + 3];
        i++;
    }

    // 이후의 라운드 키 생성 로직
    i = Nk;
    while (i < Nb * (Nr + 1)) {
        uint8_t temp[4];
        // 직전 4바이트(1 Word)를 가져옴
        temp[0] = RoundKey[(i - 1) * 4 + 0];
        temp[1] = RoundKey[(i - 1) * 4 + 1];
        temp[2] = RoundKey[(i - 1) * 4 + 2];
        temp[3] = RoundKey[(i - 1) * 4 + 3];

        // 키의 주기(Nk=4)마다 비선형 변환 수행
        if (i % Nk == 0) {
            AES_RotWord(temp);
            AES_SubWord(temp);
            temp[0] ^= Rcon[i / Nk]; // 라운드 상수 더하기
        }

        // 현재 Word = (Nk 단위 이전의 Word) XOR (변환된 temp Word)
        RoundKey[i * 4 + 0] = RoundKey[(i - Nk) * 4 + 0] ^ temp[0];
        RoundKey[i * 4 + 1] = RoundKey[(i - Nk) * 4 + 1] ^ temp[1];
        RoundKey[i * 4 + 2] = RoundKey[(i - Nk) * 4 + 2] ^ temp[2];
        RoundKey[i * 4 + 3] = RoundKey[(i - Nk) * 4 + 3] ^ temp[3];
        i++;
    }
}

// AES 상태 배열 조작 및 4가지 핵심 변환 (라운드 함수)

// 1차원 배열을 4x4 상태 행렬로 변환. 열우선 배치
static void LoadState(uint8_t state[4][4], const uint8_t* input) {
    for (int r = 0; r < Nb; ++r) {
        for (int c = 0; c < Nb; ++c) {
            state[r][c] = input[r + 4 * c];
        }
    }
}

// 4x4 상태 행렬을 다시 1차원 배열로 변환
static void StoreState(uint8_t* output, const uint8_t state[4][4]) {
    for (int r = 0; r < Nb; ++r) {
        for (int c = 0; c < Nb; ++c) {
            output[r + 4 * c] = state[r][c];
        }
    }
}

// 평문과 라운드 키를 비트 단위로 XOR
static void AES_AddRoundKey(uint8_t state[4][4], const uint8_t* RoundKey, int round) {
    for (int c = 0; c < Nb; ++c) {
        for (int r = 0; r < Nb; ++r) {
            state[r][c] ^= RoundKey[round * Nb * 4 + c * 4 + r];
        }
    }
}

// 각 바이트를 S-box를 이용하여 치환
static void AES_SubBytes(uint8_t state[4][4]) {
    for (int r = 0; r < Nb; ++r) {
        for (int c = 0; c < Nb; ++c) {
            state[r][c] = sbox[state[r][c]];
        }
    }
}

// 복호화용 역방향 S-box 치환
static void AES_InvSubBytes(uint8_t state[4][4]) {
    for (int r = 0; r < Nb; ++r) {
        for (int c = 0; c < Nb; ++c) {
            state[r][c] = rsbox[state[r][c]];
        }
    }
}

// 4x4 상태 행렬의 각 행을 왼쪽으로 시프트, ppt에서 보기 편하도록
static void AES_ShiftRows(uint8_t state[4][4]) {
    uint8_t temp;
    // 첫 번째 행(state[0])은 회전하지 않으므로 생략
    temp = state[1][0];
    state[1][0] = state[1][1];
    state[1][1] = state[1][2];
    state[1][2] = state[1][3];
    state[1][3] = temp;

    temp = state[2][0];
    state[2][0] = state[2][2];
    state[2][2] = temp;
    temp = state[2][1];
    state[2][1] = state[2][3];
    state[2][3] = temp;

    temp = state[3][3];
    state[3][3] = state[3][2];
    state[3][2] = state[3][1];
    state[3][1] = state[3][0];
    state[3][0] = temp;
}

// 복호화용 오른쪽 시프트
static void AES_InvShiftRows(uint8_t state[4][4]) {
    uint8_t temp;
    temp = state[1][3]; state[1][3] = state[1][2]; state[1][2] = state[1][1]; state[1][1] = state[1][0]; state[1][0] = temp;
    temp = state[2][0]; state[2][0] = state[2][2]; state[2][2] = temp;
    temp = state[2][1]; state[2][1] = state[2][3]; state[2][3] = temp;
    temp = state[3][0]; state[3][0] = state[3][1]; state[3][1] = state[3][2]; state[3][2] = state[3][3]; state[3][3] = temp;
}

// 특정 4x4 행렬과 상태 행렬의 열 벡터 곱
static void AES_MixColumns(uint8_t state[4][4]) { 
    for (int c = 0; c < Nb; ++c) {
        uint8_t a0 = state[0][c];
        uint8_t a1 = state[1][c];
        uint8_t a2 = state[2][c];
        uint8_t a3 = state[3][c];
        state[0][c] = xtime(a0) ^ (xtime(a1) ^ a1) ^ a2 ^ a3;
        state[1][c] = a0 ^ xtime(a1) ^ (xtime(a2) ^ a2) ^ a3;
        state[2][c] = a0 ^ a1 ^ xtime(a2) ^ (xtime(a3) ^ a3);
        state[3][c] = (xtime(a0) ^ a0) ^ a1 ^ a2 ^ xtime(a3);
    }
}

// 복호화용 역방향 MixColumns
static void AES_InvMixColumns(uint8_t state[4][4]) {
    for (int c = 0; c < Nb; ++c) {
        uint8_t a0 = state[0][c];
        uint8_t a1 = state[1][c];
        uint8_t a2 = state[2][c];
        uint8_t a3 = state[3][c];
        state[0][c] = MultiplyGF(0x0e, a0) ^ MultiplyGF(0x0b, a1) ^ MultiplyGF(0x0d, a2) ^ MultiplyGF(0x09, a3);
        state[1][c] = MultiplyGF(0x09, a0) ^ MultiplyGF(0x0e, a1) ^ MultiplyGF(0x0b, a2) ^ MultiplyGF(0x0d, a3);
        state[2][c] = MultiplyGF(0x0d, a0) ^ MultiplyGF(0x09, a1) ^ MultiplyGF(0x0e, a2) ^ MultiplyGF(0x0b, a3);
        state[3][c] = MultiplyGF(0x0b, a0) ^ MultiplyGF(0x0d, a1) ^ MultiplyGF(0x09, a2) ^ MultiplyGF(0x0e, a3);
    }
}

// AES 메인 함수

// 평문을 입력받아 암호문으로 변환
void AES_Cipher(const uint8_t* input, const uint8_t* RoundKey, uint8_t* output) {
    uint8_t state[4][4];
    LoadState(state, input); // 1차원 -> 4x4 State
    AES_AddRoundKey(state, RoundKey, 0);

    // 라운드 1~9
    for (int round = 1; round < Nr; ++round) {
        AES_SubBytes(state);
        AES_ShiftRows(state);
        AES_MixColumns(state);
        AES_AddRoundKey(state, RoundKey, round);
    }

    // 라운드 10 (MixColumns 생략)
    AES_SubBytes(state);
    AES_ShiftRows(state);
    AES_AddRoundKey(state, RoundKey, Nr);
    StoreState(output, state); // 4x4 State -> 1차원
}

// 암호문을 원본 평문으로 변환 - 암호화의 역순
void AES_InvCipher(const uint8_t* input, const uint8_t* RoundKey, uint8_t* output) {
    uint8_t state[4][4];
    LoadState(state, input);
    AES_AddRoundKey(state, RoundKey, Nr);

    for (int round = Nr - 1; round > 0; --round) {
        AES_InvShiftRows(state);
        AES_InvSubBytes(state);
        AES_AddRoundKey(state, RoundKey, round);
        AES_InvMixColumns(state);
    }

    AES_InvShiftRows(state);
    AES_InvSubBytes(state);
    AES_AddRoundKey(state, RoundKey, 0);
    StoreState(output, state);
}

// 테스트 및 유틸리티

// 16진수 배열을 공백 문자로 띄워서 출력
void PrintHex(const char* label, const uint8_t* data, int length) {
    printf("%-12s: ", label);
    for (int i = 0; i < length; i++) {
        printf("%02X ", data[i]);
    }
    printf("\n");
}

int main() {
    // 내부에서 호출하므로 필수 호출은 아니지만, 명시적 초기화도 가능하게 구성
    AES_Init(); 

    printf("=========================================================\n");
    printf("                    AES-128 테스트\n");
    printf("=========================================================\n");

    // 테스트 벡터: 평문(Plaintext) 16바이트
    uint8_t plaintext[AES_BLOCK_SIZE] = {
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff
    };

    // 테스트 벡터: 암호화 키(Key) 16바이트
    uint8_t key[AES_BLOCK_SIZE] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
    };

    
    uint8_t ciphertext[AES_BLOCK_SIZE];    
    uint8_t decryptedtext[AES_BLOCK_SIZE]; 
    uint8_t RoundKey[AES_ROUND_KEY_SIZE]; // 라운드 키 저장소

    // 입력 키를 통해 각 라운드에 사용할 라운드 키 확장
    AES_KeyExpansion(key, RoundKey);

    PrintHex("Plaintext", plaintext, AES_BLOCK_SIZE);
    PrintHex("Key", key, AES_BLOCK_SIZE);
    printf("---------------------------------------------------------\n");

    // 3. 암호화 수행
    AES_Cipher(plaintext, RoundKey, ciphertext);
    PrintHex("Ciphertext", ciphertext, AES_BLOCK_SIZE);
    
    // 암호화 자동 검증 추가 - NIST에서 제공한 공식 AES 테스트 정답값과 비교
    const uint8_t expected_ct[AES_BLOCK_SIZE] = {
        0x69, 0xC4, 0xE0, 0xD8, 0x6A, 0x7B, 0x04, 0x30,
        0xD8, 0xCD, 0xB7, 0x80, 0x70, 0xB4, 0xC5, 0x5A
    };    
    PrintHex("Expected CT", expected_ct, AES_BLOCK_SIZE);

    if (memcmp(ciphertext, expected_ct, AES_BLOCK_SIZE) == 0) {
        printf("[SUCCESS]   암호화된 데이터가 예상 값과 일치합니다!\n");
    } else {
        printf("[FAIL]      암호화 데이터가 예상 값과 다릅니다.\n");
    }
    printf("---------------------------------------------------------\n");

    // 복호화 수행, 원본 평문 복원 체크
    AES_InvCipher(ciphertext, RoundKey, decryptedtext);
    PrintHex("Decrypted", decryptedtext, AES_BLOCK_SIZE);
    
    if (memcmp(plaintext, decryptedtext, AES_BLOCK_SIZE) == 0) {
        printf("[SUCCESS]   복호화된 데이터가 원본 평문과 일치합니다!\n");
    } else {
        printf("[FAIL]      복호화 데이터가 원본과 다릅니다.\n");
    }
    printf("=========================================================\n");

    return 0;
}