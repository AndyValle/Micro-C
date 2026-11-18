#ifndef AST_PRINTER_H
#define AST_PRINTER_H

#include <iostream>
#include <string>
#include <vector>

#include "AST.h"

// ==================================================
// IMPRESOR DEL AST EN FORMA DE ARBOL
// ==================================================
// Primero convierte el AST a un arbol simple de etiquetas
// (NodoVisual) y luego lo dibuja con los conectores
// "├──", "└──" y "│".
// ==================================================

struct NodoVisual
{
    std::string etiqueta;
    std::vector<NodoVisual> hijos;
};


// -------- Expresiones --------

inline NodoVisual visualExpresion(const Expresion* expr)
{
    if (auto* e = dynamic_cast<const LiteralEntero*>(expr))
        return { "Entero (" + std::to_string(e->valor) + ")", {} };

    if (auto* e = dynamic_cast<const LiteralChar*>(expr))
        return { "Caracter (" + e->lexema + ")", {} };

    if (auto* e = dynamic_cast<const LiteralString*>(expr))
        return { "Cadena (" + e->lexema + ")", {} };

    if (auto* e = dynamic_cast<const Identificador*>(expr))
        return { "Identificador (" + e->nombre + ")", {} };

    if (auto* e = dynamic_cast<const ExpresionUnaria*>(expr))
        return { "OpUnario (" + e->operador + ")", { visualExpresion(e->operando.get()) } };

    if (auto* e = dynamic_cast<const ExpresionBinaria*>(expr))
        return { "BinOp (" + e->operador + ")",
                 { visualExpresion(e->izquierda.get()), visualExpresion(e->derecha.get()) } };

    if (auto* e = dynamic_cast<const Asignacion*>(expr))
        return { "Asignacion (" + e->nombre + ")", { visualExpresion(e->valor.get()) } };

    if (auto* e = dynamic_cast<const Llamada*>(expr))
    {
        NodoVisual nodo{ "Llamada (" + e->nombre + ")", {} };

        for (const auto& arg : e->argumentos)
            nodo.hijos.push_back(visualExpresion(arg.get()));

        return nodo;
    }

    return { "(expresion desconocida)", {} };
}


// -------- Sentencias --------

inline NodoVisual visualSentencia(const Sentencia* sent)
{
    if (auto* s = dynamic_cast<const Bloque*>(sent))
    {
        NodoVisual nodo{ "Bloque", {} };

        for (const auto& hijo : s->sentencias)
            nodo.hijos.push_back(visualSentencia(hijo.get()));

        return nodo;
    }

    if (auto* s = dynamic_cast<const SentenciaExpresion*>(sent))
        return { "SentenciaExpresion", { visualExpresion(s->expresion.get()) } };

    if (auto* s = dynamic_cast<const SentenciaReturn*>(sent))
    {
        NodoVisual nodo{ "Return", {} };

        if (s->valor)
            nodo.hijos.push_back(visualExpresion(s->valor.get()));

        return nodo;
    }

    if (auto* s = dynamic_cast<const SentenciaWhile*>(sent))
        return { "While",
                 { { "Condicion", { visualExpresion(s->condicion.get()) } },
                   { "Cuerpo",    { visualSentencia(s->cuerpo.get()) } } } };

    if (auto* s = dynamic_cast<const SentenciaIf*>(sent))
    {
        NodoVisual nodo{ "If", {} };

        nodo.hijos.push_back({ "Condicion", { visualExpresion(s->condicion.get()) } });
        nodo.hijos.push_back({ "Entonces",  { visualSentencia(s->entonces.get()) } });

        if (s->sino)
            nodo.hijos.push_back({ "Sino", { visualSentencia(s->sino.get()) } });

        return nodo;
    }

    if (dynamic_cast<const SentenciaBreak*>(sent))
        return { "Break", {} };

    if (dynamic_cast<const SentenciaContinue*>(sent))
        return { "Continue", {} };

    return { "(sentencia desconocida)", {} };
}


// -------- Declaraciones --------

inline NodoVisual visualDeclaracion(const Declaracion* decl)
{
    if (auto* d = dynamic_cast<const DeclaracionVariable*>(decl))
    {
        NodoVisual nodo{ "DeclaracionVariable (" + nombreTipo(d->tipo) + ")", {} };

        for (const auto& v : d->variables)
            nodo.hijos.push_back({ "Variable (" + v.nombre + ")", {} });

        return nodo;
    }

    if (auto* d = dynamic_cast<const DeclaracionFuncion*>(decl))
    {
        NodoVisual nodo{
            std::string(d->esDefinicion() ? "Funcion (" : "PrototipoFuncion (") +
                d->nombre + ")", {} };

        nodo.hijos.push_back({ "TipoRetorno (" + nombreTipo(d->tipoRetorno) + ")", {} });

        NodoVisual params{ d->paramsVoid ? "Parametros (void)" : "Parametros", {} };

        for (const auto& p : d->parametros)
            params.hijos.push_back({ p.nombre + ": " + nombreTipo(p.tipo) +
                                     (p.esPuntero ? "*" : ""), {} });

        if (d->variadica)
            params.hijos.push_back({ "...", {} });

        nodo.hijos.push_back(params);

        if (d->esDefinicion())
        {
            NodoVisual cuerpo{ "Cuerpo", {} };

            if (!d->cuerpo->declaraciones.empty())
            {
                NodoVisual decls{ "Declaraciones", {} };

                for (const auto& hijo : d->cuerpo->declaraciones)
                    decls.hijos.push_back(visualDeclaracion(hijo.get()));

                cuerpo.hijos.push_back(decls);
            }

            NodoVisual sents{ "Sentencias", {} };

            for (const auto& hijo : d->cuerpo->sentencias)
                sents.hijos.push_back(visualSentencia(hijo.get()));

            cuerpo.hijos.push_back(sents);
            nodo.hijos.push_back(cuerpo);
        }

        return nodo;
    }

    return { "(declaracion desconocida)", {} };
}


// -------- Dibujo --------

inline void dibujarNodo(const NodoVisual& nodo, const std::string& prefijo,
                        bool esUltimo, bool esRaiz)
{
    if (esRaiz)
    {
        std::cout << nodo.etiqueta << std::endl;
    }
    else
    {
        std::cout << prefijo
                  << (esUltimo ? "\u2514\u2500\u2500 " : "\u251c\u2500\u2500 ")
                  << nodo.etiqueta << std::endl;
    }

    std::string nuevoPrefijo =
        esRaiz ? "" : prefijo + (esUltimo ? "    " : "\u2502   ");

    for (size_t i = 0; i < nodo.hijos.size(); i++)
    {
        dibujarNodo(nodo.hijos[i], nuevoPrefijo, i + 1 == nodo.hijos.size(), false);
    }
}


inline void imprimirPrograma(const Programa& programa)
{
    NodoVisual raiz{ "Programa", {} };

    for (const auto& decl : programa.declaraciones)
        raiz.hijos.push_back(visualDeclaracion(decl.get()));

    dibujarNodo(raiz, "", true, true);
}

#endif
