#include <iostream>
#include <vector>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <oqs/oqs.h>

#define PORT 8080

// 공유 비밀키가 잘 만들어졌는지 눈으로 확인하기 위한 출력 함수 (앞 16바이트만)
void print_hex(const std::vector<uint8_t>& data) {
    for (size_t i = 0; i < 16; ++i) {
        printf("%02X", data[i]);
    }
    std::cout << "..." << std::endl;
}

int main() {
    // 1. KEM 알고리즘 세팅 (ML-KEM-768)
    const char *alg_name = OQS_KEM_alg_ml_kem_768;
    OQS_KEM *kem = OQS_KEM_new(alg_name);
    if (kem == nullptr) {
        std::cerr << "KEM 알고리즘 초기화 실패" << std::endl;
        return 1;
    }

    // 2. TCP 소켓 생성 및 바인딩 (네트워크 설정)
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    // 포트 충돌 방지 옵션
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 3); // 클라이언트 접속 대기

    std::cout << "=== [서버/Bob] 양자 내성 보안 서버 시작 ===" << std::endl;
    std::cout << "포트 " << PORT << "에서 클라이언트(IoT 기기) 접속을 기다립니다..." << std::endl;

    // 3. 클라이언트 접속 수락 (여기서 프로그램이 멈춰서 기다립니다)
    new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
    std::cout << "\n[서버] 클라이언트가 성공적으로 접속했습니다!" << std::endl;

    // 4. 클라이언트(Alice)로부터 네트워크를 통해 공개키(Public Key) 수신
    std::vector<uint8_t> public_key(kem->length_public_key);
    read(new_socket, public_key.data(), kem->length_public_key);
    std::cout << "[서버] 클라이언트의 공개키를 수신했습니다. (크기: " << kem->length_public_key << " bytes)" << std::endl;

    // 5. 캡슐화 (수신한 공개키를 이용해 암호문 및 서버용 공유 비밀키 생성)
    std::vector<uint8_t> ciphertext(kem->length_ciphertext);
    std::vector<uint8_t> shared_secret_bob(kem->length_shared_secret);
    OQS_KEM_encaps(kem, ciphertext.data(), shared_secret_bob.data(), public_key.data());
    std::cout << "[서버] 암호문(Ciphertext) 및 공유 비밀키 생성을 완료했습니다." << std::endl;

    // 6. 생성된 암호문을 클라이언트(Alice)에게 네트워크로 전송
    send(new_socket, ciphertext.data(), kem->length_ciphertext, 0);
    std::cout << "[서버] 암호문을 클라이언트에게 전송했습니다." << std::endl;

    // 결과 확인
    std::cout << "\n========================================" << std::endl;
    std::cout << "[서버 최종 결과] 안전하게 보관할 공유 비밀키: ";
    print_hex(shared_secret_bob);
    std::cout << "========================================" << std::endl;

    // 7. 자원 정리 및 소켓 닫기
    OQS_KEM_free(kem);
    close(new_socket);
    close(server_fd);

    return 0;
}