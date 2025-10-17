CC = g++                # Compiler
CFLAGS = -std=c++17 -fPIC  # Compiler flags
BISON = bison           # Bison command
FLEX = flex             # Flex command

# Updated SRC to include target_code.cpp
SRC = main.cpp symbol_table.cpp al.cpp parser.tab.cpp alpha.lex.cpp quads.cpp target_code.cpp
BISON_OUT = parser.tab.cpp parser.tab.hpp
FLEX_OUT = alpha.lex.cpp
EXEC = alpha_parser 
VM_SRC = vm.cpp
VM_EXEC = vm

all: $(EXEC) $(VM_EXEC)

parser.tab.cpp parser.tab.hpp: parser.y
	$(BISON) -d parser.y -o parser.tab.cpp

alpha.lex.cpp: alpha.l parser.tab.hpp
	$(FLEX) -o alpha.lex.cpp alpha.l

$(EXEC): $(SRC)
	$(CC) $(CFLAGS) -o $(EXEC) $(SRC)

$(VM_EXEC): $(VM_SRC)
	$(CC) $(CFLAGS) -o $(VM_EXEC) $(VM_SRC)

clean:
	rm -f $(EXEC) $(VM_EXEC) $(BISON_OUT) $(FLEX_OUT)