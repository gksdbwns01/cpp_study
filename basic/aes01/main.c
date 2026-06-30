/**
 * ============================================================================
 * [AES-128 FIPS-197 Standard Implementation - Refactored]
 * - 파일명: aes.c
 * - 특징: 외부 라이브러리 없이 AES의 모든 수학적 과정을 직접 구현
 * - 개선: 매직 넘버 제거 및 State 로드/스토어 로직 모듈화 적용
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define Nb 4   // 블록 크기 (Word 수, 128bit = 4 words)
#define Nk 4   // 키 크기 (Word 수, 128bit = 4 words)
#define Nr 10  // 라운드 수 (AES-128은 10 라운드)

 // 매직 넘버 제거를 위한 매크로 정의
#define AES_BLOCK_SIZE 16
#define AES_ROUND_KEY_SIZE (Nb * (Nr + 1) * 4)

// ============================================================================
// 1. AES 상수 (Constants)
// ============================================================================

// S-box: 바이트 치환 테이블 (비선형성 제공)
static const uint8_t sbox[256] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
};

// Inverse S-box: 복호화 시 사용
static const uint8_t rsbox[256] = {
    0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
    0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,
    0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
    0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
    0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,
    0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
    0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,
    0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,
    0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
    0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,
    0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,
    0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
    0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
    0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,
    0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
    0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d
};

// Rcon: 키 확장 시 사용되는 라운드 상수
static const uint8_t Rcon[11] = {
    0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36
};

// ============================================================================
// 2. GF(2^8) 수학 연산 (Galois Field Multiplication)
// ============================================================================

uint8_t xtime(uint8_t x) {
    return (x << 1) ^ (((x >> 7) & 1) * 0x1b);
}

uint8_t Multiply(uint8_t x, uint8_t y) {
    uint8_t res = 0;
    uint8_t temp = x;
    for (int i = 0; i < 8; i++) {
        if (y & 1) res ^= temp;
        temp = xtime(temp);
        y >>= 1;
    }
    return res;
}

// ============================================================================
// 3. 배열 변환 유틸리티 (State <-> 1D Array) 모듈화 적용
// ============================================================================

// 1차원 배열을 4x4 State 행렬로 로드 (Column-Major)
void LoadState(uint8_t state[4][4], const uint8_t* input) {
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            state[r][c] = input[r + 4 * c];
        }
    }
}

// 4x4 State 행렬을 1차원 배열로 저장 (Column-Major)
void StoreState(uint8_t* output, const uint8_t state[4][4]) {
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            output[r + 4 * c] = state[r][c];
        }
    }
}


// ============================================================================
// 4. 키 확장 (Key Expansion) 관련 함수
// ============================================================================

void SubWord(uint8_t* word) {
    word[0] = sbox[word[0]];
    word[1] = sbox[word[1]];
    word[2] = sbox[word[2]];
    word[3] = sbox[word[3]];
}

void RotWord(uint8_t* word) {
    uint8_t temp = word[0];
    word[0] = word[1];
    word[1] = word[2];
    word[2] = word[3];
    word[3] = temp;
}

void KeyExpansion(const uint8_t* key, uint8_t* RoundKey) {
    uint32_t i = 0;

    while (i < Nk) {
        RoundKey[i * 4 + 0] = key[i * 4 + 0];
        RoundKey[i * 4 + 1] = key[i * 4 + 1];
        RoundKey[i * 4 + 2] = key[i * 4 + 2];
        RoundKey[i * 4 + 3] = key[i * 4 + 3];
        i++;
    }

    i = Nk;
    while (i < Nb * (Nr + 1)) {
        uint8_t temp[4];
        temp[0] = RoundKey[(i - 1) * 4 + 0];
        temp[1] = RoundKey[(i - 1) * 4 + 1];
        temp[2] = RoundKey[(i - 1) * 4 + 2];
        temp[3] = RoundKey[(i - 1) * 4 + 3];

        if (i % Nk == 0) {
            RotWord(temp);
            SubWord(temp);
            temp[0] ^= Rcon[i / Nk];
        }

        RoundKey[i * 4 + 0] = RoundKey[(i - Nk) * 4 + 0] ^ temp[0];
        RoundKey[i * 4 + 1] = RoundKey[(i - Nk) * 4 + 1] ^ temp[1];
        RoundKey[i * 4 + 2] = RoundKey[(i - Nk) * 4 + 2] ^ temp[2];
        RoundKey[i * 4 + 3] = RoundKey[(i - Nk) * 4 + 3] ^ temp[3];
        i++;
    }
}

// ============================================================================
// 5. AES 라운드 내부 연산 (SubBytes, ShiftRows, MixColumns, AddRoundKey)
// ============================================================================

void AddRoundKey(uint8_t state[4][4], const uint8_t* RoundKey, int round) {
    for (int c = 0; c < 4; ++c) {
        for (int r = 0; r < 4; ++r) {
            state[r][c] ^= RoundKey[round * Nb * 4 + c * 4 + r];
        }
    }
}

void SubBytes(uint8_t state[4][4]) {
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            state[r][c] = sbox[state[r][c]];
        }
    }
}

void InvSubBytes(uint8_t state[4][4]) {
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            state[r][c] = rsbox[state[r][c]];
        }
    }
}

void ShiftRows(uint8_t state[4][4]) {
    uint8_t temp;

    temp = state[1][0]; state[1][0] = state[1][1]; state[1][1] = state[1][2]; state[1][2] = state[1][3]; state[1][3] = temp;

    temp = state[2][0]; state[2][0] = state[2][2]; state[2][2] = temp;
    temp = state[2][1]; state[2][1] = state[2][3]; state[2][3] = temp;

    temp = state[3][3]; state[3][3] = state[3][2]; state[3][2] = state[3][1]; state[3][1] = state[3][0]; state[3][0] = temp;
}

void InvShiftRows(uint8_t state[4][4]) {
    uint8_t temp;

    temp = state[1][3]; state[1][3] = state[1][2]; state[1][2] = state[1][1]; state[1][1] = state[1][0]; state[1][0] = temp;

    temp = state[2][0]; state[2][0] = state[2][2]; state[2][2] = temp;
    temp = state[2][1]; state[2][1] = state[2][3]; state[2][3] = temp;

    temp = state[3][0]; state[3][0] = state[3][1]; state[3][1] = state[3][2]; state[3][2] = state[3][3]; state[3][3] = temp;
}

void MixColumns(uint8_t state[4][4]) {
    for (int c = 0; c < 4; ++c) {
        uint8_t a0 = state[0][c];
        uint8_t a1 = state[1][c];
        uint8_t a2 = state[2][c];
        uint8_t a3 = state[3][c];

        state[0][c] = Multiply(0x02, a0) ^ Multiply(0x03, a1) ^ a2 ^ a3;
        state[1][c] = a0 ^ Multiply(0x02, a1) ^ Multiply(0x03, a2) ^ a3;
        state[2][c] = a0 ^ a1 ^ Multiply(0x02, a2) ^ Multiply(0x03, a3);
        state[3][c] = Multiply(0x03, a0) ^ a1 ^ a2 ^ Multiply(0x02, a3);
    }
}

void InvMixColumns(uint8_t state[4][4]) {
    for (int c = 0; c < 4; ++c) {
        uint8_t a0 = state[0][c];
        uint8_t a1 = state[1][c];
        uint8_t a2 = state[2][c];
        uint8_t a3 = state[3][c];

        state[0][c] = Multiply(0x0e, a0) ^ Multiply(0x0b, a1) ^ Multiply(0x0d, a2) ^ Multiply(0x09, a3);
        state[1][c] = Multiply(0x09, a0) ^ Multiply(0x0e, a1) ^ Multiply(0x0b, a2) ^ Multiply(0x0d, a3);
        state[2][c] = Multiply(0x0d, a0) ^ Multiply(0x09, a1) ^ Multiply(0x0e, a2) ^ Multiply(0x0b, a3);
        state[3][c] = Multiply(0x0b, a0) ^ Multiply(0x0d, a1) ^ Multiply(0x09, a2) ^ Multiply(0x0e, a3);
    }
}

// ============================================================================
// 6. 메인 암호화 및 복호화 함수 (Cipher & InvCipher)
// ============================================================================

void Cipher(const uint8_t* input, const uint8_t* RoundKey, uint8_t* output) {
    uint8_t state[4][4];

    // 개선: 모듈화된 LoadState 함수 사용
    LoadState(state, input);

    AddRoundKey(state, RoundKey, 0);

    for (int round = 1; round < Nr; ++round) {
        SubBytes(state);
        ShiftRows(state);
        MixColumns(state);
        AddRoundKey(state, RoundKey, round);
    }

    SubBytes(state);
    ShiftRows(state);
    AddRoundKey(state, RoundKey, Nr);

    // 개선: 모듈화된 StoreState 함수 사용
    StoreState(output, state);
}

void InvCipher(const uint8_t* input, const uint8_t* RoundKey, uint8_t* output) {
    uint8_t state[4][4];

    // 개선: 모듈화된 LoadState 함수 사용
    LoadState(state, input);

    AddRoundKey(state, RoundKey, Nr);

    for (int round = Nr - 1; round > 0; --round) {
        InvShiftRows(state);
        InvSubBytes(state);
        AddRoundKey(state, RoundKey, round);
        InvMixColumns(state);
    }

    InvShiftRows(state);
    InvSubBytes(state);
    AddRoundKey(state, RoundKey, 0);

    // 개선: 모듈화된 StoreState 함수 사용
    StoreState(output, state);
}

// ============================================================================
// 7. 테스트 (main 함수)
// ============================================================================

void PrintHex(const char* label, const uint8_t* data, int length) {
    printf("%-12s: ", label);
    for (int i = 0; i < length; i++) {
        printf("%02X", data[i]);
    }
    printf("\n");
}

int main() {
    printf("==========================================\n");
    printf("     AES-128 클린 코드 리팩토링 테스트\n");
    printf("==========================================\n");

    // 매크로(AES_BLOCK_SIZE) 적용
    uint8_t plaintext[AES_BLOCK_SIZE] = {
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff
    };

    uint8_t key[AES_BLOCK_SIZE] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
    };

    uint8_t ciphertext[AES_BLOCK_SIZE];
    uint8_t decryptedtext[AES_BLOCK_SIZE];

    // 매크로(AES_ROUND_KEY_SIZE) 적용
    uint8_t RoundKey[AES_ROUND_KEY_SIZE];

    KeyExpansion(key, RoundKey);

    PrintHex("Plaintext", plaintext, AES_BLOCK_SIZE);
    PrintHex("Key", key, AES_BLOCK_SIZE);
    printf("------------------------------------------\n");

    Cipher(plaintext, RoundKey, ciphertext);
    PrintHex("Ciphertext", ciphertext, AES_BLOCK_SIZE);
    printf("(Expected CT : 69C4E0D86A7B0430D8CDB78070B4C55A)\n");
    printf("------------------------------------------\n");

    InvCipher(ciphertext, RoundKey, decryptedtext);
    PrintHex("Decrypted", decryptedtext, AES_BLOCK_SIZE);

    printf("==========================================\n");

    if (memcmp(plaintext, decryptedtext, AES_BLOCK_SIZE) == 0) {
        printf("[SUCCESS] 복호화된 데이터가 원본과 일치합니다!\n");
    }
    else {
        printf("[FAIL] 복호화 데이터가 원본과 다릅니다.\n");
    }

    return 0;
}