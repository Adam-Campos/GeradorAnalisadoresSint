/*
 * ll1.c - Verificacao da condicao LL(1) (etapa 4).
 *
 * Condicoes (secao 3 do enunciado):
 *   1. Em a1 | a2 | ... | an, os FIRST das alternativas sao disjuntos dois
 *      a dois e no maximo uma alternativa e anulavel.
 *   2. Se uma alternativa ai e anulavel, FOLLOW(alternativa) e disjunto
 *      dos FIRST das demais.
 *   3. Em [ a ] e { a }, a nao e anulavel e FIRST(a) e disjunto do
 *      FOLLOW da construcao.
 *
 * Violacoes de 1, 2 e da primeira parte de 3 sao ERROS. A violacao de
 * FIRST(a) inter FOLLOW nao vazio em [ a ] ou { a } e reportada como
 * AVISO e resolvida de forma gulosa: o analisador gerado entra na
 * construcao sempre que o token estiver em FIRST(a). E o caso classico
 * do "else pendente":  if = "if" "(" expr ")" cmd [ "else" cmd ] ;
 * em que o else fica com o if mais proximo, como em C.
 *
 * Fornecido: reportar (formata as mensagens).
 * TODO(aluno): verificar_expr e ll1_verificar.
 */
#include <stdarg.h>
#include <stdio.h>
#include "ll1.h"
#include "util.h"

/* Reporta um conflito na regra do nao terminal nt, na posicao de e.
 * comum (pode ser NULL) e o conjunto de tokens em conflito. */
static void reportar(const Gramatica *g, int nt, const Expr *e, int eh_aviso,
                     const Bitset *comum, const char *fmt, ...)
{
    va_list ap;
    fprintf(stderr, "%s:%d:%d: %s: conflito LL(1) em %s: ", g->arquivo, e->linha,
            e->coluna, eh_aviso ? "aviso" : "erro", g->nts[nt].nome);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    if (comum != NULL) {
        fprintf(stderr, " ");
        gram_imprimir_conjunto(stderr, g, comum);
    }
    fputc('\n', stderr);
}

typedef struct {
    const Gramatica *g;
    int nt;          /* nao terminal da regra sendo verificada */
    int erros;
} Contexto;

static void verificar_expr(Expr *e, void *ctx)
{
    Contexto *c = (Contexto *) ctx;
    const Gramatica *g = c->g;
    Bitset comum;
    Expr *filho;
    int i, j, n;
    const char *par;

    bitset_iniciar(&comum, g->nterms);
    if (e->tipo == E_ALT) {
        n = e->nfilhos;
        for (i = 0; i < n; i++) {
            for (j = i + 1; j < n; j++) {
                /* condicao 1: no maximo uma anulavel e FIRST disjuntos */
                if (e->filhos[i]->anulavel && e->filhos[j]->anulavel) {
                    reportar(g, c->nt, e, 0, NULL,
                             "alternativas %d e %d sao anulaveis", i + 1, j + 1);
                    c->erros++;
                }
                if (bitset_intersecta(&e->filhos[i]->first, &e->filhos[j]->first)) {
                    bitset_intersecao(&comum, &e->filhos[i]->first, &e->filhos[j]->first);
                    reportar(g, c->nt, e, 0, &comum,
                             "alternativas %d e %d comecam com", i + 1, j + 1);
                    c->erros++;
                }
            }
        }
        /* condicao 2: alternativa anulavel x FOLLOW da construcao */
        for (i = 0; i < n; i++) {
            if (!e->filhos[i]->anulavel)
                continue;
            for (j = 0; j < n; j++) {
                if (j != i && bitset_intersecta(&e->follow, &e->filhos[j]->first)) {
                    bitset_intersecao(&comum, &e->follow, &e->filhos[j]->first);
                    reportar(g, c->nt, e, 0, &comum,
                             "alternativa %d e anulavel e o que vem depois "
                             "comeca como a alternativa %d em", i + 1, j + 1);
                    c->erros++;
                }
            }
        }
    } else if (e->tipo == E_OPC || e->tipo == E_REP) {
        filho = e->filhos[0];
        par = e->tipo == E_OPC ? "[ ]" : "{ }";
        /* condicao 3: o conteudo nao pode ser anulavel */
        if (filho->anulavel) {
            reportar(g, c->nt, e, 0, NULL, "o conteudo de %s e anulavel", par);
            c->erros++;
        } else if (bitset_intersecta(&filho->first, &e->follow)) {
            /* FIRST x FOLLOW: aviso, resolvido de forma gulosa (else pendente) */
            bitset_intersecao(&comum, &filho->first, &e->follow);
            reportar(g, c->nt, e, 1, &comum,
                     "FIRST e FOLLOW de %s se intersectam "
                     "(resolvido de forma gulosa) em", par);
        }
    }
    bitset_liberar(&comum);
}

/* Percorre todas as regras. Retorna o numero de erros (avisos nao contam). */
int ll1_verificar(const Gramatica *g)
{
    Contexto c;
    int i;

    c.g = g;
    c.erros = 0;
    for (i = 0; i < g->nnts; i++) {
        c.nt = i;
        expr_percorrer(g->nts[i].regra, verificar_expr, &c);
    }
    return c.erros;
}
