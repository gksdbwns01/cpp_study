#include <iostream>
#include <armadillo>
#include <string>

#include "GF.h"
#include "Euclidean.h"

using namespace std;
using namespace arma;

// ============================================================
// 기존 c2.cpp 함수 선언
// ============================================================
void mykeygen(mat& H, mat& G, mat& S, mat& P, mat& Ghat, bool random);
void adderror(mat& cipher, int weight);
mat decrypt_one(mat H, mat G, mat S, mat P, mat ciphertext, const Poly_t g_z, const Field_t GF);
mat int2vec(unsigned int input, int length);
unsigned int vec2int(mat A, int length);

// ============================================================
// 디버그용 부분 출력 함수 (원소의 앞부분만 출력)
// ============================================================
void print_partial(const string& name, const mat& A, int limit = 20)
{
    cout << name << " =" << endl;
    int count = 0;
    for (unsigned int i = 0; i < A.n_elem; ++i)
    {
        cout << (unsigned int)A(i) << " ";
        count++;
        if (count >= limit)
        {
            cout << "...";
            break;
        }
    }
    cout << endl << endl;
}

unsigned int hamming_weight(const mat& A)
{
    unsigned int weight = 0;
    for (unsigned int i = 0; i < A.n_elem; ++i)
    {
        if ((((unsigned int)A(i)) % 2) == 1)
            weight++;
    }
    return weight;
}

string bits_to_string(const mat& bits)
{
    string result = "";
    for (int i = 0; i < 19; ++i)
    {
        unsigned int value = vec2int(bits.rows(i * 8, i * 8 + 7), 8);
        result += (char)value;
    }
    return result;
}

int main()
{
    cout << "========================================" << endl;
    cout << "        McEliece Demonstration" << endl;
    cout << "========================================" << endl;

    // [1] Key Generation
    cout << "\n[Key Generation]" << endl;
    cout << "----------------------------------------" << endl;
    mat H, G, S, P, Gpub;
    mykeygen(H, G, S, P, Gpub, false);

    // 원소값 확인
    print_partial("S", S);
    print_partial("P", P);
    cout << "Gpub = S * G * P" << endl;
    print_partial("Gpub", Gpub);


    // [2] Plaintext
    string plaintext = "12345678901234567890123456789012345678";
    string plaintext1 = plaintext.substr(0, 19);
    string plaintext2 = plaintext.substr(19, 19);

    mat blk1 = int2vec((unsigned int)(unsigned char)plaintext1[0], 8);
    for (int i = 1; i < 19; ++i)
        blk1 = join_cols(blk1, int2vec((unsigned int)(unsigned char)plaintext1[i], 8));

    mat blk2 = int2vec((unsigned int)(unsigned char)plaintext2[0], 8);
    for (int i = 1; i < 19; ++i)
        blk2 = join_cols(blk2, int2vec((unsigned int)(unsigned char)plaintext2[i], 8));

    // [3] Encryption (Block 1을 기준으로 상세 출력)
    cout << "\n[Encryption] (Block 1 Trace)" << endl;
    cout << "----------------------------------------" << endl;
    
    mat cipher1 = trans(blk1) * Gpub;
    cipher1 = trans(cipher1); // 256x1 벡터화
    
    // 이진화 (mod 2) 적용
    for (unsigned int i = 0; i < cipher1.n_elem; ++i) 
        cipher1(i) = ((unsigned int)cipher1(i)) % 2;
    
    mat mGpub1 = cipher1; // 에러 더하기 전 상태 저장

    print_partial("m", blk1);
    print_partial("m * Gpub", mGpub1);

    // 에러 추가 및 역산하여 e 벡터 추출
    adderror(cipher1, 13);
    
    mat e1 = cipher1 - mGpub1;
    for (unsigned int i = 0; i < e1.n_elem; ++i) 
        e1(i) = (((int)e1(i)) % 2 + 2) % 2; // 음수 모듈러 방지

    print_partial("e", e1);
    cout << "wt(e) = " << hamming_weight(e1) << endl << endl;

    cout << "c = mGpub + e" << endl;
    print_partial("c", cipher1);

    // Block 2 처리 (디버그 출력 생략)
    mat cipher2 = trans(blk2) * Gpub;
    cipher2 = trans(cipher2);
    for (unsigned int i = 0; i < cipher2.n_elem; ++i) cipher2(i) = ((unsigned int)cipher2(i)) % 2;
    adderror(cipher2, 13);
    mat cipher = join_cols(cipher1, cipher2);


    // [4] Decryption 준비
    cout << "\n[Decryption] (Block 1 Trace)" << endl;
    cout << "----------------------------------------" << endl;
    mat cipher_block1 = trans(cipher.rows(0, 255));
    mat cipher_block2 = trans(cipher.rows(256, 511));

    Field_t GF;
    GF.P_x = 0453;
    GF.max_ele = 256;
    GF.gen[0] = 0; GF.gen_inv[0] = 0;
    GF.gen[1] = 1; GF.gen_inv[1] = 1;
    for (unsigned i = 2; i < GF.max_ele; ++i) {
        GF.gen[i] = galois_mul(GF.gen[i - 1], 2, GF.P_x);
        GF.gen_inv[GF.gen[i]] = i;
    }

    unsigned int g_z[14] = {53, 100, 17, 229, 248, 45, 120, 152, 113, 131, 133, 197, 103, 129};
    Poly_t GZ; GZ.degree = 13;
    for (int i = 0; i <= 13; ++i) GZ.coefficient[i] = g_z[i];

    // decrypt_one 내부에서 디버그 내용이 출력됨
    mat plain1 = decrypt_one(H, G, S, P, cipher_block1, GZ, GF);
    
    cout << "\n--- Block 2 Decoding (Silenced) ---" << endl;
    mat plain2 = decrypt_one(H, G, S, P, cipher_block2, GZ, GF); // 두 번째 블록 복호화

    // Recover plaintext
    for (int i = 0; i < 152; ++i) {
        plain1(0, i) = ((unsigned int)plain1(0, i)) % 2;
        plain2(0, i) = ((unsigned int)plain2(0, i)) % 2;
    }
    
    string recovered = bits_to_string(trans(plain1)) + bits_to_string(trans(plain2));

    cout << "\n========================================" << endl;
    cout << "Original  : " << plaintext << endl;
    cout << "Recovered : " << recovered << endl;
    if (plaintext == recovered) cout << "SUCCESS! (Plaintext == Recovered)" << endl;
    cout << "========================================" << endl;

    return 0;
}