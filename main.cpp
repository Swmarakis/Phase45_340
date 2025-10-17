#include <iostream>
#include <fstream>
#include <iomanip>
#include "al.hpp"
#include "symbol_table.hpp"
#include "quads.hpp"
#include "target_code.hpp"

extern int yyparse();
extern FILE *yyin;

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <input_file>" << std::endl;
        return 1;
    }

    yyin = fopen(argv[1], "r");
    if (!yyin)
    {
        std::cerr << "Cannot open input file: " << argv[1] << std::endl;
        return 1;
    }

    std::cout << "\n\n-----------    Intermediate Generation Code    -------------\n\n" << std::endl;

    yyparse();
    symbol_table.assignOffsets(0);
    print_quads();
    symbol_table.print();

    TargetCodeGenerator generator(quads, symbol_table);
    generator.generate();
    generator.printFormattedOutput(); // Use text output instead of binary
    //generator.printInstructions();    // Optional: for debugging

    fclose(yyin);
    return 0;
}