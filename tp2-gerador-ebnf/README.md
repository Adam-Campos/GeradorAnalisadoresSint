# Compiladores I — Trabalho Prático 2: Gerador de Analisadores Sintáticos Descendentes Recursivos

## Integrantes

- Fernando Costa — 202319040614
- Mauricio Raquita — 202319040460
- Adam Rian Silva Campos — 202319040398

**Uso de IA:** Sim. Consulte o arquivo `USO_IA.md` para informações sobre a utilização.

## 1. Descrição do projeto

Este projeto implementa um gerador de analisadores sintáticos descendentes recursivos.

O programa recebe uma gramática escrita em EBNF e gera automaticamente um analisador sintático em linguagem C, utilizando a estratégia de análise descendente recursiva.

O gerador realiza as seguintes etapas:

1. Leitura e análise da gramática EBNF.
2. Verificação semântica da gramática.
3. Cálculo dos conjuntos anulável, FIRST e FOLLOW.
4. Verificação das condições LL(1).
5. Geração dos arquivos `.c` e `.h` do analisador sintático.

O projeto também contempla a integração com o scanner da linguagem Micro C, desenvolvido no Trabalho Prático 1.

## 2. Estrutura do projeto

```text
tp2-gerador-ebnf/
├── src/
│   ├── automato.c/h
│   ├── bitset.c/h
│   ├── codegen.c/h
│   ├── conjuntos.c/h
│   ├── ebnf_lexer.c/h
│   ├── ebnf_parser.c/h
│   ├── gramatica.c/h
│   ├── ll1.c/h
│   ├── main.c
│   ├── microc_mapa.c/h
│   ├── util.c/h
│   └── verificacao.c/h
├── microc/
│   ├── microc.flex
│   ├── scanner.c/h
│   ├── tokens.h
│   └── LEIAME.md
├── testes/
│   ├── esperado/
│   ├── gramaticas/
│   ├── programas/
│   └── executar.sh
├── build/
├── Makefile
├── README.md
└── USO_IA.md
```

## 3. Compilação

Para compilar o projeto, utilize:

```bash
make
```

Para compilar somente o gerador:

```bash
make gerador
```

A compilação utiliza GCC com as seguintes opções:

```bash
-ansi -pedantic -Wall -Wextra
```

A integração com o scanner do Micro C também utiliza o Flex.

## 4. Execução

O gerador recebe como entrada uma gramática EBNF e um prefixo para os arquivos de saída.

Exemplo:

```bash
./gerador testes/gramaticas/expr.ebnf -o build/testes/expr
```

São gerados dois arquivos:

- `expr.c`: implementação do analisador sintático.
- `expr.h`: declarações e estruturas utilizadas pelo analisador.

Para compilar o analisador gerado:

```bash
gcc -ansi -pedantic -Wall -Wextra -o parser build/testes/expr.c
```

Para executar:

```bash
./parser programa.txt
```

## 5. Opções disponíveis

O gerador disponibiliza os seguintes parâmetros:

| Opção | Descrição |
|---|---|
| `-o` | Define o prefixo dos arquivos gerados. |
| `--sets` | Exibe os conjuntos anulável, FIRST e FOLLOW. |
| `--tree` | Habilita a impressão da árvore de derivação. |
| `--debug` | Exibe a gramática lida pelo programa. |
| `--scanner microc` | Utiliza o scanner desenvolvido para Micro C no TP1. |

Exemplo:

```bash
./gerador testes/gramaticas/microc.ebnf -o build/microc/parser --scanner microc --tree
```

## 6. Integração com Micro C

O projeto permite utilizar o scanner desenvolvido no Trabalho Prático 1.

Para isso, são utilizados os arquivos presentes na pasta `microc/`, incluindo o scanner Flex, a estrutura de tokens e o adaptador responsável por fornecer os tokens ao analisador sintático gerado.

A integração permite analisar programas escritos na linguagem Micro C.

Para gerar e compilar o analisador:

```bash
make microc
```

## 7. Testes

O projeto possui testes automatizados organizados em três categorias:

- **Gramáticas:** verificam a leitura, análise semântica, conjuntos e condições LL(1).
- **Programas:** verificam se os analisadores reconhecem entradas válidas e rejeitam entradas inválidas.
- **Micro C:** verifica o funcionamento do analisador integrado ao scanner do TP1.

Para executar os testes:

```bash
make test
```

Os resultados são comparados com os arquivos de saída esperada presentes nas pastas `testes/esperado/` e `testes/programas/`.

## 8. Limpeza dos arquivos gerados

Para remover os arquivos de compilação e executáveis gerados:

```bash
make clean
```

## 9. Utilização de inteligência artificial

As informações sobre ferramentas utilizadas, finalidades e contribuições da inteligência artificial durante o desenvolvimento estão documentadas em `USO_IA.md`.
