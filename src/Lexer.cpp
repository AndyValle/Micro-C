#include "Lexer.h"

#include <unordered_map>

// ==================================================
// UTILIDADES (solo visibles dentro de este archivo)
// ==================================================

namespace {

// Longitud maxima de un identificador segun el manual
const size_t MAX_IDENTIFICADOR = 12;

// Rango de enteros de 16 bits. El lexer acepta hasta 32768 porque
// el signo menos es un operador aparte: -32768 es valido, pero
// 32768 solo debe aceptarse con signo negativo (lo valida el parser).
const long MAX_DECIMAL = 32768;

// Hexadecimales de 16 bits: 0x0000 a 0xFFFF
const long MAX_HEX = 0xFFFF;

bool esLetra(char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           c == '_';
}

bool esDigito(char c) {
    return c >= '0' && c <= '9';
}

bool esHex(char c) {
    return esDigito(c) ||
           (c >= 'a' && c <= 'f') ||
           (c >= 'A' && c <= 'F');
}

int valorHex(char c) {
    if (esDigito(c)) return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return c - 'A' + 10;
}

// Para mostrar un caracter dentro de un mensaje de error
std::string describirChar(char c) {
    unsigned char u = static_cast<unsigned char>(c);
    if (u >= 32 && u < 127) {
        return std::string("'") + c + "'";
    }
    return "con codigo " + std::to_string(static_cast<int>(u));
}

const std::unordered_map<std::string, TokenType> PALABRAS_RESERVADAS = {
    { "int",      TokenType::INT },
    { "char",     TokenType::CHAR },
    { "void",     TokenType::VOID },
    { "return",   TokenType::RETURN },
    { "while",    TokenType::WHILE },
    { "if",       TokenType::IF },
    { "else",     TokenType::ELSE },
    { "break",    TokenType::BREAK },
    { "continue", TokenType::CONTINUE }
};

}  // namespace


// ==================================================
// CONSTRUCTOR Y NAVEGACION
// ==================================================

Lexer::Lexer(const std::string& codigo)
    : codigo(codigo), pos(0), linea(1), columna(1) {}


bool Lexer::fin() const {
    return pos >= codigo.size();
}


char Lexer::actual() const {
    return fin() ? '\0' : codigo[pos];
}


char Lexer::siguiente() const {
    return (pos + 1 < codigo.size()) ? codigo[pos + 1] : '\0';
}


// Consume un caracter y actualiza linea/columna

char Lexer::avanzar() {

    char c = codigo[pos++];

    if (c == '\n') {
        linea++;
        columna = 1;
    }
    else {
        columna++;
    }

    return c;
}


void Lexer::error(const std::string& mensaje,
                  int linea, int columna) const {

    throw ErrorCompilador{ "lexico", mensaje, linea, columna };
}


// ==================================================
// ESPACIOS Y COMENTARIOS
// ==================================================
// Se ignoran: newline, tab, CR, espacio y comentarios /* ... */
// Los comentarios no se anidan: el primer */ lo cierra.
// ==================================================

void Lexer::saltarEspaciosYComentarios() {

    while (!fin()) {

        char c = actual();

        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            avanzar();
            continue;
        }

        if (c == '/' && siguiente() == '*') {

            int lineaInicio = linea;
            int columnaInicio = columna;

            avanzar();   // '/'
            avanzar();   // '*'

            bool cerrado = false;

            while (!fin()) {

                if (actual() == '*' && siguiente() == '/') {
                    avanzar();
                    avanzar();
                    cerrado = true;
                    break;
                }

                avanzar();
            }

            if (!cerrado) {
                error("comentario sin cerrar: falta '*/'",
                      lineaInicio, columnaInicio);
            }

            continue;
        }

        break;
    }
}


// ==================================================
// IDENTIFICADORES Y PALABRAS RESERVADAS
// identificador = (letra | '_') (letra | digito)*   maximo 12
// ==================================================

Token Lexer::leerIdentificador() {

    int l = linea;
    int c = columna;
    size_t inicio = pos;

    while (!fin() && (esLetra(actual()) || esDigito(actual()))) {
        avanzar();
    }

    std::string texto = codigo.substr(inicio, pos - inicio);

    if (texto.size() > MAX_IDENTIFICADOR) {
        error("el identificador '" + texto + "' tiene " +
              std::to_string(texto.size()) +
              " caracteres; el maximo permitido es 12", l, c);
    }

    auto it = PALABRAS_RESERVADAS.find(texto);

    TokenType tipo = (it != PALABRAS_RESERVADAS.end())
                         ? it->second
                         : TokenType::IDENTIFIER;

    return Token(tipo, texto, l, c);
}


// ==================================================
// CONSTANTES NUMERICAS
// decimal: digito+      hexadecimal: 0x / 0X seguido de digitos hex
// Deben caber en 16 bits y separarse de identificadores.
// ==================================================

Token Lexer::leerNumero() {

    int l = linea;
    int c = columna;
    size_t inicio = pos;
    long valor = 0;

    if (actual() == '0' && (siguiente() == 'x' || siguiente() == 'X')) {

        avanzar();   // '0'
        avanzar();   // 'x'

        if (!esHex(actual())) {
            error("constante hexadecimal sin digitos despues de '0x'", l, c);
        }

        while (esHex(actual())) {

            valor = valor * 16 + valorHex(actual());

            if (valor > MAX_HEX) {
                error("constante hexadecimal fuera del rango de 16 bits "
                      "(maximo 0xFFFF)", l, c);
            }

            avanzar();
        }
    }
    else {

        while (esDigito(actual())) {

            valor = valor * 10 + (actual() - '0');

            if (valor > MAX_DECIMAL) {
                error("constante entera fuera del rango de 16 bits "
                      "(maximo 32768)", l, c);
            }

            avanzar();
        }
    }

    // Una constante numerica debe separarse de un identificador
    if (esLetra(actual())) {
        error("la constante numerica debe separarse de un identificador "
              "o palabra clave (se encontro " + describirChar(actual()) +
              " pegado al numero)", linea, columna);
    }

    return Token(TokenType::INTEGER_CONSTANT,
                 codigo.substr(inicio, pos - inicio),
                 l, c, static_cast<int>(valor));
}


// ==================================================
// CONSTANTE DE CARACTER
// comilla simple + un caracter ASCII extendido imprimible + comilla simple
// ==================================================

Token Lexer::leerCaracter() {

    int l = linea;
    int c = columna;
    size_t inicio = pos;

    avanzar();   // comilla de apertura

    if (fin() || actual() == '\n' || actual() == '\r') {
        error("constante de caracter sin cerrar", l, c);
    }

    if (actual() == '\'') {
        error("constante de caracter vacia", l, c);
    }

    unsigned char u = static_cast<unsigned char>(actual());

    bool imprimible = (u >= 32 && u <= 126) || u >= 128;

    if (!imprimible) {
        error("caracter no imprimible dentro de una constante de caracter",
              linea, columna);
    }

    avanzar();   // el caracter

    if (actual() != '\'') {
        error("una constante de caracter debe tener exactamente un "
              "caracter y cerrar con comilla simple", l, c);
    }

    avanzar();   // comilla de cierre

    return Token(TokenType::CHAR_CONSTANT,
                 codigo.substr(inicio, pos - inicio),
                 l, c, u);
}


// ==================================================
// CONSTANTE DE STRING
// Entre comillas dobles. Escapes permitidos: comilla doble, n y backslash
// No puede extenderse mas alla del final de la linea.
// ==================================================

Token Lexer::leerCadena() {

    int l = linea;
    int c = columna;
    size_t inicio = pos;

    avanzar();   // comilla de apertura

    while (true) {

        if (fin() || actual() == '\n' || actual() == '\r') {
            error("string sin cerrar: una constante de string no puede "
                  "extenderse mas alla del final de la linea", l, c);
        }

        char ch = actual();

        if (ch == '"') {
            avanzar();
            break;
        }

        if (ch == '\\') {

            int lineaEscape = linea;
            int columnaEscape = columna;

            avanzar();   // backslash

            if (fin() || actual() == '\n' || actual() == '\r') {
                error("string sin cerrar: una constante de string no puede "
                      "extenderse mas alla del final de la linea", l, c);
            }

            char e = actual();

            if (e != 'n' && e != '\\' && e != '"') {
                error("secuencia de escape ilegal '\\" + std::string(1, e) +
                      "': solo se permiten \\n, \\\\ y \\\"",
                      lineaEscape, columnaEscape);
            }

            avanzar();   // el caracter escapado
            continue;
        }

        avanzar();
    }

    return Token(TokenType::STRING_CONSTANT,
                 codigo.substr(inicio, pos - inicio),
                 l, c);
}


// ==================================================
// OPERADORES Y DELIMITADORES
// ==================================================

Token Lexer::leerSimbolo() {

    int l = linea;
    int c = columna;
    char ch = actual();

    switch (ch) {

        case '+': avanzar(); return Token(TokenType::PLUS,      "+", l, c);
        case '-': avanzar(); return Token(TokenType::MINUS,     "-", l, c);
        case '*': avanzar(); return Token(TokenType::STAR,      "*", l, c);
        case '/': avanzar(); return Token(TokenType::SLASH,     "/", l, c);

        case '(': avanzar(); return Token(TokenType::LPAREN,    "(", l, c);
        case ')': avanzar(); return Token(TokenType::RPAREN,    ")", l, c);
        case '{': avanzar(); return Token(TokenType::LBRACE,    "{", l, c);
        case '}': avanzar(); return Token(TokenType::RBRACE,    "}", l, c);
        case ',': avanzar(); return Token(TokenType::COMMA,     ",", l, c);
        case ';': avanzar(); return Token(TokenType::SEMICOLON, ";", l, c);

        case '=':
            avanzar();
            if (actual() == '=') {
                avanzar();
                return Token(TokenType::EQ, "==", l, c);
            }
            return Token(TokenType::ASSIGN, "=", l, c);

        case '!':
            avanzar();
            if (actual() == '=') {
                avanzar();
                return Token(TokenType::NEQ, "!=", l, c);
            }
            error("'!' solo es valido como parte de '!='", l, c);

        case '<':
            avanzar();
            if (actual() == '=') {
                avanzar();
                return Token(TokenType::LE, "<=", l, c);
            }
            return Token(TokenType::LT, "<", l, c);

        case '>':
            avanzar();
            if (actual() == '=') {
                avanzar();
                return Token(TokenType::GE, ">=", l, c);
            }
            return Token(TokenType::GT, ">", l, c);

        case '.':
            if (siguiente() == '.' &&
                pos + 2 < codigo.size() && codigo[pos + 2] == '.') {
                avanzar();
                avanzar();
                avanzar();
                return Token(TokenType::ELLIPSIS, "...", l, c);
            }
            error("'.' solo es valido como parte de '...'", l, c);

        default:
            error("caracter no valido " + describirChar(ch), l, c);
    }
}


// ==================================================
// ANALISIS COMPLETO
// ==================================================

std::vector<Token> Lexer::analizar() {

    std::vector<Token> tokens;

    while (true) {

        saltarEspaciosYComentarios();

        if (fin()) {
            break;
        }

        char c = actual();

        if (esLetra(c)) {
            tokens.push_back(leerIdentificador());
        }
        else if (esDigito(c)) {
            tokens.push_back(leerNumero());
        }
        else if (c == '\'') {
            tokens.push_back(leerCaracter());
        }
        else if (c == '"') {
            tokens.push_back(leerCadena());
        }
        else {
            tokens.push_back(leerSimbolo());
        }
    }

    tokens.push_back(Token(TokenType::END_OF_FILE, "", linea, columna));

    return tokens;
}