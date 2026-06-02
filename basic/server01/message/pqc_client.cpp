#include <iostream>
#include <vector>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <oqs/oqs.h>

#define PORT 8080

// 간단한 XOR 비트 연산 기반의 경량 대칭키 암호화/복호화 함수
std::vector<uint8_t> xor_cipher(const std::string& text, const std::vector<uint8_t>& key) {
    std::vector<uint8_t> result(text.length());
    for (size_t i = 0; i < text.length(); ++i) {
        result[i] = text[i] ^ key[i % key.size()]; // 비트 단위 XOR 연산
    }
    return result;
}

int main() {
    // 1~3. KEM 설정 및 서버 접속 (이전과 동일)
    OQS_KEM *kem = OQS_KEM_new(OQS_KEM_alg_ml_kem_768);
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
    connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    // 4~7. 양자 내성 키 교환 진행 (이전과 동일)
    std::vector<uint8_t> public_key(kem->length_public_key);
    std::vector<uint8_t> secret_key(kem->length_secret_key);
    OQS_KEM_keypair(kem, public_key.data(), secret_key.data());
    send(sock, public_key.data(), kem->length_public_key, 0);

    std::vector<uint8_t> ciphertext(kem->length_ciphertext);
    read(sock, ciphertext.data(), kem->length_ciphertext);

    std::vector<uint8_t> shared_secret_alice(kem->length_shared_secret);
    OQS_KEM_decaps(kem, shared_secret_alice.data(), ciphertext.data(), secret_key.data());
    std::cout << "[클라이언트] 공유 비밀키 생성 완료!" << std::endl;

    // ==========================================
    // [신규 추가] 8. 실제 메시지 암호화 및 전송
    // ==========================================
    std::string secret_message = "TOP SECRET: Target confirmed at coordinate 35.8, 128.5";
    std::cout << "\n[클라이언트] 원본 메시지: " << secret_message << std::endl;

    // 공유 비밀키를 이용해 메시지를 비트 연산(XOR)으로 암호화
    std::vector<uint8_t> encrypted_msg = xor_cipher(secret_message, shared_secret_alice);
    
    // 암호화된 메시지 전송
    send(sock, encrypted_msg.data(), encrypted_msg.size(), 0);
    std::cout << "[클라이언트] 암호화된 데이터를 서버로 전송했습니다." << std::endl;

    OQS_KEM_free(kem);
    close(sock);
    return 0;
}