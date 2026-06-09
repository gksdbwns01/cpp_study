#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

// 화면에 로그(출력문)를 띄우기 위한 이름표 설정
NS_LOG_COMPONENT_DEFINE ("FirstScriptExample");

int main (int argc, char *argv[])
{
  // 1. 로그 활성화: 클라이언트와 서버가 패킷을 주고받는 과정을 화면에 출력하도록 켭니다.
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  // 2. 노드(컴퓨터) 생성: 빈 껍데기 컴퓨터 2대를 만듭니다.
  NodeContainer nodes;
  nodes.Create (2);

  // 3. 랜선(링크) 특성 설정: 속도는 5Mbps, 지연 시간(Ping)은 2ms로 설정합니다.
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  // 4. 랜카드 장착 및 연결: 위에서 만든 컴퓨터들에 랜카드를 꽂고, 3번에서 만든 랜선으로 서로를 잇습니다.
  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes);

  // 5. 운영체제(네트워크 스택) 설치: 컴퓨터가 통신할 수 있게 TCP/IP 프로토콜을 설치합니다.
  InternetStackHelper stack;
  stack.Install (nodes);

  // 6. IP 주소 할당: 10.1.1.0 대역의 IP를 두 컴퓨터에 각각 부여합니다. (10.1.1.1, 10.1.1.2)
  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  // 7. 서버 프로그램 설치: Node 1번에 에코 서버(받은 걸 그대로 돌려주는 프로그램)를 9번 포트에 설치합니다.
  UdpEchoServerHelper echoServer (9);
  ApplicationContainer serverApps = echoServer.Install (nodes.Get (1));
  serverApps.Start (Seconds (1.0)); // 시뮬레이션 1초에 서버 켜기
  serverApps.Stop (Seconds (10.0)); // 10초에 서버 끄기

  // 8. 클라이언트 프로그램 설치: Node 0번에 설치하며, '서버(Node 1)의 IP'와 '9번 포트'를 목적지로 설정합니다.
  UdpEchoClientHelper echoClient (interfaces.GetAddress (1), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1)); // 딱 1개의 패킷만 보냅니다.
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0))); // 1초 간격으로 보냅니다.
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024)); // 패킷 크기는 1024바이트.

  ApplicationContainer clientApps = echoClient.Install (nodes.Get (0));
  clientApps.Start (Seconds (2.0)); // 2초에 클라이언트가 전송 시작!
  clientApps.Stop (Seconds (10.0));

  // 9. 시뮬레이션 엔진 가동!
  Simulator::Run ();
  Simulator::Destroy ();
  
  return 0;
}

// ./ns3 run p2p_sim