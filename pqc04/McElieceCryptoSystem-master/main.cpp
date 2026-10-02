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
// ============================================================
// 디버그용 부분 출력 함수 (크기 + 일부 원소 출력)
// ============================================================
void print_preview(const string& name, const mat& A, int limit = 20)
{
    cout << name << " (" << A.n_rows << " x " << A.n_cols << ")" << endl;
    
    if (A.n_rows > 1 && A.n_cols > 1) {
        // 행렬인 경우 첫 번째 행 출력
        cout << "first row:" << endl;
        int count = 0;
        for (unsigned int j = 0; j < A.n_cols; ++j) {
            cout << (unsigned int)A(0, j) << " ";
            if (++count >= limit) { cout << "..."; break; }
        }
    } else {
        // 벡터인 경우 (1 x N 또는 N x 1)
        cout << "values:" << endl;
        int count = 0;
        for (unsigned int i = 0; i < A.n_elem; ++i) {
            cout << (unsigned int)A(i) << " ";
            if (++count >= limit) { cout << "..."; break; }
        }
    }
    cout << endl << endl;
}
int main()
{
    cout << "========================================" << endl;
    cout << "        McEliece 암호화/복호화" << endl;
    cout << "========================================" << endl;

    // [1] Key Generation
    cout << "\n[Key Generation]" << endl;
    cout << "----------------------------------------" << endl;
    mat H, G, S, P, Gpub;
    mykeygen(H, G, S, P, Gpub, false);

    print_preview("G", G);
    print_preview("S", S);
    print_preview("P", P);
    
    // cout << "※ S, P는 permutation matrix이므로 S^-1 = S^T, P^-1 = P^T 가 성립함." << endl;
    cout << "Gpub = S * G * P" << endl;
    print_preview("Gpub", Gpub);

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

    // [3] Encryption (Block 1 Trace)
    cout << "\n[Encryption] (블록 처리 과정)" << endl;
    cout << "----------------------------------------" << endl;
    
    mat cipher1 = trans(blk1) * Gpub;
    cipher1 = trans(cipher1); // 256x1 벡터화
    
    for (unsigned int i = 0; i < cipher1.n_elem; ++i) 
        cipher1(i) = ((unsigned int)cipher1(i)) % 2;
    
    mat mGpub1 = cipher1;

    print_preview("m", blk1);
    print_preview("m * Gpub", mGpub1);

    adderror(cipher1, 13);
    
    mat e1 = cipher1 - mGpub1;
    for (unsigned int i = 0; i < e1.n_elem; ++i) 
        e1(i) = (((int)e1(i)) % 2 + 2) % 2;

    print_preview("e", e1);
    cout << "wt(e) = " << hamming_weight(e1) << endl << endl;

    cout << "c = mGpub + e" << endl;
    print_preview("c", cipher1);
    // ==========================================================
    // 여기에 아래의 Block 2 처리 및 cipher 병합 코드를 추가하세요.
    // ==========================================================
    mat cipher2 = trans(blk2) * Gpub;
    cipher2 = trans(cipher2); 
    
    for (unsigned int i = 0; i < cipher2.n_elem; ++i) 
        cipher2(i) = ((unsigned int)cipher2(i)) % 2;
    
    adderror(cipher2, 13);
    
    mat cipher = join_cols(cipher1, cipher2); // cipher 변수 정의 완료
    // ==========================================================

    // [4] Decryption 준비
    cout << "\n[Decryption] (블록 처리 과정)" << endl;
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
    
    // cout << "\n--- Block 2 Decoding (Silenced) ---" << endl;
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
    if (plaintext == recovered) cout << "SUCCESS! (원본 평문 == 복원된 평문)" << endl;
    cout << "========================================" << endl;

    return 0;
}