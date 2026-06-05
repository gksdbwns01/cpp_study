#include <iostream>
#include <vector>
#include <chrono>
#include <iomanip>
#include <oqs/oqs.h>
#include <openssl/evp.h>
#include <openssl/ec.h>
#include <openssl/core_names.h>

// g++ -O3 -o PQCvsECC PQCvsECC.cpp -loqs -lcrypto
// ./PQCvsECC

int main() {
    std::cout << "==================================================" << std::endl;
    std::cout << "   차세대 PQC vs 기존 ECC 연산 속도 1:1 벤치마킹   " << std::endl;
    std::cout << "==================================================\n" << std::endl;

    // 타이머 변수
    std::chrono::high_resolution_clock::time_point start, end;

    // ==========================================
    // [Round 1] 양자 내성 암호 (ML-KEM-768)
    // ==========================================
    std::cout << "[Round 1] 양자 내성 암호 (ML-KEM-768)" << std::endl;
    
    OQS_KEM *kem = OQS_KEM_new(OQS_KEM_alg_ml_kem_768);
    if (kem == nullptr) {
        std::cerr << "KEM 알고리즘 초기화 실패!" << std::endl;
        return 1;
    }

    std::vector<uint8_t> pqc_pub_key(kem->length_public_key);
    std::vector<uint8_t> pqc_sec_key(kem->length_secret_key);
    std::vector<uint8_t> pqc_ciphertext(kem->length_ciphertext);
    std::vector<uint8_t> pqc_shared_secret(kem->length_shared_secret);

    // 1-1. 키 쌍 생성 측정
    start = std::chrono::high_resolution_clock::now();
    OQS_KEM_keypair(kem, pqc_pub_key.data(), pqc_sec_key.data());
    end = std::chrono::high_resolution_clock::now();
    auto pqc_keygen_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    // 1-2. 캡슐화(비밀키 도출 포함) 측정
    start = std::chrono::high_resolution_clock::now();
    OQS_KEM_encaps(kem, pqc_ciphertext.data(), pqc_shared_secret.data(), pqc_pub_key.data());
    end = std::chrono::high_resolution_clock::now();
    auto pqc_encaps_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    std::cout << "  - 키 쌍 생성 (Keypair) 소요 시간: " << std::setw(6) << pqc_keygen_time << " μs" << std::endl;
    std::cout << "  - 캡슐화 (Encapsulation) 소요 시간: " << std::setw(4) << pqc_encaps_time << " μs\n" << std::endl;

    OQS_KEM_free(kem);


    // ==========================================
    // [Round 2] 기존 표준 암호 (ECDH secp256r1)
    // ==========================================
    std::cout << "[Round 2] 기존 시스템 (ECDH secp256r1)" << std::endl;

    EVP_PKEY_CTX *pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL);
    EVP_PKEY_keygen_init(pctx);
    EVP_PKEY_CTX_set_ec_paramgen_curve_nid(pctx, NID_X9_62_prime256v1);

    EVP_PKEY *alice_key = NULL;
    EVP_PKEY *bob_key = NULL;

    // 2-1. 키 쌍 생성 측정
    start = std::chrono::high_resolution_clock::now();
    EVP_PKEY_keygen(pctx, &alice_key);
    end = std::chrono::high_resolution_clock::now();
    auto ecc_keygen_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    // 상대방(Bob) 키 생성 (측정 외)
    EVP_PKEY_keygen(pctx, &bob_key);

    // 2-2. 비밀키 도출 측정
    EVP_PKEY_CTX *ctx_alice = EVP_PKEY_CTX_new(alice_key, NULL);
    EVP_PKEY_derive_init(ctx_alice);
    EVP_PKEY_derive_set_peer(ctx_alice, bob_key);

    size_t secret_len;
    EVP_PKEY_derive(ctx_alice, NULL, &secret_len);
    std::vector<unsigned char> ecc_shared_secret(secret_len);

    start = std::chrono::high_resolution_clock::now();
    EVP_PKEY_derive(ctx_alice, ecc_shared_secret.data(), &secret_len);
    end = std::chrono::high_resolution_clock::now();
    auto ecc_derive_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    std::cout << "  - 키 쌍 생성 (Keypair) 소요 시간: " << std::setw(6) << ecc_keygen_time << " μs" << std::endl;
    std::cout << "  - 비밀키 도출 (Derivation) 소요 시간: " << std::setw(4) << ecc_derive_time << " μs\n" << std::endl;

    // 자원 정리
    EVP_PKEY_CTX_free(ctx_alice);
    EVP_PKEY_free(alice_key);
    EVP_PKEY_free(bob_key);
    EVP_PKEY_CTX_free(pctx);

    std::cout << "==================================================" << std::endl;
    return 0;
}