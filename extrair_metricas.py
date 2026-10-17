import xml.etree.ElementTree as ET
import csv
import sys

def extrair_dados_flowmon(xml_file, csv_file):
    try:
        tree = ET.parse(xml_file)
        root = tree.getroot()
    except FileNotFoundError:
        print(f"Erro: Arquivo {xml_file} não encontrado. Rode a simulação primeiro.")
        return

    # Abre o arquivo CSV para escrita
    with open(csv_file, mode='w', newline='') as arquivo_csv:
        writer = csv.writer(arquivo_csv)
        # Cabeçalho da tabela
        writer.writerow(['FlowID', 'TxPackets', 'RxPackets', 'TxBytes', 'RxBytes', 'LossRatio(%)', 'MeanDelay(ms)'])

        flow_stats = root.find('FlowStats')
        if flow_stats is None:
            print("Nenhuma estatística encontrada no arquivo XML.")
            return

        for flow in flow_stats.findall('Flow'):
            flow_id = flow.get('flowId')
            tx_packets = int(flow.get('txPackets', 0))
            rx_packets = int(flow.get('rxPackets', 0))
            tx_bytes = flow.get('txBytes', '0')
            rx_bytes = flow.get('rxBytes', '0')
            
            # Cálculo de Perda de Pacotes (Packet Loss Ratio)
            if tx_packets > 0:
                loss_ratio = ((tx_packets - rx_packets) / tx_packets) * 100
            else:
                loss_ratio = 0.0

            # Cálculo do Atraso Médio em milissegundos
            delay_sum = flow.get('delaySum', '0ns')
            # O FlowMonitor salva o tempo com um "+", uma string numérica e "ns". Ex: "+12345ns"
            if delay_sum.endswith('ns'):
                delay_ns = float(delay_sum.replace('+', '').replace('ns', ''))
                mean_delay_ms = (delay_ns / rx_packets) / 1e6 if rx_packets > 0 else 0.0
            else:
                mean_delay_ms = 0.0

            writer.writerow([
                flow_id, 
                tx_packets, 
                rx_packets, 
                tx_bytes, 
                rx_bytes, 
                f"{loss_ratio:.2f}", 
                f"{mean_delay_ms:.4f}"
            ])

    print(f"Sucesso! Métricas exportadas para {csv_file}")

if __name__ == "__main__":
    arquivo_entrada = "gpsr-sumo-results.xml"
    arquivo_saida = "resultados_gpsr.csv"
    extrair_dados_flowmon(arquivo_entrada, arquivo_saida)
