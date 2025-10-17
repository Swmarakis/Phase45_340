#ifndef TARGET_CODE_HPP
#define TARGET_CODE_HPP

#include <vector>
#include <string>
#include <unordered_map>
#include "quads.hpp"
#include "symbol_table.hpp"

enum Opcode {
    ASSIGN, ADD, SUB, MUL, DIV, MOD, JUMP, IF_EQ, IF_NEQ, IF_LESS, IF_LESSEQ, IF_GREATER, IF_GREATEREQ,
    CALL, PARAM, RET, GETRETVAL, FUNCSTART, FUNCEND, TABLECREATE, TABLEGETELEM, TABLESETELEM, NOT, UMINUS, AND, OR, PUSHARG, FUNCEXIT, NOP
};

enum OperandType {
    GLOBAL_A, LOCAL_A, FORMAL_A, NUMBER_A, STRING_A, BOOL_A, NIL_A,
    USERFUNC_A, LIBFUNC_A, LABEL_A, RETVAL_A
};

struct Operand {
    OperandType type;
    unsigned val;
    Operand(OperandType t = OperandType::NIL_A, unsigned v = 0) : type(t), val(v) {}
};

struct Instruction {
    Opcode opcode;
    Operand result;
    Operand arg1;
    Operand arg2;
    unsigned line;
};

struct UserFunc {
    string name;
    unsigned address;
    unsigned localSize;
    unsigned numParams;
};

class TargetCodeGenerator {
private:
    const vector<Quad>& quads;
    const SymbolTable& symbol_table;
    vector<Instruction> instructions;
    vector<double> numConsts;
    vector<string> strConsts;
    vector<UserFunc> userFuncs;
    vector<string> libFuncs;
    unordered_map<string, unsigned> labelMap;

    unsigned addNumber(double n);
    unsigned addString(const string& s);
    unsigned addUserFunc(const string& name, unsigned address, unsigned localSize, unsigned numParams);
    unsigned addLibFunc(const string& name);
    Operand makeOperand(const string& op);
    void emitInstruction(const Instruction& instr);

    vector<int> current_scopes;                  // Stack of current scopes
    vector<unsigned> scope_sizes;                // Number of variables per scope
    unordered_map<string, int> func_max_temp; // Max temp index per function
    unordered_map<string, unsigned> userFuncIndices; // Map function names to their indices

public:
    TargetCodeGenerator(const vector<Quad>& q, const SymbolTable& st);
    void generate();
    void printInstructions() const;
    void printFormattedOutput() const;
    void writeBinary(const string& filename);
};

#endif // TARGET_CODE_HPP