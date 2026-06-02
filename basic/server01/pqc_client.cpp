#include <iostream>
#include <vector>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <oqs/oqs.h>

#define PORT 8080

// 공유 비밀키가 서버와 일치하는지 눈으로 확인하기 위한 출력 함수
void print_hex(const std::vector<uint8_t>& data) {
    for (size_t i = 0; i < 16; ++i) {
        printf("%02X", data[i]);
    }
    std::cout << "..." << std::endl;
}

int main() {
    // 1. KEM 알고리즘 세팅 (서버와 동일한 ML-KEM-768 사용)
    const char *alg_name = OQS_KEM_alg_ml_kem_768;
    OQS_KEM *kem = OQS_KEM_new(alg_name);
    if (kem == nullptr) {
        std::cerr << "KEM 알고리즘 초기화 실패" << std::endl;
        return 1;
    }

    // 2. TCP 소켓 생성
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "소켓 생성 실패" << std::endl;
        return 1;
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // 같은 PC 내에서 테스트하므로 루프백 주소(127.0.0.1)를 설정합니다.
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        std::cerr << "잘못된 주소 포맷" << std::endl;
        return 1;
    }

    std::cout << "=== [클라이언트/Alice] 서버 연결 시도 ===" << std::endl;
    
    // 3. 서버에 접속
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "서버 연결에 실패했습니다!" << std::endl;
        return 1;
    }
    std::cout << "[클라이언트] 서버에 성공적으로 연결되었습니다." << std::endl;

    // 4. 키 쌍(공개키 및 비밀키) 생성
    std::vector<uint8_t> public_key(kem->length_public_key);
    std::vector<uint8_t> secret_key(kem->length_secret_key);
    OQS_KEM_keypair(kem, public_key.data(), secret_key.data());
    std::cout << "[클라이언트] 양자 내성 키 쌍(공개키/비밀키) 생성을 완료했습니다." << std::endl;

    // 5. 서버(Bob)로 공개키(Public Key) 전송
    send(sock, public_key.data(), kem->length_public_key, 0);
    std::cout << "[클라이언트] 서버로 공개키를 전송했습니다." << std::endl;

    // 6. 서버로부터 암호문(Ciphertext) 수신
    std::vector<uint8_t> ciphertext(kem->length_ciphertext);
    read(sock, ciphertext.data(), kem->length_ciphertext);
    std::cout << "[클라이언트] 서버로부터 암호문을 수신했습니다. (크기: " << kem->length_ciphertext << " bytes)" << std::endl;

    // 7. 캡슐 해제 (수신한 암호문과 내 비밀키를 결합해 클라이언트용 공유 비밀키 생성)
    std::vector<uint8_t> shared_secret_alice(kem->length_shared_secret);
    OQS_KEM_decaps(kem, shared_secret_alice.data(), ciphertext.data(), secret_key.data());
    std::cout << "[클라이언트] 암호문 캡슐 해제를 완료했습니다." << std::endl;

    // 결과 확인
    std::cout << "\n========================================" << std::endl;
    std::cout << "[클라이언트 최종 결과] 안전하게 보관할 공유 비밀키: ";
    print_hex(shared_secret_alice);
    std::cout << "========================================" << std::endl;

    // 8. 자원 정리 및 소켓 닫기
    OQS_KEM_free(kem);
    close(sock);

    return 0;
}