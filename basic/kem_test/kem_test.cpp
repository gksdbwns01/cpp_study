#include <iostream>
#include <vector>
#include <cstring>
#include <iomanip>
#include <oqs/oqs.h>

// 메모리에 담긴 바이너리 암호 데이터를 16진수로 출력해 주는 시각화 함수
void print_hex(const std::string& label, const std::vector<uint8_t>& vec) {
    std::cout << "▪️ " << label << " (" << vec.size() << " bytes):\n  ";
    
    // 너무 길면 앞뒤 16바이트씩만 잘라서 출력 (가독성을 위함)
    if (vec.size() > 32) {
        for (size_t i = 0; i < 16; ++i) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)vec[i] << " ";
        }
        std::cout << "... [중략] ... ";
        for (size_t i = vec.size() - 16; i < vec.size(); ++i) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)vec[i] << " ";
        }
    } else {
        for (int val : vec) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << val << " ";
        }
    }
    std::cout << std::dec << "\n\n"; // 16진수 출력이 끝난 후 다시 10진수 모드로 복구
}

int main() {
    // 1. 알고리즘 선택 (NIST 표준 ML-KEM-768)
    const char *alg_name = OQS_KEM_alg_ml_kem_768;
    OQS_KEM *kem = OQS_KEM_new(alg_name);
    
    if (kem == nullptr) {
        std::cerr << "KEM 알고리즘 초기화 실패!" << std::endl;
        return 1;
    }

    std::cout << "==================================================\n";
    std::cout << "사용 중인 양자 내성 알고리즘: " << kem->method_name << "\n";
    std::cout << "==================================================\n\n";

    // 버퍼 할당
    std::vector<uint8_t> public_key(kem->length_public_key);
    std::vector<uint8_t> secret_key(kem->length_secret_key);
    std::vector<uint8_t> ciphertext(kem->length_ciphertext);
    std::vector<uint8_t> shared_secret_bob(kem->length_shared_secret);
    std::vector<uint8_t> shared_secret_alice(kem->length_shared_secret);

    // [Step 1] Alice: 키 쌍 생성
    std::cout << "--------------------------------------------------\n";
    std::cout << "[Step 1] Alice가 키 쌍(공개키 & 비밀키)을 생성합니다.\n";
    std::cout << "--------------------------------------------------\n";
    OQS_KEM_keypair(kem, public_key.data(), secret_key.data());
    
    print_hex("Alice의 공개키 (Public Key - 자물쇠)", public_key);
    print_hex("Alice의 비밀키 (Secret Key - 개인 열쇠)", secret_key);

    // [Step 2] Bob: 공유 비밀키 생성 및 캡슐화
    std::cout << "--------------------------------------------------\n";
    std::cout << "[Step 2] Bob이 Alice의 공개키로 공유키를 만들고 캡슐화합니다.\n";
    std::cout << "--------------------------------------------------\n";
    OQS_KEM_encaps(kem, ciphertext.data(), shared_secret_bob.data(), public_key.data());
    
    print_hex("Bob이 내부적으로 생성한 공유 비밀키 (대칭키)", shared_secret_bob);
    print_hex("인터넷으로 전송할 암호 상자 (Ciphertext / 캡슐)", ciphertext);

    // [Step 3] Alice: 암호문 캡슐 해제
    std::cout << "--------------------------------------------------\n";
    std::cout << "[Step 3] Alice가 본인의 비밀키로 Bob의 암호 상자를 엽니다.\n";
    std::cout << "--------------------------------------------------\n";
    OQS_KEM_decaps(kem, shared_secret_alice.data(), ciphertext.data(), secret_key.data());
    
    print_hex("Alice가 캡슐 해제로 획득한 공유 비밀키 (대칭키)", shared_secret_alice);

    // [Step 4] 검증: 최종 매칭 확인
    std::cout << "--------------------------------------------------\n";
    std::cout << "[Step 4] 최종 결과 확인\n";
    std::cout << "--------------------------------------------------\n";
    if (std::memcmp(shared_secret_bob.data(), shared_secret_alice.data(), kem->length_shared_secret) == 0) {
        std::cout << "⭕ 성공! 두 키가 완벽하게 일치합니다.\n";
        std::cout << "   이제 Alice와 Bob은 이 키를 사용하여 AES 등으로 암호화 통신을 할 수 있습니다.\n";
    } else {
        std::cout << "❌ 실패! 키가 일치하지 않습니다.\n";
    }

    OQS_KEM_free(kem);
    return 0;
}