#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h" 
#include "ns3/flow-monitor-module.h" // [NEW 1] FlowMonitor 모듈 헤더 추가

using namespace ns3;

int main (int argc, char *argv[])
{
    // 로그 출력이 너무 많아지면 시뮬레이션이 느려지므로 에코 로그는 잠시 끕니다.
    // LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    // LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);

    // 1. 차량(노드) 생성: 실제 지도는 차량이 많으므로 150대로 넉넉하게 잡습니다.
    NodeContainer vehicles;
    vehicles.Create(150);

    // 2. 이동성 부여: SUMO에서 추출한 실제 도로 주행 대본
    std::string traceFile = "scratch/osm_test/real_mobility.tcl";
    Ns2MobilityHelper ns2 = Ns2MobilityHelper(traceFile);
    ns2.Install(); 

    // [필터링] 대본에 실제로 좌표가 찍힌 진짜 차량만 골라내기
    NodeContainer realVehicles;
    for (uint32_t i = 0; i < vehicles.GetN(); ++i) {
        if (vehicles.Get(i)->GetObject<MobilityModel>() != nullptr) {
            realVehicles.Add(vehicles.Get(i));
        }
    }

    // 3. 차량에 Wi-Fi 안테나 달아주기
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    phy.Set("RxGain", DoubleValue(20.0));
    phy.Set("RxSensitivity", DoubleValue(-100.0));

    WifiMacHelper mac;
    mac.SetType("ns3::AdhocWifiMac"); 

    WifiHelper wifi;
    NetDeviceContainer devices = wifi.Install(phy, mac, realVehicles);

    // 4. 차량에 인터넷(IP 주소) 부여
    InternetStackHelper internet;
    internet.Install(realVehicles);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = ipv4.Assign(devices);

    // 5. 서버 설치 (모든 차량이 수신 대기)
    uint16_t port = 9;
    UdpEchoServerHelper echoServer(port);

    for (uint32_t i = 0; i < realVehicles.GetN(); ++i) {
        ApplicationContainer serverApps = echoServer.Install(realVehicles.Get(i));
        serverApps.Start(Seconds(1.0));
        serverApps.Stop(Seconds(100.0));
    }

    // 5-2. 클라이언트 설치 (전파를 쏘는 차량의 수를 5대로 제한)
    Ipv4Address broadcastAddress("10.1.1.255"); 

    // 전체(realVehicles.GetN()) 대신 5대(i < 5)만 브로드캐스트 전파 발사!
    for (uint32_t i = 0; i < 5; ++i) { 
        UdpEchoClientHelper echoClient(broadcastAddress, port);
        echoClient.SetAttribute("MaxPackets", UintegerValue(100)); 
        echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0))); 
        echoClient.SetAttribute("PacketSize", UintegerValue(1024));

        ApplicationContainer clientApps = echoClient.Install(realVehicles.Get(i));
        
        // 전파 충돌 방지를 위해 0.05초씩 발사 시간 분산
        clientApps.Start(Seconds(2.0 + (i * 0.05)));
        clientApps.Stop(Seconds(100.0));
    }

    // 6. 애니메이션(NetAnim) 녹화본 생성
    AnimationInterface anim("osm-vanet-animation.xml");
    anim.SetMaxPktsPerTraceFile(50000000); // 넉넉한 파일 용량 허용
    
    // [요청 반영] 애니메이션 이펙트(패킷 동심원)를 그대로 살려둡니다!
    anim.EnablePacketMetadata(true);

    // =========================================================
    // [NEW 2] FlowMonitor 설치 (네트워크에 성능 측정기 달기)
    // =========================================================
    FlowMonitorHelper flowmonHelper;
    Ptr<FlowMonitor> monitor = flowmonHelper.InstallAll();

    std::cout << "3. OSM 시뮬레이션 시작! (FlowMonitor 가동 중...)" << std::endl;

    // 7. 시뮬레이션 실행
    Simulator::Stop(Seconds(100.0));
    Simulator::Run();

    // =========================================================
    // [NEW 3] FlowMonitor 결과 저장 (시뮬레이션 종료 직후)
    // =========================================================
    monitor->SerializeToXmlFile("osm-flowmon-results.xml", true, true);
    std::cout << "4. FlowMonitor 통계 추출 완료! (osm-flowmon-results.xml 확인)" << std::endl;

    Simulator::Destroy();

    return 0;
}