#ifndef GENERADOR_IR_H
#define GENERADOR_IR_H

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "AST.h"
#include "CodigoIntermedio.h"

// ==================================================
// GENERADOR DE CODIGO INTERMEDIO (CUADRUPLAS)
// ==================================================
// Recorre el AST (ya validado por el analizador semantico)
// y produce las cuadruplas de cada funcion.
// ==================================================

class GeneradorIR {

private:

    const Programa& programa;

    ProgramaIR ir;

    // Estado de la funcion que se esta generando
    FuncionIR actual;
    std::unordered_set<std::string> nombresLocales;
    int contadorTemporales = 0;

    // Etiquetas unicas en todo el programa
    int contadorEtiquetas = 0;

    // (etiqueta de 'continue', etiqueta de 'break') de cada while abierto
    std::vector<std::pair<std::string, std::string>> ciclos;

    // Tipo de retorno de cada funcion (para saber si un CALL produce valor)
    std::unordered_map<std::string, Tipo> retornos;

    // Textos de string ya registrados (evita repetirlos)
    std::unordered_map<std::string, int> indiceCadenas;

    // Apoyo
    void emitir(OpIR op, const Operando& a, const Operando& b, const Operando& r);
    Operando nuevoTemporal();
    std::string nuevaEtiqueta();
    Operando variable(const std::string& nombre) const;
    int internarCadena(const std::string& lexema);
    void recolectarRetornos();

    // Generacion
    void generarFuncion(const DeclaracionFuncion& fn);
    void generarSentencia(const Sentencia* sent);
    Operando generarExpresion(const Expresion* expr);
    Operando generarLlamada(const Llamada& llamada);

public:

    explicit GeneradorIR(const Programa& programa);

    ProgramaIR generar();
};

#endif
