#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h" 
#include "ns3/flow-monitor-module.h"

using namespace ns3;

int main (int argc, char *argv[])
{
    // =========================================================
    // 출력 파일이 저장될 폴더 경로를 변수로 지정합니다.
    // =========================================================
    std::string outDir = "scratch/osm_test02/";

    NodeContainer vehicles;
    vehicles.Create(50);

    // 이동성 대본 경로
    std::string traceFile = outDir + "real_mobility.tcl";
    Ns2MobilityHelper ns2 = Ns2MobilityHelper(traceFile);
    ns2.Install(); 

    NodeContainer realVehicles;
    for (uint32_t i = 0; i < vehicles.GetN(); ++i) {
        if (vehicles.Get(i)->GetObject<MobilityModel>() != nullptr) {
            realVehicles.Add(vehicles.Get(i));
        }
    }

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    // =========================================================
    // [수정 완료 1] 802.11p (차량용 통신) 표준 적용
    // 비현실적인 RxGain 치트키를 삭제하고, WIFI_STANDARD_80211p를
    // 명시하여 현실적인 통신 반경(약 150m 내외)을 갖도록 설정합니다.
    // =========================================================
    WifiMacHelper mac;
    // OcbWifiMac 대신 기존의 AdhocWifiMac을 그대로 사용합니다.
    mac.SetType("ns3::AdhocWifiMac"); 

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211p);
    NetDeviceContainer devices = wifi.Install(phy, mac, realVehicles);

    InternetStackHelper internet;
    internet.Install(realVehicles);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = ipv4.Assign(devices);

    uint16_t port = 9;

    // =========================================================
    // [수정 완료 2-1] 수신 전용 서버 (답장 안 함)
    // UdpEchoServer 대신 일반 UdpServer를 사용하여 수신만 하도록 설정
    // =========================================================
    UdpServerHelper oneWayServer(port);

    for (uint32_t i = 0; i < realVehicles.GetN(); ++i) {
        ApplicationContainer serverApps = oneWayServer.Install(realVehicles.Get(i));
        serverApps.Start(Seconds(1.0));
        serverApps.Stop(Seconds(100.0));
    }

    // =========================================================
    // [수정 완료 2-2] 단방향 브로드캐스트 클라이언트
    // UdpEchoClient 대신 일반 UdpClient를 사용하여 전송만 하도록 설정
    // =========================================================
    // [수정됨] 브로드캐스트(10.1.1.255) 대신, 특정 목적지(예: 6번째 차량의 IP) 지정
    // 10.1.1.0 네트워크에서 노드들은 10.1.1.1 부터 IP를 할당받으므로 10.1.1.6을 타겟으로 합니다.
    Ipv4Address destAddress("10.1.1.6");
    for (uint32_t i = 0; i < 5; ++i) { 
        UdpClientHelper oneWayClient(destAddress, port);
        oneWayClient.SetAttribute("MaxPackets", UintegerValue(100)); 
        oneWayClient.SetAttribute("Interval", TimeValue(Seconds(1.0))); 
        oneWayClient.SetAttribute("PacketSize", UintegerValue(1024));

        ApplicationContainer clientApps = oneWayClient.Install(realVehicles.Get(i));
        clientApps.Start(Seconds(2.0 + (i * 0.05)));
        clientApps.Stop(Seconds(100.0));
    }

    // 파일 이름 앞에 경로(outDir) 추가
    AnimationInterface anim(outDir + "osm-vanet-animation.xml");
    anim.SetMaxPktsPerTraceFile(50000000); 
    anim.EnablePacketMetadata(true);

    FlowMonitorHelper flowmonHelper;
    Ptr<FlowMonitor> monitor = flowmonHelper.InstallAll();

    std::cout << "3. OSM 시뮬레이션 시작! (현실적 통신 반경 및 단방향 브로드캐스트 적용됨)" << std::endl;

    Simulator::Stop(Seconds(100.0));
    Simulator::Run();

    // 파일 이름 앞에 경로(outDir) 추가
    monitor->SerializeToXmlFile(outDir + "osm-flowmon-results.xml", true, true);
    std::cout << "4. FlowMonitor 통계 추출 완료! (" << outDir << " 확인)" << std::endl;

    Simulator::Destroy();

    return 0;
}