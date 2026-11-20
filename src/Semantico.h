#ifndef SEMANTICO_H
#define SEMANTICO_H

#include <string>
#include <vector>

#include "AST.h"
#include "Error.h"
#include "TablaSimbolos.h"

// ==================================================
// ANALIZADOR SEMANTICO DE MICRO-C
// ==================================================
// Recorre el AST una sola vez, en orden, y verifica:
//  - identificadores declarados antes de usarse y sin repetirse
//    dentro del mismo ambito
//  - existencia de main sin parametros
//  - tipos en expresiones, asignaciones y return
//  - numero y tipo de argumentos en las llamadas
//  - return, break y continue usados donde corresponde
// Al encontrar el primer error lanza ErrorCompilador.
// ==================================================

class Semantico {

private:

    const Programa& programa;

    TablaSimbolos tabla;
    std::vector<EntradaTabla> registroTabla;

    const DeclaracionFuncion* funcionActual = nullptr;
    int profundidadCiclo = 0;
    std::string nombreAmbito;

    // Errores
    [[noreturn]] void error(const std::string& mensaje,
                            int linea, int columna) const;
    [[noreturn]] void error(const std::string& mensaje, const Nodo& nodo) const;
    [[noreturn]] void errorDuplicado(const std::string& nombre,
                                     const Simbolo& previo,
                                     int linea, int columna) const;

    // Tabla
    void registrar(const std::string& nombre, const std::string& clase,
                   const std::string& tipo, int linea, int columna);
    void registrarBiblioteca();
    Simbolo simboloDe(const DeclaracionFuncion& fn) const;
    std::string describirFirma(const Simbolo& s) const;
    bool mismaFirma(const Simbolo& a, const Simbolo& b) const;

    // Declaraciones
    void analizarDeclaracion(const Declaracion* decl);
    void declararVariables(const DeclaracionVariable& decl);
    void analizarFuncion(const DeclaracionFuncion& fn);
    void analizarCuerpo(const DeclaracionFuncion& fn);
    void verificarMain();

    // Sentencias
    void analizarSentencia(const Sentencia* sent);
    void analizarReturn(const SentenciaReturn& sent);
    void exigirCondicion(const Expresion* condicion, const std::string& donde);

    // Expresiones: devuelven el tipo de la expresion
    TipoSem tipoExpresion(const Expresion* expr);
    TipoSem tipoLlamada(const Llamada& llamada);
    TipoSem validarPrintf(const Llamada& llamada);
    TipoSem validarScanf(const Llamada& llamada);
    std::vector<char> conversionesDe(const LiteralString& formato) const;

public:

    explicit Semantico(const Programa& programa);

    void analizar();

    const std::vector<EntradaTabla>& registro() const;
};

#endif
