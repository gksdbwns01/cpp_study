// AES. 외부 라이브러리 없이 AES의 모든 수학적 과정을 직접 구현

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
    0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36 // x2(<< 1)
}; // Rcon: 키 확장 시 사용되는 라운드 상수 (128의 경우 10번 반복). 0x8d는 더미->배열 크기를 맞추기 위해 포함됨

// ============================================================================
// 2. GF(2^8) 수학 연산 및 S-box 동적 생성 (추가된 부분)
// ============================================================================

// (기존에 있던 xtime과 Multiply 함수는 그대로 유지)
uint8_t xtime(uint8_t x) { // GF(2^8)에서 x를 2배 곱하는 연산
    return (x << 1) ^ (((x >> 7) & 1) * 0x1b);
}

uint8_t Multiply(uint8_t x, uint8_t y) { // GF(2^8)에서 x와 y를 곱하는 연산
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

// 1차원 배열을 4x4 State 행렬로 로드 (열우선)
void LoadState(uint8_t state[4][4], const uint8_t* input) {
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            state[r][c] = input[r + 4 * c];
        }
    }
}

// 4x4 State 행렬을 1차원 배열로 저장 (열우선)
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

void SubWord(uint8_t* word) { // 4바이트 단어의 각 바이트를 S-box로 치환
    word[0] = sbox[word[0]];
    word[1] = sbox[word[1]];
    word[2] = sbox[word[2]];
    word[3] = sbox[word[3]];
}

void RotWord(uint8_t* word) { // 4바이트 단어를 왼쪽으로 한 바이트 회전
    uint8_t temp = word[0];
    word[0] = word[1];
    word[1] = word[2];
    word[2] = word[3];
    word[3] = temp;
}

void KeyExpansion(const uint8_t* key, uint8_t* RoundKey) { // AES-128 키 확장
    uint32_t i = 0;

    // 마스터 키 복사 (최초의 4 워드)
    // AES-128의 원본 키 16바이트를 그대로 RoundKey 배열의 맨 앞에 채워 넣음.
    // Nk는 4 (1 워드 = 4바이트이므로, 총 16바이트)
    while (i < Nk) {
        RoundKey[i * 4 + 0] = key[i * 4 + 0];
        RoundKey[i * 4 + 1] = key[i * 4 + 1];
        RoundKey[i * 4 + 2] = key[i * 4 + 2];
        RoundKey[i * 4 + 3] = key[i * 4 + 3];
        i++;
    }

    // 나머지 라운드 키 생성 (총 44 워드가 될 때까지 반복)
    // Nb = 4 (블록 크기), Nr = 10 (라운드 수) -> 총 4 * 11 = 44 워드 생성 필요
    i = Nk; // i는 4부터 시작
    while (i < Nb * (Nr + 1)) {
        uint8_t temp[4]; // 바로 이전 워드(i-1)의 값을 담을 임시 보관소
        
        // 이전 워드 (i-1)번째 값을 temp에 복사해 옴
        temp[0] = RoundKey[(i - 1) * 4 + 0];
        temp[1] = RoundKey[(i - 1) * 4 + 1];
        temp[2] = RoundKey[(i - 1) * 4 + 2];
        temp[3] = RoundKey[(i - 1) * 4 + 3];

        // 4번째 워드마다 특수한 복잡화 과정(비선형 변환)을 거침
        if (i % Nk == 0) {
            // 1. RotWord: 바이트의 순서를 왼쪽으로 한 칸씩 밂
            RotWord(temp);
            // 2. SubWord: 밀린 바이트들을 S-box를 이용해 완전히 다른 값으로 치환함 (패턴 파괴)
            SubWord(temp);
            // 3. Rcon 적용: 첫 번째 바이트에 라운드 상수(Rcon)를 XOR 하여 대칭성을 한 번 더 부숨
            temp[0] ^= Rcon[i / Nk];
        }

        // 새로운 워드 생성 공식: W[i] = W[i-Nk] ^ temp
        // 즉, '4칸 전의 워드'와 '방금 계산/변형한 이전 워드(temp)'를 XOR 해서 새로운 4바이트를 만듦
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

void AddRoundKey(uint8_t state[4][4], const uint8_t* RoundKey, int round) { // 현재 라운드에 해당하는 라운드 키를 State에 XOR 적용
    for (int c = 0; c < 4; ++c) {
        for (int r = 0; r < 4; ++r) {
            state[r][c] ^= RoundKey[round * Nb * 4 + c * 4 + r];
        }
    }
}

void SubBytes(uint8_t state[4][4]) { // State의 각 바이트를 S-box로 치환
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            state[r][c] = sbox[state[r][c]];
        }
    }
}

void InvSubBytes(uint8_t state[4][4]) { // State의 각 바이트를 Inverse S-box로 치환
    for (int r = 0; r < 4; ++r) {
        for (int c = 0; c < 4; ++c) {
            state[r][c] = rsbox[state[r][c]];
        }
    }
}

void ShiftRows(uint8_t state[4][4]) { // State의 각 행을 왼쪽으로 순환 이동 (1행은 1칸, 2행은 2칸, 3행은 3칸)
    uint8_t temp;

    temp = state[1][0]; state[1][0] = state[1][1]; state[1][1] = state[1][2]; state[1][2] = state[1][3]; state[1][3] = temp;

    temp = state[2][0]; state[2][0] = state[2][2]; state[2][2] = temp;
    temp = state[2][1]; state[2][1] = state[2][3]; state[2][3] = temp;

    temp = state[3][3]; state[3][3] = state[3][2]; state[3][2] = state[3][1]; state[3][1] = state[3][0]; state[3][0] = temp;
}

void InvShiftRows(uint8_t state[4][4]) { // State의 각 행을 오른쪽으로 순환 이동 (1행은 1칸, 2행은 2칸, 3행은 3칸)
    uint8_t temp;

    temp = state[1][3]; state[1][3] = state[1][2]; state[1][2] = state[1][1]; state[1][1] = state[1][0]; state[1][0] = temp;

    temp = state[2][0]; state[2][0] = state[2][2]; state[2][2] = temp;
    temp = state[2][1]; state[2][1] = state[2][3]; state[2][3] = temp;

    temp = state[3][0]; state[3][0] = state[3][1]; state[3][1] = state[3][2]; state[3][2] = state[3][3]; state[3][3] = temp;
}

void MixColumns(uint8_t state[4][4]) { // State의 각 열을 GF(2^8)에서 다항식 곱셈으로 변환
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

void InvMixColumns(uint8_t state[4][4]) { // State의 각 열을 GF(2^8)에서 다항식 곱셈으로 변환
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

void Cipher(const uint8_t* input, const uint8_t* RoundKey, uint8_t* output) { // AES 암호화 함수
    uint8_t state[4][4]; // 데이터를 조작할 4x4 2차원 작업 공간

    // 16바이트의 1차원 평문 배열을 4x4 state 행렬로 열우선으로 채워 넣음
    LoadState(state, input);

    // 암호화를 시작하기 전, 가장 먼저 원본 마스터 키(0번 라운드 키)를 데이터에 씌움
    AddRoundKey(state, RoundKey, 0);

    // AES-128 기준 총 10라운드 중, 1라운드부터 9라운드까지 동일한 4개의 과정을 반복함
    for (int round = 1; round < Nr; ++round) {
        SubBytes(state);      // 1. 데이터를 S-box를 통해 완전히 다른 값으로 치환 (비선형성 부여)
        ShiftRows(state);     // 2. 가로줄을 각각 다른 칸 수만큼 왼쪽으로 밀어 위치를 섞음
        MixColumns(state);    // 3. 세로줄 단위로 갈루아 필드 곱셈을 수행해 데이터를 심하게 버무림
        AddRoundKey(state, RoundKey, round); // 4. 이번 라운드에 해당하는 확장된 키를 XOR 연산으로 결합
    }

    // 마지막 라운드(10번째)에서는 MixColumns를 생략
    SubBytes(state);          // 1. 치환
    ShiftRows(state);         // 2. 행 이동
    AddRoundKey(state, RoundKey, Nr); // 3. 마지막 10번째 라운드 키 결합

    // 4x4 state 행렬을 다시 1차원 암호문 배열로 변환
    StoreState(output, state);
}

void InvCipher(const uint8_t* input, const uint8_t* RoundKey, uint8_t* output) { // AES 복호화 함수
    uint8_t state[4][4]; // 데이터를 복구할 4x4 2차원 작업 공간

    // 16바이트의 암호문 배열을 4x4 state 행렬로 세로 방향으로 로드
    LoadState(state, input);

    // 암호화의 가장 마지막에 씌웠던 마지막 라운드 키(Nr, 10번째)를 제일 먼저 XOR하여 벗겨냄
    AddRoundKey(state, RoundKey, Nr);

    // 암호화가 1라운드부터 9라운드까지 진행되었다면, 복호화는 9라운드부터 1라운드까지 거꾸로 내려감
    for (int round = Nr - 1; round > 0; --round) {
        InvShiftRows(state);  // 1. 왼쪽으로 밀렸던 가로줄들을 오른쪽으로 당겨서 원위치 복구
        InvSubBytes(state);   // 2. rsbox(역 S-box)를 사용해 치환되었던 바이트를 원래 값으로 복구
        AddRoundKey(state, RoundKey, round); // 3. 현재 라운드(9~1)에 해당하는 확장 키를 XOR하여 복구
        InvMixColumns(state); // 4. 섞였던 세로줄을 역방향 행렬 곱셈을 통해 복구
    }

    // 복호화에서도 InvMixColumns 과정은 없음
    InvShiftRows(state);      // 1. 행 원위치
    InvSubBytes(state);       // 2. 바이트 원위치
    AddRoundKey(state, RoundKey, 0); // 3. 암호화 맨 처음 씌웠던 원본 마스터 키(0번째)를 XOR하여 최종 평문 복원

    // 복원된 평문 상태의 4x4 state 행렬을 1차원 배열로 변환
    StoreState(output, state);
}

// ============================================================================
// 7. 테스트 (main 함수)
// ============================================================================

void PrintHex(const char* label, const uint8_t* data, int length) { // 데이터를 16진수로 출력하는 함수
    printf("%-12s: ", label);
    for (int i = 0; i < length; i++) {
        printf("%02X", data[i]);
    }
    printf("\n");
}

int main() {
    // 프로그램 시작 시 S-box 배열을 수학적으로 생성
    Generate_SBox();
    printf("==========================================\n");
    printf("     AES-128 클린 코드 리팩토링 테스트\n");
    printf("==========================================\n");

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

int main() {
    // 프로그램 시작 시 S-box 배열을 생성
    Generate_SBox();
    
    printf("==========================================\n");
    printf("     AES-128 클린 코드 리팩토링 테스트\n");
    printf("==========================================\n");

    // 16바이트 평문(원본 메시지) 배열을 생성
    uint8_t plaintext[AES_BLOCK_SIZE] = {
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff
    };

    // 암호화에 사용할 16바이트 마스터 키를 생성
    uint8_t key[AES_BLOCK_SIZE] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
    };

    uint8_t ciphertext[AES_BLOCK_SIZE];    // 암호문이 저장될 공간
    uint8_t decryptedtext[AES_BLOCK_SIZE]; // 다시 복구된 평문이 저장될 공간

    // 확장된 키를 담을 배열을 준비
    uint8_t RoundKey[AES_ROUND_KEY_SIZE];

    // 16바이트 마스터 키로 176바이트 길이의 전체 RoundKey를 생성
    KeyExpansion(key, RoundKey);

    // 원본 데이터와 키를 출력하여 확인
    PrintHex("Plaintext", plaintext, AES_BLOCK_SIZE);
    PrintHex("Key", key, AES_BLOCK_SIZE);
    printf("------------------------------------------\n");

    // 평문을 확장된 키로 암호화하여 암호문 배열에 저장
    Cipher(plaintext, RoundKey, ciphertext);
    
    // 만들어진 암호문을 출력하고, 표준 문서의 정답(Expected CT)과 눈으로 비교
    PrintHex("Ciphertext", ciphertext, AES_BLOCK_SIZE);
    printf("(Expected CT : 69C4E0D86A7B0430D8CDB78070B4C55A)\n");
    printf("------------------------------------------\n");


    // 복호화 진행
    // ciphertext을 다시 InvCipher에 넣어 원본으로 복구
    InvCipher(ciphertext, RoundKey, decryptedtext);
    
    // 복구된 데이터 출력
    PrintHex("Decrypted", decryptedtext, AES_BLOCK_SIZE);
    printf("==========================================\n");

    // 검증
    // plaintext와 decryptedtext의 16바이트 전체가 똑같은지 검사
    if (memcmp(plaintext, decryptedtext, AES_BLOCK_SIZE) == 0) {
        printf("[SUCCESS] 복호화된 데이터가 원본과 일치합니다!\n");
    }
    else {
        printf("[FAIL] 복호화 데이터가 원본과 다릅니다.\n");
    }

    return 0;
}