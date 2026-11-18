#ifndef TOKEN_H
#define TOKEN_H

#include <string>

// ==================================================
// TIPOS DE TOKEN DE MICRO-C
// ==================================================

enum class TokenType {

    // Palabras reservadas
    INT, CHAR, VOID,
    RETURN, WHILE, IF, ELSE, BREAK, CONTINUE,

    // Identificadores y constantes
    IDENTIFIER,
    INTEGER_CONSTANT,
    CHAR_CONSTANT,
    STRING_CONSTANT,

    // Operadores
    PLUS, MINUS, STAR, SLASH,          // + - * /
    ASSIGN,                            // =
    EQ, NEQ,                           // == !=
    LT, LE, GE, GT,                    // < <= >= >

    // Delimitadores
    LPAREN, RPAREN,                    // ( )
    LBRACE, RBRACE,                    // { }
    COMMA, SEMICOLON,                  // , ;
    ELLIPSIS,                          // ...

    END_OF_FILE
};

// ==================================================
// TOKEN
// ==================================================
// linea y columna empiezan en 1.
// valor: numero de INTEGER_CONSTANT o codigo ASCII de CHAR_CONSTANT.
// ==================================================

struct Token {

    TokenType tipo;
    std::string lexema;
    int linea;
    int columna;
    int valor;

    Token(TokenType tipo, const std::string& lexema,
          int linea, int columna, int valor = 0)
        : tipo(tipo), lexema(lexema),
          linea(linea), columna(columna), valor(valor) {}
};

// Nombre legible del tipo de token (para imprimir la tabla)

inline std::string nombreToken(TokenType t) {

    switch (t) {

        case TokenType::INT:              return "INT";
        case TokenType::CHAR:             return "CHAR";
        case TokenType::VOID:             return "VOID";
        case TokenType::RETURN:           return "RETURN";
        case TokenType::WHILE:            return "WHILE";
        case TokenType::IF:               return "IF";
        case TokenType::ELSE:             return "ELSE";
        case TokenType::BREAK:            return "BREAK";
        case TokenType::CONTINUE:         return "CONTINUE";

        case TokenType::IDENTIFIER:       return "IDENTIFIER";
        case TokenType::INTEGER_CONSTANT: return "INTEGER_CONSTANT";
        case TokenType::CHAR_CONSTANT:    return "CHAR_CONSTANT";
        case TokenType::STRING_CONSTANT:  return "STRING_CONSTANT";

        case TokenType::PLUS:             return "PLUS";
        case TokenType::MINUS:            return "MINUS";
        case TokenType::STAR:             return "STAR";
        case TokenType::SLASH:            return "SLASH";
        case TokenType::ASSIGN:           return "ASSIGN";
        case TokenType::EQ:               return "EQ";
        case TokenType::NEQ:              return "NEQ";
        case TokenType::LT:               return "LT";
        case TokenType::LE:               return "LE";
        case TokenType::GE:               return "GE";
        case TokenType::GT:               return "GT";

        case TokenType::LPAREN:           return "LPAREN";
        case TokenType::RPAREN:           return "RPAREN";
        case TokenType::LBRACE:           return "LBRACE";
        case TokenType::RBRACE:           return "RBRACE";
        case TokenType::COMMA:            return "COMMA";
        case TokenType::SEMICOLON:        return "SEMICOLON";
        case TokenType::ELLIPSIS:         return "ELLIPSIS";

        case TokenType::END_OF_FILE:      return "EOF";
    }

    return "DESCONOCIDO";
}

#endif