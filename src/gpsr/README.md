# Modernização do Módulo GPSR no ns-3 para Redes Veiculares (VANETs)

Este repositório documenta o desenvolvimento técnico do Trabalho de Conclusão de Curso do 1º ciclo em Tecnologia da Informação (2024-2026), preparatório para o 2º ciclo em Engenharia de Computação (2027-2028) na Universidade Federal Rural do Semi-Árido (UFERSA - Campus Pau dos Ferros).

**Autor:** Maycon Soares Maia


**Orientador:** Prof. Francisco Carlos Gurgel da Silva Segundo


## 🎯 Resumo do Projeto
O objetivo geral deste trabalho é modernizar e avaliar uma codificação não-oficial do algoritmo de roteamento geográfico GPSR (Greedy Perimeter Stateless Routing) para torná-la compatível com uma versão atual do simulador de eventos discretos ns-3. A implementação será aplicada a um contexto de Redes Veiculares ad hoc (VANETs) e validada através de simulações comparativas contra os protocolos topológicos AODV e OLSR, nativos da ferramenta.

## 🛠️ Guia de Execução (Passo a Passo Metodológico)

O desenvolvimento técnico está estruturado nas seguintes etapas, conforme a metodologia do projeto:

### 1. Levantamento e Análise Técnica
- [ ] Realizar investigação bibliográfica sistemática sobre extensibilidade do ns-3 em VANETs e algoritmos geográficos.
- [ ] Analisar as implementações não-oficiais do GPSR (Katsaros et al., 2012) para mapear incompatibilidades com as interfaces de programação (API) atuais do simulador.

### 2. Modernização do Código-Fonte
- [ ] Substituir o sistema de compilação obsoleto (`waf`) criando e configurando o sistema de build `CMake` para o módulo GPSR.
- [ ] Adequar o código-fonte em C++ para resolver incompatibilidades com a API atual de roteamento e mobilidade do ns-3.

### 3. Implementação do Cenário de Simulação (VANET)
- [ ] Montar um experimento de rede veicular utilizando exclusivamente modelos de mobilidade nativos do ns-3 (sem ferramentas externas).
- [ ] Configurar a variação de parâmetros experimentais, como a densidade da rede e o número de veículos.

### 4. Configuração de Métricas
- [ ] Integrar o módulo `FlowMonitor` ao script de simulação da VANET.
- [ ] Configurar a coleta das quatro métricas principais: taxa de transferência (*throughput*), atraso médio (*delay*), taxa de entrega de pacotes (PDR) e *overhead* de roteamento.

### 5. Experimentos Comparativos e Documentação
- [ ] Executar o cenário desenvolvido utilizando o módulo GPSR modernizado.
- [ ] Executar o mesmo cenário utilizando os algoritmos de roteamento topológicos nativos AODV e OLSR.
- [ ] Realizar a análise estatística dos resultados gerados pelo FlowMonitor.
- [ ] Documentar o código-fonte modernizado e disponibilizá-lo como uma contribuição metodológica reprodutível para a comunidade de usuários do ns-3.