# parallel-cdn-log-analyzer ⚡

## 🌍 Contexto: por que esse projeto existe?

Hoje, praticamente tudo o que a gente faz na internet passa por uma **CDN (Content Delivery Network)**: assistir séries na **Netflix**, ver vídeos no **YouTube**, jogar online, abrir posts do **Instagram**, carregar páginas de e-commerce como **Amazon** ou **Mercado Livre**, etc.

Essas empresas distribuem cópias dos conteúdos em vários servidores espalhados pelo mundo ("edge servers"). Quando um vídeo, imagem ou página começa a ser muito acessado, ele se torna um **hot content** (conteúdo quente) e precisa ser replicado para mais servidores para evitar:

* sobrecarga em poucos servidores,
* aumento de latência (site lento),
* quedas de serviço em horários de pico.

Como a CDN descobre **quais conteúdos estão quentes**? Analisando **logs de acesso**.

Esse projeto simula exatamente essa situação: temos arquivos de log gigantes (com milhões de requisições HTTP), e precisamos:

1. **contar quantos acessos cada URL recebeu**,
2. fazer isso de forma **rápida**, aproveitando vários núcleos de CPU (paralelismo),
3. entender como diferentes estratégias de sincronização influenciam o desempenho.

Em vez de trabalhar diretamente com logs reais da Cloudflare, Akamai, Netflix Open Connect ou Google, geramos logs sintéticos, mas com padrões realistas:

* um cenário **distribuído**, em que o tráfego é mais homogêneo;
* um cenário **concorrente / hotspot**, em que poucas URLs concentram quase todo o tráfego, como quando uma série nova lança na Netflix ou um vídeo viraliza no YouTube.

A partir daí, o foco do projeto é **técnico**: usar C + OpenMP + tabela hash para implementar três versões de analisador de log (sequencial, paralela com `critical` e paralela com `atomic`) e comparar como cada abordagem se comporta nesses cenários.

---

Analisador **paralelo de logs de CDN** escrito em **C** com **OpenMP**, focado em comparar uma versão **sequencial** com duas versões **paralelas** (usando `critical` e `atomic`) em cenários realistas com **10 milhões de requisições**.

Este repositório serve como **portfólio de Computação Paralela + Estruturas de Dados**, mostrando:

* modelagem de um problema real (análise de logs de CDN),
* implementação de **tabela hash** em C,
* uso de **OpenMP** para paralelizar um pipeline de processamento de logs,
* análise de **speedup, eficiência e contenção** em diferentes padrões de acesso.

---

## 🧭 Visão geral rápida

* **Entrada:** arquivos de log HTTP simulando o acesso a uma CDN (10M linhas).
* **Saída:** arquivo CSV `results.csv` no formato `URL,hit_count` com o total de acessos por URL.
* **3 implementações comparadas:**

  * `analyzer_seq.c` – versão sequencial (baseline),
  * `analyzer_par_critical.c` – paralela com `#pragma omp critical`,
  * `analyzer_par_atomic.c` – paralela com `#pragma omp atomic`.
* **Cenários de teste:**

  * `log_distribuido.txt` – baixa contenção (acessos bem distribuídos),
  * `log_concorrente.txt` – alta contenção (hotspots).

---

## 📁 Organização do repositório

Sugestão de layout (pode ser adaptado conforme seu uso):

```text
parallel-cdn-log-analyzer/
├── README.md
├── Makefile                  # script de compilação (gcc + OpenMP)
├── .gitignore
├── src/
│   ├── analyzer_seq.c        # versão sequencial
│   ├── analyzer_par_critical.c # versão paralela com critical
│   ├── analyzer_par_atomic.c   # versão paralela com atomic
│   ├── hash_table.c          # implementação da tabela hash
│   └── hash_table.h          # interface da tabela hash
├── scripts/
│   └── generate_cdn_data.py  # gerador de logs e gabaritos
├── report/
│   └── relatorio_lab2_cdn.pdf  # relatório do projeto (opcional)
└── examples/
    └── (opcional) arquivos pequenos de exemplo de log/csv
```

> 🔒 **Não versionar**: arquivos de log reais (`log_*.txt` com ~900MB), gabaritos completos (`gabarito_*.csv` com milhões de linhas), `cdn_data_logs.zip` e executáveis.

Exemplo de `.gitignore` mínimo:

```gitignore
# Binários
analyzer_seq
analyzer_par_critical
analyzer_par_atomic

# Dados grandes gerados
*.txt
*.csv
*.zip

# Mas permita exemplos pequenos
!examples/*.txt
!examples/*.csv

# Objetos e temporários
*.o
*.out

# Config de IDEs
.vscode/
.idea/
.DS_Store
```

---

## 🎯 Objetivos do projeto

* Simular o tráfego de uma **CDN (Content Delivery Network)** com:

  * catálogo de URLs (`manifest.txt`),
  * padrões de acesso **distribuídos** e **concorrentes (hotspot)**.
* Implementar uma **estrutura de dados eficiente** para contagem de acessos:

  * tabela hash com encadeamento separado, escrita em C puro.
* Comparar desempenho entre três abordagens:

  * versão **sequencial** (1 thread),
  * versão paralela com **região crítica** (`#pragma omp critical`),
  * versão paralela com **operações atômicas** (`#pragma omp atomic`).
* Discutir, na prática:

  * impacto de **contenção** em variáveis compartilhadas,
  * custo de sincronização vs. ganho de paralelismo,
  * **speedup** e **eficiência** em função do número de threads e do padrão de acesso.

---

## 🧱 Arquitetura da solução

### 1. Geração de dados (Python)

**Arquivo:** `scripts/generate_cdn_data.py`

Este script gera todo o conjunto de dados sintéticos:

* `manifest.txt`
  Lista de todas as URLs disponíveis na CDN (catálogo de conteúdos).

* `log_distribuido.txt`
  Log de acessos com **distribuição uniforme** entre as URLs (baixa contenção).

* `log_concorrente.txt`
  Log de acessos com **hotspots** (poucas URLs recebem a maior parte dos acessos).

* `gabarito_distribuido.csv`

* `gabarito_concorrente.csv`

Arquivos CSV com o gabarito de contagem no formato:

```text
URL,hit_count
```

> Esses gabaritos são usados para validar a saída dos analisadores em C via `diff`.

O script também pode compactar tudo em `cdn_data_logs.zip` e remover os arquivos originais.

---

### 2. Estrutura de dados: Tabela Hash

**Arquivos:** `src/hash_table.h`, `src/hash_table.c`

A tabela hash mapeia:

```text
URL -> hit_count
```

Características principais:

* **Encadeamento separado** para colisões (cada bucket é uma lista encadeada de `CacheNode`).
* Estrutura do `CacheNode`:

  * `char* url` – chave (URL),
  * `long hit_count` – contador de acessos,
  * `CacheNode* next` – próximo nó da lista encadeada.
* Hash de string baseado em `djb2`.

API pública:

* `HashTable* ht_create(size_t size);`
* `void ht_destroy(HashTable* ht);`
* `void ht_insert(HashTable* ht, const char* url);`
* `CacheNode* ht_get(HashTable* ht, const char* url);`
* `void ht_save_results(HashTable* ht, const char* filename);`

A mesma tabela hash é reutilizada por todas as versões (sequencial e paralelas), permitindo comparar diretamente o efeito da sincronização.

---

### 3. Analisadores de log (C + OpenMP)

Fluxo geral de todas as versões:

1. Ler `manifest.txt` e inserir todas as URLs na tabela hash com `hit_count = 0`.
2. Ler todas as linhas do arquivo de log (`log_distribuido.txt` ou `log_concorrente.txt`) em memória.
3. Para cada linha de log:

   * extrair a URL a partir do padrão HTTP:

     ```text
     127.0.0.1 - - [data] "GET /alguma/url.mp4 HTTP/1.1" 200 1500
     ```

   * buscar essa URL na hash (`ht_get`),

   * incrementar o contador `hit_count` correspondente.
4. Salvar o resultado em `results.csv`.
5. Medir o tempo de processamento com `omp_get_wtime()`.
6. (Opcional) Validar a saída com `diff` em relação ao gabarito.

#### `analyzer_seq.c` – Versão sequencial (baseline)

* Processa o log com **1 thread**.
* Percorre todas as linhas, extrai a URL, busca na hash e incrementa o contador.
* Serve como **baseline** (`T_seq`) para cálculo de **speedup** e **eficiência**.

#### `analyzer_par_critical.c` – Paralelo com `#pragma omp critical`

* Usa `#pragma omp parallel for` para distribuir as linhas entre múltiplas threads.
* A atualização da contagem é protegida por uma **região crítica global**:

```c
#pragma omp parallel for
for (size_t i = 0; i < num; i++) {
    // extrai URL e busca na hash
    CacheNode* node = ht_get(ht, url);
    if (node) {
        #pragma omp critical
        node->hit_count++;
    }
}
```

* Garante corretude, mas pode sofrer com **alta contenção** quando muitas threads disputam os mesmos poucos contadores.

#### `analyzer_par_atomic.c` – Paralelo com `#pragma omp atomic`

* Também usa `#pragma omp parallel for`.
* A diferença está na sincronização do incremento, feita com operação atômica:

```c
#pragma omp parallel for
for (size_t i = 0; i < num; i++) {
    CacheNode* node = ht_get(ht, url);
    if (node) {
        #pragma omp atomic update
        node->hit_count++;
    }
}
```

---

## 📊 Resultados de desempenho (resumo)

Os testes foram feitos com arquivos de log de **10 milhões de linhas** em ambiente Linux/WSL.

### 🔹 Cenário 1 – Log distribuído (baixa contenção)

Tempo sequencial usado como baseline:

* `T_seq_distribuido ≈ 3.82 s`

Comparação das versões paralelas em função do número de threads:

| Threads | Versão   | Tempo (s) | Speedup vs seq |
| ------: | -------- | --------: | -------------: |
|       1 | critical |      3.76 |          1.01× |
|       1 | atomic   |      2.77 |          1.38× |
|       2 | critical |      2.10 |          1.82× |
|       2 | atomic   |      1.44 |          2.66× |
|       4 | critical |      1.57 |          2.43× |
|       4 | atomic   |      0.90 |          4.25× |
|       8 | critical |      3.88 |          0.98× |
|       8 | atomic   |      0.47 |          8.05× |

> Em baixa contenção, a versão com `atomic` consegue aproveitar bem o paralelismo, chegando próximo de **8× de speedup com 8 threads**, enquanto `critical` sofre mais com overhead de sincronização.

### 🔹 Cenário 2 – Log concorrente (alta contenção / hotspot)

Aqui poucas URLs concentram ~90% dos acessos (hot contents).

| Versão                  | Threads | Tempo (s) | Speedup vs seq |
| ----------------------- | ------: | --------: | -------------: |
| Sequencial              |       1 |      1.21 |          1.00× |
| Paralela com `critical` |       8 |      2.98 |          0.40× |
| Paralela com `atomic`   |       8 |      0.36 |          3.30× |

> Em alta contenção, `critical` vira gargalo (todas as threads disputam uma única região crítica), chegando a ficar **mais lenta que a versão sequencial**.
> Com `atomic`, a sincronização é mais leve e granular, permitindo um speedup de ~**3.3×** mesmo em um cenário com muitos acessos às mesmas URLs.

---

## 📈 Visualização – Speedup por número de threads

Para o cenário distribuído, também geramos um gráfico de **speedup vs número de threads**:

```text
Speedup
 9 |                            x (atomic, N=8)
 8 |                         x
 7 |
 6 |
 5 |                    x
 4 |                 x
 3 |
 2 |           x           x
 1 |      x  x
 0 +-----------------------------------------
      N=1    2           4            8

      • linha critical  ~ 1.0, 1.8, 2.4, 1.0
      • linha atomic    ~ 1.4, 2.7, 4.3, 8.0
```

Se você quiser incluir o gráfico como imagem no repositório, salve o PNG em `docs/speedup_distribuido.png` e adicione:

```markdown
![Gráfico de speedup – log distribuído](docs/speedup_distribuido.png)
```

---

## 🧠 Skills demonstradas

* Programação paralela em **C + OpenMP** (`parallel for`, `critical`, `atomic`).
* Implementação de **tabela hash** com encadeamento separado para alto volume de dados.
* Medição e análise de **desempenho**, **speedup**, **eficiência** e **contenção**.
* Modelagem de um problema real de **CDN / sistemas distribuídos** em um experimento reprodutível.
