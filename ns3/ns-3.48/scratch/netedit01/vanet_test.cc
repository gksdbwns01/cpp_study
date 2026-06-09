#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h" // [NEW] 애니메이션 모듈 추가

using namespace ns3;

int main (int argc, char *argv[])
{
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);

    // 1. 차량(노드) 50대 생성 및 SUMO 이동성 부여
    NodeContainer vehicles;
    vehicles.Create(50);

    std::string traceFile = "scratch/netedit01/cross_mobility.tcl";
    Ns2MobilityHelper ns2 = Ns2MobilityHelper(traceFile);
    ns2.Install(); 

    // [필터링] 좌표가 있는 진짜 차량만 골라내기
    NodeContainer realVehicles;
    for (uint32_t i = 0; i < vehicles.GetN(); ++i) {
        if (vehicles.Get(i)->GetObject<MobilityModel>() != nullptr) {
            realVehicles.Add(vehicles.Get(i));
        }
    }

    // 2. 차량에 Wi-Fi 안테나 달아주기
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    // =========================================================
    // [NEW] 통신 성능 강제 업그레이드 (수신 감도 극대화)
    // =========================================================
    phy.Set("RxGain", DoubleValue(20.0));        // 수신 안테나 성능 뻥튀기
    phy.Set("RxSensitivity", DoubleValue(-100.0)); // 미세한 신호도 다 잡아냄

    WifiMacHelper mac;
    mac.SetType("ns3::AdhocWifiMac"); 

    WifiHelper wifi;
    NetDeviceContainer devices = wifi.Install(phy, mac, realVehicles);

    // 3. 차량에 인터넷(IP 주소) 부여
    InternetStackHelper internet;
    internet.Install(realVehicles);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = ipv4.Assign(devices);

    // 4. 통신 어플리케이션 설치 (0번 차량 ↔ 1번 차량)
    UdpEchoServerHelper echoServer(9);
    ApplicationContainer serverApps = echoServer.Install(realVehicles.Get(0));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(100.0));

    UdpEchoClientHelper echoClient(interfaces.GetAddress(0), 9);
    echoClient.SetAttribute("MaxPackets", UintegerValue(100)); 
    echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0))); 
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApps = echoClient.Install(realVehicles.Get(1));
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(100.0));

    // Pcap 녹화
    phy.EnablePcap("vanet-test", devices);

    // =========================================================
    // [NEW] 애니메이션(NetAnim) 녹화본 생성 및 시각적 꾸미기
    // =========================================================
    AnimationInterface anim("vanet-animation.xml"); // 파일명 지정

    // 화면에서 차량이 잘 보이도록 크기와 색상 지정
    for (uint32_t i = 0; i < realVehicles.GetN(); ++i) {
        anim.UpdateNodeSize(realVehicles.Get(i)->GetId(), 5.0, 5.0); // 차량 크기 확대
    }
    // 주인공 차량들 눈에 띄게 색상 변경 (R, G, B)
    anim.UpdateNodeColor(realVehicles.Get(0), 255, 0, 0); // 0번 서버 차량: 빨간색
    anim.UpdateNodeColor(realVehicles.Get(1), 0, 255, 0); // 1번 클라이언트: 초록색
    // [수정된 부분] 애니메이션 녹화 시작 명령 추가
    anim.EnablePacketMetadata(true); // 패킷이 움직이는 것도 보이게 설정
    anim.EnableIpv4RouteTracking("routing-table.xml", Seconds(0), Seconds(100), Seconds(1));
    
    std::cout << "3. 시뮬레이션 시작!" << std::endl;

    // 6. 시뮬레이션 실행
    Simulator::Stop(Seconds(100.0));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}