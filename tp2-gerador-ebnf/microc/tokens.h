/*
 * tokens.h - Vocabulario de tokens de Micro C e valor semantico do scanner.
 *
 * Extraido do microc.flex do TP1 para que o scanner (flex) e o analisador
 * gerado compartilhem as mesmas definicoes (secao 3.1 do enunciado do TP2).
 * Os tokens de um caractere usam os valores deste enum, e nao o codigo ASCII.
 */
#ifndef TOKENS_H
#define TOKENS_H

typedef enum {
    /* Tokens fundamentais */
    UNDEF,          /* token indefinido (usado para reportar erros) */
    ID,             /* identificador                                */
    END_OF_FILE,    /* fim de arquivo                                */

    /* Constantes literais */
    INTEGERCONST,
    CHARCONST,
    STRINGCONST,

    /* Operadores aritmeticos */
    PLUS, MINUS, MUL, DIV, MOD,

    /* Operadores relacionais e logicos */
    EQ, NEQ, LT, GT, LEQ, GEQ, AND, OR, NOT,

    /* Simbolos de atribuicao e pontuacao */
    ASSIGN, SEMICOLON, COMMA, LPAREN, RPAREN,
    LBRACE, RBRACE, LBRACKET, RBRACKET,

    /* Palavras reservadas */
    MAIN, IF, ELSE, FOR, RETURN, INT, CHAR, PRINT
} TokenType;

/* Um token entregue ao analisador. */
typedef struct {
    TokenType tipo;
    char lexema[100];
    int linha;
} Token;

typedef struct Symbol {
    char *texto;             /* copia dos bytes do lexema, com um '\0' extra no final */
    int tamanho;             /* quantidade de bytes validos (pode haver '\0' no meio) */
    struct Symbol *proximo;  /* proximo simbolo no mesmo balde (encadeamento) */
} Symbol;

typedef struct {
    Symbol *symbol;
    char *error_msg;
} YYSTYPE;

/* Definidos em microc.flex */
extern YYSTYPE microc_yylval;
extern int linha_atual;
void libera_memoria_scanner(void);

#endif
