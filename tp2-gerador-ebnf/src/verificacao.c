/*
 * verificacao.c - Verificacoes semanticas da gramatica (etapa 2).
 *
 * Ja implementados (use como modelo):
 *   - verif_indefinidos: nao terminais usados mas nao definidos;
 *   - verif_terminais:   terminais nomeados sem declaracao.
 *
 * TODO(aluno):
 *   - verif_inalcancaveis      (aviso)
 *   - verif_improdutivos       (erro)
 *   - verif_recursao_esquerda  (erro; direta e indireta)
 *
 * A duplicidade de definicoes ja e detectada durante a leitura
 * (ebnf_parser.c).
 */
#include <stdlib.h>
#include <string.h>
#include "verificacao.h"
#include "microc_mapa.h"
#include "util.h"

/* ------------------------------------------------------------------ */
/* Fornecido                                                          */
/* ------------------------------------------------------------------ */

static int verif_indefinidos(Gramatica *g)
{
    int i, erros = 0;
    for (i = 0; i < g->nnts; i++) {
        if (g->nts[i].regra == NULL) {
            diag_erro(g->nts[i].linha, g->nts[i].coluna,
                      "nao terminal %s usado mas nao definido", g->nts[i].nome);
            erros++;
        }
    }
    return erros;
}

static int verif_terminais(Gramatica *g, int modo_microc)
{
    int i, erros = 0, carregado = 0;
    Terminal *t;

    if (modo_microc)
        carregado = microc_carregar_tokens_h(g->tokens_externos) == 0;

    for (i = 1; i < g->nterms; i++) {
        t = &g->terms[i];
        if (modo_microc) {
            if (t->literal && microc_nome_literal(t->nome) == NULL) {
                diag_erro(t->linha, t->coluna, "literal %s nao corresponde a nenhum "
                          "token de Micro C", t->exibicao);
                erros++;
            } else if (!t->literal && carregado && !microc_token_existe(t->nome)) {
                diag_erro(t->linha, t->coluna, "terminal %s nao existe na enumeracao "
                          "TokenType de %s", t->nome, g->tokens_externos);
                erros++;
            }
        } else if (!t->declarado) {
            diag_erro(t->linha, t->coluna, "terminal %s nao declarado em TOKENS", t->nome);
            erros++;
        }
    }
    return erros;
}

/* ------------------------------------------------------------------ */
/* A implementar                                                      */
/* ------------------------------------------------------------------ */

/* Marca em marca[] todo nao terminal que aparece em e. */
static void marcar_usos(Expr *e, void *ctx)
{
    if (e->tipo == E_NAOTERM)
        ((int *) ctx)[e->simbolo] = 1;
}

static int contar(const int *v, int n)
{
    int i, total = 0;
    for (i = 0; i < n; i++)
        total += v[i] != 0;
    return total;
}

/* Aviso para cada nao terminal inalcancavel a partir do simbolo inicial. */
static int verif_inalcancaveis(Gramatica *g)
{
    int i, antes, depois;
    int *alc = (int *) xcalloc((size_t) g->nnts, sizeof(int));

    alc[g->inicial] = 1;
    do {
        antes = contar(alc, g->nnts);
        for (i = 0; i < g->nnts; i++)
            if (alc[i])
                expr_percorrer(g->nts[i].regra, marcar_usos, alc);
        depois = contar(alc, g->nnts);
    } while (depois != antes);

    for (i = 0; i < g->nnts; i++)
        if (!alc[i])
            diag_aviso(g->nts[i].linha_def, 0,
                       "nao terminal %s inalcancavel a partir do simbolo inicial",
                       g->nts[i].nome);
    free(alc);
    return 0;
}

/* se a expressao deriva alguma cadeia de terminais, dado prod[]. */
static int produtiva(const Expr *e, const int *prod)
{
    int i, todos, algum;

    switch (e->tipo) {
    case E_TERM:
    case E_VAZIO:
    case E_OPC:
    case E_REP:
        return 1;
    case E_NAOTERM:
        return prod[e->simbolo];
    case E_SEQ:
        todos = 1;
        for (i = 0; i < e->nfilhos; i++)
            if (!produtiva(e->filhos[i], prod))
                todos = 0;
        return todos;
    case E_ALT:
        algum = 0;
        for (i = 0; i < e->nfilhos; i++)
            if (produtiva(e->filhos[i], prod))
                algum = 1;
        return algum;
    }
    return 0;
}

/* Erro para cada nao terminal que nao deriva nenhuma cadeia de terminais. */
static int verif_improdutivos(Gramatica *g)
{
    int i, mudou, erros = 0;
    int *prod = (int *) xcalloc((size_t) g->nnts, sizeof(int));

    do {
        mudou = 0;
        for (i = 0; i < g->nnts; i++)
            if (!prod[i] && produtiva(g->nts[i].regra, prod)) {
                prod[i] = 1;
                mudou = 1;
            }
    } while (mudou);

    for (i = 0; i < g->nnts; i++)
        if (!prod[i]) {
            diag_erro(g->nts[i].linha_def, 0,
                      "nao terminal %s e improdutivo: nao deriva nenhuma cadeia "
                      "de terminais", g->nts[i].nome);
            erros++;
        }
    free(prod);
    return erros;
}

/* Marca em linha[] os nao terminais que podem aparecer no INICIO de uma
 * derivacao de e. */
static void cabecas(const Expr *e, char *linha)
{
    int i;

    switch (e->tipo) {
    case E_NAOTERM:
        linha[e->simbolo] = 1;
        break;
    case E_SEQ:
        for (i = 0; i < e->nfilhos; i++) {
            cabecas(e->filhos[i], linha);
            if (!e->filhos[i]->anulavel)
                break;
        }
        break;
    case E_ALT:
    case E_OPC:
    case E_REP:
        for (i = 0; i < e->nfilhos; i++)
            cabecas(e->filhos[i], linha);
        break;
    case E_TERM:
    case E_VAZIO:
        break;
    }
}

/* Recursao a esquerda */
int verif_recursao_esquerda(Gramatica *g)
{
    int i, j, k, n = g->nnts, erros = 0;
    char *alcance = (char *) xcalloc((size_t) n * (size_t) n, 1);
    char *direta = (char *) xcalloc((size_t) n, 1);

    for (i = 0; i < n; i++) {
        cabecas(g->nts[i].regra, alcance + (size_t) i * (size_t) n);
        direta[i] = alcance[(size_t) i * (size_t) n + (size_t) i];
    }
    for (k = 0; k < n; k++)
        for (i = 0; i < n; i++)
            if (alcance[(size_t) i * (size_t) n + (size_t) k])
                for (j = 0; j < n; j++)
                    if (alcance[(size_t) k * (size_t) n + (size_t) j])
                        alcance[(size_t) i * (size_t) n + (size_t) j] = 1;

    for (i = 0; i < n; i++) {
        if (!alcance[(size_t) i * (size_t) n + (size_t) i])
            continue;
        diag_erro(g->nts[i].linha_def, 0,
                  "nao terminal %s tem recursao a esquerda %s",
                  g->nts[i].nome, direta[i] ? "direta" : "indireta");
        erros++;
    }
    free(alcance);
    free(direta);
    return erros;
}

int verif_basica(Gramatica *g, int modo_microc)
{
    int erros = 0;
    if (g->inicial < 0) {
        diag_erro(0, 0, "a gramatica nao tem nenhuma regra");
        return 1;
    }
    erros += verif_indefinidos(g);
    erros += verif_terminais(g, modo_microc);
    erros += verif_inalcancaveis(g);
    if (erros == 0)
        erros += verif_improdutivos(g);
    return erros;
}
