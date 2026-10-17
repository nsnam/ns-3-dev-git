import pandas as pd
import matplotlib.pyplot as plt

# Configuração de estilo acadêmico (fontes serifadas casam bem com LaTeX)
plt.rcParams.update({
    "font.family": "serif",
    "font.size": 12,
    "axes.grid": True,
    "grid.linestyle": "--",
    "grid.alpha": 0.7
})

# 1. Carregar os dados
df = pd.read_csv('resultados_gpsr.csv')

# 2. Calcular o PDR (Packet Delivery Ratio)
df['PDR(%)'] = (df['RxPackets'] / df['TxPackets']) * 100

# 3. Gráfico 1: Taxa de Entrega (PDR)
plt.figure(figsize=(6, 4))
plt.bar(df['FlowID'].astype(str), df['PDR(%)'], color='skyblue', edgecolor='black', width=0.4)
plt.xlabel('Fluxo de Dados (Flow ID)')
plt.ylabel('Packet Delivery Ratio - PDR (%)')
plt.title('Taxa de Entrega de Pacotes (GPSR)')
plt.ylim(0, 110) # Margem superior para o gráfico respirar
plt.tight_layout()
plt.savefig('grafico_pdr_gpsr.pdf')
plt.close()

# 4. Gráfico 2: Atraso Médio
plt.figure(figsize=(6, 4))
plt.bar(df['FlowID'].astype(str), df['MeanDelay(ms)'], color='lightcoral', edgecolor='black', width=0.4)
plt.xlabel('Fluxo de Dados (Flow ID)')
plt.ylabel('Atraso Médio (ms)')
plt.title('Atraso Médio Fim-a-Fim (GPSR)')
plt.tight_layout()
plt.savefig('grafico_delay_gpsr.pdf')
plt.close()

print("Gráficos gerados com sucesso: 'grafico_pdr_gpsr.pdf' e 'grafico_delay_gpsr.pdf'")
