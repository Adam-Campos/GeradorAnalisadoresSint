/*
 * conjuntos.c - Calculo de anulavel, FIRST e FOLLOW (etapa 3).
 *
 * Fornecido: conj_iniciar e conj_imprimir.
 *
 * TODO(aluno): conj_anulavel, conj_first e conj_follow. Os tres sao
 * calculados por iteracao ate ponto fixo: repita uma passada sobre todas
 * as regras enquanto algum conjunto (ou flag) mudar. bitset_unir devolve
 * 1 quando o conjunto de destino muda, o que ajuda a detectar o ponto fixo.
 */
#include <stdio.h>
#include <string.h>
#include "conjuntos.h"
#include "util.h"

/* ------------------------------------------------------------------ */
/* Fornecido                                                          */
/* ------------------------------------------------------------------ */

static void iniciar_expr(Expr *e, void *ctx)
{
    int n = *(int *) ctx;
    bitset_liberar(&e->first);
    bitset_liberar(&e->follow);
    bitset_iniciar(&e->first, n);
    bitset_iniciar(&e->follow, n);
    e->anulavel = 0;
}

void conj_iniciar(Gramatica *g)
{
    int i, n = g->nterms;
    for (i = 0; i < g->nnts; i++) {
        bitset_liberar(&g->nts[i].first);
        bitset_liberar(&g->nts[i].follow);
        bitset_iniciar(&g->nts[i].first, n);
        bitset_iniciar(&g->nts[i].follow, n);
        g->nts[i].anulavel = 0;
        expr_percorrer(g->nts[i].regra, iniciar_expr, &n);
    }
}

void conj_imprimir(FILE *f, const Gramatica *g)
{
    int i;
    size_t largura = strlen("NAO TERMINAL");
    for (i = 0; i < g->nnts; i++)
        if (strlen(g->nts[i].nome) > largura)
            largura = strlen(g->nts[i].nome);
    fprintf(f, "%-*s  ANULAVEL  FIRST / FOLLOW\n", (int) largura, "NAO TERMINAL");
    for (i = 0; i < g->nnts; i++) {
        fprintf(f, "%-*s  %-8s  FIRST  = ", (int) largura, g->nts[i].nome,
                g->nts[i].anulavel ? "sim" : "nao");
        gram_imprimir_conjunto(f, g, &g->nts[i].first);
        fprintf(f, "\n%-*s            FOLLOW = ", (int) largura, "");
        gram_imprimir_conjunto(f, g, &g->nts[i].follow);
        fputc('\n', f);
    }
}

/* ------------------------------------------------------------------ */
/* A implementar                                                      */
/* ------------------------------------------------------------------ */

/* TODO(aluno): calcular e->anulavel para cada no Expr e g->nts[i].anulavel.
 *   E_TERM: 0        E_VAZIO: 1        E_NAOTERM: anulavel do nao terminal
 *   E_SEQ: 1 se todos os filhos forem anulaveis
 *   E_ALT: 1 se algum filho for anulavel
 *   E_OPC, E_REP: 1 (calcule tambem o do filho)
 * Repita ate nada mudar. */

static int anul_expr(const Gramatica *g, Expr *e, int *mudou)
{
    int i, v = 0, filho;

    switch (e->tipo) {
    case E_TERM:
        v = 0;
        break;
    case E_VAZIO:
        v = 1;
        break;
    case E_NAOTERM:
        v = g->nts[e->simbolo].anulavel;
        break;
    case E_SEQ:
        v = 1;
        for (i = 0; i < e->nfilhos; i++) {
            filho = anul_expr(g, e->filhos[i], mudou);
            if (!filho)
                v = 0;
        }
        break;
    case E_ALT:
        v = 0;
        for (i = 0; i < e->nfilhos; i++) {
            filho = anul_expr(g, e->filhos[i], mudou);
            if (filho)
                v = 1;
        }
        break;
    case E_OPC:
    case E_REP:
        anul_expr(g, e->filhos[0], mudou);
        v = 1;
        break;
    }
    if (v != e->anulavel) {
        e->anulavel = v;
        *mudou = 1;
    }
    return v;
}

void conj_anulavel(Gramatica *g)
{
    int i, v, mudou;

    do {
        mudou = 0;
        for (i = 0; i < g->nnts; i++) {
            v = anul_expr(g, g->nts[i].regra, &mudou);
            if (v != g->nts[i].anulavel) {
                g->nts[i].anulavel = v;
                mudou = 1;
            }
        }
    } while (mudou); 
}

/* TODO(aluno): calcular e->first para cada no Expr e g->nts[i].first.
 *   E_TERM t: {t}     E_NAOTERM B: FIRST(B)     E_VAZIO: {}
 *   E_SEQ a1..an: FIRST(a1) U FIRST(a2) ... ate o primeiro ai nao anulavel
 *   E_ALT, E_OPC, E_REP: uniao dos FIRST dos filhos
 * Calcule o FIRST de TODOS os filhos de uma sequencia, mesmo os que
 * nao contribuem para o FIRST dela: o gerador de codigo usa-os. */

static void first_expr(const Gramatica *g, Expr *e, int *mudou)
{
    int i;

    for (i = 0; i < e->nfilhos; i++)          
        first_expr(g, e->filhos[i], mudou);

    switch (e->tipo) {
    case E_TERM:
        if (!bitset_tem(&e->first, e->simbolo)) {
            bitset_add(&e->first, e->simbolo);
            *mudou = 1;
        }
        break;
    case E_NAOTERM:
        if (bitset_unir(&e->first, &g->nts[e->simbolo].first))
            *mudou = 1;
        break;
    case E_SEQ:
        for (i = 0; i < e->nfilhos; i++) {
            if (bitset_unir(&e->first, &e->filhos[i]->first))
                *mudou = 1;
            if (!e->filhos[i]->anulavel)
                break;
        }
        break;
    case E_ALT:
    case E_OPC:
    case E_REP:
        for (i = 0; i < e->nfilhos; i++)
            if (bitset_unir(&e->first, &e->filhos[i]->first))
                *mudou = 1;
        break;
    case E_VAZIO:
        break;
    }
}

void conj_first(Gramatica *g)
{
    int i, mudou;
    do {
        mudou = 0;
        for (i = 0; i < g->nnts; i++) {
            first_expr(g, g->nts[i].regra, &mudou);
            if (bitset_unir(&g->nts[i].first, &g->nts[i].regra->first))
                mudou = 1;
        }
    } while (mudou);
}
/* TODO(aluno): calcular e->follow (o que pode vir logo apos cada
 * construcao) e g->nts[i].follow. FOLLOW(inicial) contem $ (TERM_FIM).
 * Para cada regra A = alfa, faca follow(alfa) U= FOLLOW(A) e propague:
 *   E_SEQ a1..an:  follow(ai) U= FIRST(ai+1 ... an), mais follow(seq)
 *                  se ai+1 ... an for anulavel
 *   E_ALT, E_OPC:  follow(filho) U= follow(no)
 *   E_REP { a }:   follow(a) U= follow(no) U FIRST(a)
 *   E_NAOTERM B:   FOLLOW(B) U= follow(no)
 * Repita ate nada mudar. */
 
static void follow_expr(Gramatica *g, Expr *e, int *mudou)
{
    int i, j;
    Expr *f;

    switch (e->tipo) {
    case E_NAOTERM:                       /* o que segue o uso entra no FOLLOW(B) */
        if (bitset_unir(&g->nts[e->simbolo].follow, &e->follow))
            *mudou = 1;
        return;
    case E_SEQ:
        for (i = 0; i < e->nfilhos; i++) {
            f = e->filhos[i];
            for (j = i + 1; j < e->nfilhos; j++) {
                if (bitset_unir(&f->follow, &e->filhos[j]->first))
                    *mudou = 1;
                if (!e->filhos[j]->anulavel)
                    break;
            }
            /* tudo depois de f e anulavel: herda o FOLLOW da sequencia */
            if (j == e->nfilhos && bitset_unir(&f->follow, &e->follow))
                *mudou = 1;
        }
        break;
    case E_ALT:
    case E_OPC:
        for (i = 0; i < e->nfilhos; i++)
            if (bitset_unir(&e->filhos[i]->follow, &e->follow))
                *mudou = 1;
        break;
    case E_REP:
        f = e->filhos[0];
        if (bitset_unir(&f->follow, &e->follow)) *mudou = 1;
        if (bitset_unir(&f->follow, &f->first))  *mudou = 1;  /* recomeca */
        break;
    case E_TERM:
    case E_VAZIO:
        return;
    }
    for (i = 0; i < e->nfilhos; i++)
        follow_expr(g, e->filhos[i], mudou);
}

void conj_follow(Gramatica *g)
{
    int i, mudou;

    bitset_add(&g->nts[g->inicial].follow, TERM_FIM);   /* $ no inicial */
    do {
        mudou = 0;
        for (i = 0; i < g->nnts; i++) {
            if (bitset_unir(&g->nts[i].regra->follow, &g->nts[i].follow))
                mudou = 1;
            follow_expr(g, g->nts[i].regra, &mudou);
        }
    } while (mudou);
}
