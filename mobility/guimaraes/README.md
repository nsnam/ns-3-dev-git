<!-- Documentation is maintained in Portuguese. -->
# Cenário Multimodal de Guimarães

Este diretório contém um cenário SUMO urbano para o Centro Histórico, o campus
da Universidade do Minho em Azurém, Veiga de Creixomil e acessos urbanos às
N101/N206. A área OSM é `west=-8.36, south=41.40, east=-8.25, north=41.47`.
O mapa inclui vias, paragens/plataformas e relações de linhas de autocarro.

Para a lista dos scripts, argumentos detalhados, ficheiros que cada etapa
consome/produz e exemplos de execução individual, consulte
[scripts/README.md](../../scripts/README.md).

## Requisitos

- ns-3 configurado no checkout, SUMO e os executáveis `sumo`, `netconvert` e
	`duarouter` disponíveis no `PATH`.
- Ferramentas de importação do SUMO em `$SUMO_HOME/tools`, incluindo
	`tools/import/gtfs/gtfs2pt.py` e `tools/randomTrips.py`.
- Python 3.12 recomendado. O pipeline usa o venv em `../bin/python` quando
	existe; se `pandas` estiver ausente, instala
	`scripts/requirements-mobility.txt` nesse ambiente.
- Acesso à Internet para descarregar OSM via Overpass e o GTFS da Guimabus.
	Se o Overpass ou a transferência GTFS não estiverem disponíveis, consulte
	abaixo os comportamentos de recuperação.

## Executar

Na raiz de `ns-3-dev`:

```sh
scripts/build_guimaraes_scenario.sh
sumo-gui -c mobility/guimaraes/guimaraes.sumocfg
```

Para executar sem GUI:

```sh
sumo -c mobility/guimaraes/guimaraes.sumocfg \
	--no-step-log true --duration-log.disable true
```

O pipeline tem duração de 300 s e passo de 0,1 s. Por omissão, a janela é
07:00-07:05 de 30/09/2026: `SIM_BEGIN=25200` e `SIM_END=25500`, em segundos
desde a meia-noite. A data `SIM_DATE` tem de existir no calendário do GTFS.

## Configurar

As variáveis de ambiente seguintes podem ser definidas antes de executar o
script:

| Variável | Valor por omissão | Efeito |
| --- | --- | --- |
| `SIM_DATE` | `20260930` | Dia de serviço usado no calendário GTFS (`YYYYMMDD`). |
| `SIM_BEGIN` | `25200` | Segundo do dia em que começa a simulação. |
| `SIM_END` | `25500` | Segundo do dia em que termina; a diferença para `SIM_BEGIN` tem de ser 300. |
| `CAR_RATE` | `600` | Taxa de entrada de carros, veículos/hora para `randomTrips.py`. |
| `BIKE_RATE` | `120` | Taxa de entrada de bicicletas, veículos/hora. |
| `MAP_BBOX` | `-8.36,41.40,-8.25,41.47` | Bounding box OSM na ordem oeste,sul,leste,norte. |
| `OSM_TILES_X`, `OSM_TILES_Y` | `3`, `1` | Número inicial de tiles longitudinais/latitudinais do Overpass; falhas subdividem tiles recursivamente. |
| `FORCE_MAP_DOWNLOAD` | `0` | Use `1` para descarregar novamente o OSM mesmo que `guimaraes.osm` exista. |
| `GTFS_IMPORT_LOOKAHEAD` | `3600` | Segundos adicionais lidos pelo importador para encontrar paragens seguintes das linhas. |
| `BUS_STOP_MAX_DISTANCE` | `350` | Distância máxima, em metros, para associar uma paragem GTFS a uma via de autocarro no fallback. |
| `SUMO_BINARY` | `sumo` | Executável usado na validação e exportação TraCI. |
| `NETCONVERT_BINARY` | `netconvert` | Conversor OSM para rede SUMO. |
| `DUAROUTER_BINARY` | `duarouter` | Roteador para as trips de carros e bicicletas. |
| `PYTHON` | venv `../bin/python` ou `python3` | Intérprete Python usado pelo pipeline. |
| `SUMO_HOME` | detetado em `/usr/share/sumo` ou `/usr/local/share/sumo` | Localização das ferramentas e do módulo TraCI. |

Exemplo de alteração da janela para 08:00-08:05:

```sh
SIM_DATE=20260930 SIM_BEGIN=28800 SIM_END=29100 \
	CAR_RATE=900 BIKE_RATE=180 scripts/build_guimaraes_scenario.sh
```

O exportador escreve os tempos desde zero, embora o SUMO execute entre
`SIM_BEGIN` e `SIM_END`.

## Pipeline

1. `download_guimaraes_osm.py` consulta o endpoint usado pelo Overpass Turbo,
	 limita a frequência de pedidos, tenta endpoints alternativos, divide tiles
	 que excedem o limite e mantém cache em `generated/osm-tiles/`.
2. `merge_osm_tiles.py` elimina elementos OSM duplicados por tipo e ID e grava
	 `guimaraes.osm`.
3. `netconvert` grava `guimaraes.net.xml`, `ptstops.xml` e `ptlines.xml`,
	 ativando `--crossings.guess`, `--sidewalks.guess`, `--geometry.remove`,
	 `--ramps.guess` e `--junctions.corner-detail 5`.
4. `fetch_gtfs.py` descarrega e verifica o ZIP da Guimabus. O original fica em
	 `guimabus_gtfs.zip`; `normalize_gtfs.py` cria a cópia de trabalho
	 `generated/guimabus_gtfs_sumo.zip`, removendo colunas opcionais duplicadas
	 que entram em conflito na junção pandas do SUMO.
5. O pipeline tenta primeiro o `gtfs2pt.py` oficial. Se o mapeamento abortar
	 ou não criar viagens, `build_gtfs_bus_routes.py` lê diretamente o calendário,
	 paragens e horários GTFS, associa as paragens a faixas acessíveis a
	 autocarros, calcula caminhos no `sumolib` e grava `buses.rou.xml`/
	 `buses.add.xml`. Se também não houver viagens reais roteáveis, usa um feed
	 demonstrativo válido.
6. `randomTrips.py` gera trips de passageiros e bicicletas; `duarouter`
	 converte-as em `cars.rou.xml` e `bikes.rou.xml`. As sementes são 42 para
	carros e 84 para bicicletas. O mínimo origem-destino é 1000 m para carros e
	500 m para bicicletas; as taxas de entrada são `CAR_RATE` e `BIKE_RATE`.
7. `prepare_gtfs_routes.py` remove definições de tipos duplicadas nos três
	 ficheiros de rotas, consolida os `vType` em `vtypes.xml` e acrescenta entre
	 0 e 8 s de dwell às paragens dos autocarros, com semente 42.
8. O SUMO executa a validação e escreve `validation-summary.xml` e os logs.
	 `summarize_guimaraes.py` verifica contagens, erros de runtime, colisões e
	 teletransportes e grava `summary.txt`.
9. `export_ns3_trace.py` executa novamente o cenário via TraCI e exporta os
	 ficheiros CSV, ns-2 e mapa de nós.

## Ficheiros

| Ficheiro/diretório | Entrada/saída | Finalidade |
| --- | --- | --- |
| `guimaraes.osm` | Saída do downloader | Recorte OSM usado para construir a rede. |
| `guimaraes.net.xml` | Saída do `netconvert` | Rede SUMO, permissões, faixas e geometria. |
| `ptstops.xml`, `ptlines.xml` | Saída do `netconvert` | Paragens e relações de transporte público identificadas no OSM. |
| `guimabus_gtfs.zip` | Entrada baixada | Feed Guimabus original, sem normalização. |
| `guimabus_gtfs.source.txt` | Saída do downloader | URL de origem e contagens das tabelas GTFS. |
| `generated/` | Cache/intermediários | Tiles OSM, FCD/GPS do `gtfs2pt`, redes tipadas, cópia GTFS para SUMO e `vType` temporários. |
| `buses.rou.xml` | Saída GTFS | Veículos, sequência de arestas e paragens de autocarros. |
| `buses.add.xml` | Saída GTFS | `busStop` criadas a partir do feed quando é usado o roteador direto. |
| `cars.trip.xml`, `bikes.trip.xml` | Saída de `randomTrips.py` | Viagens origem-destino antes do roteamento. |
| `cars.rou.xml`, `bikes.rou.xml` | Saída de `duarouter` | Rotas executáveis dos automóveis e bicicletas. |
| `*.rou.alt.xml` | Saída auxiliar do roteador | Alternativas de rota; não são carregadas pela configuração principal. |
| `vtypes.xml` | Entrada SUMO | Dimensões, dinâmica, classes e modelos de mudança de faixa. |
| `guimaraes.sumocfg` | Configuração | Rede, rotas, ficheiros adicionais, duração e resolução sublane. |
| `validation-summary.xml` | Saída SUMO | Estatísticas de cada passo para validar a execução. |
| `sumo-validation.stdout.log`, `sumo-validation.stderr.log` | Saída SUMO | Logs da execução não interativa; erros de runtime interrompem o pipeline. |
| `gtfs-import.log` | Saída dos importadores | Diagnóstico do `gtfs2pt.py` e resultado do fallback direto. |
| `summary.txt` | Saída do relatório | Contagens de veículos, método de importação e validação SUMO. |
| `ns3-mobility.csv` | Saída do exportador | Uma linha por amostra: tempo, índice/ID do veículo, classe, X/Y/Z e velocidade. |
| `ns3-mobility.ns2.tcl` | Saída do exportador | Posições iniciais `$node_` e eventos `$ns_ at ... setdest ...`. |
| `ns3-mobility.nodes.json` | Saída do exportador | Associação estável entre índice ns-2 e ID/classe SUMO. |

O conteúdo de `generated/`, as rotas e os traces podem ser regenerados. O ZIP
GTFS original, a rede OSM e os artefactos finais ficam disponíveis para
inspeção.

## Tipos e mobilidade lateral

`vtypes.xml` define:

- `cars_passenger`: automóvel, `SL2015`, velocidade máxima de 15,3 m/s e
	urgência estratégica de mudança de faixa aumentada para acessos que usam
	apenas uma faixa.
- `bus`: autocarro de 12 m com aceleração e velocidade urbana.
- `bikes_bicycle`: bicicleta a 5 m/s (18 km/h), largura 0,6 m, `SL2015`,
	`latAlignment="arbitrary"` e `minGapLat="0.5"`.

`guimaraes.sumocfg` define `lateral-resolution=0.5`, necessária ao modelo
sublane. Mantenha os IDs dos tipos sincronizados com os prefixos `cars` e
`bikes` usados pelo `randomTrips.py`.

## Integração com ns-3

O CSV preserva o ID de veículo e a classe e deve ser usado quando a aplicação
ns-3 precisa gerir entrada/saída dinâmica de veículos. O ficheiro `.ns2.tcl`
atribui um índice a cada veículo observado e usa posições XY locais em metros.
`Ns2MobilityHelper` não representa criação/remoção dinâmica: pré-crie os nós
antes da simulação e consulte `ns3-mobility.nodes.json` para a correspondência
dos IDs. Exemplo:

```sh
./ns3 run "seu-programa --trace=mobility/guimaraes/ns3-mobility.ns2.tcl"
```

Confirme a unidade e a origem XY da rede do ns-3 antes de combinar o trace com
outros modelos geográficos. O código C++ de acoplamento TraCI existente está
em `contrib/traci-applications/examples/sumo-coupling-simple.cc`; esse exemplo
continua configurado para outra rede e Wi-Fi 802.11p. O exemplo `cttc-nr-demo`
do 5G-LENA é separado e não liga automaticamente este mapa ao NR.

## GTFS e limitações

O feed usado é o Mobility Database
[`mdb-2838-202512190141`](https://files.mobilitydatabase.org/mdb-2838/mdb-2838-202512190141/mdb-2838-202512190141.zip).
O relatório publicado tem muitos avisos e alguns problemas de consistência; o
pipeline verifica os ficheiros e referências necessárias, mas isso não equivale
a declarar o feed completo como GTFS conforme. O `gtfs2pt.py` pode falhar durante
o mapeamento de paragens; nesse caso o fallback direto informa no log quantas
partidas no intervalo foram encontradas e quantas foram roteadas. Paragens fora
da rede ou sem faixa de autocarro compatível são omitidas da sequência roteável.

Durante `netconvert`, avisos sobre paragens/linhas OSM não associadas podem
aparecer. A validação de runtime continua separada: consulte `summary.txt`,
`sumo-validation.stderr.log` e `validation-summary.xml`.

## Licença e atribuição

O mapa deriva de dados OpenStreetMap, sob ODbL. Atribuição: © OpenStreetMap
contributors, <https://www.openstreetmap.org/copyright>. O feed de autocarros
vem da Mobility Database/Guimabus; consulte os termos publicados pelo
fornecedor antes de redistribuir os dados.