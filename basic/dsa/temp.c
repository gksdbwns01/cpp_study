// C 프로그램의 실행이 시작되는 메인 함수의 진입점입니다.[cite: 1]
int main() {
    // 타원곡선(P-256) 암호화 수학 연산에 필요한 전역 파라미터들을 초기화합니다.[cite: 1]
    // 이 과정에서 소수 p, 곡선의 계수 a와 b, 생성점 G, 위수(Order) n 등의 상수값이 메모리에 설정됩니다.[cite: 1]
    EC_InitParameters(); //[cite: 1]

    // Alice의 개인키(Private Key)를 저장하기 위해 거대한 정수를 다루는 BigInt 타입의 변수를 선언합니다.[cite: 1]
    BigInt alice_priv; //[cite: 1]
    
    // Alice의 공개키(Public Key)를 저장하기 위해 타원곡선 상의 2차원 좌표(x, y)를 나타내는 EC_Point 타입의 변수를 선언합니다.[cite: 1]
    EC_Point alice_pub; //[cite: 1]
    
    // 1. Alice 키 쌍 생성
    // 안전한 난수 생성기를 사용하여 타원곡선의 위수(n) 범위 안에서 무작위 숫자를 뽑아 Alice의 개인키를 생성합니다.[cite: 1]
    EC_GeneratePrivateKey(&alice_priv); //[cite: 1]
    
    // 타원곡선의 기준이 되는 생성점(P256_G)에 Alice의 개인키만큼 스칼라 곱셈(반복 덧셈)을 수행하여 공개키를 계산합니다.[cite: 1]
    EC_Scalar_Mul(&alice_pub, &P256_G, &alice_priv, &P256_a, &P256_p); //[cite: 1]

    // 2. 전송할 원본 메시지 준비
    // 암호화 서명을 진행할 대상인 원본 평문 문자열을 메모리에 선언합니다.[cite: 1]
    const char* original_message = "ECDSA 원본 메시지"; //[cite: 1]

    // 3. 서명 생성 (메시지 원본을 직접 서명)
    // 서명 알고리즘의 최종 결과물인 두 개의 거대한 정수 값 r과 s를 담을 구조체 변수를 선언합니다.[cite: 1]
    ECDSA_Signature signature; //[cite: 1]
    
    // 원본 메시지를 내부적으로 SHA-256 해시 함수로 변환한 뒤, Alice의 개인키를 이용해 서명을 수행하고 그 결과를 저장합니다.[cite: 1]
    ECDSA_Sign_Message(&signature, original_message, &alice_priv); //[cite: 1]
    
    // 4. 서명 검증 (Verification)
    // 생성된 서명, 원본 메시지, 그리고 Alice의 공개키를 사용하여 서명의 유효성을 검증합니다.[cite: 1]
    // 복원된 좌표와 서명의 r 값이 일치하여 유효한 서명으로 판명되면 true를 반환합니다.[cite: 1]
    bool is_valid = ECDSA_Verify_Message(&signature, original_message, &alice_pub); //[cite: 1]
    
    // 5. 서명 위변조 테스트
    // 메시지 내용이 변경되었을 때 서명이 무효화되는지 확인하기 위해 가짜 메시지를 생성합니다.[cite: 1]
    const char* fake_message = "해커가 변조한 가짜 메시지"; //[cite: 1]
    
    // 조작된 메시지와 원본 서명, 그리고 공개키를 이용해 다시 서명 검증을 시도합니다.[cite: 1]
    // 메시지 내용이 변경되어 해시값이 원본과 달라졌으므로, 이 결과는 반드시 false로 반환됩니다.[cite: 1]
    bool is_fake_valid = ECDSA_Verify_Message(&signature, fake_message, &alice_pub); //[cite: 1]

    // 모든 과정을 마치고 운영체제에 성공 상태 코드(0)를 반환하며 프로그램을 종료합니다.[cite: 1]
    return 0; //[cite: 1]
}