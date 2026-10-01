# Mobilidade SUMO: Campus de Azurém

Este cenário usa a rede viária OSM de Azurém, em Guimarães, e gera rotas para
autocarros, carros e bicicletas. O SUMO calcula cada percurso a partir das
arestas de origem e destino, respeitando a classe (`vClass`) de cada veículo.

## Executar

Na raiz de `ns-3-dev`:

```sh
sumo-gui -c scratch/azurem-sumo/campus.sumocfg
```

Para executar sem interface gráfica:

```sh
sumo -c scratch/azurem-sumo/campus.sumocfg --no-step-log true
```

O cenário termina aos 900 segundos. As 34 viagens estão definidas em
`mobility.rou.xml`: 6 autocarros, 18 carros e 10 bicicletas. As bicicletas usam
rotas calculadas para a classe `bicycle`, incluindo trechos da rede que não
permitem tráfego de automóveis.

## Dados geográficos

`campus.osm` contém o recorte de OpenStreetMap entre 41.448509 N, 8.297693 W e
41.456775 N, 8.282146 W. `campus.net.xml` é a rede SUMO convertida desse recorte.
Os dados do OpenStreetMap são disponibilizados sob a licença ODbL; atribuição:
© OpenStreetMap contributors, https://www.openstreetmap.org/copyright.

## Integração ns-3

O exemplo local `contrib/traci-applications/examples/sumo-coupling-simple.cc`
inicia um cliente `TraciClient` e associa veículos a um conjunto pré-criado de
nós ns-3. Para usá-lo com este mapa, o atributo `SumoConfigPath` do exemplo deve
apontar para `scratch/azurem-sumo/campus.sumocfg`; o exemplo atual está ligado
ao cenário circular e usa Wi-Fi 802.11p. O exemplo `cttc-nr-demo` do 5G-LENA é
uma simulação NR separada, não uma integração TraCI pronta.

## Cenário urbano de Guimarães

Para o cenário multimodal com Centro Histórico, Azurém, Veiga de Creixomil,
GTFS Guimabus, tráfego de pico e exportação de traces ns-3, consulte
[mobility/guimaraes/README.md](../../mobility/guimaraes/README.md). O cenário
de Guimarães é mais amplo e independente deste recorte de campus.