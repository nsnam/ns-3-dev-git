# Guia de Desenvolvimento e Testes: GPSR Modernizado em VANETs

Este documento estabelece o fluxo de trabalho oficial para validar as modificações no protocolo geográfico durante as orientações do professor Francisco Carlos Gurgel da Silva Segundo para o TCC na UFERSA. Siga estas etapas sempre que precisar alterar a lógica de roteamento ou escalar a frota de veículos.

## 1. Mapeamento da Arquitetura
* **`src/gpsr/model/gpsr.cc`:** Contém a matemática do *Greedy Forwarding*, Oráculo de posições e tratamento da fila ARP (`DeferredRouteOutput`).
* **`scratch/vanet-test.cc`:** Gerencia as configurações da sub-rede IPv4, alcance do rádio Wi-Fi 802.11p, aplicações UDP e importação do trace de mobilidade.
* **`extrair_metricas.py`:** Faz o parser do `gpsr-sumo-results.xml` (gerado pelo FlowMonitor) para um CSV tabular pronto para gráficos.
* **`trace.tcl`:** Roteiro de coordenadas físicas e trajetórias exportado pelas ferramentas de trânsito.

## 2. Alteração e Compilação do Código (ns-3)
Sempre que modificar a lógica de repasse ou os logs do C++, é obrigatório recompilar o módulo para injetar as mudanças no núcleo do simulador.
1. Abra o arquivo fonte principal: `nano src/gpsr/model/gpsr.cc`
2. Realize as edições necessárias na lógica de saltos ou resolução de IPs.
3. Salve o arquivo (`Ctrl + O`, `Enter`, `Ctrl + X`).
4. Inicie a compilação a partir da raiz do ns-3: `./ns3 build`

## 3. Escalonamento de Topologia (SUMO)
Para testar a resiliência do protocolo contra falhas de *Local Maximum*, gere novos cenários urbanos alterando os parâmetros de densidade veicular e malha viária.
1. Defina o diretório base de ferramentas: `export SUMO_HOME=/usr/share/sumo`
2. Crie uma nova grade de ruas (ex: 5x5 quarteirões): `netgenerate --grid --grid.number 5 --grid.length 200 -o mapa.net.xml`
3. Gere viagens aleatórias para a frota desejada: `python3 $SUMO_HOME/tools/randomTrips.py -n mapa.net.xml -e 300 -l -o rotas.rou.xml`
4. Processe a simulação viária: `sumo -n mapa.net.xml -r rotas.rou.xml --fcd-output trace.xml`
5. Exporte a movimentação para o formato nativo do ns-3: `python3 $SUMO_HOME/tools/traceExporter.py --fcd-input trace.xml --ns2mobility-output trace.tcl`
6. Ajuste a máscara de sub-rede (`address.SetBase`) e a variável `numNodes` no arquivo `scratch/vanet-test.cc` para refletir o novo total de veículos.

## 4. Ciclo de Validação e Extração de Dados
Com o código atualizado e o trânsito gerado, execute a simulação final para registrar o comportamento da rede em tempo real.
1. Rode o cenário principal: `./ns3 run vanet-test`
2. Converta os resultados do XML bruto para estrutura tabular: `python3 extrair_metricas.py`
3. Inspecione as taxas de entrega (`RxPackets`), vazão e atraso: `cat resultados_gpsr.csv`
