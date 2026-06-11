import xml.etree.ElementTree as ET
import matplotlib.pyplot as plt

# 1. NS-3 FlowMonitor XML 파일 불러오기
file_name = 'osm-flowmon-results.xml'
print(f"[{file_name}] 종합 성능 지표 분석을 시작합니다...")

tree = ET.parse(file_name)
root = tree.getroot()

flow_ids = []
pdrs = []
delays = []
rx_packets_list = []

# 2. 패킷 송수신 데이터 추출 및 지표 계산
for flow in root.findall('.//FlowStats/Flow'):
    flow_id = int(flow.get('flowId'))
    txPackets = float(flow.get('txPackets'))
    rxPackets = float(flow.get('rxPackets'))
    
    # NS-3의 시간 포맷(예: +12345.6ns)에서 기호(+)와 단위(ns)를 제거하고 숫자만 추출
    delay_str = flow.get('delaySum')
    delay_ns = float(delay_str.replace('+', '').replace('ns', ''))
    
    if txPackets > 0:
        # 지표 1: PDR (Packet Delivery Ratio, 전송 성공률)
        pdr = (rxPackets / txPackets) * 100.0
        
        # 지표 2: Average E2E Delay (평균 지연 시간, 단위: ms)
        if rxPackets > 0:
            # 나노초(ns)를 밀리초(ms)로 변환 (1,000,000으로 나눔)
            avg_delay_ms = (delay_ns / 1000000.0) / rxPackets
        else:
            avg_delay_ms = 0.0 # 수신 실패 시 지연 시간은 0으로 처리
            
        flow_ids.append(str(flow_id))
        pdrs.append(pdr)
        delays.append(avg_delay_ms)
        rx_packets_list.append(rxPackets)

# 3. 그래프 그리기 (상위 20개 통신 흐름)
limit = 20
flow_ids = flow_ids[:limit]
pdrs = pdrs[:limit]
delays = delays[:limit]
rx_packets_list = rx_packets_list[:limit]

# 3개의 그래프를 세로로 나란히 배치 (크기: 가로 12, 세로 14)
fig, (ax1, ax2, ax3) = plt.subplots(3, 1, figsize=(12, 14))

# [그래프 1] PDR (전송 성공률)
ax1.bar(flow_ids, pdrs, color='cornflowerblue', edgecolor='black')
ax1.set_title('1. Packet Delivery Ratio (PDR) per Flow', fontsize=14, fontweight='bold')
ax1.set_ylabel('PDR (%)', fontsize=12)
ax1.set_ylim(0, 110)
ax1.grid(axis='y', linestyle='--', alpha=0.7)

# [그래프 2] Average E2E Delay (평균 지연 시간)
ax2.bar(flow_ids, delays, color='lightcoral', edgecolor='black')
ax2.set_title('2. Average End-to-End Delay per Flow', fontsize=14, fontweight='bold')
ax2.set_ylabel('Delay (ms)', fontsize=12)
ax2.grid(axis='y', linestyle='--', alpha=0.7)

# [그래프 3] Received Packets (수신된 총 패킷 수)
ax3.bar(flow_ids, rx_packets_list, color='mediumseagreen', edgecolor='black')
ax3.set_title('3. Total Received Packets per Flow', fontsize=14, fontweight='bold')
ax3.set_xlabel('Flow ID (Communication Pair)', fontsize=12)
ax3.set_ylabel('Packets', fontsize=12)
ax3.grid(axis='y', linestyle='--', alpha=0.7)

# 레이아웃 정리 및 저장
plt.tight_layout()
save_path = 'v2x_comprehensive_analysis.png'
plt.savefig(save_path, dpi=300, bbox_inches='tight')
print(f"완료! 윈도우 탐색기에서 [{save_path}] 파일을 열어 결과를 확인하세요!")