/*
 * automato.c - Geracao do reconhecedor de uma expressao regular.
 *
 * Etapas:
 *   1. analise da expressao (descida recursiva) construindo direto o AFN
 *      pela construcao de Thompson;
 *   2. determinizacao pela construcao de subconjuntos;
 *   3. emissao do AFD como codigo C (um case por estado) que devolve o
 *      comprimento do maior prefixo reconhecido.
 *
 * As transicoes do AFN guardam CONJUNTOS de caracteres (256 bits), e nao
 * um simbolo so, para que classes como [a-zA-Z0-9_] ocupem uma aresta.
 * O byte 0 nunca faz parte de um conjunto: ele marca o fim da entrada.
 */
#include <stdlib.h>
#include <string.h>
#include "automato.h"
#include "util.h"

/* ------------------------------------------------------------------ */
/* Conjuntos de caracteres                                            */
/* ------------------------------------------------------------------ */

typedef struct {
    unsigned char b[32];
} CharSet;

static void cs_add(CharSet *s, int c)
{
    if (c > 0 && c < 256)
        s->b[c >> 3] = (unsigned char) (s->b[c >> 3] | (1u << (c & 7)));
}

static int cs_tem(const CharSet *s, int c)
{
    return (s->b[c >> 3] >> (c & 7)) & 1;
}

static void cs_negar(CharSet *s)
{
    int c;
    CharSet n;
    memset(&n, 0, sizeof n);
    for (c = 1; c < 256; c++)
        if (!cs_tem(s, c))
            cs_add(&n, c);
    *s = n;
}

/* ------------------------------------------------------------------ */
/* AFN de Thompson                                                    */
/* ------------------------------------------------------------------ */

typedef struct {
    CharSet set;
    int tem_set;        /* 1 se ha transicao por set para next */
    int next;
    int eps1, eps2;     /* transicoes epsilon (-1 = nenhuma) */
} EstadoN;

typedef struct {
    EstadoN *e;
    int n, cap;
} AFN;

typedef struct {
    int ini, fim;       /* o estado fim nunca tem arestas de saida */
} Frag;

static int novo_estado(AFN *a)
{
    EstadoN *s;
    if (a->n == a->cap) {
        a->cap = a->cap ? 2 * a->cap : 32;
        a->e = (EstadoN *) xrealloc(a->e, (size_t) a->cap * sizeof(EstadoN));
    }
    s = &a->e[a->n];
    memset(s, 0, sizeof *s);
    s->next = s->eps1 = s->eps2 = -1;
    return a->n++;
}

static Frag frag_conjunto(AFN *a, const CharSet *set)
{
    Frag f;
    f.ini = novo_estado(a);
    f.fim = novo_estado(a);
    a->e[f.ini].set = *set;
    a->e[f.ini].tem_set = 1;
    a->e[f.ini].next = f.fim;
    return f;
}

/* ------------------------------------------------------------------ */
/* Analise da expressao regular                                       */
/*                                                                    */
/*   alt  = cat { "|" cat }                                           */
/*   cat  = { rep }                                                   */
/*   rep  = atomo { "*" | "+" | "?" }                                 */
/*   atomo = "(" alt ")" | "[" classe "]" | "." | "\" c | literal      */
/* ------------------------------------------------------------------ */

typedef struct {
    const char *s;
    size_t i;
    AFN *afn;
    const char *erro;
} RP;

static Frag rp_alt(RP *p);

static int escape(int c)
{
    switch (c) {
    case 'n': return '\n';
    case 't': return '\t';
    case 'r': return '\r';
    case 'f': return '\f';
    case 'v': return '\v';
    default:  return c;
    }
}

/* Le um caractere (possivelmente escapado); -1 em erro. */
static int ler_char(RP *p)
{
    int c = (unsigned char) p->s[p->i];
    if (c == '\\') {
        p->i++;
        c = (unsigned char) p->s[p->i];
        if (c == '\0') {
            p->erro = "barra invertida no fim da expressao";
            return -1;
        }
        c = escape(c);
    }
    p->i++;
    return c;
}

static Frag rp_classe(RP *p)
{
    CharSet set;
    int neg = 0, primeiro = 1, lo, hi, c;
    Frag f;

    memset(&set, 0, sizeof set);
    memset(&f, 0, sizeof f);
    p->i++;                                   /* [ */
    if (p->s[p->i] == '^') {
        neg = 1;
        p->i++;
    }
    for (;;) {
        if (p->s[p->i] == '\0') {
            p->erro = "classe de caracteres '[' nao fechada";
            return f;
        }
        if (p->s[p->i] == ']' && !primeiro)
            break;
        primeiro = 0;
        lo = ler_char(p);
        if (lo < 0)
            return f;
        if (p->s[p->i] == '-' && p->s[p->i + 1] != ']' && p->s[p->i + 1] != '\0') {
            p->i++;
            hi = ler_char(p);
            if (hi < 0)
                return f;
            if (hi < lo) {
                p->erro = "intervalo invertido em classe de caracteres";
                return f;
            }
            for (c = lo; c <= hi; c++)
                cs_add(&set, c);
        } else {
            cs_add(&set, lo);
        }
    }
    p->i++;                                   /* ] */
    if (neg)
        cs_negar(&set);
    return frag_conjunto(p->afn, &set);
}

static Frag rp_atomo(RP *p)
{
    CharSet set;
    Frag f;
    int c;

    memset(&f, 0, sizeof f);
    memset(&set, 0, sizeof set);
    c = (unsigned char) p->s[p->i];
    if (c == '(') {
        p->i++;
        f = rp_alt(p);
        if (p->erro != NULL)
            return f;
        if (p->s[p->i] != ')') {
            p->erro = "parentese '(' nao fechado";
            return f;
        }
        p->i++;
        return f;
    }
    if (c == '[')
        return rp_classe(p);
    if (c == '*' || c == '+' || c == '?') {
        p->erro = "operador de repeticao sem operando";
        return f;
    }
    if (c == '.') {
        p->i++;
        cs_add(&set, '\n');
        cs_negar(&set);
        return frag_conjunto(p->afn, &set);
    }
    c = ler_char(p);
    if (c < 0)
        return f;
    cs_add(&set, c);
    return frag_conjunto(p->afn, &set);
}

static Frag rp_rep(RP *p)
{
    Frag f = rp_atomo(p), r;
    AFN *a = p->afn;
    int n0, n1, op;

    while (p->erro == NULL && (p->s[p->i] == '*' || p->s[p->i] == '+'
                               || p->s[p->i] == '?')) {
        op = p->s[p->i++];
        n0 = novo_estado(a);
        n1 = novo_estado(a);
        if (op == '*') {                      /* n0 -> f -> n1, com laco e salto */
            a->e[n0].eps1 = f.ini;
            a->e[n0].eps2 = n1;
            a->e[f.fim].eps1 = f.ini;
            a->e[f.fim].eps2 = n1;
            r.ini = n0;
        } else if (op == '+') {               /* f -> n1, com laco */
            a->e[f.fim].eps1 = f.ini;
            a->e[f.fim].eps2 = n1;
            r.ini = f.ini;
        } else {                              /* ?: n0 -> f -> n1 ou n0 -> n1 */
            a->e[n0].eps1 = f.ini;
            a->e[n0].eps2 = n1;
            a->e[f.fim].eps1 = n1;
            r.ini = n0;
        }
        r.fim = n1;
        f = r;
    }
    return f;
}

static Frag rp_cat(RP *p)
{
    Frag f, g;
    int tem = 0;

    memset(&f, 0, sizeof f);
    while (p->erro == NULL && p->s[p->i] != '\0' && p->s[p->i] != '|'
           && p->s[p->i] != ')') {
        g = rp_rep(p);
        if (p->erro != NULL)
            return f;
        if (!tem) {
            f = g;
            tem = 1;
        } else {
            p->afn->e[f.fim].eps1 = g.ini;    /* concatenacao */
            f.fim = g.fim;
        }
    }
    if (!tem) {                               /* sequencia vazia: so epsilon */
        f.ini = f.fim = novo_estado(p->afn);
    }
    return f;
}

static Frag rp_alt(RP *p)
{
    Frag a = rp_cat(p), b;
    int n0, n1;

    while (p->erro == NULL && p->s[p->i] == '|') {
        p->i++;
        b = rp_cat(p);
        if (p->erro != NULL)
            return a;
        n0 = novo_estado(p->afn);
        n1 = novo_estado(p->afn);
        p->afn->e[n0].eps1 = a.ini;
        p->afn->e[n0].eps2 = b.ini;
        p->afn->e[a.fim].eps1 = n1;
        p->afn->e[b.fim].eps1 = n1;
        a.ini = n0;
        a.fim = n1;
    }
    return a;
}

/* ------------------------------------------------------------------ */
/* AFD (construcao de subconjuntos)                                   */
/* ------------------------------------------------------------------ */

typedef struct {
    char *conj;          /* conj[i] = 1 se o estado i do AFN pertence */
    int final;
    int next[256];       /* -1 = sem transicao */
} EstadoD;

typedef struct {
    EstadoD *e;
    int n, cap;
} AFD;

/* Fecho-epsilon de conj (modifica-o). */
static void fecho(const AFN *a, char *conj)
{
    int *pilha = (int *) xmalloc((size_t) a->n * sizeof(int));
    int sp = 0, i, s, t;

    for (i = 0; i < a->n; i++)
        if (conj[i])
            pilha[sp++] = i;
    while (sp > 0) {
        s = pilha[--sp];
        t = a->e[s].eps1;
        if (t >= 0 && !conj[t]) {
            conj[t] = 1;
            pilha[sp++] = t;
        }
        t = a->e[s].eps2;
        if (t >= 0 && !conj[t]) {
            conj[t] = 1;
            pilha[sp++] = t;
        }
    }
    free(pilha);
}

/* Procura o subconjunto no AFD; se nao existir, cria. Libera conj se ja existia. */
static int achar_ou_criar(AFD *d, const AFN *a, char *conj, int fim_afn)
{
    int i;
    EstadoD *s;

    for (i = 0; i < d->n; i++)
        if (memcmp(d->e[i].conj, conj, (size_t) a->n) == 0) {
            free(conj);
            return i;
        }
    if (d->n == d->cap) {
        d->cap = d->cap ? 2 * d->cap : 16;
        d->e = (EstadoD *) xrealloc(d->e, (size_t) d->cap * sizeof(EstadoD));
    }
    s = &d->e[d->n];
    s->conj = conj;
    s->final = conj[fim_afn] != 0;
    for (i = 0; i < 256; i++)
        s->next[i] = -1;
    return d->n++;
}

static void determinizar(const AFN *a, int ini, int fim, AFD *d)
{
    int i, c, s, k;
    char *conj;

    conj = (char *) xcalloc((size_t) a->n, 1);
    conj[ini] = 1;
    fecho(a, conj);
    achar_ou_criar(d, a, conj, fim);

    for (i = 0; i < d->n; i++) {              /* d->n cresce durante o laco */
        for (c = 1; c < 256; c++) {
            conj = (char *) xcalloc((size_t) a->n, 1);
            k = 0;
            for (s = 0; s < a->n; s++)
                if (d->e[i].conj[s] && a->e[s].tem_set && cs_tem(&a->e[s].set, c)) {
                    conj[a->e[s].next] = 1;
                    k = 1;
                }
            if (!k) {
                free(conj);
                continue;
            }
            fecho(a, conj);
            k = achar_ou_criar(d, a, conj, fim);
            d->e[i].next[c] = k;              /* d->e pode ter sido realocado */
        }
    }
}

/* ------------------------------------------------------------------ */
/* Emissao do codigo C                                                */
/* ------------------------------------------------------------------ */

/* Constante de caractere: 'x' se imprimivel, senao o numero. */
static void emitir_char(FILE *f, int c)
{
    if (c >= 32 && c <= 126 && c != '\'' && c != '\\')
        fprintf(f, "'%c'", c);
    else
        fprintf(f, "%d", c);
}

/* Condicao sobre c para o conjunto de bytes cujo destino e alvo. */
static void emitir_condicao(FILE *f, const EstadoD *s, int alvo)
{
    int lo, hi, nfaixas = 0, k = 0;

    for (lo = 1; lo < 256; lo++)
        if (s->next[lo] == alvo && (lo == 1 || s->next[lo - 1] != alvo))
            nfaixas++;

    for (lo = 1; lo < 256; lo++) {
        if (s->next[lo] != alvo || (lo > 1 && s->next[lo - 1] == alvo))
            continue;
        for (hi = lo; hi < 255 && s->next[hi + 1] == alvo; hi++)
            ;
        if (k++ > 0)
            fputs(" || ", f);
        if (lo == hi) {
            fputs("c == ", f);
            emitir_char(f, lo);
        } else if (hi == 255) {
            fputs(nfaixas > 1 ? "(c >= " : "c >= ", f);
            emitir_char(f, lo);
            if (nfaixas > 1)
                fputc(')', f);
        } else {
            fputs(nfaixas > 1 ? "(c >= " : "c >= ", f);
            emitir_char(f, lo);
            fputs(" && c <= ", f);
            emitir_char(f, hi);
            if (nfaixas > 1)
                fputc(')', f);
        }
    }
}

static void emitir_afd(FILE *f, const char *nome, const AFD *d)
{
    int i, c, alvo, primeiro;
    char *visto;

    fprintf(f, "static size_t %s(const char *s)\n{\n", nome);
    fputs("    size_t i = 0, ultimo = 0;\n    int estado = 0;\n\n", f);
    fputs("    for (;;) {\n        unsigned char c = (unsigned char) s[i];\n\n", f);
    fputs("        switch (estado) {\n", f);
    visto = (char *) xcalloc((size_t) d->n, 1);
    for (i = 0; i < d->n; i++) {
        fprintf(f, "        case %d:\n", i);
        memset(visto, 0, (size_t) d->n);
        primeiro = 1;
        for (c = 1; c < 256; c++) {
            alvo = d->e[i].next[c];
            if (alvo < 0 || visto[alvo])
                continue;
            visto[alvo] = 1;
            fputs(primeiro ? "            if (" : "            } else if (", f);
            emitir_condicao(f, &d->e[i], alvo);
            fputs(") {\n", f);
            fprintf(f, "                estado = %d;\n", alvo);
            if (d->e[alvo].final)
                fputs("                ultimo = i + 1;\n", f);
            primeiro = 0;
        }
        if (primeiro) {
            fputs("            return ultimo;\n", f);
        } else {
            fputs("            } else {\n                return ultimo;\n            }\n", f);
            fputs("            break;\n", f);
        }
    }
    free(visto);
    fputs("        default:\n            return ultimo;\n        }\n", f);
    fputs("        i++;\n    }\n}\n\n", f);
}

int automato_emitir(FILE *f, const char *nome, const char *regex, const char **erro)
{
    AFN afn;
    AFD afd;
    RP p;
    Frag r;
    int i;

    memset(&afn, 0, sizeof afn);
    memset(&afd, 0, sizeof afd);
    p.s = regex;
    p.i = 0;
    p.afn = &afn;
    p.erro = NULL;

    r = rp_alt(&p);
    if (p.erro == NULL && regex[p.i] == ')')
        p.erro = "parentese ')' sem o '(' correspondente";
    if (p.erro != NULL) {
        *erro = p.erro;
        free(afn.e);
        return -1;
    }
    determinizar(&afn, r.ini, r.fim, &afd);
    emitir_afd(f, nome, &afd);

    for (i = 0; i < afd.n; i++)
        free(afd.e[i].conj);
    free(afd.e);
    free(afn.e);
    return 0;
}
