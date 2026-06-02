#include <iostream>
#include <vector>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <oqs/oqs.h>

#define PORT 8080

// XOR 암호화/복호화 함수 (암호화할 때와 똑같은 연산을 한 번 더 하면 원본이 나옵니다)
std::string xor_decipher(const std::vector<uint8_t>& encrypted_data, const std::vector<uint8_t>& key) {
    std::string result(encrypted_data.size(), '\0');
    for (size_t i = 0; i < encrypted_data.size(); ++i) {
        result[i] = encrypted_data[i] ^ key[i % key.size()];
    }
    return result;
}

int main() {
    // 1~3. KEM 설정 및 소켓 대기 (이전과 동일)
    OQS_KEM *kem = OQS_KEM_new(OQS_KEM_alg_ml_kem_768);
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
    
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 3);
    
    int addrlen = sizeof(address);
    int new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);

    // 4~6. 양자 내성 키 교환 진행 (이전과 동일)
    std::vector<uint8_t> public_key(kem->length_public_key);
    read(new_socket, public_key.data(), kem->length_public_key);

    std::vector<uint8_t> ciphertext(kem->length_ciphertext);
    std::vector<uint8_t> shared_secret_bob(kem->length_shared_secret);
    OQS_KEM_encaps(kem, ciphertext.data(), shared_secret_bob.data(), public_key.data());
    
    send(new_socket, ciphertext.data(), kem->length_ciphertext, 0);
    std::cout << "[서버] 공유 비밀키 생성 완료!" << std::endl;

    // ==========================================
    // [신규 추가] 7. 암호화된 메시지 수신 및 복호화
    // ==========================================
    // 넉넉한 버퍼를 준비하여 암호화된 데이터를 수신
    std::vector<uint8_t> buffer(1024);
    int bytes_read = read(new_socket, buffer.data(), buffer.size());
    buffer.resize(bytes_read); // 실제 읽어온 크기만큼 버퍼 조정

    std::cout << "\n[서버] 암호화된 데이터를 수신했습니다. (가로채도 읽을 수 없는 형태)" << std::endl;

    // 공유 비밀키를 이용해 수신한 데이터를 비트 연산(XOR)으로 복호화
    std::string decrypted_msg = xor_decipher(buffer, shared_secret_bob);
    
    std::cout << "========================================" << std::endl;
    std::cout << "[서버 최종 결과] 복호화된 메시지: " << decrypted_msg << std::endl;
    std::cout << "========================================" << std::endl;

    OQS_KEM_free(kem);
    close(new_socket);
    close(server_fd);
    return 0;
}