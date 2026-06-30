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
// 1. AES 상수 (Constants) - 하드코딩 제거 및 빈 배열 선언
// ============================================================================

uint8_t sbox[256];
uint8_t rsbox[256];

static const uint8_t Rcon[11] = {
    0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36
};

// ============================================================================
// 2. GF(2^8) 수학 연산 및 S-box 동적 생성 (추가된 부분)
// ============================================================================

// (기존에 있던 xtime과 Multiply 함수는 그대로 유지)
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

// 2-1. GF(2^8) 상에서의 곱셈 역원 구하기
uint8_t GF_Inverse(uint8_t a) {
    // 0의 역원은 0으로 정의됨
    if (a == 0) return 0;

    // 유한체 내에서 곱했을 때 1이 되는 값(역원)을 브루트포스로 탐색
    for (int i = 1; i < 256; i++) {
        if (Multiply(a, (uint8_t)i) == 1) return i;
    }
    return 0;
}

// 2-2. 아핀 변환 (Affine Transformation)
uint8_t Affine_Transformation(uint8_t x) {
    uint8_t res = 0;
    // PDF 공식: b'_i = b_i ^ b_(i+4) ^ b_(i+5) ^ b_(i+6) ^ b_(i+7) ^ c_i
    for (int i = 0; i < 8; i++) {
        uint8_t bit = (x >> i) & 1;
        bit ^= (x >> ((i + 4) % 8)) & 1;
        bit ^= (x >> ((i + 5) % 8)) & 1;
        bit ^= (x >> ((i + 6) % 8)) & 1;
        bit ^= (x >> ((i + 7) % 8)) & 1;
        bit ^= (0x63 >> i) & 1; // AES 상수 c = 0x63

        res |= (bit << i);
    }
    return res;
}

// 2-3. S-box 및 Inverse S-box 초기화 함수
void Generate_SBox() {
    for (int i = 0; i < 256; i++) {
        // 1. 역원을 구하고
        uint8_t inv = GF_Inverse((uint8_t)i);
        // 2. 아핀 변환을 적용하여 S-box 생성
        sbox[i] = Affine_Transformation(inv);

        // Inverse S-box는 S-box의 역함수이므로, 인덱스와 값을 뒤집어서 간단히 생성
        rsbox[sbox[i]] = i;
    }
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
    // 프로그램 시작 시 S-box 배열을 수학적으로 생성!
    Generate_SBox();
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