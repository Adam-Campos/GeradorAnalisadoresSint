/*
 * automato.h - Expressao regular -> AFN (Thompson) -> AFD (subconjuntos)
 * -> codigo C.
 *
 * Subconjunto aceito: concatenacao, |, *, +, ?, parenteses, classes
 * [a-z] e [^...], o ponto (qualquer caractere menos \n) e escapes
 * (\n \t \r \f \v e \c para qualquer outro caractere c).
 */
#ifndef AUTOMATO_H
#define AUTOMATO_H

#include <stdio.h>

/* Emite em f a funcao
 *     static size_t <nome>(const char *s)
 * que devolve o comprimento do MAIOR prefixo de s reconhecido pela
 * expressao regular (0 se nenhum). Retorna 0 em caso de sucesso; se a
 * expressao for invalida, nada e escrito, *erro recebe a causa e o
 * retorno e -1. */
int automato_emitir(FILE *f, const char *nome, const char *regex, const char **erro);

#endif
