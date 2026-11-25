#include "GeneradorIR.h"

#include <utility>

// ==================================================
// UTILIDADES
// ==================================================

namespace {

// Quita las comillas y convierte las secuencias de escape en los
// caracteres reales: \n \\ \"

std::string decodificar(const std::string& lexema)
{
    std::string texto;

    for (size_t i = 1; i + 1 < lexema.size(); i++)
    {
        if (lexema[i] == '\\' && i + 2 < lexema.size())
        {
            i++;

            if (lexema[i] == 'n')
            {
                texto += '\n';
            }
            else
            {
                texto += lexema[i];   // \\ y \"
            }
        }
        else
        {
            texto += lexema[i];
        }
    }

    return texto;
}


OpIR operacionDe(const std::string& op)
{
    if (op == "+")  return OpIR::ADD;
    if (op == "-")  return OpIR::SUB;
    if (op == "*")  return OpIR::MUL;
    if (op == "/")  return OpIR::DIV;
    if (op == "<")  return OpIR::LT;
    if (op == "<=") return OpIR::LE;
    if (op == ">")  return OpIR::GT;
    if (op == ">=") return OpIR::GE;
    if (op == "==") return OpIR::EQ;
    return OpIR::NE;   // "!="
}

}  // namespace


// ==================================================
// APOYO
// ==================================================

GeneradorIR::GeneradorIR(const Programa& programa) : programa(programa) {}


void GeneradorIR::emitir(OpIR op, const Operando& a, const Operando& b,
                         const Operando& r)
{
    actual.codigo.push_back({ op, a, b, r });
}


Operando GeneradorIR::nuevoTemporal()
{
    contadorTemporales++;

    return Operando::temporal(contadorTemporales);
}


std::string GeneradorIR::nuevaEtiqueta()
{
    contadorEtiquetas++;

    return "L" + std::to_string(contadorEtiquetas);
}


// Una variable es local si es parametro o fue declarada en la funcion;
// si no, es global

Operando GeneradorIR::variable(const std::string& nombre) const
{
    if (nombresLocales.count(nombre) > 0)
    {
        return Operando::local(nombre);
    }

    return Operando::global(nombre);
}


int GeneradorIR::internarCadena(const std::string& lexema)
{
    std::string texto = decodificar(lexema);

    auto it = indiceCadenas.find(texto);

    if (it != indiceCadenas.end())
    {
        return it->second;
    }

    int indice = static_cast<int>(ir.cadenas.size());

    ir.cadenas.push_back(texto);
    indiceCadenas[texto] = indice;

    return indice;
}


// Se recorre todo el programa para saber que funciones devuelven valor

void GeneradorIR::recolectarRetornos()
{
    retornos["printf"] = Tipo::INT;
    retornos["scanf"] = Tipo::INT;

    for (const auto& decl : programa.declaraciones)
    {
        if (auto* fn = dynamic_cast<const DeclaracionFuncion*>(decl.get()))
        {
            retornos[fn->nombre] = fn->tipoRetorno;

            if (fn->cuerpo)
            {
                for (const auto& local : fn->cuerpo->declaraciones)
                {
                    if (auto* proto = dynamic_cast<const DeclaracionFuncion*>(local.get()))
                    {
                        retornos[proto->nombre] = proto->tipoRetorno;
                    }
                }
            }
        }
    }
}


// ==================================================
// PROGRAMA Y FUNCIONES
// ==================================================

ProgramaIR GeneradorIR::generar()
{
    recolectarRetornos();

    for (const auto& decl : programa.declaraciones)
    {
        if (auto* variables = dynamic_cast<const DeclaracionVariable*>(decl.get()))
        {
            for (const auto& v : variables->variables)
            {
                ir.globales.push_back({ v.nombre, variables->tipo });
            }
        }
        else if (auto* fn = dynamic_cast<const DeclaracionFuncion*>(decl.get()))
        {
            if (fn->esDefinicion())
            {
                generarFuncion(*fn);
            }
        }
    }

    return ir;
}


void GeneradorIR::generarFuncion(const DeclaracionFuncion& fn)
{
    actual = FuncionIR();
    actual.nombre = fn.nombre;
    actual.retorno = fn.tipoRetorno;

    nombresLocales.clear();
    contadorTemporales = 0;

    for (const auto& p : fn.parametros)
    {
        actual.params.push_back({ p.nombre, p.tipo });
        nombresLocales.insert(p.nombre);
    }

    for (const auto& decl : fn.cuerpo->declaraciones)
    {
        if (auto* variables = dynamic_cast<const DeclaracionVariable*>(decl.get()))
        {
            for (const auto& v : variables->variables)
            {
                actual.locales.push_back({ v.nombre, variables->tipo });
                nombresLocales.insert(v.nombre);
            }
        }
    }

    for (const auto& sent : fn.cuerpo->sentencias)
    {
        generarSentencia(sent.get());
    }

    // Si el cuerpo no termina en return, se agrega uno al final
    if (actual.codigo.empty() || actual.codigo.back().op != OpIR::RETURN)
    {
        emitir(OpIR::RETURN, Operando::ninguno(), Operando::ninguno(),
               Operando::ninguno());
    }

    actual.temporales = contadorTemporales;

    ir.funciones.push_back(actual);
}


// ==================================================
// SENTENCIAS
// ==================================================

void GeneradorIR::generarSentencia(const Sentencia* sent)
{
    if (auto* bloque = dynamic_cast<const Bloque*>(sent))
    {
        for (const auto& hijo : bloque->sentencias)
        {
            generarSentencia(hijo.get());
        }

        return;
    }

    if (auto* s = dynamic_cast<const SentenciaExpresion*>(sent))
    {
        generarExpresion(s->expresion.get());
        return;
    }

    if (auto* s = dynamic_cast<const SentenciaReturn*>(sent))
    {
        Operando valor = Operando::ninguno();

        if (s->valor)
        {
            valor = generarExpresion(s->valor.get());
        }

        emitir(OpIR::RETURN, valor, Operando::ninguno(), Operando::ninguno());
        return;
    }

    // while (cond) cuerpo
    //
    //   Linicio:
    //     cond
    //     JUMPF cond, Lfin
    //     cuerpo
    //     JUMP Linicio
    //   Lfin:

    if (auto* s = dynamic_cast<const SentenciaWhile*>(sent))
    {
        std::string inicio = nuevaEtiqueta();
        std::string fin = nuevaEtiqueta();

        emitir(OpIR::LABEL, Operando::ninguno(), Operando::ninguno(),
               Operando::etiqueta(inicio));

        Operando condicion = generarExpresion(s->condicion.get());

        emitir(OpIR::JUMPF, condicion, Operando::ninguno(), Operando::etiqueta(fin));

        // 'continue' vuelve al inicio y 'break' sale al final
        ciclos.push_back({ inicio, fin });
        generarSentencia(s->cuerpo.get());
        ciclos.pop_back();

        emitir(OpIR::JUMP, Operando::ninguno(), Operando::ninguno(),
               Operando::etiqueta(inicio));

        emitir(OpIR::LABEL, Operando::ninguno(), Operando::ninguno(),
               Operando::etiqueta(fin));

        return;
    }

    // if (cond) entonces [else sino]

    if (auto* s = dynamic_cast<const SentenciaIf*>(sent))
    {
        Operando condicion = generarExpresion(s->condicion.get());

        if (!s->sino)
        {
            std::string fin = nuevaEtiqueta();

            emitir(OpIR::JUMPF, condicion, Operando::ninguno(), Operando::etiqueta(fin));

            generarSentencia(s->entonces.get());

            emitir(OpIR::LABEL, Operando::ninguno(), Operando::ninguno(),
                   Operando::etiqueta(fin));
        }
        else
        {
            std::string sino = nuevaEtiqueta();
            std::string fin = nuevaEtiqueta();

            emitir(OpIR::JUMPF, condicion, Operando::ninguno(), Operando::etiqueta(sino));

            generarSentencia(s->entonces.get());

            emitir(OpIR::JUMP, Operando::ninguno(), Operando::ninguno(),
                   Operando::etiqueta(fin));

            emitir(OpIR::LABEL, Operando::ninguno(), Operando::ninguno(),
                   Operando::etiqueta(sino));

            generarSentencia(s->sino.get());

            emitir(OpIR::LABEL, Operando::ninguno(), Operando::ninguno(),
                   Operando::etiqueta(fin));
        }

        return;
    }

    if (dynamic_cast<const SentenciaBreak*>(sent))
    {
        emitir(OpIR::JUMP, Operando::ninguno(), Operando::ninguno(),
               Operando::etiqueta(ciclos.back().second));
        return;
    }

    if (dynamic_cast<const SentenciaContinue*>(sent))
    {
        emitir(OpIR::JUMP, Operando::ninguno(), Operando::ninguno(),
               Operando::etiqueta(ciclos.back().first));
        return;
    }
}


// ==================================================
// EXPRESIONES
// Devuelven el operando donde queda el valor de la expresion.
// ==================================================

Operando GeneradorIR::generarExpresion(const Expresion* expr)
{
    if (auto* e = dynamic_cast<const LiteralEntero*>(expr))
    {
        return Operando::constante(e->valor);
    }

    if (auto* e = dynamic_cast<const LiteralChar*>(expr))
    {
        return Operando::constante(e->valor);
    }

    if (auto* e = dynamic_cast<const LiteralString*>(expr))
    {
        return Operando::cadena(internarCadena(e->lexema));
    }

    if (auto* e = dynamic_cast<const Identificador*>(expr))
    {
        return variable(e->nombre);
    }

    if (auto* e = dynamic_cast<const ExpresionUnaria*>(expr))
    {
        Operando operando = generarExpresion(e->operando.get());

        if (e->operador == "+")
        {
            return operando;
        }

        // -constante se resuelve de una vez (asi tambien se obtiene -32768)
        if (operando.clase == Operando::Clase::CONSTANTE)
        {
            return Operando::constante(-operando.valor);
        }

        Operando resultado = nuevoTemporal();

        emitir(OpIR::NEG, operando, Operando::ninguno(), resultado);

        return resultado;
    }

    if (auto* e = dynamic_cast<const ExpresionBinaria*>(expr))
    {
        Operando izquierda = generarExpresion(e->izquierda.get());
        Operando derecha = generarExpresion(e->derecha.get());

        Operando resultado = nuevoTemporal();

        emitir(operacionDe(e->operador), izquierda, derecha, resultado);

        return resultado;
    }

    if (auto* e = dynamic_cast<const Asignacion*>(expr))
    {
        Operando valor = generarExpresion(e->valor.get());

        Operando destino = variable(e->nombre);

        emitir(OpIR::ASSIGN, valor, Operando::ninguno(), destino);

        return destino;
    }

    if (auto* e = dynamic_cast<const Llamada*>(expr))
    {
        return generarLlamada(*e);
    }

    return Operando::ninguno();
}


// Primero se evalua TODOS los argumentos y despues se emiten los PARAM
// seguidos, para que una llamada anidada como f(a, g(b)) no mezcle sus
// parametros con los de g.

Operando GeneradorIR::generarLlamada(const Llamada& llamada)
{
    std::vector<Operando> argumentos;

    for (const auto& arg : llamada.argumentos)
    {
        argumentos.push_back(generarExpresion(arg.get()));
    }

    for (const Operando& arg : argumentos)
    {
        emitir(OpIR::PARAM, arg, Operando::ninguno(), Operando::ninguno());
    }

    Operando resultado = Operando::ninguno();

    if (retornos[llamada.nombre] != Tipo::VOID)
    {
        resultado = nuevoTemporal();
    }

    emitir(OpIR::CALL, Operando::funcion(llamada.nombre),
           Operando::constante(static_cast<int>(argumentos.size())), resultado);

    return resultado;
}
