#include <iostream>
#include <vector>
#include <cstring>
#include <oqs/oqs.h> // liboqs(Open Quantum Safe) 라이브러리 헤더: 양자 내성 암호 API 제공

int main() {
    // =========================================================================
    // 1. 알고리즘 선택 및 객체 초기화 (NIST 표준 ML-KEM-768)
    // =========================================================================
    
    // 미국 표준기술연구소(NIST)가 채택한 격자 기반 양자 내성 암호 표준 알고리즘명 지정
    const char *alg_name = OQS_KEM_alg_ml_kem_768; 
    
    // 선택한 알고리즘을 구동하기 위한 KEM 객체를 메모리에 동적 할당
    OQS_KEM *kem = OQS_KEM_new(alg_name);
    
    // 객체 생성 실패 시(예: 지원하지 않는 알고리즘 등) 예외 처리 후 프로그램 종료
    if (kem == nullptr) {
        std::cerr << "KEM 알고리즘 초기화 실패!" << std::endl;
        return 1;
    }

    // 초기화된 구조체 내부에 저장된 실제 알고리즘의 공식 명칭을 출력
    std::cout << "사용 중인 양자 내성 알고리즘: " << kem->method_name << std::endl;

    // =========================================================================
    // 2. 안전한 메모리 공간 할당 (Buffer Initialization)
    // =========================================================================
    // 양자 내성 암호는 일반 암호보다 키 크기가 크므로, 오버플로우 방지를 위해 
    // 라이브러리(kem 객체)가 요구하는 정확한 바이트 크기만큼 std::vector로 동적 배열 선언
    
    // Alice의 공개키(자물쇠) 저장 공간: 외부 통신망으로 전달되는 용도
    std::vector<uint8_t> public_key(kem->length_public_key);
    
    // Alice의 비밀키(열쇠) 저장 공간: Alice 자신만 절대적으로 격리·보관해야 하는 내부 용도
    std::vector<uint8_t> secret_key(kem->length_secret_key);
    
    // Bob이 비밀번호를 숨겨서 보낼 암호 상자(캡슐화된 암호문) 저장 공간
    std::vector<uint8_t> ciphertext(kem->length_ciphertext);
    
    // Bob 측에서 무작위로 생성하여 최종 획득할 공유 비밀키(대칭키) 저장 공간
    std::vector<uint8_t> shared_secret_bob(kem->length_shared_secret);
    
    // Alice 측에서 Bob의 암호 상자를 열어 최종 획득할 공유 비밀키(대칭키) 저장 공간
    std::vector<uint8_t> shared_secret_alice(kem->length_shared_secret);

    // =========================================================================
    // [Step 1] Alice: 키 쌍(Keypair) 생성
    // =========================================================================
    // 수학적으로 긴밀하게 연결된 한 쌍의 자물쇠(공개키)와 열쇠(비밀키)를 동시에 생성
    // .data()를 사용해 벡터의 실제 내부 버퍼 시작 메모리 주소(raw pointer)를 전달
    std::cout << "\n[Alice] 키 쌍 생성 중..." << std::endl;
    OQS_KEM_keypair(kem, public_key.data(), secret_key.data());

    // =========================================================================
    // [Step 2] Bob: 공유 비밀키 생성 및 캡슐화(Encapsulation)
    // =========================================================================
    // Bob이 Alice의 공개키(public_key)를 입력받아 내부적으로 무작위 공유키를 생성하고,
    // 이를 Alice만 열 수 있도록 공개키로 안전하게 밀봉(캡슐화)하여 암호문(ciphertext)을 만들어냄
    std::cout << "[Bob] 공유 비밀키 생성 및 암호문(Ciphertext) 캡슐화 중..." << std::endl;
    OQS_KEM_encaps(kem, ciphertext.data(), shared_secret_bob.data(), public_key.data());

    // =========================================================================
    // [Step 3] Alice: 암호문 캡슐 해제 및 역캡슐화(Decapsulation)
    // =========================================================================
    // 인터넷을 통해 Bob의 암호 상자(ciphertext)를 받은 Alice가 
    // 본인이 소중히 보관하던 고유 열쇠(secret_key)를 사용해 상자를 열어 비밀번호를 추출함
    std::cout << "[Alice] 암호문 캡슐 해제(Decapsulation) 중..." << std::endl;
    OQS_KEM_decaps(kem, shared_secret_alice.data(), ciphertext.data(), secret_key.data());

    // =========================================================================
    // [Step 4] 검증 (Verification)
    // =========================================================================
    // Bob이 만들어낸 비밀번호와 Alice가 역캡슐화로 꺼낸 비밀번호가 완전히 일치하는지 비교
    std::cout << "\n[결과 확인]" << std::endl;
    
    // std::memcmp를 통해 두 비밀키 버퍼의 메모리 바이너리를 바이트 단위로 정밀 비교
    if (std::memcmp(shared_secret_bob.data(), shared_secret_alice.data(), kem->length_shared_secret) == 0) {
        // 일치할 경우: 키 교환 프로토콜 성공 (이후 이 대칭키로 AES 암호화 통신 진행 가능)
        std::cout << "=> 성공! Alice와 Bob이 동일한 비밀키를 안전하게 공유했습니다." << std::endl;
    } else {
        // 불일치할 경우: 위조, 도청, 혹은 메모리 오염으로 인한 프로토콜 실패
        std::cout << "=> 실패! 비밀키가 일치하지 않습니다." << std::endl;
    }

    // =========================================================================
    // 5. 자원 반환 (Memory Cleanup)
    // =========================================================================
    // C++ 환경에서 발생할 수 있는 메모리 누수(Memory Leak)를 원천 차단하기 위해
    // 프로그램 종료 직전 OQS_KEM_new로 동적 할당했던 KEM 구조체 인스턴스를 수동 해제
    OQS_KEM_free(kem);

    return 0;
}