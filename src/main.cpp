#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

#include "Lexer.h"

using namespace std;

// ==================================================
// MAIN
// Uso:  ./microc archivo.c
// ==================================================

int main(int argc, char* argv[])
{

    if (argc < 2)
    {
        cerr << "Uso: ./microc archivo.c" << endl;
        return 1;
    }

    string ruta = argv[1];

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
        Lexer lexer(buffer.str());

        vector<Token> tokens = lexer.analizar();

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
        cout << "Analisis lexico exitoso: " << (numero - 1)
             << " tokens." << endl;
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