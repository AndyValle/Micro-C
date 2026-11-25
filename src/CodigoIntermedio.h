#ifndef CODIGO_INTERMEDIO_H
#define CODIGO_INTERMEDIO_H

#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "AST.h"

// ==================================================
// CODIGO INTERMEDIO: CUADRUPLAS
// ==================================================
// Cada instruccion tiene la forma (operacion, arg1, arg2, resultado).
// Los valores intermedios se guardan en temporales (t1, t2, ...) y
// el control de flujo se hace con etiquetas (L1, L2, ...) y saltos.
// ==================================================

struct Operando
{
    enum class Clase
    {
        NINGUNO,
        LOCAL,       // variable local o parametro
        GLOBAL,      // variable global
        TEMPORAL,    // t1, t2, ...
        CONSTANTE,   // numero o caracter (guardado como su valor)
        CADENA,      // indice en la tabla de strings
        ETIQUETA,    // destino de un salto
        FUNCION      // nombre de la funcion llamada
    };

    Clase clase = Clase::NINGUNO;
    std::string nombre;
    int valor = 0;      // CONSTANTE: el numero; CADENA: el indice

    bool vacio() const { return clase == Clase::NINGUNO; }

    static Operando ninguno()                  { return Operando(); }
    static Operando local(const std::string& n)    { return make(Clase::LOCAL, n, 0); }
    static Operando global(const std::string& n)   { return make(Clase::GLOBAL, n, 0); }
    static Operando temporal(int numero)       { return make(Clase::TEMPORAL, "t" + std::to_string(numero), numero); }
    static Operando constante(int v)           { return make(Clase::CONSTANTE, "", v); }
    static Operando cadena(int indice)         { return make(Clase::CADENA, "", indice); }
    static Operando etiqueta(const std::string& n) { return make(Clase::ETIQUETA, n, 0); }
    static Operando funcion(const std::string& n)  { return make(Clase::FUNCION, n, 0); }

private:

    static Operando make(Clase c, const std::string& n, int v)
    {
        Operando o;
        o.clase = c;
        o.nombre = n;
        o.valor = v;
        return o;
    }
};


enum class OpIR
{
    ADD, SUB, MUL, DIV,          // resultado = arg1 op arg2
    NEG,                         // resultado = -arg1
    LT, LE, GT, GE, EQ, NE,      // resultado = 1 si se cumple, 0 si no
    ASSIGN,                      // resultado = arg1
    LABEL,                       // resultado: etiqueta
    JUMP,                        // salta a resultado
    JUMPF,                       // si arg1 == 0, salta a resultado
    PARAM,                       // pone arg1 como argumento de la proxima llamada
    CALL,                        // resultado = arg1(funcion) con arg2 argumentos
    RETURN                       // retorna arg1 (o nada)
};

inline std::string nombreOp(OpIR op)
{
    switch (op)
    {
        case OpIR::ADD:    return "ADD";
        case OpIR::SUB:    return "SUB";
        case OpIR::MUL:    return "MUL";
        case OpIR::DIV:    return "DIV";
        case OpIR::NEG:    return "NEG";
        case OpIR::LT:     return "LT";
        case OpIR::LE:     return "LE";
        case OpIR::GT:     return "GT";
        case OpIR::GE:     return "GE";
        case OpIR::EQ:     return "EQ";
        case OpIR::NE:     return "NE";
        case OpIR::ASSIGN: return "ASSIGN";
        case OpIR::LABEL:  return "LABEL";
        case OpIR::JUMP:   return "JUMP";
        case OpIR::JUMPF:  return "JUMPF";
        case OpIR::PARAM:  return "PARAM";
        case OpIR::CALL:   return "CALL";
        case OpIR::RETURN: return "RETURN";
    }
    return "?";
}


struct Cuadrupla
{
    OpIR op;
    Operando arg1;
    Operando arg2;
    Operando resultado;
};


struct VariableIR
{
    std::string nombre;
    Tipo tipo;
};


struct FuncionIR
{
    std::string nombre;
    Tipo retorno = Tipo::VOID;
    std::vector<VariableIR> params;
    std::vector<VariableIR> locales;
    int temporales = 0;                 // cuantos temporales usa
    std::vector<Cuadrupla> codigo;
};


struct ProgramaIR
{
    std::vector<VariableIR> globales;
    std::vector<std::string> cadenas;   // texto ya sin escapes
    std::vector<FuncionIR> funciones;

    int totalCuadruplas() const
    {
        int total = 0;

        for (const auto& f : funciones)
        {
            total += static_cast<int>(f.codigo.size());
        }

        return total;
    }
};


// ==================================================
// IMPRESION
// ==================================================

// Muestra un string con sus escapes visibles (\n, \", \\)

inline std::string textoEscapado(const std::string& texto)
{
    std::string salida = "\"";

    for (char c : texto)
    {
        if (c == '\n')      salida += "\\n";
        else if (c == '"')  salida += "\\\"";
        else if (c == '\\') salida += "\\\\";
        else                salida += c;
    }

    return salida + "\"";
}


inline std::string textoOperando(const Operando& o, const ProgramaIR& ir)
{
    switch (o.clase)
    {
        case Operando::Clase::NINGUNO:    return "";
        case Operando::Clase::CONSTANTE:  return std::to_string(o.valor);
        case Operando::Clase::CADENA:     return textoEscapado(ir.cadenas[o.valor]);
        default:                          return o.nombre;
    }
}


inline void imprimirCodigoIntermedio(const ProgramaIR& ir)
{
    std::cout << "==========================================================" << std::endl;
    std::cout << "                    CODIGO INTERMEDIO                     " << std::endl;
    std::cout << "==========================================================" << std::endl;
    std::cout << std::endl;

    if (!ir.globales.empty())
    {
        std::cout << "Variables globales:";

        for (const auto& g : ir.globales)
        {
            std::cout << " " << g.nombre << " (" << nombreTipo(g.tipo) << ")";
        }

        std::cout << std::endl;
    }

    for (size_t i = 0; i < ir.cadenas.size(); i++)
    {
        std::cout << "Cadena " << i << ": " << textoEscapado(ir.cadenas[i]) << std::endl;
    }

    for (const auto& f : ir.funciones)
    {
        std::cout << std::endl;
        std::cout << "FUNCION " << f.nombre << " (";

        for (size_t i = 0; i < f.params.size(); i++)
        {
            std::cout << (i > 0 ? ", " : "") << f.params[i].nombre;
        }

        std::cout << ")   retorna " << nombreTipo(f.retorno) << std::endl;

        std::cout << "  locales:";

        for (const auto& l : f.locales)
        {
            std::cout << " " << l.nombre;
        }

        std::cout << "   temporales: " << f.temporales << std::endl;
        std::cout << std::endl;

        std::cout << "  "
                  << std::left << std::setw(5)  << "#"
                  << std::left << std::setw(9)  << "OP"
                  << std::left << std::setw(26) << "ARG1"
                  << std::left << std::setw(12) << "ARG2"
                  << "RESULTADO" << std::endl;

        std::cout << "  "
                  << std::left << std::setw(5)  << "---"
                  << std::left << std::setw(9)  << "--"
                  << std::left << std::setw(26) << "----"
                  << std::left << std::setw(12) << "----"
                  << "---------" << std::endl;

        for (size_t i = 0; i < f.codigo.size(); i++)
        {
            const Cuadrupla& q = f.codigo[i];

            std::cout << "  "
                      << std::left << std::setw(5)  << i
                      << std::left << std::setw(9)  << nombreOp(q.op)
                      << std::left << std::setw(26) << textoOperando(q.arg1, ir)
                      << std::left << std::setw(12) << textoOperando(q.arg2, ir)
                      << textoOperando(q.resultado, ir) << std::endl;
        }
    }

    std::cout << std::endl;
}

#endif
