#include <iostream>
#include <oqs/oqs.h>

int main() {
    // liboqs가 정상적으로 로드되었는지 버전 정보 출력
    std::cout << "liboqs 라이브러리 연동 성공!" << std::endl;
    std::cout << "설치된 버전: " << OQS_version() << std::endl;
    
    return 0;
}