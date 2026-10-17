import pandas as pd
import matplotlib.pyplot as plt

# Configuração de estilo académico (compatível com LaTeX)
plt.rcParams.update({
    "font.family": "serif",
    "font.size": 12,
    "axes.grid": True,
    "grid.linestyle": "--",
    "grid.alpha": 0.7
})

protocolos = ['AODV', 'OLSR', 'GPSR']
arquivos = ['resultados_aodv.csv', 'resultados_olsr.csv', 'resultados_gpsr.csv']
cores = ['#ff9999', '#66b3ff', '#99ff99']

pdrs = []
delays = []

# Processar os 3 ficheiros de simulação
for arq in arquivos:
    try:
        df = pd.read_csv(arq)
        # Calcula o PDR total
        total_tx = df['TxPackets'].sum()
        total_rx = df['RxPackets'].sum()
        pdr = (total_rx / total_tx) * 100 if total_tx > 0 else 0
        
        # Calcula a média do atraso
        mean_delay = df['MeanDelay(ms)'].mean()
        
        pdrs.append(pdr)
        delays.append(mean_delay)
    except Exception as e:
        print(f"Aviso: Não foi possível ler {arq} - {e}")
        pdrs.append(0)
        delays.append(0)

# ==========================================
# Gráfico 1: PDR Comparativo
# ==========================================
plt.figure(figsize=(7, 5))
barras = plt.bar(protocolos, pdrs, color=cores, edgecolor='black', width=0.5)
plt.ylabel('Packet Delivery Ratio - PDR (%)')
plt.title('Comparação de Taxa de Entrega (Mobilidade Realista)')

# Ajusta o eixo Y dinamicamente consoante o PDR máximo
max_pdr = max(pdrs) if max(pdrs) > 0 else 100
plt.ylim(0, max_pdr * 1.2)

# Adiciona os valores percentuais no topo de cada barra
for barra in barras:
    yval = barra.get_height()
    plt.text(barra.get_x() + barra.get_width()/2, yval + (max_pdr * 0.02), f'{yval:.1f}%', ha='center', va='bottom', fontweight='bold')

plt.tight_layout()
plt.savefig('comparativo_pdr.pdf')
plt.close()

# ==========================================
# Gráfico 2: Atraso Comparativo
# ==========================================
plt.figure(figsize=(7, 5))
barras = plt.bar(protocolos, delays, color=cores, edgecolor='black', width=0.5)
plt.ylabel('Atraso Médio Fim-a-Fim (ms)')
plt.title('Comparação de Latência (Mobilidade Realista)')

# Ajusta o eixo Y dinamicamente
max_delay = max(delays) if max(delays) > 0 else 1
plt.ylim(0, max_delay * 1.2) 

# Adiciona os valores no topo de cada barra
for barra in barras:
    yval = barra.get_height()
    plt.text(barra.get_x() + barra.get_width()/2, yval + (max_delay * 0.02), f'{yval:.4f}', ha='center', va='bottom', fontweight='bold')

plt.tight_layout()
plt.savefig('comparativo_delay.pdf')
plt.close()

print("Sucesso! Gráficos gerados: 'comparativo_pdr.pdf' e 'comparativo_delay.pdf'")
