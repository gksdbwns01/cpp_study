import xml.etree.ElementTree as ET
import matplotlib.pyplot as plt
import os

# 1. 파일 경로 설정 (NS-3 시뮬레이션 폴더 기준)
target_dir = 'scratch/osm_test02/'
file_name = os.path.join(target_dir, 'osm-flowmon-results.xml')

print(f"[{file_name}] 파일 분석을 시작합니다...")

try:
    tree = ET.parse(file_name)
    root = tree.getroot()
except FileNotFoundError:
    print(f"오류: {file_name} 파일을 찾을 수 없습니다. 시뮬레이션을 먼저 실행해주세요.")
    exit()

flow_ids = []
pdrs = []
delays = []
rx_packets_list = []

# 2. 패킷 송수신 데이터 추출 및 지표 계산
# FlowMonitor.xml 구조를 탐색하여 데이터 수집
for flow in root.findall('.//FlowStats/Flow'):
    flow_id = int(flow.get('flowId'))
    txPackets = float(flow.get('txPackets'))
    rxPackets = float(flow.get('rxPackets'))
    
    # NS-3의 시간 포맷(예: +12345.6ns)에서 기호(+)와 단위(ns)를 제거하고 숫자만 추출
    delay_str = flow.get('delaySum')
    delay_ns = float(delay_str.replace('+', '').replace('ns', ''))
    
    if txPackets > 0:
        # 지표 1: PDR (Packet Delivery Ratio, 전송 성공률)
        # 브로드캐스트의 경우 목적지 노드가 여러 개일 수 있으므로 수치가 100%를 넘을 수도 있습니다.
        pdr = (rxPackets / txPackets) * 100.0
        
        # 지표 2: Average E2E Delay (평균 지연 시간, 단위: ms)
        if rxPackets > 0:
            # 나노초(ns)를 밀리초(ms)로 변환 (1,000,000으로 나눔)
            avg_delay_ms = (delay_ns / 1000000.0) / rxPackets
        else:
            avg_delay_ms = 0.0 # 수신 실패 시 지연 시간은 0
            
        # 데이터를 리스트에 저장
        flow_ids.append(str(flow_id))
        pdrs.append(pdr)
        delays.append(avg_delay_ms)
        rx_packets_list.append(rxPackets)

# 3. 그래프 그리기 (상위 20개 Flow만 추출하여 가독성 확보)
limit = 20
flow_ids = flow_ids[:limit]
pdrs = pdrs[:limit]
delays = delays[:limit]
rx_packets_list = rx_packets_list[:limit]

# 3개의 그래프를 세로로 나란히 배치
fig, (ax1, ax2, ax3) = plt.subplots(3, 1, figsize=(10, 12))

# [그래프 1] PDR (전송 성공률)
ax1.bar(flow_ids, pdrs, color='cornflowerblue', edgecolor='black')
ax1.set_title('1. Packet Delivery Ratio (PDR) per Flow', fontsize=14, fontweight='bold')
ax1.set_ylabel('PDR (%)', fontsize=12)
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
save_path = os.path.join(target_dir, 'v2x_comprehensive_analysis.png')
plt.savefig(save_path, dpi=300, bbox_inches='tight')

print(f"분석 완료! 그래프가 다음 경로에 저장되었습니다:")
print(f"👉 {os.path.abspath(save_path)}")