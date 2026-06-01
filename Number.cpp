#include <iostream>
#include <cstdlib>
#include <ctime>

using namespace std;

int main() {
    // 매번 다른 무작위 숫자를 생성하기 위해 시드 설정
    srand(time(0));
    
    // 1부터 100 사이의 무작위 숫자 생성
    int secretNumber = rand() % 100 + 1;
    int guess = 0;
    int attempts = 0;

    cout << "============= 숫자 맞추기 게임 =============" << endl;
    cout << "1부터 100 사이의 숫자를 맞춰보세요!" << endl;

    // 사용자가 정답을 맞출 때까지 반복
    while (guess != secretNumber) {
        cout << "숫자를 입력하세요: ";
        cin >> guess;
        attempts++; // 시도 횟수 증가

        if (guess > secretNumber) {
            cout << "더 작은 숫자입니다! 👇" << endl;
        } else if (guess < secretNumber) {
            cout << "더 큰 숫자입니다! 👆" << endl;
        } else {
            cout << "\n🎉 축하합니다! 정답입니다!" << endl;
            cout << "시도한 횟수: " << attempts << "번" << endl;
        }
        cout << "--------------------------------------------" << endl;
    }

    return 0;
}