#ifndef TABLA_SIMBOLOS_H
#define TABLA_SIMBOLOS_H

#include <deque>
#include <string>
#include <unordered_map>
#include <vector>

#include "AST.h"

// ==================================================
// TIPOS PARA EL ANALISIS SEMANTICO
// ==================================================
// Ademas de int, char y void, las constantes de string
// tienen su propio tipo (CADENA): solo pueden usarse como
// argumento de un parametro "char *" (por ejemplo, el
// formato de printf y scanf).
// ==================================================

enum class TipoSem { INT, CHAR, VOID, CADENA };

inline std::string nombreTipoSem(TipoSem t)
{
    switch (t)
    {
        case TipoSem::INT:    return "int";
        case TipoSem::CHAR:   return "char";
        case TipoSem::VOID:   return "void";
        case TipoSem::CADENA: return "string";
    }
    return "?";
}

inline TipoSem aTipoSem(Tipo t)
{
    switch (t)
    {
        case Tipo::INT:  return TipoSem::INT;
        case Tipo::CHAR: return TipoSem::CHAR;
        case Tipo::VOID: return TipoSem::VOID;
    }
    return TipoSem::VOID;
}


// ==================================================
// SIMBOLO
// ==================================================

struct ParamSim
{
    TipoSem tipo;
    bool esPuntero;   // "char *nombre"
};

struct Simbolo
{
    enum class Clase { VARIABLE, FUNCION };

    Clase clase = Clase::VARIABLE;
    std::string nombre;
    TipoSem tipo = TipoSem::INT;   // tipo de la variable o tipo de retorno
    int linea = 0;
    int columna = 0;

    // Solo para funciones
    std::vector<ParamSim> params;
    bool variadica = false;
    bool definida = false;
    bool esBiblioteca = false;     // printf y scanf
};


// Una fila de la tabla que se muestra al usuario (--simbolos)

struct EntradaTabla
{
    std::string ambito;
    std::string nombre;
    std::string clase;
    std::string tipo;
    int linea;
    int columna;
};


// ==================================================
// TABLA DE SIMBOLOS CON AMBITOS
// ==================================================
// Una pila de ambitos: el primero es el global y cada
// funcion abre uno nuevo. Buscar un nombre recorre desde
// el ambito mas interno hacia el global (reglas usuales
// de scoping).
// ==================================================

class TablaSimbolos
{
private:

    std::deque<std::unordered_map<std::string, Simbolo>> ambitos;

public:

    TablaSimbolos()
    {
        ambitos.emplace_back();   // ambito global
    }

    void entrarAmbito()
    {
        ambitos.emplace_back();
    }

    void salirAmbito()
    {
        ambitos.pop_back();
    }

    // Busca solo en el ambito actual (para detectar duplicados)
    Simbolo* buscarEnActual(const std::string& nombre)
    {
        auto it = ambitos.back().find(nombre);

        return it == ambitos.back().end() ? nullptr : &it->second;
    }

    // Busca desde el ambito mas interno hasta el global
    Simbolo* buscar(const std::string& nombre)
    {
        for (auto ambito = ambitos.rbegin(); ambito != ambitos.rend(); ++ambito)
        {
            auto it = ambito->find(nombre);

            if (it != ambito->end())
            {
                return &it->second;
            }
        }

        return nullptr;
    }

    // Inserta en el ambito actual (el llamador ya verifico que no existe)
    Simbolo& insertar(const Simbolo& simbolo)
    {
        return ambitos.back()[simbolo.nombre] = simbolo;
    }
};

#endif
