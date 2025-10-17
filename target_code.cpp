#include "target_code.hpp"
#include <iostream>
#include <algorithm>

string escapeString(const string &s)
{
    string escaped;
    for (char c : s)
    {
        switch (c)
        {
        case '\n':
            escaped += "\\n";
            break; // Newline becomes "\n"
        case '\t':
            escaped += "\\t";
            break; // Tab becomes "\t"
        case '\\':
            escaped += "\\\\";
            break; // Backslash becomes "\\"
        case '"':
            escaped += "\\\"";
            break; // Quote becomes "\""
        default:
            escaped += c;
            break; // Other characters unchanged
        }
    }
    return escaped;
}

// Constructor
TargetCodeGenerator::TargetCodeGenerator(const vector<Quad> &q, const SymbolTable &st)
    : quads(q), symbol_table(st) {}

// Add user function to userFuncs and return its index
unsigned TargetCodeGenerator::addUserFunc(const string &name, unsigned address, unsigned localSize, unsigned numParams)
{
    userFuncs.push_back({name, address, localSize, numParams});
    return userFuncs.size() - 1;
}

// Add string to strConsts and return its index
unsigned TargetCodeGenerator::addString(const string &s)
{
    auto it = find(strConsts.begin(), strConsts.end(), s);
    if (it != strConsts.end())
    {
        return it - strConsts.begin();
    }
    strConsts.push_back(s);
    return strConsts.size() - 1;
}

// Add number to numConsts and return its index
unsigned TargetCodeGenerator::addNumber(double n)
{
    auto it = find(numConsts.begin(), numConsts.end(), n);
    if (it != numConsts.end())
    {
        return it - numConsts.begin();
    }
    numConsts.push_back(n);
    return numConsts.size() - 1;
}

// Add library function to libFuncs and return its index
unsigned TargetCodeGenerator::addLibFunc(const string &name)
{
    auto it = find(libFuncs.begin(), libFuncs.end(), name);
    if (it != libFuncs.end())
    {
        return it - libFuncs.begin();
    }
    libFuncs.push_back(name);
    return libFuncs.size() - 1;
}

Operand TargetCodeGenerator::makeOperand(const string &op)
{
    if (op.empty())
    {
        return Operand(OperandType::NIL_A, 0); // Empty operand is nil
    }

    // Handle numeric literals
    try
    {
        size_t pos;
        double num = stod(op, &pos); // Attempt to convert the string to a number
        if (pos == op.size())
        {                                                          // If the entire string is a valid number
            return Operand(OperandType::NUMBER_A, addNumber(num)); // Add to numConsts and return
        }
    }
    catch (...)
    {
        // Not a number, proceed to other checks
    }

    // Handle temporary variables (e.g., ^0, ^1)
    if (op[0] == '^')
    {
        unsigned temp_idx = stoi(op.substr(1));
        // For temporaries, use the current scope size as base offset
        unsigned base_offset = scope_sizes[current_scopes.back()];
        unsigned offset = base_offset + temp_idx;
        return Operand(OperandType::LOCAL_A, offset);
    }

    // Handle string literals (e.g., "hello")
    if (op.size() >= 2 && op.front() == '"' && op.back() == '"')
    {
        string s = op.substr(1, op.size() - 2);
        return Operand(OperandType::STRING_A, addString(s));
    }

    // Handle boolean constants (e.g., 'true', 'false')
    if (op.size() >= 2 && op.front() == '\'' && op.back() == '\'')
    {
        string inner = op.substr(1, op.size() - 2);
        if (inner == "true")
            return Operand(OperandType::BOOL_A, 1);
        if (inner == "false")
            return Operand(OperandType::BOOL_A, 0);
    }

    // Look up identifiers in the symbol table
    int current_scope = current_scopes.back();
    const SymbolEntry *entry = symbol_table.lookup_from_scope(op, current_scope);
    if (entry)
    {
        switch (entry->type)
        {
        case SymbolType::GLOBAL_VARIABLE:
            return Operand(OperandType::GLOBAL_A, entry->offset);
        case SymbolType::LOCAL_VARIABLE:
            return Operand(OperandType::LOCAL_A, entry->offset);
        case SymbolType::FORMAL_ARGUMENT:
            return Operand(OperandType::FORMAL_A, entry->offset);
        case SymbolType::LIBRARY_FUNCTION:
            return Operand(OperandType::LIBFUNC_A, addLibFunc(op));
        case SymbolType::USER_FUNCTION:
            // Use the pre-existing function index instead of adding a new one
            if (userFuncIndices.find(op) != userFuncIndices.end())
            {
                return Operand(OperandType::USERFUNC_A, userFuncIndices[op]);
            }
            return Operand(OperandType::USERFUNC_A, addUserFunc(op, 0, 0, 0));
        default:
            throw runtime_error("Unsupported symbol type: " + op);
        }
    }

    // Handle nil
    if (op == "nil")
    {
        return Operand(OperandType::NIL_A, 0);
    }

    throw invalid_argument("Invalid operand: " + op); // If all else fails
}

// Emit an instruction to the instructions vector
void TargetCodeGenerator::emitInstruction(const Instruction &instr)
{
    instructions.push_back(instr);
}

void TargetCodeGenerator::generate()
{
    

    // Step 1: Build label map for jump targets
    
    map<string, unsigned> labelMap;
    unsigned instr_index = 0;
    for (const auto &q : quads)
    {
        
        if (q.op == "label")
        {
            labelMap[q.target_label] = instr_index;
            
        }
        else
        {
            instr_index++;
            
        }
    }
    

    // Step 2: Compute scope sizes
    
    scope_sizes.resize(symbol_table.get_num_scopes());
    
    for (size_t s = 0; s < symbol_table.get_num_scopes(); ++s)
    {
        scope_sizes[s] = symbol_table.get_scope_size(s);
        
    }

    // Step 3: Compute max temporary index per function
    
    string current_func;
    for (const auto &q : quads)
    {
        
        if (q.op == "funcstart")
        {
            current_func = q.result;
            func_max_temp[current_func] = -1; // No temporaries yet
           
        }
        else if (q.op == "funcend")
        {
            
                     
            current_func = "";
        }
        else if (!current_func.empty())
        {
            auto check_temp = [&](const string &operand)
            {
                if (!operand.empty() && operand[0] == '^')
                {
                    int idx = stoi(operand.substr(1));
                    
                    if (func_max_temp[current_func] < idx)
                    {
                        func_max_temp[current_func] = idx;
                        
                    }
                }
            };
            
            check_temp(q.result);
            check_temp(q.arg1);
            check_temp(q.arg2);
        }
    }

    // Step 3.5: Pre-populate user functions to avoid duplicates
    
    for (const auto &q : quads)
    {
        if (q.op == "funcstart")
        {
            string func_name = q.result;
            if (userFuncIndices.find(func_name) == userFuncIndices.end())
            {
                const SymbolEntry *func_entry = symbol_table.lookup(func_name);
                if (func_entry && func_entry->type == SymbolType::USER_FUNCTION)
                {
                    int body_scope = func_entry->body_scope;
                    unsigned localSize = scope_sizes[body_scope];
                    unsigned numParams = 0;
                    // Count the number of formal arguments in the function's scope
                    for (const auto &sym : symbol_table.getScopeSymbols(body_scope))
                    {
                        if (sym.type == SymbolType::FORMAL_ARGUMENT)
                        {
                            numParams++;
                        }
                        else
                        {
                            break; // Formal arguments are first in scope
                        }
                    }
                    if (func_max_temp.count(func_name) && func_max_temp[func_name] >= 0)
                    {
                        localSize += func_max_temp[func_name] + 1;
                    }
                    unsigned func_index = addUserFunc(func_name, 0, localSize, numParams);
                    userFuncIndices[func_name] = func_index;
                    
                }
            }
        }
    }

    // Step 4: Initialize scope stack to global scope
    
    current_scopes = {0};
    

    // Step 5: Process all quads and generate instructions
   
    for (const auto &q : quads)
    {



        if (q.op == "label")
        {
           
            continue; // Skip label quads; they are for positioning only
        }

        Instruction instr;
        instr.line = q.line;

        if (q.op == "funcstart")
        {
            string func_name = q.result;
            
            const SymbolEntry *func_entry = symbol_table.lookup(func_name);
            if (func_entry && func_entry->type == SymbolType::USER_FUNCTION)
            {
                int body_scope = func_entry->body_scope;
                
                current_scopes.push_back(body_scope);
               

                // Update the address in the pre-existing function entry
                unsigned func_index = userFuncIndices[func_name];
                userFuncs[func_index].address = instructions.size();

                instr.opcode = Opcode::FUNCSTART;
                instr.arg1 = Operand(OperandType::USERFUNC_A, func_index);
                instr.result = Operand(OperandType::NIL_A, 0);
                instr.arg2 = Operand(OperandType::NIL_A, 0);
                
                emitInstruction(instr);
            }
            else
            {
                cout << "      Error: Function '" << func_name << "' not found or not a user function" << endl;
            }
        }
        else if (q.op == "funcend")
        {
            

            // Step 1: Emit RET instruction before FUNCEND
            Instruction retInstr;
            retInstr.opcode = Opcode::RET;
            retInstr.arg1 = Operand(OperandType::NIL_A, 0); // No return value specified
            retInstr.result = Operand(OperandType::NIL_A, 0);
            retInstr.arg2 = Operand(OperandType::NIL_A, 0);
            retInstr.line = q.line;
            emitInstruction(retInstr);
            

            // Step 2: Emit FUNCEND instruction as before
            Instruction funcEndInstr;
            funcEndInstr.opcode = Opcode::FUNCEND;
            string func_name = q.result;
            unsigned func_index = userFuncIndices[func_name];
            funcEndInstr.arg1 = Operand(OperandType::USERFUNC_A, func_index);
            funcEndInstr.result = Operand(OperandType::NIL_A, 0);
            funcEndInstr.arg2 = Operand(OperandType::NIL_A, 0);
            funcEndInstr.line = q.line;
            emitInstruction(funcEndInstr);
           

            // Step 3: Manage scope stack
            if (!current_scopes.empty())
            {
               
                current_scopes.pop_back();
            }
            else
            {
                cout << "      Warning: Scope stack is empty, cannot pop" << endl;
            }
        }
        else if (q.op == "assign")
        {
            
            instr.opcode = Opcode::ASSIGN;
            instr.result = makeOperand(q.result);
            instr.arg1 = makeOperand(q.arg1);
            instr.arg2 = Operand(OperandType::NIL_A, 0);
            
            emitInstruction(instr);
        }
        else if (q.op == "add")
        {
           
            instr.opcode = Opcode::ADD;
            instr.result = makeOperand(q.result);
            instr.arg1 = makeOperand(q.arg1);
            instr.arg2 = makeOperand(q.arg2);
            
            emitInstruction(instr);
        }
        else if (q.op == "sub")
        {
            
            instr.opcode = Opcode::SUB;
            instr.result = makeOperand(q.result);
            instr.arg1 = makeOperand(q.arg1);
            instr.arg2 = makeOperand(q.arg2);
           
            emitInstruction(instr);
        }
        else if (q.op == "mul")
        {
            
            instr.opcode = Opcode::MUL;
            instr.result = makeOperand(q.result);
            instr.arg1 = makeOperand(q.arg1);
            instr.arg2 = makeOperand(q.arg2);
            
            emitInstruction(instr);
        }
        else if (q.op == "div")
        {
            
            instr.opcode = Opcode::DIV;
            instr.result = makeOperand(q.result);
            instr.arg1 = makeOperand(q.arg1);
            instr.arg2 = makeOperand(q.arg2);
            
            emitInstruction(instr);
        }
        else if (q.op == "mod")
        {
            
            instr.opcode = Opcode::MOD;
            instr.result = makeOperand(q.result);
            instr.arg1 = makeOperand(q.arg1);
            instr.arg2 = makeOperand(q.arg2);
           
            emitInstruction(instr);
        }
        else if (q.op == "uminus")
        {
           
            instr.opcode = Opcode::UMINUS;
            instr.result = makeOperand(q.result);
            instr.arg1 = makeOperand(q.arg1);
            instr.arg2 = Operand(OperandType::NIL_A, 0);
            
            emitInstruction(instr);
        }
        else if (q.op == "not")
        {
           
            instr.opcode = Opcode::NOT;
            instr.result = makeOperand(q.result);
            instr.arg1 = makeOperand(q.arg1);
            instr.arg2 = Operand(OperandType::NIL_A, 0);
          
            emitInstruction(instr);
        }
        else if (q.op == "and")
        {
           
            instr.opcode = Opcode::AND;
            instr.result = makeOperand(q.result);
            instr.arg1 = makeOperand(q.arg1);
            instr.arg2 = makeOperand(q.arg2);
            
            emitInstruction(instr);
        }
        else if (q.op == "or")
        {
            
            instr.opcode = Opcode::OR;
            instr.result = makeOperand(q.result);
            instr.arg1 = makeOperand(q.arg1);
            instr.arg2 = makeOperand(q.arg2);
            
            emitInstruction(instr);
        }
        else if (q.op == "pusharg")
        {
           
            instr.opcode = Opcode::PUSHARG;
            instr.arg1 = makeOperand(q.arg1);
            instr.result = Operand(OperandType::NIL_A, 0);
            instr.arg2 = Operand(OperandType::NIL_A, 0);
           
            emitInstruction(instr);
        }
        else if (q.op == "funcexit")
        {
            
            instr.opcode = Opcode::FUNCEXIT;
            instr.arg1 = Operand(OperandType::NIL_A, 0);
            instr.result = Operand(OperandType::NIL_A, 0);
            instr.arg2 = Operand(OperandType::NIL_A, 0);
           
            emitInstruction(instr);
        }
        else if (q.op == "nop")
        {
           
            instr.opcode = Opcode::NOP;
            instr.arg1 = Operand(OperandType::NIL_A, 0);
            instr.result = Operand(OperandType::NIL_A, 0);
            instr.arg2 = Operand(OperandType::NIL_A, 0);
           
            emitInstruction(instr);
        }
        else if (q.op == "jump")
        {
            
            instr.opcode = Opcode::JUMP;
            instr.result = Operand(OperandType::LABEL_A, labelMap[q.target_label]);
            instr.arg1 = Operand(OperandType::NIL_A, 0);
            instr.arg2 = Operand(OperandType::NIL_A, 0);
           
            emitInstruction(instr);
        }
        else if (q.op == "if_eq")
        {
           
            instr.opcode = Opcode::IF_EQ;
            instr.arg1 = makeOperand(q.arg1);
            instr.arg2 = makeOperand(q.arg2);
            instr.result = Operand(OperandType::LABEL_A, labelMap[q.target_label]);
           
            emitInstruction(instr);
        }
        else if (q.op == "if_neq")
        {
           
            instr.opcode = Opcode::IF_NEQ;
            instr.arg1 = makeOperand(q.arg1);
            instr.arg2 = makeOperand(q.arg2);
            instr.result = Operand(OperandType::LABEL_A, labelMap[q.target_label]);
           
            emitInstruction(instr);
        }
        else if (q.op == "if_less")
        {
           
            instr.opcode = Opcode::IF_LESS;
            instr.arg1 = makeOperand(q.arg1);
            instr.arg2 = makeOperand(q.arg2);
            instr.result = Operand(OperandType::LABEL_A, labelMap[q.target_label]);
          
            emitInstruction(instr);
        }
        else if (q.op == "if_lesseq")
        {
           
            instr.opcode = Opcode::IF_LESSEQ;
            instr.arg1 = makeOperand(q.arg1);
            instr.arg2 = makeOperand(q.arg2);
            instr.result = Operand(OperandType::LABEL_A, labelMap[q.target_label]);
           
            emitInstruction(instr);
        }
        else if (q.op == "if_greater")
        {
           
            instr.opcode = Opcode::IF_GREATER;
            instr.arg1 = makeOperand(q.arg1);
            instr.arg2 = makeOperand(q.arg2);
            instr.result = Operand(OperandType::LABEL_A, labelMap[q.target_label]);
          
            emitInstruction(instr);
        }
        else if (q.op == "if_greatereq")
        {
            
            instr.opcode = Opcode::IF_GREATEREQ;
            instr.arg1 = makeOperand(q.arg1);
            instr.arg2 = makeOperand(q.arg2);
            instr.result = Operand(OperandType::LABEL_A, labelMap[q.target_label]);
          
            emitInstruction(instr);
        }
        else if (q.op == "param")
        {
            
            instr.opcode = Opcode::PARAM;
            instr.arg1 = makeOperand(q.arg1);
            instr.result = Operand(OperandType::NIL_A, 0);
            instr.arg2 = Operand(OperandType::NIL_A, 0);
           
            emitInstruction(instr);
        }
        else if (q.op == "call")
        {
            
            instr.opcode = Opcode::CALL;
            instr.arg1 = makeOperand(q.arg1);
            instr.result = Operand(OperandType::NIL_A, 0);
            instr.arg2 = Operand(OperandType::NIL_A, 0);
            
            emitInstruction(instr);
        }
        else if (q.op == "getretval")
        {
            
            instr.opcode = Opcode::GETRETVAL;
            instr.result = makeOperand(q.result);
            instr.arg1 = Operand(OperandType::NIL_A, 0);
            instr.arg2 = Operand(OperandType::NIL_A, 0);
           
            emitInstruction(instr);
        }
        else if (q.op == "ret")
        {
            
            instr.opcode = Opcode::RET;
            instr.arg1 = makeOperand(q.result);
            instr.result = Operand(OperandType::NIL_A, 0);
            instr.arg2 = Operand(OperandType::NIL_A, 0);
            
            emitInstruction(instr);
        }
        else if (q.op == "tablecreate")
        {
           
            instr.opcode = Opcode::TABLECREATE;
            instr.result = makeOperand(q.result);
            instr.arg1 = Operand(OperandType::NIL_A, 0);
            instr.arg2 = Operand(OperandType::NIL_A, 0);
           
            emitInstruction(instr);
        }
        else if (q.op == "tablegetelem")
        {
            
            instr.opcode = Opcode::TABLEGETELEM;
            instr.result = makeOperand(q.result);
            instr.arg1 = makeOperand(q.arg1);
            instr.arg2 = makeOperand(q.arg2);
         
            emitInstruction(instr);
        }
        else if (q.op == "tablesetelem")
        {
           
            instr.opcode = Opcode::TABLESETELEM;
            instr.result = makeOperand(q.result);
            instr.arg1 = makeOperand(q.arg1);
            instr.arg2 = makeOperand(q.arg2);
            
            emitInstruction(instr);
        }
        else if (q.op == "return")
        {
           
            instr.opcode = Opcode::RET;
            if (!q.arg1.empty())
            {
                instr.arg1 = makeOperand(q.arg1); // Convert ^0 to LOCAL_A operand
               
            }
            else
            {
                instr.arg1 = Operand(OperandType::NIL_A, 0);
                
            }
            instr.result = Operand(OperandType::NIL_A, 0);
            instr.arg2 = Operand(OperandType::NIL_A, 0);
            instr.line = q.line; // Ensure line number is set correctly
           
            emitInstruction(instr);
        }
        else
        {
            cerr << "    Unsupported quad operation: " << q.op << " at line " << q.line << endl;
            continue;
        }
    }
   
}

void TargetCodeGenerator::printFormattedOutput() const
{
    cout << "\n\n---------------------  Final Target Code  ---------------------\n\n";

    // Strings section
    cout << "strings\n"
              << strConsts.size() << "\n";
    for (const auto &s : strConsts)
    {
        cout << s.length() << " \"" << s << "\"\n";
    }

    // Numbers section
    cout << "numbers\n"
              << numConsts.size() << "\n";
    for (const auto &n : numConsts)
    {
        cout << n << "\n";
    }

    // Userfunctions section
    cout << "userfunctions\n"
              << userFuncs.size() << "\n";
    for (const auto &uf : userFuncs)
    {
        cout << uf.address << " " << uf.localSize << " " << uf.numParams << " " << uf.name << "\n";
    }

    // Libfunctions section
    cout << "libfunctions\n"
              << libFuncs.size() << "\n";
    for (const auto &lf : libFuncs)
    {
        cout << lf << "\n";
    }

    // Target_code section
    cout << "target_code\n"
              << instructions.size() << "\n";
    for (const auto &instr : instructions)
    {
        cout << static_cast<int>(instr.opcode) << "\t\t"
                  << static_cast<int>(instr.result.type) << " " << instr.result.val << "\t"
                  << static_cast<int>(instr.arg1.type) << " " << instr.arg1.val << "\t"
                  << static_cast<int>(instr.arg2.type) << " " << instr.arg2.val << "\t"
                  << instr.line << "\n";
    }
}