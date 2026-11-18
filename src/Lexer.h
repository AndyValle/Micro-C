#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>

#include "Token.h"
#include "Error.h"

// ==================================================
// ANALIZADOR LEXICO DE MICRO-C
// ==================================================
// analizar() recorre el codigo fuente y devuelve la lista
// de tokens. Al encontrar el primer error lexico lanza una
// excepcion ErrorCompilador (el compilador se detiene ahi).
// ==================================================

class Lexer {

private:

    std::string codigo;
    size_t pos;
    int linea;
    int columna;

    // Navegacion
    bool fin() const;
    char actual() const;
    char siguiente() const;
    char avanzar();

    [[noreturn]] void error(const std::string& mensaje,
                            int linea, int columna) const;

    // Espacios y comentarios /* ... */
    void saltarEspaciosYComentarios();

    // Un metodo por cada clase de token
    Token leerIdentificador();
    Token leerNumero();
    Token leerCaracter();
    Token leerCadena();
    Token leerSimbolo();

public:

    explicit Lexer(const std::string& codigo);

    std::vector<Token> analizar();
};

#endif