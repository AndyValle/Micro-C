#ifndef ERROR_H
#define ERROR_H

#include <string>

// ==================================================
// ERROR DEL COMPILADOR
// ==================================================
// Se lanza (throw) al encontrar el PRIMER error, ya que
// el manual pide que el compilador se detenga ahi.
// Se reutilizara en las fases siguientes (parser, semantico).
// ==================================================

struct ErrorCompilador {
    std::string fase;      // "lexico", "sintactico" o "semantico"
    std::string mensaje;   // descripcion util del error
    int linea;
    int columna;
};

#endif