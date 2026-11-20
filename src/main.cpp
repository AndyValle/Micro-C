#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>

#include "Lexer.h"
#include "Parser.h"
#include "Semantico.h"
#include "ASTPrinter.h"

using namespace std;

// ==================================================
// MAIN
// Uso:  ./microc archivo.c [--tokens] [--simbolos]
//
//   (sin opciones)  analiza el archivo y muestra el AST
//   --tokens        muestra tambien la tabla de tokens
//   --simbolos      muestra tambien la tabla de simbolos
//
// El compilador se detiene en el primer error.
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


void imprimirSimbolos(const vector<EntradaTabla>& registro)
{
    cout << "==========================================================" << endl;
    cout << "                   TABLA DE SIMBOLOS                      " << endl;
    cout << "==========================================================" << endl;
    cout << endl;

    cout << "  "
         << left << setw(18) << "AMBITO"
         << left << setw(14) << "NOMBRE"
         << left << setw(12) << "CLASE"
         << left << setw(24) << "TIPO"
         << "UBICACION" << endl;

    cout << "  "
         << left << setw(18) << "------"
         << left << setw(14) << "------"
         << left << setw(12) << "-----"
         << left << setw(24) << "----"
         << "---------" << endl;

    for (const EntradaTabla& fila : registro)
    {
        string ubicacion = (fila.linea == 0)
                               ? "(biblioteca)"
                               : to_string(fila.linea) + ":" + to_string(fila.columna);

        cout << "  "
             << left << setw(18) << fila.ambito
             << left << setw(14) << fila.nombre
             << left << setw(12) << fila.clase
             << left << setw(24) << fila.tipo
             << ubicacion << endl;
    }

    cout << endl;
}


int main(int argc, char* argv[])
{
    string ruta;
    bool mostrarTokens = false;
    bool mostrarSimbolos = false;

    for (int i = 1; i < argc; i++)
    {
        string arg = argv[i];

        if (arg == "--tokens")
        {
            mostrarTokens = true;
        }
        else if (arg == "--simbolos")
        {
            mostrarSimbolos = true;
        }
        else
        {
            ruta = arg;
        }
    }

    if (ruta.empty())
    {
        cerr << "Uso: ./microc archivo.c [--tokens] [--simbolos]" << endl;
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

        // 3. Analisis semantico
        Semantico semantico(*programa);
        semantico.analizar();

        // Todo correcto: se muestran los resultados

        cout << "==========================================================" << endl;
        cout << "                  ANALIZADOR SINTACTICO                   " << endl;
        cout << "==========================================================" << endl;
        cout << endl;
        cout << "Analisis sintactico exitoso. AST resultante:" << endl;
        cout << endl;

        imprimirPrograma(*programa);

        cout << endl;
        cout << "==========================================================" << endl;
        cout << "                   ANALIZADOR SEMANTICO                   " << endl;
        cout << "==========================================================" << endl;
        cout << endl;
        cout << "Analisis semantico exitoso: no se encontraron errores." << endl;
        cout << endl;

        if (mostrarSimbolos)
        {
            imprimirSimbolos(semantico.registro());
        }
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
