#include "Semantico.h"

#include <utility>

// ==================================================
// UTILIDADES
// ==================================================

namespace {

// int y char se consideran tipos numericos compatibles entre si
// (char se promueve a int, como en C). Si el ingeniero pide un
// chequeo mas estricto, basta con cambiar esta funcion.

bool esNumerico(TipoSem t)
{
    return t == TipoSem::INT || t == TipoSem::CHAR;
}

bool asignable(TipoSem destino, TipoSem origen)
{
    return esNumerico(destino) && esNumerico(origen);
}

}  // namespace


Semantico::Semantico(const Programa& programa)
    : programa(programa), nombreAmbito("global") {}


const std::vector<EntradaTabla>& Semantico::registro() const
{
    return registroTabla;
}


// ==================================================
// ERRORES
// ==================================================

void Semantico::error(const std::string& mensaje, int linea, int columna) const
{
    throw ErrorCompilador{ "semantico", mensaje, linea, columna };
}


void Semantico::error(const std::string& mensaje, const Nodo& nodo) const
{
    error(mensaje, nodo.linea, nodo.columna);
}


void Semantico::errorDuplicado(const std::string& nombre, const Simbolo& previo,
                               int linea, int columna) const
{
    if (previo.esBiblioteca)
    {
        error("el identificador '" + nombre + "' ya esta declarado como "
              "funcion de la biblioteca de runtime", linea, columna);
    }

    error("el identificador '" + nombre + "' ya fue declarado en este ambito "
          "(linea " + std::to_string(previo.linea) + ", columna " +
          std::to_string(previo.columna) + ")", linea, columna);
}


// ==================================================
// TABLA DE SIMBOLOS: apoyo
// ==================================================

void Semantico::registrar(const std::string& nombre, const std::string& clase,
                          const std::string& tipo, int linea, int columna)
{
    registroTabla.push_back({ nombreAmbito, nombre, clase, tipo, linea, columna });
}


// printf y scanf vienen ya declaradas: int f(char *formato, ...)

void Semantico::registrarBiblioteca()
{
    for (const std::string nombre : { "printf", "scanf" })
    {
        Simbolo s;
        s.clase = Simbolo::Clase::FUNCION;
        s.nombre = nombre;
        s.tipo = TipoSem::INT;
        s.params.push_back({ TipoSem::CHAR, true });
        s.variadica = true;
        s.definida = true;
        s.esBiblioteca = true;

        tabla.insertar(s);

        registrar(nombre, "biblioteca", describirFirma(s), 0, 0);
    }
}


Simbolo Semantico::simboloDe(const DeclaracionFuncion& fn) const
{
    Simbolo s;
    s.clase = Simbolo::Clase::FUNCION;
    s.nombre = fn.nombre;
    s.tipo = aTipoSem(fn.tipoRetorno);
    s.linea = fn.linea;
    s.columna = fn.columna;
    s.variadica = fn.variadica;

    for (const auto& p : fn.parametros)
    {
        s.params.push_back({ aTipoSem(p.tipo), p.esPuntero });
    }

    return s;
}


// Ejemplo: "int (int, char*, ...)"

std::string Semantico::describirFirma(const Simbolo& s) const
{
    std::string texto = nombreTipoSem(s.tipo) + " (";

    if (s.params.empty() && !s.variadica)
    {
        texto += "void";
    }

    for (size_t i = 0; i < s.params.size(); i++)
    {
        if (i > 0)
        {
            texto += ", ";
        }

        texto += nombreTipoSem(s.params[i].tipo);

        if (s.params[i].esPuntero)
        {
            texto += "*";
        }
    }

    if (s.variadica)
    {
        texto += ", ...";
    }

    return texto + ")";
}


bool Semantico::mismaFirma(const Simbolo& a, const Simbolo& b) const
{
    if (a.tipo != b.tipo || a.variadica != b.variadica ||
        a.params.size() != b.params.size())
    {
        return false;
    }

    for (size_t i = 0; i < a.params.size(); i++)
    {
        if (a.params[i].tipo != b.params[i].tipo ||
            a.params[i].esPuntero != b.params[i].esPuntero)
        {
            return false;
        }
    }

    return true;
}


// ==================================================
// PROGRAMA
// ==================================================

void Semantico::analizar()
{
    registrarBiblioteca();

    for (const auto& decl : programa.declaraciones)
    {
        analizarDeclaracion(decl.get());
    }

    verificarMain();
}


// El programa debe tener una funcion main, sin parametros

void Semantico::verificarMain()
{
    const Nodo& ultima = *programa.declaraciones.back();

    Simbolo* main = tabla.buscar("main");

    if (main == nullptr)
    {
        error("el programa debe definir la funcion 'main'",
              ultima.linea, ultima.columna);
    }

    if (main->clase != Simbolo::Clase::FUNCION)
    {
        error("'main' debe ser una funcion, pero fue declarado como variable",
              main->linea, main->columna);
    }

    if (!main->definida)
    {
        error("la funcion 'main' fue declarada pero nunca se define",
              main->linea, main->columna);
    }

    if (!main->params.empty())
    {
        error("la funcion 'main' no debe tener parametros",
              main->linea, main->columna);
    }
}


// ==================================================
// DECLARACIONES
// ==================================================

void Semantico::analizarDeclaracion(const Declaracion* decl)
{
    if (auto* variable = dynamic_cast<const DeclaracionVariable*>(decl))
    {
        declararVariables(*variable);
        return;
    }

    if (auto* funcion = dynamic_cast<const DeclaracionFuncion*>(decl))
    {
        analizarFuncion(*funcion);
    }
}


// Ningun identificador puede declararse dos veces en el mismo ambito

void Semantico::declararVariables(const DeclaracionVariable& decl)
{
    for (const auto& var : decl.variables)
    {
        Simbolo* previo = tabla.buscarEnActual(var.nombre);

        if (previo != nullptr)
        {
            errorDuplicado(var.nombre, *previo, var.linea, var.columna);
        }

        Simbolo s;
        s.clase = Simbolo::Clase::VARIABLE;
        s.nombre = var.nombre;
        s.tipo = aTipoSem(decl.tipo);
        s.linea = var.linea;
        s.columna = var.columna;

        tabla.insertar(s);

        registrar(var.nombre, "variable", nombreTipoSem(s.tipo),
                  var.linea, var.columna);
    }
}


// Prototipos y definiciones. Un prototipo y su definicion deben coincidir.

void Semantico::analizarFuncion(const DeclaracionFuncion& fn)
{
    Simbolo nuevo = simboloDe(fn);

    Simbolo* previo = tabla.buscarEnActual(fn.nombre);

    if (previo != nullptr)
    {
        if (previo->clase != Simbolo::Clase::FUNCION)
        {
            errorDuplicado(fn.nombre, *previo, fn.linea, fn.columna);
        }

        if (!mismaFirma(*previo, nuevo))
        {
            error("la declaracion de la funcion '" + fn.nombre +
                  "' no coincide con la anterior" +
                  (previo->esBiblioteca
                       ? " (funcion de la biblioteca: " + describirFirma(*previo) + ")"
                       : " (linea " + std::to_string(previo->linea) + ": " +
                             describirFirma(*previo) + ")"),
                  fn);
        }

        if (fn.esDefinicion())
        {
            if (previo->esBiblioteca)
            {
                error("no se puede redefinir la funcion de la biblioteca '" +
                      fn.nombre + "'", fn);
            }

            if (previo->definida)
            {
                error("la funcion '" + fn.nombre + "' ya fue definida "
                      "(linea " + std::to_string(previo->linea) + ")", fn);
            }

            previo->definida = true;

            // El prototipo registrado pasa a ser la definicion
            for (auto& fila : registroTabla)
            {
                if (fila.ambito == nombreAmbito && fila.nombre == fn.nombre &&
                    fila.clase == "prototipo")
                {
                    fila.clase = "funcion";
                    break;
                }
            }
        }
    }
    else
    {
        nuevo.definida = fn.esDefinicion();

        tabla.insertar(nuevo);

        registrar(fn.nombre, fn.esDefinicion() ? "funcion" : "prototipo",
                  describirFirma(nuevo), fn.linea, fn.columna);
    }

    if (fn.esDefinicion())
    {
        analizarCuerpo(fn);
    }
}


// Los parametros y las variables locales comparten un mismo ambito

void Semantico::analizarCuerpo(const DeclaracionFuncion& fn)
{
    std::string ambitoAnterior = nombreAmbito;

    nombreAmbito = "funcion " + fn.nombre;
    funcionActual = &fn;

    tabla.entrarAmbito();

    for (const auto& p : fn.parametros)
    {
        if (tabla.buscarEnActual(p.nombre) != nullptr)
        {
            error("el parametro '" + p.nombre + "' esta repetido en la "
                  "funcion '" + fn.nombre + "'", p.linea, p.columna);
        }

        Simbolo s;
        s.clase = Simbolo::Clase::VARIABLE;
        s.nombre = p.nombre;
        s.tipo = aTipoSem(p.tipo);
        s.linea = p.linea;
        s.columna = p.columna;

        tabla.insertar(s);

        registrar(p.nombre, "parametro",
                  nombreTipoSem(s.tipo) + (p.esPuntero ? "*" : ""),
                  p.linea, p.columna);
    }

    for (const auto& decl : fn.cuerpo->declaraciones)
    {
        analizarDeclaracion(decl.get());
    }

    for (const auto& sent : fn.cuerpo->sentencias)
    {
        analizarSentencia(sent.get());
    }

    tabla.salirAmbito();

    funcionActual = nullptr;
    nombreAmbito = ambitoAnterior;
}


// ==================================================
// SENTENCIAS
// ==================================================

void Semantico::analizarSentencia(const Sentencia* sent)
{
    if (auto* bloque = dynamic_cast<const Bloque*>(sent))
    {
        for (const auto& hijo : bloque->sentencias)
        {
            analizarSentencia(hijo.get());
        }

        return;
    }

    if (auto* s = dynamic_cast<const SentenciaExpresion*>(sent))
    {
        tipoExpresion(s->expresion.get());
        return;
    }

    if (auto* s = dynamic_cast<const SentenciaReturn*>(sent))
    {
        analizarReturn(*s);
        return;
    }

    if (auto* s = dynamic_cast<const SentenciaWhile*>(sent))
    {
        exigirCondicion(s->condicion.get(), "while");

        profundidadCiclo++;
        analizarSentencia(s->cuerpo.get());
        profundidadCiclo--;

        return;
    }

    if (auto* s = dynamic_cast<const SentenciaIf*>(sent))
    {
        exigirCondicion(s->condicion.get(), "if");

        analizarSentencia(s->entonces.get());

        if (s->sino)
        {
            analizarSentencia(s->sino.get());
        }

        return;
    }

    if (dynamic_cast<const SentenciaBreak*>(sent))
    {
        if (profundidadCiclo == 0)
        {
            error("'break' solo puede usarse dentro de un ciclo 'while'", *sent);
        }

        return;
    }

    if (dynamic_cast<const SentenciaContinue*>(sent))
    {
        if (profundidadCiclo == 0)
        {
            error("'continue' solo puede usarse dentro de un ciclo 'while'", *sent);
        }

        return;
    }
}


void Semantico::exigirCondicion(const Expresion* condicion, const std::string& donde)
{
    TipoSem tipo = tipoExpresion(condicion);

    if (!esNumerico(tipo))
    {
        error("la condicion del '" + donde + "' debe ser de tipo int o char "
              "(se encontro " + nombreTipoSem(tipo) + ")", *condicion);
    }
}


// - Un return no lleva valor si la funcion es void
// - La expresion debe tener el tipo de la funcion

void Semantico::analizarReturn(const SentenciaReturn& sent)
{
    TipoSem esperado = aTipoSem(funcionActual->tipoRetorno);
    const std::string& nombre = funcionActual->nombre;

    if (sent.valor == nullptr)
    {
        if (esperado != TipoSem::VOID)
        {
            error("la funcion '" + nombre + "' retorna " +
                  nombreTipoSem(esperado) + ": 'return' debe llevar un valor",
                  sent);
        }

        return;
    }

    if (esperado == TipoSem::VOID)
    {
        error("la funcion '" + nombre + "' es void y no puede retornar un valor",
              sent);
    }

    TipoSem obtenido = tipoExpresion(sent.valor.get());

    if (!asignable(esperado, obtenido))
    {
        error("el valor retornado es de tipo " + nombreTipoSem(obtenido) +
              " pero la funcion '" + nombre + "' retorna " +
              nombreTipoSem(esperado), *sent.valor);
    }
}


// ==================================================
// EXPRESIONES
// ==================================================

TipoSem Semantico::tipoExpresion(const Expresion* expr)
{
    if (dynamic_cast<const LiteralEntero*>(expr)) return TipoSem::INT;
    if (dynamic_cast<const LiteralChar*>(expr))   return TipoSem::CHAR;
    if (dynamic_cast<const LiteralString*>(expr)) return TipoSem::CADENA;

    if (auto* id = dynamic_cast<const Identificador*>(expr))
    {
        Simbolo* s = tabla.buscar(id->nombre);

        if (s == nullptr)
        {
            error("el identificador '" + id->nombre + "' se usa sin haber "
                  "sido declarado", *id);
        }

        if (s->clase == Simbolo::Clase::FUNCION)
        {
            error("'" + id->nombre + "' es una funcion; para llamarla use '" +
                  id->nombre + "(...)'", *id);
        }

        return s->tipo;
    }

    if (auto* un = dynamic_cast<const ExpresionUnaria*>(expr))
    {
        TipoSem tipo = tipoExpresion(un->operando.get());

        if (!esNumerico(tipo))
        {
            error("el operador unario '" + un->operador + "' requiere un "
                  "operando int o char (se encontro " + nombreTipoSem(tipo) + ")",
                  *un);
        }

        return TipoSem::INT;
    }

    if (auto* bin = dynamic_cast<const ExpresionBinaria*>(expr))
    {
        TipoSem izq = tipoExpresion(bin->izquierda.get());
        TipoSem der = tipoExpresion(bin->derecha.get());

        if (!esNumerico(izq))
        {
            error("el operador '" + bin->operador + "' requiere operandos int o "
                  "char (el izquierdo es " + nombreTipoSem(izq) + ")", *bin);
        }

        if (!esNumerico(der))
        {
            error("el operador '" + bin->operador + "' requiere operandos int o "
                  "char (el derecho es " + nombreTipoSem(der) + ")", *bin);
        }

        // Con argumentos enteros, toda operacion devuelve un entero
        return TipoSem::INT;
    }

    if (auto* asig = dynamic_cast<const Asignacion*>(expr))
    {
        Simbolo* destino = tabla.buscar(asig->nombre);

        if (destino == nullptr)
        {
            error("el identificador '" + asig->nombre + "' se usa sin haber "
                  "sido declarado", *asig);
        }

        if (destino->clase == Simbolo::Clase::FUNCION)
        {
            error("no se puede asignar a '" + asig->nombre +
                  "' porque es una funcion", *asig);
        }

        TipoSem valor = tipoExpresion(asig->valor.get());

        if (!asignable(destino->tipo, valor))
        {
            error("no se puede asignar un valor de tipo " + nombreTipoSem(valor) +
                  " a la variable '" + asig->nombre + "' de tipo " +
                  nombreTipoSem(destino->tipo), *asig->valor);
        }

        return destino->tipo;
    }

    if (auto* llamada = dynamic_cast<const Llamada*>(expr))
    {
        return tipoLlamada(*llamada);
    }

    return TipoSem::VOID;
}


// El numero y los tipos de los argumentos deben coincidir con la declaracion

TipoSem Semantico::tipoLlamada(const Llamada& llamada)
{
    Simbolo* s = tabla.buscar(llamada.nombre);

    if (s == nullptr)
    {
        error("la funcion '" + llamada.nombre + "' se usa sin haber sido "
              "declarada", llamada);
    }

    if (s->clase != Simbolo::Clase::FUNCION)
    {
        error("'" + llamada.nombre + "' es una variable, no una funcion",
              llamada);
    }

    // printf y scanf tienen reglas propias segun el manual
    if (s->esBiblioteca)
    {
        return llamada.nombre == "printf" ? validarPrintf(llamada)
                                          : validarScanf(llamada);
    }

    size_t fijos = s->params.size();
    size_t recibidos = llamada.argumentos.size();

    bool cantidadValida = s->variadica ? recibidos >= fijos : recibidos == fijos;

    if (!cantidadValida)
    {
        error("la funcion '" + llamada.nombre + "' espera " +
              (s->variadica ? "al menos " : "") + std::to_string(fijos) +
              " argumento(s) pero se recibieron " + std::to_string(recibidos),
              llamada);
    }

    for (size_t i = 0; i < recibidos; i++)
    {
        const Expresion* arg = llamada.argumentos[i].get();

        TipoSem tipo = tipoExpresion(arg);

        // Los argumentos extra de una funcion variadica solo deben ser valores
        bool esperaCadena = i < fijos && s->params[i].esPuntero;

        if (esperaCadena)
        {
            if (tipo != TipoSem::CADENA)
            {
                error("el argumento " + std::to_string(i + 1) + " de la llamada "
                      "a '" + llamada.nombre + "' debe ser un string (se "
                      "encontro " + nombreTipoSem(tipo) + ")", *arg);
            }
        }
        else if (!esNumerico(tipo))
        {
            error("el argumento " + std::to_string(i + 1) + " de la llamada a '" +
                  llamada.nombre + "' debe ser de tipo int o char (se encontro " +
                  nombreTipoSem(tipo) + ")", *arg);
        }
    }

    return s->tipo;
}


// ==================================================
// printf y scanf
// ==================================================

// Devuelve las conversiones del formato ('d' o 'c'), en orden.
// Rechaza cualquier directiva distinta de %d, %c y %%.

std::vector<char> Semantico::conversionesDe(const LiteralString& formato) const
{
    std::vector<char> conversiones;

    const std::string& texto = formato.lexema;   // incluye las comillas

    for (size_t i = 1; i + 1 < texto.size(); i++)
    {
        if (texto[i] == '\\')
        {
            i++;   // saltar la secuencia de escape
            continue;
        }

        if (texto[i] != '%')
        {
            continue;
        }

        if (i + 2 >= texto.size())
        {
            error("el formato termina en un '%' sin directiva de conversion",
                  formato);
        }

        char directiva = texto[i + 1];

        if (directiva == '%')
        {
            i++;
        }
        else if (directiva == 'd' || directiva == 'c')
        {
            conversiones.push_back(directiva);
            i++;
        }
        else
        {
            error(std::string("directiva de conversion invalida '%") + directiva +
                  "': solo se permiten %d, %c y %%", formato);
        }
    }

    return conversiones;
}


// printf: uno o dos argumentos. El primero es un formato (constante de
// string) con a lo sumo una conversion; el segundo se imprime con ella.

TipoSem Semantico::validarPrintf(const Llamada& llamada)
{
    size_t cantidad = llamada.argumentos.size();

    if (cantidad < 1 || cantidad > 2)
    {
        error("printf acepta uno o dos argumentos (el formato y un valor "
              "opcional) pero se recibieron " + std::to_string(cantidad),
              llamada);
    }

    auto* formato = dynamic_cast<const LiteralString*>(llamada.argumentos[0].get());

    if (formato == nullptr)
    {
        error("el primer argumento de printf debe ser una constante de string",
              *llamada.argumentos[0]);
    }

    std::vector<char> conversiones = conversionesDe(*formato);

    if (conversiones.size() > 1)
    {
        error("el formato de printf admite una sola conversion (%d o %c)",
              *formato);
    }

    if (conversiones.size() == 1 && cantidad == 1)
    {
        error("el formato de printf tiene una conversion pero falta el valor "
              "a imprimir", llamada);
    }

    if (conversiones.empty() && cantidad == 2)
    {
        error("el formato de printf no tiene conversion, pero se paso un "
              "valor a imprimir", *llamada.argumentos[1]);
    }

    if (cantidad == 2)
    {
        TipoSem tipo = tipoExpresion(llamada.argumentos[1].get());

        if (!esNumerico(tipo))
        {
            error("el valor a imprimir con printf debe ser de tipo int o char "
                  "(se encontro " + nombreTipoSem(tipo) + ")",
                  *llamada.argumentos[1]);
        }
    }

    return TipoSem::INT;
}


// scanf: un solo argumento, el formato "%d" o "%c". Devuelve el valor leido.

TipoSem Semantico::validarScanf(const Llamada& llamada)
{
    if (llamada.argumentos.size() != 1)
    {
        error("scanf requiere exactamente un argumento (el formato) pero se "
              "recibieron " + std::to_string(llamada.argumentos.size()),
              llamada);
    }

    auto* formato = dynamic_cast<const LiteralString*>(llamada.argumentos[0].get());

    if (formato == nullptr)
    {
        error("el argumento de scanf debe ser una constante de string",
              *llamada.argumentos[0]);
    }

    if (formato->lexema == "\"%d\"")
    {
        return TipoSem::INT;
    }

    if (formato->lexema == "\"%c\"")
    {
        return TipoSem::CHAR;
    }

    error("el formato de scanf debe ser exactamente \"%d\" o \"%c\"", *formato);
}
