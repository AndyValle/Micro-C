#include "Parser.h"

#include <utility>

// ==================================================
// UTILIDADES
// ==================================================

namespace {

bool esTipo(TokenType t)
{
    return t == TokenType::INT || t == TokenType::CHAR || t == TokenType::VOID;
}

Tipo aTipo(TokenType t)
{
    if (t == TokenType::INT)  return Tipo::INT;
    if (t == TokenType::CHAR) return Tipo::CHAR;
    return Tipo::VOID;
}

bool esHexa(const std::string& lexema)
{
    return lexema.size() > 1 && lexema[0] == '0' &&
           (lexema[1] == 'x' || lexema[1] == 'X');
}

}  // namespace


// ==================================================
// CONSTRUCTOR Y NAVEGACION
// ==================================================

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens), pos(0) {}


const Token& Parser::actual() const
{
    return tokens[pos];
}


bool Parser::revisar(TokenType tipo) const
{
    return actual().tipo == tipo;
}


bool Parser::coincide(TokenType tipo)
{
    if (revisar(tipo))
    {
        avanzar();
        return true;
    }

    return false;
}


// Devuelve el token consumido; nunca avanza mas alla del EOF

const Token& Parser::avanzar()
{
    const Token& token = tokens[pos];

    if (token.tipo != TokenType::END_OF_FILE)
    {
        pos++;
    }

    return token;
}


std::string Parser::describir(const Token& token) const
{
    if (token.tipo == TokenType::END_OF_FILE)
    {
        return "fin de archivo";
    }

    return "'" + token.lexema + "'";
}


void Parser::error(const Token& token, const std::string& mensaje) const
{
    throw ErrorCompilador{ "sintactico", mensaje, token.linea, token.columna };
}


const Token& Parser::consumir(TokenType tipo, const std::string& esperado)
{
    if (revisar(tipo))
    {
        return avanzar();
    }

    error(actual(), "se esperaba " + esperado +
                    " pero se encontro " + describir(actual()));
}


// ==================================================
// PROGRAMA
// translation_unit := external_declaration+
// ==================================================

std::unique_ptr<Programa> Parser::parsear()
{
    auto programa = std::make_unique<Programa>();

    if (revisar(TokenType::END_OF_FILE))
    {
        error(actual(), "el programa esta vacio: se esperaba al menos una "
                        "declaracion o definicion de funcion");
    }

    while (!revisar(TokenType::END_OF_FILE))
    {
        if (!esTipo(actual().tipo))
        {
            error(actual(), "se esperaba una declaracion de variable o de "
                            "funcion (empieza con int, char o void) pero se "
                            "encontro " + describir(actual()));
        }

        programa->declaraciones.push_back(parsearDeclaracion(true));
    }

    return programa;
}


// ==================================================
// DECLARACIONES
// variable_declaration := type identifier_list ";"
// function_declaration := return_type identifier "(" ... ")" ";"
// function_definition  := return_type identifier "(" ... ")" function_body
// ==================================================

DeclaracionPtr Parser::parsearDeclaracion(bool permitirDefinicion)
{
    const Token& tokTipo = actual();

    avanzar();

    Tipo tipo = aTipo(tokTipo.tipo);

    const Token& tokNombre = consumir(TokenType::IDENTIFIER, "un identificador");

    // Con '(' es una funcion (prototipo o definicion)
    if (revisar(TokenType::LPAREN))
    {
        return parsearFuncion(tipo, tokNombre, permitirDefinicion);
    }

    // Si no, es una declaracion de variables
    if (tipo == Tipo::VOID)
    {
        error(tokTipo, "una variable no puede ser de tipo 'void'");
    }

    auto decl = std::make_unique<DeclaracionVariable>(
        tokTipo.linea, tokTipo.columna, tipo);

    decl->variables.push_back(
        { tokNombre.lexema, tokNombre.linea, tokNombre.columna });

    while (coincide(TokenType::COMMA))
    {
        const Token& otro = consumir(TokenType::IDENTIFIER,
                                     "un identificador despues de ','");

        decl->variables.push_back({ otro.lexema, otro.linea, otro.columna });
    }

    consumir(TokenType::SEMICOLON, "';' al final de la declaracion");

    return decl;
}


DeclaracionPtr Parser::parsearFuncion(Tipo tipoRetorno, const Token& nombre,
                                      bool permitirDefinicion)
{
    auto fn = std::make_unique<DeclaracionFuncion>(nombre.linea, nombre.columna);

    fn->tipoRetorno = tipoRetorno;
    fn->nombre = nombre.lexema;

    consumir(TokenType::LPAREN, "'('");

    // Se recuerdan estos tokens para rechazarlos si resulta ser una
    // definicion (solo los prototipos admiten "char *x" y "...")
    const Token* tokPuntero = nullptr;
    const Token* tokVariadica = nullptr;

    if (revisar(TokenType::VOID))
    {
        avanzar();
        fn->paramsVoid = true;
    }
    else if (revisar(TokenType::INT) || revisar(TokenType::CHAR))
    {
        while (true)
        {
            const Token& tokTipo = avanzar();
            Tipo tipo = aTipo(tokTipo.tipo);
            bool puntero = false;

            if (tipo == Tipo::CHAR && revisar(TokenType::STAR))
            {
                tokPuntero = &actual();
                avanzar();
                puntero = true;
            }

            const Token& tokParam = consumir(TokenType::IDENTIFIER,
                                             "el nombre del parametro");

            fn->parametros.push_back(
                { tipo, puntero, tokParam.lexema, tokParam.linea, tokParam.columna });

            if (!coincide(TokenType::COMMA))
            {
                break;
            }

            if (revisar(TokenType::ELLIPSIS))
            {
                tokVariadica = &actual();
                avanzar();
                fn->variadica = true;
                break;
            }

            if (!revisar(TokenType::INT) && !revisar(TokenType::CHAR))
            {
                error(actual(), "se esperaba un tipo de parametro (int o char) "
                                "pero se encontro " + describir(actual()));
            }
        }
    }
    else
    {
        error(actual(), "se esperaba 'void' o un parametro (int o char) pero se "
                        "encontro " + describir(actual()) +
                        "; el manual no permite una lista de parametros vacia");
    }

    consumir(TokenType::RPAREN, "')' al final de la lista de parametros");

    if (revisar(TokenType::LBRACE))
    {
        if (!permitirDefinicion)
        {
            error(actual(), "no se puede definir una funcion dentro de otra; "
                            "solo se admiten declaraciones (prototipos)");
        }

        if (tokPuntero != nullptr)
        {
            error(*tokPuntero, "las definiciones de funcion no admiten "
                               "parametros de tipo puntero ('char *')");
        }

        if (tokVariadica != nullptr)
        {
            error(*tokVariadica, "las definiciones de funcion no admiten "
                                 "parametros variables ('...')");
        }

        fn->cuerpo = parsearCuerpo();
    }
    else if (revisar(TokenType::SEMICOLON))
    {
        avanzar();
    }
    else
    {
        error(actual(), "se esperaba ';' (declaracion) o '{' (definicion) "
                        "despues de la lista de parametros pero se encontro " +
                        describir(actual()));
    }

    return fn;
}


// ==================================================
// CUERPO DE FUNCION
// function_body := "{" declarations statement_list "}"
// Las declaraciones van primero, antes de las sentencias.
// ==================================================

std::unique_ptr<CuerpoFuncion> Parser::parsearCuerpo()
{
    const Token& llave = consumir(TokenType::LBRACE, "'{'");

    auto cuerpo = std::make_unique<CuerpoFuncion>();

    while (esTipo(actual().tipo))
    {
        cuerpo->declaraciones.push_back(parsearDeclaracion(false));
    }

    while (!revisar(TokenType::RBRACE))
    {
        if (revisar(TokenType::END_OF_FILE))
        {
            error(actual(), "se encontro fin de archivo; falta '}' para cerrar "
                            "la llave abierta en la linea " +
                            std::to_string(llave.linea) + ", columna " +
                            std::to_string(llave.columna));
        }

        cuerpo->sentencias.push_back(parsearSentencia());
    }

    avanzar();   // '}'

    return cuerpo;
}


// ==================================================
// SENTENCIAS
// ==================================================

SentenciaPtr Parser::parsearSentencia()
{
    if (esTipo(actual().tipo))
    {
        error(actual(), "las declaraciones deben ir al inicio del cuerpo de la "
                        "funcion, antes de cualquier sentencia (no se permiten "
                        "dentro de bloques anidados)");
    }

    if (revisar(TokenType::LBRACE))   return parsearBloque();
    if (revisar(TokenType::RETURN))   return parsearReturn();
    if (revisar(TokenType::WHILE))    return parsearWhile();
    if (revisar(TokenType::IF))       return parsearIf();

    if (revisar(TokenType::BREAK))
    {
        const Token& token = avanzar();
        consumir(TokenType::SEMICOLON, "';' despues de 'break'");
        return std::make_unique<SentenciaBreak>(token.linea, token.columna);
    }

    if (revisar(TokenType::CONTINUE))
    {
        const Token& token = avanzar();
        consumir(TokenType::SEMICOLON, "';' despues de 'continue'");
        return std::make_unique<SentenciaContinue>(token.linea, token.columna);
    }

    // expression ";"
    const Token& inicio = actual();

    ExpresionPtr expresion = parsearExpresion();

    consumir(TokenType::SEMICOLON, "';' al final de la sentencia");

    return std::make_unique<SentenciaExpresion>(
        inicio.linea, inicio.columna, std::move(expresion));
}


// "{" statement_list "}"

SentenciaPtr Parser::parsearBloque()
{
    const Token& llave = consumir(TokenType::LBRACE, "'{'");

    auto bloque = std::make_unique<Bloque>(llave.linea, llave.columna);

    while (!revisar(TokenType::RBRACE))
    {
        if (revisar(TokenType::END_OF_FILE))
        {
            error(actual(), "se encontro fin de archivo; falta '}' para cerrar "
                            "la llave abierta en la linea " +
                            std::to_string(llave.linea) + ", columna " +
                            std::to_string(llave.columna));
        }

        bloque->sentencias.push_back(parsearSentencia());
    }

    avanzar();   // '}'

    return bloque;
}


// "return" ";"  |  "return" expression ";"

SentenciaPtr Parser::parsearReturn()
{
    const Token& token = consumir(TokenType::RETURN, "'return'");

    ExpresionPtr valor = nullptr;

    if (!revisar(TokenType::SEMICOLON))
    {
        valor = parsearExpresion();
    }

    consumir(TokenType::SEMICOLON, "';' despues de 'return'");

    return std::make_unique<SentenciaReturn>(
        token.linea, token.columna, std::move(valor));
}


// "while" "(" expression ")" statement

SentenciaPtr Parser::parsearWhile()
{
    const Token& token = consumir(TokenType::WHILE, "'while'");

    consumir(TokenType::LPAREN, "'(' despues de 'while'");

    ExpresionPtr condicion = parsearExpresion();

    consumir(TokenType::RPAREN, "')' despues de la condicion del 'while'");

    SentenciaPtr cuerpo = parsearSentencia();

    return std::make_unique<SentenciaWhile>(
        token.linea, token.columna, std::move(condicion), std::move(cuerpo));
}


// "if" "(" expression ")" statement [ "else" statement ]
// Un 'else' se une al 'if' mas cercano (igual que en C).

SentenciaPtr Parser::parsearIf()
{
    const Token& token = consumir(TokenType::IF, "'if'");

    consumir(TokenType::LPAREN, "'(' despues de 'if'");

    ExpresionPtr condicion = parsearExpresion();

    consumir(TokenType::RPAREN, "')' despues de la condicion del 'if'");

    SentenciaPtr entonces = parsearSentencia();

    SentenciaPtr sino = nullptr;

    if (coincide(TokenType::ELSE))
    {
        sino = parsearSentencia();
    }

    return std::make_unique<SentenciaIf>(
        token.linea, token.columna,
        std::move(condicion), std::move(entonces), std::move(sino));
}


// ==================================================
// EXPRESIONES
// ==================================================

// expression := equality "=" equality | equality
// (segun la gramatica, la asignacion no se encadena)

ExpresionPtr Parser::parsearExpresion()
{
    ExpresionPtr izquierda = parsearIgualdad();

    if (revisar(TokenType::ASSIGN))
    {
        const Token& igual = avanzar();

        auto* destino = dynamic_cast<Identificador*>(izquierda.get());

        if (destino == nullptr)
        {
            error(igual, "el lado izquierdo de '=' debe ser una variable");
        }

        int linea = destino->linea;
        int columna = destino->columna;
        std::string nombre = destino->nombre;

        ExpresionPtr valor = parsearIgualdad();

        return std::make_unique<Asignacion>(
            linea, columna, nombre, std::move(valor));
    }

    return izquierda;
}


// equality := relational (("==" | "!=") relational)?

ExpresionPtr Parser::parsearIgualdad()
{
    ExpresionPtr izquierda = parsearRelacional();

    if (revisar(TokenType::EQ) || revisar(TokenType::NEQ))
    {
        const Token& op = avanzar();

        ExpresionPtr derecha = parsearRelacional();

        return std::make_unique<ExpresionBinaria>(
            op.linea, op.columna, std::move(izquierda), op.lexema, std::move(derecha));
    }

    return izquierda;
}


// relational := simple (("<" | "<=" | ">=" | ">") simple)?

ExpresionPtr Parser::parsearRelacional()
{
    ExpresionPtr izquierda = parsearSimple();

    if (revisar(TokenType::LT) || revisar(TokenType::LE) ||
        revisar(TokenType::GE) || revisar(TokenType::GT))
    {
        const Token& op = avanzar();

        ExpresionPtr derecha = parsearSimple();

        return std::make_unique<ExpresionBinaria>(
            op.linea, op.columna, std::move(izquierda), op.lexema, std::move(derecha));
    }

    return izquierda;
}


// simple := term (("+" | "-") term)*

ExpresionPtr Parser::parsearSimple()
{
    ExpresionPtr izquierda = parsearTermino();

    while (revisar(TokenType::PLUS) || revisar(TokenType::MINUS))
    {
        const Token& op = avanzar();

        ExpresionPtr derecha = parsearTermino();

        izquierda = std::make_unique<ExpresionBinaria>(
            op.linea, op.columna, std::move(izquierda), op.lexema, std::move(derecha));
    }

    return izquierda;
}


// term := factor (("*" | "/") factor)*

ExpresionPtr Parser::parsearTermino()
{
    ExpresionPtr izquierda = parsearFactor();

    while (revisar(TokenType::STAR) || revisar(TokenType::SLASH))
    {
        const Token& op = avanzar();

        ExpresionPtr derecha = parsearFactor();

        izquierda = std::make_unique<ExpresionBinaria>(
            op.linea, op.columna, std::move(izquierda), op.lexema, std::move(derecha));
    }

    return izquierda;
}


// factor := constant | identifier | "(" expression ")" | add_op factor
//         | identifier "(" [expression_list] ")"

ExpresionPtr Parser::parsearFactor()
{
    // Operador unario + o -
    if (revisar(TokenType::PLUS) || revisar(TokenType::MINUS))
    {
        const Token& op = avanzar();

        ExpresionPtr operando;

        // "-32768" es valido aunque "32768" solo no lo sea
        if (op.tipo == TokenType::MINUS && revisar(TokenType::INTEGER_CONSTANT))
        {
            operando = crearLiteralEntero(avanzar(), true);
        }
        else
        {
            operando = parsearFactor();
        }

        return std::make_unique<ExpresionUnaria>(
            op.linea, op.columna, op.lexema, std::move(operando));
    }

    if (revisar(TokenType::INTEGER_CONSTANT))
    {
        return crearLiteralEntero(avanzar(), false);
    }

    if (revisar(TokenType::CHAR_CONSTANT))
    {
        const Token& token = avanzar();

        return std::make_unique<LiteralChar>(
            token.linea, token.columna, token.lexema, token.valor);
    }

    if (revisar(TokenType::STRING_CONSTANT))
    {
        const Token& token = avanzar();

        return std::make_unique<LiteralString>(
            token.linea, token.columna, token.lexema);
    }

    if (revisar(TokenType::IDENTIFIER))
    {
        const Token& token = avanzar();

        // Llamada a funcion
        if (revisar(TokenType::LPAREN))
        {
            avanzar();   // '('

            auto llamada = std::make_unique<Llamada>(
                token.linea, token.columna, token.lexema);

            if (!revisar(TokenType::RPAREN))
            {
                llamada->argumentos.push_back(parsearExpresion());

                while (coincide(TokenType::COMMA))
                {
                    llamada->argumentos.push_back(parsearExpresion());
                }
            }

            consumir(TokenType::RPAREN, "')' despues de los argumentos");

            return llamada;
        }

        return std::make_unique<Identificador>(
            token.linea, token.columna, token.lexema);
    }

    if (revisar(TokenType::LPAREN))
    {
        avanzar();

        ExpresionPtr expresion = parsearExpresion();

        consumir(TokenType::RPAREN, "')' para cerrar la expresion");

        return expresion;
    }

    error(actual(), "se esperaba una expresion (constante, identificador o "
                    "'(') pero se encontro " + describir(actual()));
}


// ==================================================
// ENTEROS DE 16 BITS (complemento a dos)
// - 32768 solo es valido con signo negativo (-32768)
// - Los hexadecimales >= 0x8000 representan negativos
// ==================================================

ExpresionPtr Parser::crearLiteralEntero(const Token& token, bool conSignoNegativo)
{
    int valor = token.valor;

    if (esHexa(token.lexema))
    {
        if (valor >= 0x8000)
        {
            valor -= 65536;
        }
    }
    else if (valor == 32768 && !conSignoNegativo)
    {
        error(token, "la constante 32768 esta fuera del rango de 16 bits "
                     "(solo -32768 es valido)");
    }

    return std::make_unique<LiteralEntero>(token.linea, token.columna, valor);
}
