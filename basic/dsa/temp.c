int main() {
    // 타원곡선 파라미터 초기화
    EC_InitParameters(); //[cite: 1]

    BigInt alice_priv; //[cite: 1]
    EC_Point alice_pub; //[cite: 1]
    
    // 1. Alice 키 쌍 생성
    EC_GeneratePrivateKey(&alice_priv); //[cite: 1]
    EC_Scalar_Mul(&alice_pub, &P256_G, &alice_priv, &P256_a, &P256_p); //[cite: 1]

    // ... (Alice 키 출력 부분 생략) ...

    // 2. 전송할 원본 메시지 준비
    const char* original_message = "ECDSA 원본 메시지"; //[cite: 1]

    // 3. 서명 생성 (메시지 원본을 직접 서명)
    ECDSA_Signature signature; //[cite: 1]
    ECDSA_Sign_Message(&signature, original_message, &alice_priv); //[cite: 1]
    
    // ... (생성된 서명 r, s 출력 부분 생략) ...

    // 4. 서명 검증 (Verification)
    bool is_valid = ECDSA_Verify_Message(&signature, original_message, &alice_pub); //[cite: 1]
    
    // ... (서명 유효성 검증 성공/실패 결과 출력 생략) ...

    // 5. 서명 위변조 테스트
    const char* fake_message = "해커가 변조한 가짜 메시지"; //[cite: 1]
    bool is_fake_valid = ECDSA_Verify_Message(&signature, fake_message, &alice_pub); //[cite: 1]

    // ... (위변조 검증 성공/실패 결과 출력 생략) ...

    return 0; //[cite: 1]
}