#ifndef PARSER_H
#define PARSER_H

#include <memory>
#include <string>
#include <vector>

#include "AST.h"
#include "Error.h"
#include "Token.h"

// ==================================================
// ANALIZADOR SINTACTICO DE MICRO-C
// ==================================================
// Parser descendente recursivo. Al encontrar el primer
// error de sintaxis lanza ErrorCompilador y se detiene.
// ==================================================

class Parser {

private:

    std::vector<Token> tokens;
    size_t pos;

    // Navegacion
    const Token& actual() const;
    bool revisar(TokenType tipo) const;
    bool coincide(TokenType tipo);
    const Token& avanzar();
    const Token& consumir(TokenType tipo, const std::string& esperado);

    std::string describir(const Token& token) const;

    [[noreturn]] void error(const Token& token,
                            const std::string& mensaje) const;

    // Declaraciones
    DeclaracionPtr parsearDeclaracion(bool permitirDefinicion);
    DeclaracionPtr parsearFuncion(Tipo tipoRetorno, const Token& nombre,
                                  bool permitirDefinicion);
    std::unique_ptr<CuerpoFuncion> parsearCuerpo();

    // Sentencias
    SentenciaPtr parsearSentencia();
    SentenciaPtr parsearBloque();
    SentenciaPtr parsearReturn();
    SentenciaPtr parsearWhile();
    SentenciaPtr parsearIf();

    // Expresiones (de menor a mayor precedencia)
    ExpresionPtr parsearExpresion();
    ExpresionPtr parsearIgualdad();
    ExpresionPtr parsearRelacional();
    ExpresionPtr parsearSimple();
    ExpresionPtr parsearTermino();
    ExpresionPtr parsearFactor();

    ExpresionPtr crearLiteralEntero(const Token& token, bool conSignoNegativo);

public:

    explicit Parser(const std::vector<Token>& tokens);

    std::unique_ptr<Programa> parsear();
};

#endif
