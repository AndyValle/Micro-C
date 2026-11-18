#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>

#include "Lexer.h"
#include "Parser.h"
#include "ASTPrinter.h"

using namespace std;

// ==================================================
// MAIN
// Uso:  ./microc archivo.c            -> muestra el AST
//       ./microc archivo.c --tokens   -> muestra tambien la tabla de tokens
// ==================================================

void imprimirTokens(const vector<Token>& tokens)
{
    cout << "==========================================================" << endl;
    cout << "                    ANALIZADOR LEXICO                     " << endl;
    cout << "==========================================================" << endl;
    cout << endl;

    cout << "  "
         << left << setw(6)  << "#"
         << left << setw(7)  << "LINEA"
         << left << setw(5)  << "COL"
         << left << setw(19) << "TOKEN"
         << "LEXEMA" << endl;

    cout << "  "
         << left << setw(6)  << "----"
         << left << setw(7)  << "-----"
         << left << setw(5)  << "---"
         << left << setw(19) << "----------------"
         << "------" << endl;

    int numero = 1;

    for (const Token& token : tokens)
    {
        if (token.tipo == TokenType::END_OF_FILE)
        {
            continue;
        }

        cout << "  "
             << left << setw(6)  << numero
             << left << setw(7)  << token.linea
             << left << setw(5)  << token.columna
             << left << setw(19) << nombreToken(token.tipo)
             << token.lexema << endl;

        numero++;
    }

    cout << endl;
    cout << "Analisis lexico exitoso: " << (numero - 1) << " tokens." << endl;
    cout << endl;
}


int main(int argc, char* argv[])
{
    string ruta;
    bool mostrarTokens = false;

    for (int i = 1; i < argc; i++)
    {
        string arg = argv[i];

        if (arg == "--tokens")
        {
            mostrarTokens = true;
        }
        else
        {
            ruta = arg;
        }
    }

    if (ruta.empty())
    {
        cerr << "Uso: ./microc archivo.c [--tokens]" << endl;
        return 1;
    }

    ifstream archivo(ruta);

    if (!archivo.is_open())
    {
        cerr << "Error: no se pudo abrir el archivo '" << ruta << "'" << endl;
        return 1;
    }

    stringstream buffer;
    buffer << archivo.rdbuf();

    try
    {
        // 1. Analisis lexico
        Lexer lexer(buffer.str());
        vector<Token> tokens = lexer.analizar();

        if (mostrarTokens)
        {
            imprimirTokens(tokens);
        }

        // 2. Analisis sintactico
        Parser parser(tokens);
        unique_ptr<Programa> programa = parser.parsear();

        cout << "==========================================================" << endl;
        cout << "                  ANALIZADOR SINTACTICO                   " << endl;
        cout << "==========================================================" << endl;
        cout << endl;
        cout << "Analisis sintactico exitoso. AST resultante:" << endl;
        cout << endl;

        imprimirPrograma(*programa);
    }
    catch (const ErrorCompilador& e)
    {
        cerr << "Error " << e.fase
             << " [linea " << e.linea
             << ", columna " << e.columna << "]: "
             << e.mensaje << endl;

        return 1;
    }

    return 0;
}
