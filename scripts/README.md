# Scripts de Mobilidade Guimarães

Este diretório implementa o pipeline entre OpenStreetMap/Overpass, GTFS,
SUMO e traces consumíveis pelo ns-3. Os scripts assumem execução a partir da
raiz do checkout `ns-3-dev`; a descrição do cenário, configuração SUMO e
artefactos estão em [mobility/guimaraes/README.md](../mobility/guimaraes/README.md).

## Execução recomendada

```sh
scripts/build_guimaraes_scenario.sh
sumo-gui -c mobility/guimaraes/guimaraes.sumocfg
```

O script verifica ferramentas e `pandas`, prepara a rede, gera as rotas,
executa o SUMO em modo headless, grava `mobility/guimaraes/summary.txt` e
exporta CSV, ns-2 e JSON. Para executar sem interface, substitua `sumo-gui`
por `sumo`.

## Dependências

- SUMO com `sumo`, `netconvert`, `duarouter`, `tools/randomTrips.py` e
  `tools/import/gtfs/gtfs2pt.py`.
- `SUMO_HOME` a apontar para a instalação. O orquestrador tenta
  `/usr/share/sumo` e `/usr/local/share/sumo` quando a variável não está definida.
- Python 3. O venv `../bin/python` é preferido se existir; caso contrário, é
  usado `python3`.
- `pandas`, listado em `requirements-mobility.txt`. O pipeline instala a
  dependência no intérprete selecionado se o import falhar.
- Conectividade de rede para Overpass e para o feed GTFS, exceto se os mapas
  estiverem em cache e o modo demo for usado.

Instalação manual da dependência:

```sh
../bin/python -m pip install -r scripts/requirements-mobility.txt
```

## Orquestrador

### `build_guimaraes_scenario.sh`

**Entradas:** ferramentas SUMO, scripts deste diretório, o bounding box,
calendário/data GTFS e variáveis de ambiente abaixo. Não recebe argumentos
posicionais.

**Etapas:** descarrega/mescla o mapa; converte OSM em rede; baixa e valida o
GTFS; normaliza uma cópia para o importador; tenta `gtfs2pt.py`; se necessário,
usa `build_gtfs_bus_routes.py`; gera trips de carros/bicicletas e converte-as
com `duarouter`; consolida tipos; valida o SUMO e executa o exportador TraCI.

**Saídas:** ficheiros de rede, transporte público, GTFS, rotas, tipos, logs,
`summary.txt` e os três traces em `mobility/guimaraes/`. A lista completa está
na documentação do cenário.

**Variáveis configuráveis:**

| Variável | Padrão | Utilização |
| --- | --- | --- |
| `PYTHON` | `../bin/python` ou `python3` | Seleciona o intérprete. |
| `SUMO_HOME` | deteção automática | Raiz de instalação SUMO. |
| `SUMO_BINARY` | `sumo` | Validação e exportação. |
| `NETCONVERT_BINARY` | `netconvert` | Construção da rede. |
| `DUAROUTER_BINARY` | `duarouter` | Roteamento de trips. |
| `MAP_BBOX` | `-8.36,41.40,-8.25,41.47` | Oeste,sul,leste,norte, graus. |
| `OSM_TILES_X`, `OSM_TILES_Y` | `3`, `1` | Divisão inicial do pedido Overpass; falhas subdividem recursivamente. |
| `FORCE_MAP_DOWNLOAD` | `0` | `1` força novo download OSM. |
| `SIM_DATE` | `20260930` | Data de serviço `YYYYMMDD`. |
| `SIM_BEGIN`, `SIM_END` | `25200`, `25500` | Janela absoluta do SUMO, em segundos desde meia-noite. A diferença deve ser 300. |
| `GTFS_IMPORT_LOOKAHEAD` | `3600` | Janela adicional lida pelo importador GTFS. |
| `CAR_RATE`, `BIKE_RATE` | `600`, `120` | Taxa de chegada, veículos/hora. |
| `BUS_STOP_MAX_DISTANCE` | `350` | Raio de associação do fallback GTFS, metros. |
| `OPENCELLID_API_KEY` | não definido | Necessário somente se `gnb_positions.json` não existir e não for fornecido `OPENCELLID_INPUT`. |
| `OPENCELLID_INPUT` | não definido | Dump CSV/JSON OpenCellID local; tem prioridade sobre o cache. |
| `FORCE_GNB_REFRESH` | `0` | `1` força nova consulta da API mesmo quando `gnb_positions.json` já existe. |

O parser grava o catálogo filtrado em `mobility/guimaraes/gnb_positions.json`.
O runner reutiliza esse arquivo por omissão, inclusive quando `OPENCELLID_API_KEY`
continua definida, para evitar consumo repetido de créditos. Só descarrega dados
novamente se o cache faltar, se for indicado `OPENCELLID_INPUT`, ou se executar
com `FORCE_GNB_REFRESH=1`.

Exemplo:

```sh
SIM_DATE=20260930 SIM_BEGIN=28800 SIM_END=29100 \
  CAR_RATE=900 BIKE_RATE=180 scripts/build_guimaraes_scenario.sh
```

## Scripts Python

### Executar etapas individualmente

Todos os caminhos nos exemplos partem da raiz de `ns-3-dev`. Use o mesmo
intérprete/`SUMO_HOME` que o orquestrador para os scripts que importam
`sumolib` ou pandas.

Descarregar e mesclar o OSM (tiles em cache podem ser reutilizados):

```sh
python3 scripts/download_guimaraes_osm.py \
  --bbox=-8.36,41.40,-8.25,41.47 --tiles-x 3 --tiles-y 1 \
  --cache-dir mobility/guimaraes/generated/osm-tiles \
  --output mobility/guimaraes/guimaraes.osm
```

Descarregar GTFS real ou criar o demo, e preparar a cópia compatível com SUMO:

```sh
python3 scripts/fetch_gtfs.py --output mobility/guimaraes/guimabus_gtfs.zip
python3 scripts/fetch_gtfs.py --demo --output /tmp/guimabus-demo.zip
python3 scripts/normalize_gtfs.py \
  --input mobility/guimaraes/guimabus_gtfs.zip \
  --output mobility/guimaraes/generated/guimabus_gtfs_sumo.zip
```

Exportar a mobilidade da configuração atual ou atualizar o relatório a partir
de um sumário SUMO existente:

```sh
python3 scripts/export_ns3_trace.py \
  --config mobility/guimaraes/guimaraes.sumocfg \
  --output-prefix mobility/guimaraes/ns3-mobility
python3 scripts/summarize_guimaraes.py \
  --directory mobility/guimaraes \
  --summary-xml mobility/guimaraes/validation-summary.xml
```

`build_gtfs_bus_routes.py` e `prepare_gtfs_routes.py` são principalmente
chamados pelo orquestrador; as respetivas opções obrigatórias e ficheiros que
modificam estão documentados abaixo. Para ver a ajuda completa de qualquer
interface CLI, execute `python3 scripts/<script>.py --help`.

### `download_guimaraes_osm.py`

Descarrega um recorte OSM usando a API Overpass configurada pelo Overpass Turbo.
Consulta vias `highway`, paragens/plataformas e relações de linhas de autocarro.
Aplica intervalo entre pedidos, backoff para `Retry-After`, endpoints
alternativos, cache e subdivisão adaptativa.

**Argumentos:** `--bbox west,south,east,north` (padrão Guimarães), `--tiles-x`/
`--tiles-y` (3/1), `--cache-dir` e `--output` (obrigatórios), `--timeout` (90 s),
`--attempts` (2), `--max-depth` (4) e `--endpoint URL` repetível.

**Saídas:** OSM XML em cache por tile e um único `guimaraes.osm` mesclado.
Falha se um tile continuar indisponível após retries e subdivisão máxima.

### `merge_osm_tiles.py`

Mescla XMLs OSM, removendo duplicados por par tipo/ID (`node`, `way`,
`relation`). Pode ser chamado isoladamente:

```sh
python3 scripts/merge_osm_tiles.py --output merged.osm tile-a.osm tile-b.osm
```

**Entradas:** `--output` e um ou mais caminhos de tiles.
**Saída:** um XML `<osm>` único; imprime contagens de nós, vias e relações.

### `fetch_gtfs.py`

Descarrega o ZIP Guimabus (URL padrão da Mobility Database), verifica os ficheiros
obrigatórios e referências route/trip/stop. Se o download ou a validação falhar,
grava um feed demo GTFS válido.

**Argumentos:** `--url` substitui a origem; `--output` muda o ZIP de destino;
`--demo` ignora a rede e gera sempre o demo.

**Saídas:** ZIP GTFS e `<nome>.source.txt`, com origem e contagens.
O modo demo contém duas linhas, paragens de referência próximas de vias,
calendário semanal e viagens demonstrativas; não representa serviço publicado.

### `normalize_gtfs.py`

Cria uma cópia de trabalho sem alterar o GTFS original. Normaliza `routes.txt`
e remove `route_short_name` duplicado de `trips.txt`/`stop_times.txt`, que causa
colisão de colunas na versão instalada do `gtfs2pt.py`.

**Argumentos:** `--input ZIP` e `--output ZIP`, ambos obrigatórios.
**Saída:** ZIP normalizado e sidecar `.source.txt` com as alterações feitas.

### `build_gtfs_bus_routes.py`

Importador de contingência. Usa `calendar.txt`/`calendar_dates.txt` para
selecionar serviços ativos, filtra viagens por `--date`, `--begin` e `--end`,
associa paragens a faixas que permitem `bus` e calcula caminhos com `sumolib`.
Se uma sequência inclui pontos fora da rede ou sem ligação, conserva o maior
segmento consecutivo roteável.

**Argumentos obrigatórios:** `--network`, `--gtfs`, `--routes`, `--stops`,
`--date`, `--begin`, `--end`.
**Opcionais:** `--max-stop-distance` (1500 m por omissão; o pipeline usa 350),
`--dwell-extra-max` (8 s), `--seed` (42).

**Saídas:** rotas em `--routes`, definições `busStop` em `--stops` e contagem
de partidas consideradas/roteadas no stdout. Falha se nenhuma viagem puder ser
roteada.

### `prepare_gtfs_routes.py`

Consolida os ficheiros de rotas e tipos antes da simulação. Remove `vType`
embutido que já existe em `vtypes.xml`, importa tipos gerados que ainda não
existam e aplica jitter determinístico de dwell apenas ao primeiro arquivo de
rotas (o de autocarros).

**Argumentos:** `--routes` (um ou mais arquivos; primeiro é buses),
`--generated-vtypes` (um ou mais adicionais), `--vtypes`, `--seed` (42) e
`--dwell-extra-max` (8 s).
**Saídas:** atualiza os arquivos de rotas indicados e `vtypes.xml` in-place.

### `summarize_guimaraes.py`

Conta veículos nos três arquivos `.rou.xml` e lê o último passo do sumário SUMO.
Falha se uma classe estiver vazia, se SUMO não carregar todas as rotas ou se
houver erros no stderr.

**Argumentos:** `--directory` (pasta dos resultados) e `--summary-xml`.
**Saída:** `<directory>/summary.txt`, com contagens, chegadas/em execução,
colisões/teletransportes, avisos/erros e método de importação de autocarros.

### `export_ns3_trace.py`

Executa a configuração SUMO em modo headless via TraCI. Numera veículos à
medida que aparecem, amostra posição/classe/velocidade em cada passo e usa
tempo relativo ao início da configuração.

**Argumentos:** `--config` (por omissão `mobility/guimaraes/guimaraes.sumocfg`),
`--sumo-binary` (padrão `sumo`) e `--output-prefix` (padrão
`<pasta-config>/ns3-mobility`).

**Saídas:** `<prefix>.csv`, `<prefix>.ns2.tcl` e `<prefix>.nodes.json`.
O CSV retém ID e classe SUMO; o JSON mapeia índices ns-2 para IDs/classes. O
formato ns-2 não modela entrada/saída dinâmica de nós: pré-crie os nós no ns-3
e use o CSV quando precisar reproduzir o ciclo de vida dos veículos.

### `requirements-mobility.txt`

Dependências Python do pipeline. Atualmente lista `pandas`, exigido pelo
importador de GTFS do SUMO.

## Alterações comuns

- **Área do mapa:** altere `MAP_BBOX`; em download forçado, use
  `FORCE_MAP_DOWNLOAD=1`. A rede SUMO e as rotas têm de ser regeneradas.
- **Período/volume:** altere `SIM_DATE`, a janela de 300 s, `CAR_RATE` e
  `BIKE_RATE`. A data precisa de serviço no calendário GTFS.
- **Seleção de autocarros:** ajuste `BUS_STOP_MAX_DISTANCE` para controlar a
  associação de paragens no fallback; diminuir tende a remover mais viagens
  sem paragem próxima.
- **Dinâmica:** altere `mobility/guimaraes/vtypes.xml`. Preserve os IDs
  `cars_passenger`, `bus` e `bikes_bicycle`, usados pelos arquivos de rotas.
  O `SL2015` requer `lateral-resolution` em `guimaraes.sumocfg`.
- **Servidores Overpass:** passe `--endpoint URL` ao downloader ou altere a
  lista `DEFAULT_ENDPOINTS`; use pedidos moderados e respeite rate limits.
- **GTFS:** use `fetch_gtfs.py --url URL` para outra origem compatível ou
  `--demo` para testar sem Internet.

Após mudar a área, rede, vTypes ou procura, execute novamente o pipeline e
confira `summary.txt`, `gtfs-import.log` e `sumo-validation.stderr.log`.