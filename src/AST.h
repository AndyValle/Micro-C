#ifndef AST_H
#define AST_H

#include <memory>
#include <string>
#include <utility>
#include <vector>

// ==================================================
// ARBOL DE SINTAXIS ABSTRACTA (AST) DE MICRO-C
// ==================================================
// Todos los nodos guardan linea y columna para poder
// reportar errores semanticos con posicion exacta.
// ==================================================

enum class Tipo { INT, CHAR, VOID };

inline std::string nombreTipo(Tipo t)
{
    switch (t)
    {
        case Tipo::INT:  return "int";
        case Tipo::CHAR: return "char";
        case Tipo::VOID: return "void";
    }
    return "?";
}

struct Nodo
{
    int linea;
    int columna;

    Nodo(int linea, int columna) : linea(linea), columna(columna) {}
    virtual ~Nodo() = default;
};


// ==================================================
// EXPRESIONES
// ==================================================

struct Expresion : public Nodo
{
    using Nodo::Nodo;
};

using ExpresionPtr = std::unique_ptr<Expresion>;


struct LiteralEntero : public Expresion
{
    int valor;   // ya con signo (los hexadecimales de 16 bits se interpretan en complemento a dos)

    LiteralEntero(int l, int c, int valor)
        : Expresion(l, c), valor(valor) {}
};

struct LiteralChar : public Expresion
{
    std::string lexema;   // tal como aparece, con comillas
    int valor;            // codigo ASCII

    LiteralChar(int l, int c, std::string lexema, int valor)
        : Expresion(l, c), lexema(std::move(lexema)), valor(valor) {}
};

struct LiteralString : public Expresion
{
    std::string lexema;   // tal como aparece, con comillas y escapes

    LiteralString(int l, int c, std::string lexema)
        : Expresion(l, c), lexema(std::move(lexema)) {}
};

struct Identificador : public Expresion
{
    std::string nombre;

    Identificador(int l, int c, std::string nombre)
        : Expresion(l, c), nombre(std::move(nombre)) {}
};

struct ExpresionUnaria : public Expresion
{
    std::string operador;   // "+" o "-"
    ExpresionPtr operando;

    ExpresionUnaria(int l, int c, std::string operador, ExpresionPtr operando)
        : Expresion(l, c), operador(std::move(operador)),
          operando(std::move(operando)) {}
};

struct ExpresionBinaria : public Expresion
{
    ExpresionPtr izquierda;
    std::string operador;   // + - * / < <= >= > == !=
    ExpresionPtr derecha;

    ExpresionBinaria(int l, int c, ExpresionPtr izquierda,
                     std::string operador, ExpresionPtr derecha)
        : Expresion(l, c), izquierda(std::move(izquierda)),
          operador(std::move(operador)), derecha(std::move(derecha)) {}
};

struct Asignacion : public Expresion
{
    std::string nombre;   // variable a la que se asigna
    ExpresionPtr valor;

    Asignacion(int l, int c, std::string nombre, ExpresionPtr valor)
        : Expresion(l, c), nombre(std::move(nombre)),
          valor(std::move(valor)) {}
};

struct Llamada : public Expresion
{
    std::string nombre;
    std::vector<ExpresionPtr> argumentos;

    Llamada(int l, int c, std::string nombre)
        : Expresion(l, c), nombre(std::move(nombre)) {}
};


// ==================================================
// SENTENCIAS
// ==================================================

struct Sentencia : public Nodo
{
    using Nodo::Nodo;
};

using SentenciaPtr = std::unique_ptr<Sentencia>;


struct Bloque : public Sentencia
{
    std::vector<SentenciaPtr> sentencias;

    Bloque(int l, int c) : Sentencia(l, c) {}
};

struct SentenciaExpresion : public Sentencia
{
    ExpresionPtr expresion;

    SentenciaExpresion(int l, int c, ExpresionPtr expresion)
        : Sentencia(l, c), expresion(std::move(expresion)) {}
};

struct SentenciaReturn : public Sentencia
{
    ExpresionPtr valor;   // nullptr si es "return;"

    SentenciaReturn(int l, int c, ExpresionPtr valor)
        : Sentencia(l, c), valor(std::move(valor)) {}
};

struct SentenciaWhile : public Sentencia
{
    ExpresionPtr condicion;
    SentenciaPtr cuerpo;

    SentenciaWhile(int l, int c, ExpresionPtr condicion, SentenciaPtr cuerpo)
        : Sentencia(l, c), condicion(std::move(condicion)),
          cuerpo(std::move(cuerpo)) {}
};

struct SentenciaIf : public Sentencia
{
    ExpresionPtr condicion;
    SentenciaPtr entonces;
    SentenciaPtr sino;   // nullptr si no hay else

    SentenciaIf(int l, int c, ExpresionPtr condicion,
                SentenciaPtr entonces, SentenciaPtr sino)
        : Sentencia(l, c), condicion(std::move(condicion)),
          entonces(std::move(entonces)), sino(std::move(sino)) {}
};

struct SentenciaBreak : public Sentencia
{
    SentenciaBreak(int l, int c) : Sentencia(l, c) {}
};

struct SentenciaContinue : public Sentencia
{
    SentenciaContinue(int l, int c) : Sentencia(l, c) {}
};


// ==================================================
// DECLARACIONES
// ==================================================

struct Declaracion : public Nodo
{
    using Nodo::Nodo;
};

using DeclaracionPtr = std::unique_ptr<Declaracion>;


struct Declarador
{
    std::string nombre;
    int linea;
    int columna;
};

struct DeclaracionVariable : public Declaracion
{
    Tipo tipo;
    std::vector<Declarador> variables;

    DeclaracionVariable(int l, int c, Tipo tipo)
        : Declaracion(l, c), tipo(tipo) {}
};

struct Parametro
{
    Tipo tipo;
    bool esPuntero;   // solo "char *nombre" en prototipos
    std::string nombre;
    int linea;
    int columna;
};

// Cuerpo de una funcion: primero declaraciones, luego sentencias
struct CuerpoFuncion
{
    std::vector<DeclaracionPtr> declaraciones;
    std::vector<SentenciaPtr> sentencias;
};

// Sirve tanto para prototipos (cuerpo == nullptr) como para definiciones
struct DeclaracionFuncion : public Declaracion
{
    Tipo tipoRetorno = Tipo::VOID;
    std::string nombre;
    std::vector<Parametro> parametros;
    bool paramsVoid = false;   // se declaro como f(void)
    bool variadica = false;    // termina en "..."
    std::unique_ptr<CuerpoFuncion> cuerpo;

    DeclaracionFuncion(int l, int c) : Declaracion(l, c) {}

    bool esDefinicion() const { return cuerpo != nullptr; }
};


struct Programa
{
    std::vector<DeclaracionPtr> declaraciones;
};

#endif
