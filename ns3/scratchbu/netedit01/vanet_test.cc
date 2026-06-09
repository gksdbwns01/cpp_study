#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"

using namespace ns3;

int main (int argc, char *argv[])
{
    // 1. 차량(노드) 생성: TCL 파일에 기록된 차량 대수만큼 넉넉하게 50대를 만듭니다.
    NodeContainer vehicles;
    vehicles.Create(50);

    // 2. 이동성(Mobility) 부여: 방금 만든 TCL 대본 파일을 불러옵니다.
    // 주의: 경로는 ns-3 실행 폴더 기준이므로 아래와 같이 적습니다.
    std::string traceFile = "scratch/netedit01/cross_mobility.tcl";
    
    Ns2MobilityHelper ns2 = Ns2MobilityHelper(traceFile);
    ns2.Install(); // 50대의 차량에 SUMO의 움직임을 그대로 덮어씌웁니다.

    std::cout << "SUMO 차량 이동 기록 로드 완료!" << std::endl;

    // 3. 시뮬레이션 실행 (SUMO에서 100초까지 설정했으므로 동일하게 100초로 맞춤)
    Simulator::Stop(Seconds(100.0));
    Simulator::Run();
    Simulator::Destroy();

    std::cout << "NS-3 VANET 이동성 시뮬레이션 종료." << std::endl;
    return 0;
}