#include <iostream>
#include <vector>
#include <string>
#include <variant>
#include <sstream>
#include <stdexcept>
#include <cmath>
#include <unordered_map>
#include <memory>
#include <fstream>
#include <iostream>


// Forward declaration
struct Table;

using namespace std;

// Define enums matching target_code.hpp
enum Opcode
{
    ASSIGN = 0,
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,
    JUMP,
    IF_EQ,
    IF_NEQ,
    IF_LESS,
    IF_LESSEQ,
    IF_GREATER,
    IF_GREATEREQ,
    CALL,
    PARAM,
    RET,
    GETRETVAL,
    FUNCSTART,
    FUNCEND,
    TABLECREATE,
    TABLEGETELEM,
    TABLESETELEM,
    NOT,
    UMINUS,   // Added: Unary minus operation
    AND,      // Added: Logical AND operation
    OR,       // Added: Logical OR operation
    PUSHARG,  // Added: Push argument onto stack
    FUNCEXIT, // Added: Exit a function
    NOP       // Added: No operation
};

enum OperandType
{
    GLOBAL_A = 0,
    LOCAL_A,
    FORMAL_A,
    NUMBER_A,
    STRING_A,
    BOOL_A,
    NIL_A,
    USERFUNC_A,
    LIBFUNC_A,
    LABEL_A
};

struct Operand
{
    OperandType type;
    unsigned val;
};

struct Instruction
{
    Opcode opcode;
    Operand result;
    Operand arg1;
    Operand arg2;
    unsigned line;
};

struct UserFunc
{
    string name;
    unsigned address;
    unsigned localSize;
    unsigned numParams;
};

using Value = std::variant<std::string, double, bool, std::monostate, std::shared_ptr<Table>>;

// Hash function for Value
struct ValueHash
{
    size_t operator()(const Value &v) const
    {
        size_t h1 = std::hash<size_t>{}(v.index());
        size_t h2 = std::visit([](const auto &x)
                               {
            using T = std::decay_t<decltype(x)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                return size_t(0);
            } else if constexpr (std::is_same_v<T, std::shared_ptr<Table>>) {
                return std::hash<void*>{}(x.get());
            } else {
                return std::hash<T>{}(x);
            } }, v);
        return h1 ^ (h2 << 1);
    }
};

// Equality function for Value
struct ValueEqual
{
    bool operator()(const Value &a, const Value &b) const
    {
        if (a.index() != b.index())
            return false;
        switch (a.index())
        {
        case 0: // std::string
            return std::get<0>(a) == std::get<0>(b);
        case 1: // double
            return std::get<1>(a) == std::get<1>(b);
        case 2: // bool
            return std::get<2>(a) == std::get<2>(b);
        case 3:                                      // std::monostate
            return true;                             // Both are nil
        case 4:                                      // std::shared_ptr<Table>
            return std::get<4>(a) == std::get<4>(b); // Compare pointers
        default:
            throw std::runtime_error("Invalid variant index");
        }
    }
};

// Define Table structure
struct Table
{
    std::unordered_map<Value, Value, ValueHash, ValueEqual> data;
};

class VM
{
private:
    vector<string> strConsts;
    vector<double> numConsts;
    vector<UserFunc> userFuncs;
    vector<string> libFuncs;
    vector<Instruction> instructions;
    vector<Value> stack;      // Execution stack for parameters and temporary values
    vector<Value> globals;    // Global variables
    vector<Value> locals;     // Local variables
    vector<size_t> callStack; // Stack for return addresses
    size_t pc;                // Program counter
    Value returnValue;        // Return value of functions

    // New member variables for table set workaround
    bool isLastOperationTableGetElem = false;
    Value lastTableGetElemTable;
    Value lastTableGetElemKey;
    unsigned lastTableGetElemTempIndex;

    // Process a string to interpret escape sequences like \\n
    string processString(const string &s)
    {
        string result;
        for (size_t i = 0; i < s.size(); ++i)
        {
            if (s[i] == '\\' && i + 1 < s.size())
            {
                char next = s[i + 1];
                if (next == 'n')
                {
                    result += '\n';
                    i++; // Skip the 'n'
                }
                else if (next == '\\')
                {
                    result += '\\';
                    i++; // Skip the second '\\'
                }
                else
                {
                    result += s[i]; // Keep the '\\' as is
                }
            }
            else
            {
                result += s[i];
            }
        }
        return result;
    }

    // Get the value of an operand
    Value getValue(const Operand &op)
    {
        switch (op.type)
        {
        case GLOBAL_A:
            if (op.val >= globals.size())
                globals.resize(op.val + 1, monostate());
            return globals[op.val];
        case LOCAL_A:
            if (op.val >= locals.size())
                locals.resize(op.val + 1, monostate());
            return locals[op.val];
        case FORMAL_A:
            // For simplicity, treat FORMAL_A as LOCAL_A in this VM
            if (op.val >= locals.size())
                locals.resize(op.val + 1, monostate());
            return locals[op.val];
        case NUMBER_A:
            if (op.val >= numConsts.size())
                throw runtime_error("Number index out of bounds");
            return numConsts[op.val];
        case STRING_A:
            if (op.val >= strConsts.size())
                throw runtime_error("String index out of bounds");
            return strConsts[op.val];
        case BOOL_A:
            return (op.val != 0);
        case NIL_A:
            return monostate();
        default:
            throw runtime_error("Unsupported operand type in getValue");
        }
    }

    // Set the value of an operand
    void setValue(const Operand &op, const Value &val)
    {
        switch (op.type)
        {
        case GLOBAL_A:
            if (op.val >= globals.size())
                globals.resize(op.val + 1, monostate());
            globals[op.val] = val;
            break;
        case LOCAL_A:
        case FORMAL_A: // Treat FORMAL_A as LOCAL_A for simplicity
            if (op.val >= locals.size())
                locals.resize(op.val + 1, monostate());
            locals[op.val] = val;
            break;
        default:
            throw runtime_error("Can only set GLOBAL_A, LOCAL_A, or FORMAL_A variables");
        }
    }

    // Load the program from input stream
    void loadProgram(istream &is)
    {
        string line;
        while (getline(is, line))
        {
            if (line == "strings")
            {
                int count;
                is >> count;
                is.ignore();
                for (int i = 0; i < count; ++i)
                {
                    getline(is, line);
                    size_t pos = line.find('"');
                    size_t len = stoi(line.substr(0, pos));
                    string str = line.substr(pos + 1, len);
                    strConsts.push_back(str);
                }
            }
            else if (line == "numbers")
            {
                int count;
                is >> count;
                is.ignore();
                for (int i = 0; i < count; ++i)
                {
                    double n;
                    is >> n;
                    numConsts.push_back(n);
                    is.ignore();
                }
            }
            else if (line == "libfunctions")
            {
                int count;
                is >> count;
                is.ignore();
                for (int i = 0; i < count; ++i)
                {
                    getline(is, line);
                    libFuncs.push_back(line);
                }
            }
            else if (line == "userfunctions")
            {
                int count;
                is >> count;
                is.ignore();
                for (int i = 0; i < count; ++i)
                {
                    getline(is, line);
                    istringstream iss(line);
                    unsigned address, localSize, numParams;
                    string name;
                    iss >> address >> localSize >> numParams >> name;
                    userFuncs.push_back({name, address, localSize, numParams});
                }
            }
            else if (line == "target_code")
            {
                int count;
                is >> count;
                is.ignore();
                for (int i = 0; i < count; ++i)
                {
                    getline(is, line);
                    istringstream iss(line);
                    Instruction instr;
                    int opcode, resType, resVal, arg1Type, arg1Val, arg2Type, arg2Val, lineNum;
                    iss >> opcode >> resType >> resVal >> arg1Type >> arg1Val >> arg2Type >> arg2Val >> lineNum;
                    instr.opcode = static_cast<Opcode>(opcode);
                    instr.result = {static_cast<OperandType>(resType), static_cast<unsigned>(resVal)};
                    instr.arg1 = {static_cast<OperandType>(arg1Type), static_cast<unsigned>(arg1Val)};
                    instr.arg2 = {static_cast<OperandType>(arg2Type), static_cast<unsigned>(arg2Val)};
                    instr.line = lineNum;
                    instructions.push_back(instr);
                }
            }
        }
    }

    // Execute the instructions
    void execute()
    {
        pc = 0;
        while (pc < instructions.size())
        {
            const Instruction &instr = instructions[pc];
            pc++;
            switch (instr.opcode)
            {
            case ASSIGN:
            {
                Value val = getValue(instr.arg1);
                // Check if this assignment follows a tablegetelem and targets the same temporary
                if (isLastOperationTableGetElem &&
                    instr.result.type == LOCAL_A &&
                    instr.result.val == lastTableGetElemTempIndex)
                {
                    // Perform tablesetelem instead of assignment
                    if (!holds_alternative<shared_ptr<Table>>(lastTableGetElemTable))
                    {
                        throw runtime_error("Last tablegetelem table is not a table");
                    }
                    auto table = get<shared_ptr<Table>>(lastTableGetElemTable);
                    table->data[lastTableGetElemKey] = val;
                    isLastOperationTableGetElem = false; // Reset the flag
                }
                else
                {
                    setValue(instr.result, val); // Regular assignment
                }
                break;
            }
            case ADD:
            {
                Value a = getValue(instr.arg1);
                Value b = getValue(instr.arg2);
                if (holds_alternative<double>(a) && holds_alternative<double>(b))
                {
                    setValue(instr.result, get<double>(a) + get<double>(b));
                }
                else
                {
                    throw runtime_error("ADD requires numeric operands");
                }
                break;
            }
            case SUB:
            {
                Value a = getValue(instr.arg1);
                Value b = getValue(instr.arg2);
                if (holds_alternative<double>(a) && holds_alternative<double>(b))
                {
                    setValue(instr.result, get<double>(a) - get<double>(b));
                }
                else
                {
                    throw runtime_error("SUB requires numeric operands");
                }
                break;
            }
            case MUL:
            {
                Value a = getValue(instr.arg1);
                Value b = getValue(instr.arg2);
                if (holds_alternative<double>(a) && holds_alternative<double>(b))
                {
                    setValue(instr.result, get<double>(a) * get<double>(b));
                }
                else
                {
                    throw runtime_error("MUL requires numeric operands");
                }
                break;
            }
            case DIV:
            {
                Value a = getValue(instr.arg1);
                Value b = getValue(instr.arg2);
                if (holds_alternative<double>(a) && holds_alternative<double>(b))
                {
                    double divisor = get<double>(b);
                    if (divisor == 0)
                        throw runtime_error("Division by zero");
                    setValue(instr.result, get<double>(a) / divisor);
                }
                else
                {
                    throw runtime_error("DIV requires numeric operands");
                }
                break;
            }
            case MOD:
            {
                Value a = getValue(instr.arg1);
                Value b = getValue(instr.arg2);
                if (holds_alternative<double>(a) && holds_alternative<double>(b))
                {
                    double divisor = get<double>(b);
                    if (divisor == 0)
                        throw runtime_error("Modulo by zero");
                    setValue(instr.result, fmod(get<double>(a), divisor));
                }
                else
                {
                    throw runtime_error("MOD requires numeric operands");
                }
                break;
            }
            case JUMP:
            {
                if (instr.result.type == LABEL_A)
                {
                    pc = instr.result.val;
                }
                else
                {
                    throw runtime_error("JUMP requires LABEL_A operand");
                }
                break;
            }
            case IF_EQ:
            {
                Value a = getValue(instr.arg1);
                Value b = getValue(instr.arg2);
                bool jump = false;
                if (a.index() == b.index())
                {
                    if (holds_alternative<double>(a))
                        jump = get<double>(a) == get<double>(b);
                    else if (holds_alternative<string>(a))
                        jump = get<string>(a) == get<string>(b);
                    else if (holds_alternative<bool>(a))
                        jump = get<bool>(a) == get<bool>(b);
                    else
                        jump = true; // Both nil
                }
                if (jump && instr.result.type == LABEL_A)
                {
                    pc = instr.result.val;
                }
                break;
            }
            case IF_NEQ:
            {
                Value a = getValue(instr.arg1);
                Value b = getValue(instr.arg2);
                bool jump = true;
                if (a.index() == b.index())
                {
                    if (holds_alternative<double>(a))
                        jump = get<double>(a) != get<double>(b);
                    else if (holds_alternative<string>(a))
                        jump = get<string>(a) != get<string>(b);
                    else if (holds_alternative<bool>(a))
                        jump = get<bool>(a) != get<bool>(b);
                    else
                        jump = false; // Both nil
                }
                if (jump && instr.result.type == LABEL_A)
                {
                    pc = instr.result.val;
                }
                break;
            }
            case IF_LESS:
            {
                Value a = getValue(instr.arg1);
                Value b = getValue(instr.arg2);
                if (holds_alternative<double>(a) && holds_alternative<double>(b) &&
                    instr.result.type == LABEL_A && get<double>(a) < get<double>(b))
                {
                    pc = instr.result.val;
                }
                break;
            }
            case IF_LESSEQ:
            {
                Value a = getValue(instr.arg1);
                Value b = getValue(instr.arg2);
                if (holds_alternative<double>(a) && holds_alternative<double>(b) &&
                    instr.result.type == LABEL_A && get<double>(a) <= get<double>(b))
                {
                    pc = instr.result.val;
                }
                break;
            }
            case IF_GREATER:
            {
                Value a = getValue(instr.arg1);
                Value b = getValue(instr.arg2);
                if (holds_alternative<double>(a) && holds_alternative<double>(b) &&
                    instr.result.type == LABEL_A && get<double>(a) > get<double>(b))
                {
                    pc = instr.result.val;
                }
                break;
            }
            case IF_GREATEREQ:
            {
                Value a = getValue(instr.arg1);
                Value b = getValue(instr.arg2);
                if (holds_alternative<double>(a) && holds_alternative<double>(b) &&
                    instr.result.type == LABEL_A && get<double>(a) >= get<double>(b))
                {
                    pc = instr.result.val;
                }
                break;
            }
            case PARAM:
            {
                Value val = getValue(instr.arg1);
                stack.push_back(val);
                break;
            }
            case CALL:
            {
                if (instr.arg1.type == USERFUNC_A)
                {
                    unsigned funcIndex = instr.arg1.val;
                    if (funcIndex >= userFuncs.size())
                    {
                        throw runtime_error("User function index out of bounds");
                    }
                    UserFunc &func = userFuncs[funcIndex];
                    callStack.push_back(pc);
                    locals.resize(func.localSize, monostate());

                    // Assign parameters from stack to local variables
                    if (func.numParams > 0)
                    {
                        if (stack.size() < func.numParams)
                        {
                            throw runtime_error("Not enough parameters for function " + func.name);
                        }
                        for (unsigned i = 0; i < func.numParams; ++i)
                        {
                            Value param = stack.back();
                            stack.pop_back();
                            locals[i] = param;
                        }
                    }

                    pc = func.address;
                }
                else if (instr.arg1.type == LIBFUNC_A)
                {
                    // ... existing code remains unchanged ...
                    if (instr.arg1.val >= libFuncs.size())
                    {
                        throw runtime_error("Library function index out of bounds");
                    }
                    string funcName = libFuncs[instr.arg1.val];
                    if (funcName == "print")
                    {
                        if (stack.empty())
                            throw runtime_error("Stack empty for print");
                        Value arg = stack.back();
                        stack.pop_back();
                        if (holds_alternative<string>(arg))
                        {
                            string str = get<string>(arg);
                            string processed = processString(str);
                            cout << processed;
                        }
                        else if (holds_alternative<double>(arg))
                            cout << get<double>(arg);
                        else if (holds_alternative<bool>(arg))
                            cout << (get<bool>(arg) ? "true" : "false");
                        else
                            cout << "nil";
                        cout << endl;
                        returnValue = monostate();
                    }
                    else
                    {
                        throw runtime_error("Unsupported library function: " + funcName);
                    }
                }
                break;
            }
            case RET:
            {
                if (callStack.empty())
                    throw runtime_error("Return outside function");
                pc = callStack.back();
                callStack.pop_back();
                if (instr.arg1.type != NIL_A)
                {
                    returnValue = getValue(instr.arg1); // Set returnValue to locals[2]
                }
                else
                {
                    returnValue = monostate(); // Set to nil if no return value specified
                }
                locals.clear();
                break;
            }
            case GETRETVAL:
            {
                setValue(instr.result, returnValue);
                break;
            }
            case FUNCSTART:
            {
                // No-op in this VM; handled by CALL
                break;
            }
            case FUNCEND:
            {
                // No-op in this VM; handled by RET
                break;
            }
            case TABLECREATE:
            {
                auto table = std::make_shared<Table>();
                setValue(instr.result, table);
                break;
            }
            case TABLEGETELEM:
            {
                Value table_val = getValue(instr.arg1);
                if (!std::holds_alternative<std::shared_ptr<Table>>(table_val))
                {
                    throw std::runtime_error("TABLEGETELEM: arg1 is not a table");
                }
                auto table = std::get<std::shared_ptr<Table>>(table_val);
                Value key = getValue(instr.arg2);
                auto it = table->data.find(key);
                if (it != table->data.end())
                {
                    setValue(instr.result, it->second);
                }
                else
                {
                    setValue(instr.result, std::monostate()); // Return nil if key not found
                }
                // Store context for potential table set operation
                lastTableGetElemTable = table_val;
                lastTableGetElemKey = key;
                lastTableGetElemTempIndex = instr.result.val; // Store the temporary index
                isLastOperationTableGetElem = true;
                break;
            }
            case TABLESETELEM:
            {
                Value table_val = getValue(instr.result);
                if (!std::holds_alternative<std::shared_ptr<Table>>(table_val))
                {
                    throw std::runtime_error("TABLESETELEM: result is not a table");
                }
                auto table = std::get<std::shared_ptr<Table>>(table_val);
                Value key = getValue(instr.arg1);
                Value val = getValue(instr.arg2);
                table->data[key] = val;
                break;
            }
            case NOT:
            {
                Value a = getValue(instr.arg1);
                if (holds_alternative<bool>(a))
                {
                    setValue(instr.result, !get<bool>(a));
                }
                else
                {
                    throw runtime_error("NOT requires boolean operand");
                }
                break;
            }
            case UMINUS:
            {
                Value a = getValue(instr.arg1);
                if (holds_alternative<double>(a))
                {
                    setValue(instr.result, -get<double>(a));
                }
                else
                {
                    throw runtime_error("UMINUS requires numeric operand");
                }
                break;
            }
            case AND:
            {
                Value a = getValue(instr.arg1);
                Value b = getValue(instr.arg2);
                if (holds_alternative<bool>(a) && holds_alternative<bool>(b))
                {
                    setValue(instr.result, get<bool>(a) && get<bool>(b));
                }
                else
                {
                    throw runtime_error("AND requires boolean operands");
                }
                break;
            }
            case OR:
            {
                Value a = getValue(instr.arg1);
                Value b = getValue(instr.arg2);
                if (holds_alternative<bool>(a) && holds_alternative<bool>(b))
                {
                    setValue(instr.result, get<bool>(a) || get<bool>(b));
                }
                else
                {
                    throw runtime_error("OR requires boolean operands");
                }
                break;
            }
            case PUSHARG:
            {
                Value val = getValue(instr.arg1);
                stack.push_back(val);
                break;
            }
            case FUNCEXIT:
            {
                if (callStack.empty())
                    throw runtime_error("FUNCEXIT outside function");
                pc = callStack.back();
                callStack.pop_back();
                locals.clear();
                break;
            }
            case NOP:
            {
                break;
            }
            default:
                throw runtime_error("Unsupported opcode");
            }
        }
    }

public:
    VM(istream &is) : pc(0), returnValue(monostate())
    {
        loadProgram(is);
    }

    void run()
    {
        execute();
    }
};



int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <input>" << std::endl;
        return 1;
    }
    std::ifstream file(argv[1]);
    if (!file.is_open())
    {
        std::cerr << "Cannot open file: " << argv[1] << std::endl;
        return 1;
    }
    VM vm(file);
    vm.run();
    return 0;
}