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
    // [NEW] 출력 파일이 저장될 폴더 경로를 변수로 지정합니다.
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
    // [수정됨] 비현실적인 RxGain 치트키 삭제! 
    // 이제 NS-3의 기본 WAVE(802.11p) 스펙이 적용되어 현실적인 
    // 통신 반경(약 150m) 안에서만 패킷이 전달됩니다.
    // =========================================================

    WifiMacHelper mac;
    mac.SetType("ns3::AdhocWifiMac"); 

    WifiHelper wifi;
    NetDeviceContainer devices = wifi.Install(phy, mac, realVehicles);

    InternetStackHelper internet;
    internet.Install(realVehicles);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = ipv4.Assign(devices);

    uint16_t port = 9;
    UdpEchoServerHelper echoServer(port);

    for (uint32_t i = 0; i < realVehicles.GetN(); ++i) {
        ApplicationContainer serverApps = echoServer.Install(realVehicles.Get(i));
        serverApps.Start(Seconds(1.0));
        serverApps.Stop(Seconds(100.0));
    }

    Ipv4Address broadcastAddress("10.1.1.255"); 
    for (uint32_t i = 0; i < 5; ++i) { 
        UdpEchoClientHelper echoClient(broadcastAddress, port);
        echoClient.SetAttribute("MaxPackets", UintegerValue(100)); 
        echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0))); 
        echoClient.SetAttribute("PacketSize", UintegerValue(1024));

        ApplicationContainer clientApps = echoClient.Install(realVehicles.Get(i));
        clientApps.Start(Seconds(2.0 + (i * 0.05)));
        clientApps.Stop(Seconds(100.0));
    }

    // 파일 이름 앞에 경로(outDir) 추가
    AnimationInterface anim(outDir + "osm-vanet-animation.xml");
    anim.SetMaxPktsPerTraceFile(50000000); 
    anim.EnablePacketMetadata(true);

    FlowMonitorHelper flowmonHelper;
    Ptr<FlowMonitor> monitor = flowmonHelper.InstallAll();

    std::cout << "3. OSM 시뮬레이션 시작! (현실적 통신 반경 적용됨)" << std::endl;

    Simulator::Stop(Seconds(100.0));
    Simulator::Run();

    // 파일 이름 앞에 경로(outDir) 추가
    monitor->SerializeToXmlFile(outDir + "osm-flowmon-results.xml", true, true);
    std::cout << "4. FlowMonitor 통계 추출 완료! (" << outDir << " 확인)" << std::endl;

    Simulator::Destroy();

    return 0;
}