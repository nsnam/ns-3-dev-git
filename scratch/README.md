# Simulação MEC 5G NR + SUMO de Guimarães

Este README documenta o programa principal
[`mec-5g-guimaraes.cc`](mec-5g-guimaraes.cc), a execução TraCI/ns-3 e todos os
logs e traces gravados em `results/raw_data/`. O cenário SUMO, a rede e as
rotas estão descritos em [mobility/guimaraes/README.md](../mobility/guimaraes/README.md);
as ferramentas auxiliares estão em [scripts/README.md](../scripts/README.md).

## O que é simulado

- Mobilidade SUMO de Guimarães: carros, autocarros Guimabus e bicicletas.
- Sincronização SUMO/ns-3 por TraCI a cada 100 ms; a execução é em tempo
  simulado, não é um relógio de parede em tempo real.
- Um gNB NR por célula lógica OpenCellID filtrada para NOS (MCC 268/MNC 03),
  colocado nas coordenadas XY da rede SUMO e com Z=15 m.
- NR n78 centrado em 3,5 GHz, 40 MHz, numerologia μ=1 (30 kHz), beamforming
  ideal e canal 3GPP UMa/ThreeGpp com atualizações a cada 100 ms.
- EPC NR com host MEC remoto ligado ao PGW por P2P de 10 Gbps e 1 ms.
- Aplicações UDP por perfil: autocarro 2 Mbps, carro 300 bytes/100 ms e
  bicicleta 100 bytes/1 s.

OpenCellID contém observações de **células/setores**, não uma confirmação da
localização física de cada torre. O download atual forneceu 13 registos LTE e
nenhum registo etiquetado como NR; essas posições são usadas como candidatos
geográficos para gNBs da simulação NR, não como prova de sites 5G reais da NOS.

## Executar

Na raiz de `ns-3-dev`, com SUMO, 5G-LENA e TraCI instalados:

```sh
./ns3 configure --build-profile=optimized
./ns3 build
scripts/run_simulation.sh
```

O runner usa o catálogo local `mobility/guimaraes/gnb_positions.json` se já
existir; portanto, não consulta OpenCellID nem consome créditos novamente.
Para atualizá-lo, defina `FORCE_GNB_REFRESH=1` e forneça
`OPENCELLID_API_KEY` no ambiente. Também é possível fornecer um dump local em
`OPENCELLID_INPUT`. O token não é escrito no código ou nos resultados.

Por omissão, a simulação dura 120 s a partir de 07:00 (`SUMO_START_TIME=25200`).
O pool UE é dimensionado com uma passagem SUMO de contagem de concorrência
máxima, acrescida de quatro nós. Variáveis úteis do runner:

| Variável | Padrão | Efeito |
| --- | --- | --- |
| `SIM_DURATION` | `120` | Duração simulada do ns-3, segundos. |
| `SUMO_START_TIME` | `25200` | Hora absoluta inicial SUMO, segundos desde a meia-noite. |
| `UE_POOL_MARGIN` | `4` | UEs extra sobre o pico estimado para partidas simultâneas. |
| `SUMO_BINARY` | `sumo` | Executável SUMO usado na estimativa e na TraCI. |
| `PYTHON` | venv `../bin/python`, senão `python3` | Intérprete do parser/conversor. |
| `OPENCELLID_INPUT` | não definido | CSV/JSON OpenCellID local; reprocessa o catálogo. |
| `OPENCELLID_API_KEY` | não definido | Credencial para obter dados se for necessário atualizar o cache. |
| `FORCE_GNB_REFRESH` | `0` | `1` ignora o catálogo local e consulta a API. |

Para executar o binário diretamente, sem atualizar o catálogo nem recompilar:

```sh
./ns3 run "mec-5g-guimaraes \
  --sumoConfig=mobility/guimaraes/guimaraes.sumocfg \
  --gnbPositions=mobility/guimaraes/gnb_positions.json \
  --outputDirectory=results/raw_data --duration=120 \
  --maxUes=41 --sumoStartTime=25200"
```

Argumentos C++: `--sumoConfig`, `--gnbPositions`, `--outputDirectory`,
`--duration`, `--maxUes` e `--sumoStartTime`. O `--maxUes` tem de ser pelo
menos o pico de veículos simultâneos; se for pequeno, o callback TraCI aborta
quando o pool esgota.

## Fluxo interno

1. O runner reutiliza ou atualiza o JSON OpenCellID com
   `parse_opencellid_gnb.py`.
2. `estimate_ue_pool.py` executa SUMO sem NR na janela pretendida e calcula o
   máximo de veículos simultaneamente inseridos menos os que já chegaram.
3. O C++ lê o JSON, cria os nós gNB com mobilidade constante e instala
   dispositivos NR no BWP n78. O `cell_id` OpenCellID é mantido no catálogo; o
   CellId NR é atribuído pelo ns-3.
4. É criado um pool de nós com Internet Stack e `NrUeNetDevice` já instalados.
   `TraciClient::SumoSetup` inicia SUMO e, em cada partida, o callback retira
   um nó livre, associa-o ao ID/type SUMO e ativa a aplicação da classe.
5. A posição do UE é atualizada pelo TraCI durante o movimento. Na chegada, a
   aplicação pára, o nó é afastado da área e devolvido ao pool. A pilha NR não
   é destruída/recriada a cada viagem.
6. A associação inicial escolhe o gNB mais próximo. Quando um nó reutilizado
   passa a representar um veículo cuja célula mais próxima é diferente, o
   código solicita handover NR. Os eventos RRC/handover são registados.
7. `NrHelper` recolhe SINR/RxPacket PHY, MAC scheduler e RLC E2E. Os servidores
   UDP no MEC contam datagramas por perfil e imprimem `MEC_RX` no fim normal.
8. O runner converte os TSV/TXT nativos para CSV depois de o processo terminar
   com sucesso. Se houver abort/crash, essa conversão final não é executada.

Nota de fidelidade: o código pede handover ao reutilizar um UE para outro
veículo/célula, mas ainda não executa uma política de handover periódica para
cada UE durante toda a sua viagem. Os traces de handover estão ligados; podem
ficar vazios quando nenhuma solicitação é iniciada.

## Ficheiros de entrada e configuração

| Ficheiro | Utilização |
| --- | --- |
| `mobility/guimaraes/guimaraes.sumocfg` | Configuração SUMO de rede, rotas, tipos, janela e resolução lateral. |
| `mobility/guimaraes/guimaraes.net.xml` | Rede SUMO e geometria que determina as posições XY e a mobilidade TraCI. |
| `mobility/guimaraes/cars.rou.xml`, `bikes.rou.xml`, `buses.rou.xml` | Veículos/trips de cada classe. |
| `mobility/guimaraes/vtypes.xml` | Dimensões, classes e comportamento de carros, autocarros e bicicletas. |
| `mobility/guimaraes/gnb_positions.json` | gNBs: IDs OpenCellID, rádio, MCC/MNC, lat/lon, X/Y/Z e atribuição/licença. |

O programa espera que `x` e `y` do JSON já estejam na projeção local da rede
SUMO, em metros; não passa coordenadas GPS diretamente para o MobilityModel.

## Saídas e traces

Todos os resultados vão para `results/raw_data/`, salvo `SumoError.log`, que o
TraCI grava junto de `guimaraes.sumocfg`. Os traces NR nativos são texto
tabulado; o conversor mantém o ficheiro original e cria um `.csv` com o mesmo
prefixo.

### Logs e associação

| Ficheiro | Conteúdo |
| --- | --- |
| `simulation.log` | stdout/stderr do runner e ns-3: configuração, binds, RRC_CONNECTED, handover e `MEC_RX` se a execução terminar normalmente. |
| `vehicle_events.csv` | CSV `time_s,event,vehicle_id,vehicle_type,node_id,imsi,nr_cell_id,rnti,details`. Eventos: `SUMO_BIND`, `RRC_CONNECTED`, `HANDOVER_REQUEST`, `HANDOVER_START`, `HANDOVER_END`, `SUMO_ARRIVE` e `MEC_RX_SUMMARY`. `nr_cell_id=0` no `SUMO_BIND` significa que a conexão RRC ainda não ocorreu. |
| `gnb_catalog.csv` | CSV `nr_cell_id,opencellid_cell_id,radio,mcc,mnc,x_m,y_m,z_m`: associação entre CellId interno NR e registo OpenCellID. |
| `SumoError.log` | Erros do processo SUMO iniciado pelo TraCI. |

### PHY

| Ficheiro nativo | Conteúdo |
| --- | --- |
| `DlDataSinrmec-guimaraes.txt` | SINR de dados DL por tempo, CellId, RNTI e BWP, em dB. |
| `RxPacketTracemec-guimaraes.txt` | Recepções DL/UL: tempo, direção, frame/subframe/slot, símbolos, CellId/BWP, RNTI, tamanho TB, MCS, rank, RV, SINR, CQI, corrupção e TBLER. O mesmo ficheiro agrega callbacks UE e gNB. |

O `RxPacketTrace` desta versão **não grava o bitmap nem a contagem de PRBs**,
apesar de guardar MCS, TB size e símbolos. O trace MAC abaixo também não expõe
PRBs; para esse KPI é necessário acrescentar um sink/campo de scheduler no NR.

### MAC scheduler

| Ficheiro nativo | Conteúdo |
| --- | --- |
| `nr_mac_dl.tsv` | Eventos de escalonamento DL: tempo, CellId, BWP, IMSI/RNTI, frame/subframe/slot, símbolo inicial/número de símbolos, HARQ, NDI, RV, MCS e tamanho TB. |
| `nr_mac_ul.tsv` | Os mesmos campos para escalonamento UL. |

### RLC E2E

| Ficheiro nativo | Conteúdo |
| --- | --- |
| `nr_rlc_dl.tsv` | DL por época/fluxo: início/fim, célula, IMSI, RNTI, LCID, PDUs/bytes TX e RX, atraso médio/desvio/mínimo/máximo e estatísticas de tamanho PDU. |
| `nr_rlc_ul.tsv` | Os mesmos contadores/atrasos na direção UL. |

O calculador RLC usa épocas de 0,25 s por omissão. Em execuções abortadas,
ficheiros RLC podem não ter cabeçalho/dados porque o calculador escreve no fim
da época ou no descarte normal do objeto.

### Outros ficheiros

| Ficheiro | Conteúdo |
| --- | --- |
| `DlDataSinrmec-guimaraes.csv`, `RxPacketTracemec-guimaraes.csv`, `nr_mac_*.csv`, `nr_rlc_*.csv` | Conversões dos traces tabulados correspondentes. Só são criados se `convert_nr_traces_to_csv.py` executar após sucesso do ns-3. |
| `simulation.log` | Log principal; serve também de prova rápida de binds/RRC/MEC. |
| `gnb_catalog.csv` | Catálogo espacial para correlacionar CellId NR com cell IDs e coordenadas OpenCellID. Pode ficar vazio/truncado após crash. |

Ficheiros de controlo PHY/MAC e pathloss com nomes como `Rxed*CtrlMsgs*`,
`Txed*CtrlMsgs*`, `DlCtrlSinr*` e `*Pathloss*` não são habilitados pelo código
principal atual. Se aparecerem no diretório, podem ser resíduos de uma execução
anterior que usou o conjunto completo `NrHelper::EnableTraces()`. O runner não
limpa a pasta automaticamente; use um diretório de saída novo para comparar
execuções e evitar misturar traces antigos com parciais.

## Estado de validação e interpretação

O OpenCellID autenticado gerou localmente 13 posições NOS LTE; o segredo não
fica nos arquivos. O target C++ compila em perfil `optimized` e já houve logs
de associação RRC com veículos/células. No entanto, a execução de 120 s não foi
validada até ao fim. O smoke test recente terminou com `SIGSEGV` durante o
ciclo de reutilização do UE; por isso, não há `MEC_RX` final nem CSVs finais
garantidos. Um servidor mais rápido ajuda o custo CPU-bound, mas não corrige
esse crash por si só.