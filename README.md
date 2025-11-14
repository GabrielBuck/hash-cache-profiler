# parallel-cdn-log-analyzer ⚡

Analisador paralelo de logs de CDN escrito em **C** com **OpenMP**, focado em comparar versões sequencial e paralelas (usando `critical` e `atomic`) em cenários com milhões de requisições.

O projeto simula o comportamento de uma **CDN (Content Delivery Network)**, processando grandes arquivos de log, contando hits por URL com uma **tabela hash** e medindo o impacto da sincronização na performance.

---

## 🎯 Objetivos do projeto

- Simular o tráfego de uma CDN com:
  - muitas URLs diferentes (manifesto de conteúdo)
  - padrões de acesso **distribuídos** e **concorrentes (hotspot)**
- Implementar uma **estrutura de dados eficiente** para contagem de acessos:
  - tabela hash com encadeamento separado
- Comparar desempenho entre três implementações:
  - versão **sequencial**
  - versão paralela com **região crítica** (`#pragma omp critical`)
  - versão paralela com **operações atômicas** (`#pragma omp atomic`)
- Discutir:
  - impacto de **contenção** em variáveis compartilhadas
  - custo de sincronização
  - ganho real de **speedup** e **eficiência** em função do tamanho da entrada.

---

## 🧱 Arquitetura da solução

### 1. Geração de dados (Python)

**Arquivo:** `generate_cdn_data.py`

Esse script cria todo o conjunto de dados sintéticos:

- `manifest.txt`  
  Lista de todas as URLs disponíveis na CDN (catálogo de conteúdos).

- `log_distribuido.txt`  
  Log de acessos com **distribuição uniforme** entre as URLs.

- `log_concorrente.txt`  
  Log de acessos com **hotspots** (poucas URLs recebem a maior parte dos acessos).

- `gabarito_distribuido.csv`  
- `gabarito_concorrente.csv`  

Arquivos CSV com o gabarito de contagem no formato:

```text
URL,hit_count
```

Esses gabaritos são usados para validar a saída dos analisadores em C via `diff`.

> 🔎 Observação: o script compacta tudo em `cdn_data_logs.zip` e apaga os arquivos originais.  
> Para utilizar, basta rodar o script e depois descompactar:
>
> ```bash
> python3 generate_cdn_data.py
> unzip cdn_data_logs.zip
> ```

---

### 2. Estrutura de dados: Tabela Hash

**Arquivos:**

- `hash_table.h`
- `hash_table.c`

A tabela hash mapeia:

```text
URL -> hit_count
```

Características principais:

- **Encadeamento separado** para colisões (cada bucket é uma lista encadeada de `CacheNode`).
- Campos do `CacheNode`:
  - `char* url` – chave (URL)
  - `long hit_count` – contador de acessos
  - `CacheNode* next` – próximo nó na lista encadeada

API pública:

- `HashTable* ht_create(size_t size);`
- `void ht_destroy(HashTable* ht);`
- `void ht_insert(HashTable* ht, const char* url);`
- `CacheNode* ht_get(HashTable* ht, const char* url);`
- `void ht_save_results(HashTable* ht, const char* filename);`

A tabela hash é compartilhada entre as versões sequencial e paralelas.

---

### 3. Analisadores de log

Todos os analisadores seguem o mesmo fluxo geral:

1. Ler `manifest.txt` e inserir todas as URLs na tabela hash com `hit_count = 0`.
2. Ler todas as linhas do arquivo de log (`log_distribuido.txt` ou `log_concorrente.txt`).
3. Para cada linha de log:
   - extrair a URL do padrão HTTP:
     
     ```text
     GET /alguma/url.mp4 HTTP/1.1
     ```
   - buscar essa URL na hash (`ht_get`)
   - incrementar o contador da URL.
4. Salvar o resultado em `results.csv`.
5. Comparar `results.csv` com o gabarito usando `diff`.

#### `analyzer_seq.c` – Versão sequencial

- Processa o log com **1 thread**.
- Percorre todas as linhas, extrai a URL, faz a busca na tabela hash e incrementa o contador.
- Usado como **baseline** para cálculo de speedup e eficiência.

#### `analyzer_par_critical.c` – Versão paralela com `critical`

- Utiliza `#pragma omp parallel for` para dividir as linhas entre múltiplas threads.
- Cada thread:
  - extrai a URL da linha de log
  - busca na hash
  - incrementa o contador dentro de uma região crítica:

```c
#pragma omp critical
node->hit_count++;
```

- Garante correção, mas pode sofrer com **alta contenção**, especialmente no cenário concorrente (hotspot).

#### `analyzer_par_atomic.c` – Versão paralela com `atomic`

- Também usa `#pragma omp parallel for`.
- O incremento é protegido com uma operação atômica:

```c
#pragma omp atomic update
node->hit_count++;
```

- A sincronização é mais granular do que `critical`, permitindo melhor escalabilidade quando muitas URLs diferentes são atualizadas em paralelo.

---

## 🛠️ Tecnologias utilizadas

- **C** (compilado com `gcc`)
- **OpenMP**
  - `#pragma omp parallel for`
  - `#pragma omp critical`
  - `#pragma omp atomic`
- **Python 3** para geração dos dados sintéticos
- Ambiente de desenvolvimento/teste:
  - Linux / WSL2
  - `gcc` com suporte a `-fopenmp`

---

## ▶️ Como executar o projeto

### 1. Pré-requisitos

```bash
sudo apt update
sudo apt install -y build-essential python3 unzip
```

### 2. Gerar o conjunto de dados

Na pasta do projeto:

```bash
python3 generate_cdn_data.py
unzip cdn_data_logs.zip
```

Arquivos gerados:

- `manifest.txt`
- `log_distribuido.txt`
- `log_concorrente.txt`
- `gabarito_distribuido.csv`
- `gabarito_concorrente.csv`

### 3. Compilar os analisadores

```bash
gcc -Wall -O2 -fopenmp analyzer_seq.c          hash_table.c -o analyzer_seq
gcc -Wall -O2 -fopenmp analyzer_par_critical.c hash_table.c -o analyzer_par_critical
gcc -Wall -O2 -fopenmp analyzer_par_atomic.c   hash_table.c -o analyzer_par_atomic
```

### 4. Rodar e validar (exemplo)

#### Log distribuído

```bash
# Versão sequencial
./analyzer_seq log_distribuido.txt
tr -d '\r' < results.csv > results_seq_distribuido_unix.csv
tr -d '\r' < gabarito_distribuido.csv > gabarito_distribuido_unix.csv
diff gabarito_distribuido_unix.csv results_seq_distribuido_unix.csv
```

```bash
# Versão paralela com critical (exemplo com 8 threads)
export OMP_NUM_THREADS=8
./analyzer_par_critical log_distribuido.txt
tr -d '\r' < results.csv > results_critical_distribuido_unix.csv
diff gabarito_distribuido_unix.csv results_critical_distribuido_unix.csv
```

```bash
# Versão paralela com atomic (exemplo com 8 threads)
export OMP_NUM_THREADS=8
./analyzer_par_atomic log_distribuido.txt
tr -d '\r' < results.csv > results_atomic_distribuido_unix.csv
diff gabarito_distribuido_unix.csv results_atomic_distribuido_unix.csv
```

#### Log concorrente (hotspot)

Repita o mesmo processo usando `log_concorrente.txt` e `gabarito_concorrente.csv`.

---

## 📊 Estrutura sugerida para resultados

> Preencha esta seção com os tempos medidos na sua máquina.

### Log distribuído

| Versão             | Threads | Tempo (s) | Speedup | Eficiência |
|--------------------|---------|-----------|---------|-----------|
| Sequencial         | 1       |           | 1.00    | 1.00      |
| Paralelo critical  | 2       |           |         |           |
| Paralelo critical  | 4       |           |         |           |
| Paralelo critical  | 8       |           |         |           |
| Paralelo atomic    | 2       |           |         |           |
| Paralelo atomic    | 4       |           |         |           |
| Paralelo atomic    | 8       |           |         |           |

### Log concorrente (hotspot)

| Versão             | Threads | Tempo (s) | Speedup | Eficiência |
|--------------------|---------|-----------|---------|-----------|
| Sequencial         | 1       |           | 1.00    | 1.00      |
| Paralelo critical  | 8       |           |         |           |
| Paralelo atomic    | 8       |           |         |           |

---

## 🧠 Principais aprendizados

- Diferença entre **desempenho teórico** e **desempenho real** em paralelismo:
  - overhead de criação de threads
  - custo de sincronização
  - impacto da hierarquia de memória
- Efeito do **padrão de acesso aos dados**:
  - tráfego distribuído tende a escalar melhor
  - hotspots geram contenção intensa nas mesmas variáveis (URLs quentes)
- Comparação prática entre `#pragma omp critical` e `#pragma omp atomic` em um cenário realista de processamento de logs.
- Implementação de **tabela hash** em C e suas implicações em ambiente paralelo.

---

## 🚀 Ideias de extensões

- Usar reduções locais por thread e fazer merge dos resultados para diminuir contenção.
- Adotar partição da tabela hash (por exemplo, uma hash por thread ou por conjunto de buckets).
- Gerar relatórios automáticos com Top N URLs mais acessadas (direto em C ou via Python).
- Adicionar métricas extras: tempo de parsing, tempo de IO, tempo de atualização de hash, etc.

