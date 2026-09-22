#include <iostream>
#include <armadillo>
#include <string>

#include "GF.h"
#include "Euclidean.h"

using namespace std;
using namespace arma;


// ============================================================
// 기존 c2.cpp에서 재사용하는 함수
// ============================================================

void mykeygen(
    mat& H,
    mat& G,
    mat& S,
    mat& P,
    mat& Ghat,
    bool random
);

void adderror(mat& cipher, int weight);

mat decrypt_one(
    mat H,
    mat G,
    mat S,
    mat P,
    mat ciphertext,
    const Poly_t g_z,
    const Field_t GF
);

mat int2vec(unsigned int input, int length);

unsigned int vec2int(mat A, int length);


// ============================================================
// 행렬 크기 출력
// ============================================================

void print_size(const string& name, const mat& A)
{
    cout << name
         << " : "
         << A.n_rows << " x "
         << A.n_cols
         << endl;
}


// ============================================================
// 0/1 행렬의 Hamming Weight
// ============================================================

unsigned int hamming_weight(const mat& A)
{
    unsigned int weight = 0;

    for (unsigned int i = 0; i < A.n_elem; ++i)
    {
        if (((unsigned int)A(i)) % 2 == 1)
            weight++;
    }

    return weight;
}


// ============================================================
// bit vector -> string
// ============================================================

string bits_to_string(const mat& bits)
{
    string result = "";

    for (int i = 0; i < 19; ++i)
    {
        unsigned int value =
            vec2int(bits.rows(i * 8, i * 8 + 7), 8);

        result += (char)value;
    }

    return result;
}


// ============================================================
// MAIN
// ============================================================

int main()
{
    cout << "========================================" << endl;
    cout << "        McEliece Demonstration" << endl;
    cout << "========================================" << endl;


    // ========================================================
    // 1. KEY GENERATION
    // ========================================================

    cout << endl;
    cout << "[1] Key Generation" << endl;
    cout << "----------------------------------------" << endl;

    mat H;
    mat G;
    mat S;
    mat P;
    mat Gpub;

    // random = true
    //
    // 기존 c2.cpp의 mykeygen()을 그대로 사용
    //
    // G     : private Goppa generator matrix
    // S     : secret scrambling matrix
    // P     : secret permutation matrix
    // Gpub  : public generator matrix

    mykeygen(H, G, S, P, Gpub, false);


    cout << "Key generation complete." << endl;

    print_size("H", H);
    print_size("G", G);
    print_size("S", S);
    print_size("P", P);
    print_size("Gpub", Gpub);


    cout << endl;
    cout << "Gpub = S * G * P" << endl;


    // ========================================================
    // 2. PLAINTEXT
    // ========================================================

    cout << endl;
    cout << "[2] Plaintext" << endl;
    cout << "----------------------------------------" << endl;

    string plaintext =
        "12345678901234567890123456789012345678";

    cout << "Plaintext : " << plaintext << endl;
    cout << "Length    : " << plaintext.length()
         << " bytes" << endl;


    // 이 프로젝트는 38 byte plaintext를
    // 19 byte + 19 byte로 나눠서 처리한다.

    string plaintext1 = plaintext.substr(0, 19);
    string plaintext2 = plaintext.substr(19, 19);

    cout << "Block 1   : " << plaintext1 << endl;
    cout << "Block 2   : " << plaintext2 << endl;


    // ========================================================
    // 3. PLAINTEXT -> BIT VECTOR
    // ========================================================

    cout << endl;
    cout << "[3] Encode plaintext into bit vectors" << endl;
    cout << "----------------------------------------" << endl;

    mat blk1 = int2vec(
        (unsigned int)(unsigned char)plaintext1[0],
        8
    );

    for (int i = 1; i < 19; ++i)
    {
        blk1 = join_cols(
            blk1,
            int2vec(
                (unsigned int)(unsigned char)plaintext1[i],
                8
            )
        );
    }


    mat blk2 = int2vec(
        (unsigned int)(unsigned char)plaintext2[0],
        8
    );

    for (int i = 1; i < 19; ++i)
    {
        blk2 = join_cols(
            blk2,
            int2vec(
                (unsigned int)(unsigned char)plaintext2[i],
                8
            )
        );
    }


    cout << "Block 1 bits : "
         << blk1.n_rows << " x "
         << blk1.n_cols << endl;

    cout << "Block 2 bits : "
         << blk2.n_rows << " x "
         << blk2.n_cols << endl;


    // ========================================================
    // 4. ENCRYPTION
    // ========================================================

    cout << endl;
    cout << "[4] Encryption" << endl;
    cout << "----------------------------------------" << endl;

    cout << "Formula:" << endl;
    cout << "    c = m * Gpub + e" << endl;


    // --------------------------------------------------------
    // m * Gpub
    // --------------------------------------------------------

    mat cipher1 = trans(blk1) * Gpub;
    mat cipher2 = trans(blk2) * Gpub;

    cipher1 = trans(cipher1);
    cipher2 = trans(cipher2);


    cout << endl;
    cout << "Before adding error:" << endl;

    print_size("c1", cipher1);
    print_size("c2", cipher2);


    // --------------------------------------------------------
    // Add intentional errors
    // --------------------------------------------------------

    cout << endl;
    cout << "Adding error vector..." << endl;

    cout << "Error weight = 13" << endl;

    adderror(cipher1, 13);
    adderror(cipher2, 13);


    cout << "Error added." << endl;


    // --------------------------------------------------------
    // Combine two ciphertext blocks
    // --------------------------------------------------------

    mat cipher = join_cols(cipher1, cipher2);

    cout << endl;
    print_size("Ciphertext", cipher);

    cout << "Ciphertext Hamming weight = "
         << hamming_weight(cipher)
         << endl;


    cout << endl;
    cout << "Encryption complete." << endl;


    // ========================================================
    // 5. DECRYPTION
    // ========================================================

    cout << endl;
    cout << "[5] Decryption" << endl;
    cout << "----------------------------------------" << endl;


    // --------------------------------------------------------
    // Ciphertext를 두 블록으로 분리
    // --------------------------------------------------------

    mat cipher_block1 =
        cipher.rows(0, 255);

    mat cipher_block2 =
        cipher.rows(256, 511);


    // decrypt_one()은 기존 코드에서
    // 1 x 256 형태의 ciphertext를 받는다.

    cipher_block1 = trans(cipher_block1);
    cipher_block2 = trans(cipher_block2);


    // ========================================================
    // 6. Goppa Decoder 준비
    // ========================================================

    cout << endl;
    cout << "[6] Prepare Goppa decoder" << endl;
    cout << "----------------------------------------" << endl;


    Field_t GF;

    GF.P_x = 0453;
    GF.max_ele = 256;

    GF.gen[0] = 0;
    GF.gen_inv[0] = 0;

    GF.gen[1] = 1;
    GF.gen_inv[1] = 1;

    for (unsigned i = 2; i < GF.max_ele; ++i)
    {
        GF.gen[i] =
            galois_mul(
                GF.gen[i - 1],
                2,
                GF.P_x
            );

        GF.gen_inv[GF.gen[i]] = i;
    }


    // --------------------------------------------------------
    // Goppa polynomial
    // --------------------------------------------------------

    unsigned int g_z[14] =
    {
        53,
        100,
        17,
        229,
        248,
        45,
        120,
        152,
        113,
        131,
        133,
        197,
        103,
        129
    };


    Poly_t GZ;

    GZ.degree = 13;

    for (int i = 0; i <= 13; ++i)
        GZ.coefficient[i] = g_z[i];


    cout << "Goppa polynomial degree = "
         << GZ.degree << endl;


    // ========================================================
    // 7. DECRYPT BLOCK 1
    // ========================================================

    cout << endl;
    cout << "[7] Decode block 1" << endl;
    cout << "----------------------------------------" << endl;


    mat plain1 =
        decrypt_one(
            H,
            G,
            S,
            P,
            cipher_block1,
            GZ,
            GF
        );


    cout << "Block 1 decoding complete." << endl;


    // ========================================================
    // 8. DECRYPT BLOCK 2
    // ========================================================

    cout << endl;
    cout << "[8] Decode block 2" << endl;
    cout << "----------------------------------------" << endl;


    mat plain2 =
        decrypt_one(
            H,
            G,
            S,
            P,
            cipher_block2,
            GZ,
            GF
        );


    cout << "Block 2 decoding complete." << endl;


    // ========================================================
    // 9. Recover plaintext
    // ========================================================

    cout << endl;
    cout << "[9] Recover plaintext" << endl;
    cout << "----------------------------------------" << endl;


    // decrypt_one()의 결과는
    // 1 x 152 형태의 bit vector

    for (int i = 0; i < 152; ++i)
    {
        plain1(0, i) =
            ((unsigned int)plain1(0, i)) % 2;

        plain2(0, i) =
            ((unsigned int)plain2(0, i)) % 2;
    }


    plain1 = trans(plain1);
    plain2 = trans(plain2);


    string recovered1 =
        bits_to_string(plain1);

    string recovered2 =
        bits_to_string(plain2);


    string recovered =
        recovered1 + recovered2;


    // ========================================================
    // 10. RESULT
    // ========================================================

    cout << endl;
    cout << "========================================" << endl;
    cout << "              RESULT" << endl;
    cout << "========================================" << endl;

    cout << "Original  : "
         << plaintext << endl;

    cout << "Recovered : "
         << recovered << endl;


    if (plaintext == recovered)
    {
        cout << endl;
        cout << "SUCCESS!" << endl;
        cout << "Plaintext == Recovered plaintext" << endl;
    }
    else
    {
        cout << endl;
        cout << "FAIL!" << endl;
    }


    cout << "========================================" << endl;

    return 0;
}