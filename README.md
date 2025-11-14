# CDN Log Analyzer com OpenMP ⚡

Projeto prático da disciplina de **Computação Paralela** focado em analisar acessos a uma CDN (Content Delivery Network) usando:

- uma **tabela hash** para contagem de hits por URL
- três versões de analisadores:
  - implementação **sequencial**
  - implementação paralela com `#pragma omp critical`
  - implementação paralela com `#pragma omp atomic`

O objetivo é estudar **concorrência, contenção e overhead de sincronização** em um cenário realista de logs massivos (milhões de acessos).

---

## 🎯 Objetivos do projeto

- Simular o comportamento de uma CDN com:
  - muitas URLs diferentes (conteúdos do cache)
  - padrões de acesso **distribuídos** e **concorrentes (hotspot)**
- Implementar uma **estrutura de dados eficiente** para contagem de hits:
  - tabela hash com encadeamento separado
- Comparar o desempenho de:
  - uma versão **sequencial**
  - uma versão paralela com **região crítica**
  - uma versão paralela com **atômicos**
- Discutir:
  - impacto de **contenção** em variáveis quentes (hotspot)
  - custo de sincronização (`critical` vs `atomic`)
  - ganho real de desempenho em função do tamanho da entrada

---

## 🧱 Arquitetura da solução

### 1. Geração de dados (Python)

Arquivo: `generate_cdn_data.py`

Esse script cria todo o conjunto de dados do projeto:

- `manifest.txt`  
  Lista de todas as URLs da CDN (catálogo de conteúdos).

- `log_distribuido.txt`  
  Log de acessos com **distribuição uniforme** entre as URLs.

- `log_concorrente.txt`  
  Log de acessos com **hotspots** (poucas URLs recebem a maioria dos acessos).

- `gabarito_distribuido.csv`  
- `gabarito_concorrente.csv`  

Gabaritos no formato:

```text
URL,hit_count
