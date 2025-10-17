#include "symbol_table.hpp"
#include <iomanip>

SymbolTable symbol_table; // Define the global variable

SymbolTable::SymbolTable() : current_scope(0)
{
    scopes.push_back(vector<SymbolEntry>());
    // Add library functions to scope 0
    const char *lib_funcs[] = {"print", "totalarguments", "input", "objectmemberkeys",
                               "sqrt", "cos", "objecttotalmembers", "objectcopy",
                               "argument", "typeof", "sin", "strtonum"};
    for (const char *func : lib_funcs)
    {
        scopes[0].emplace_back(func, SymbolType::LIBRARY_FUNCTION, 0, 0);
    }
}

void SymbolTable::increase_scope()
{
    current_scope++;
    if (scopes.size() <= current_scope)
    {
        scopes.push_back(vector<SymbolEntry>());
    }
}

void SymbolTable::decrease_scope()
{
    current_scope--;
}

void SymbolTable::insert(const string &name, SymbolType type, int line)
{
    // Check for library function collision
    for (const auto &entry : scopes[0])
    {
        if (entry.name == name && entry.type == SymbolType::LIBRARY_FUNCTION)
        {
            cerr << "\033[31mError\033[0m: User function '" << name << "', collision with '"
                 << name << "' library function" << endl;
            return; // Do not insert
        }
    }

    // Check for redefinition in current scope only
    for (const auto &entry : scopes[current_scope])
    {
        if (entry.name == name && entry.type != SymbolType::FORMAL_ARGUMENT)
        {
            if (type == SymbolType::USER_FUNCTION && entry.type == SymbolType::USER_FUNCTION)
            {
                cerr << "\033[31mError\033[0m: Collision with function named '" << name
                     << "' defined at line " << entry.line << endl;
            }
            else
            {
                cerr << "\033[31mError\033[0m: Variable '" << name << "' already defined at line: "
                     << entry.line << endl;
            }
            return; // Do not insert
        }
    }

    // Insert the new symbol
    scopes[current_scope].emplace_back(name, type, line, current_scope);
}

bool SymbolTable::is_library_function(const string &name)
{
    for (const auto &entry : scopes[0])
    {
        if (entry.name == name && entry.type == SymbolType::LIBRARY_FUNCTION)
        {
            return true;
        }
    }
    return false;
}

SymbolEntry *SymbolTable::lookup(const string &name)
{
    for (int s = current_scope; s >= 0; --s)
    {
        for (auto &entry : scopes[s])
        {
            if (entry.name == name)
            {
                return &entry;
            }
        }
    }
    return nullptr;
}

SymbolEntry *SymbolTable::lookup(const string &name, int scope)
{
    if (scope >= 0 && scope < scopes.size())
    {
        for (auto &entry : scopes[scope])
        {
            if (entry.name == name)
            {
                return &entry;
            }
        }
    }
    return nullptr;
}

const SymbolEntry *SymbolTable::lookup(const string &name) const
{
    for (int s = current_scope; s >= 0; --s)
    {
        for (const auto &entry : scopes[s])
        {
            if (entry.name == name)
            {
                return &entry;
            }
        }
    }
    return nullptr;
}

SymbolEntry *SymbolTable::lookup_global(const string &name)
{
    for (auto &entry : scopes[0])
    {
        if (entry.name == name)
        {
            return &entry;
        }
    }
    return nullptr;
}

SymbolEntry *SymbolTable::lookup_function(const string &name)
{
    // Search the current scope (function's local scope)
    for (auto &entry : scopes[current_scope])
    {
        if (entry.name == name)
        {
            return &entry;
        }
    }
    // Search the global scope (scope 0)
    for (auto &entry : scopes[0])
    {
        if (entry.name == name)
        {
            return &entry;
        }
    }
    return nullptr;
}

void SymbolTable::print() const
{
    const int name_width = 20;
    const int type_width = 20;
    const int line_width = 10;
    const int scope_width = 10;

    for (int scope = 0; scope < scopes.size(); ++scope)
    {
        if (scopes[scope].empty())
            continue;
        cout << "\n---------------------     Scope #" << scope << "     ---------------------\n";
        cout << left
             << setw(name_width) << "\"name\""
             << setw(type_width) << "\"type\""
             << setw(line_width) << "\"line\""
             << setw(scope_width) << "\"scope\""
             << endl;
        for (const auto &entry : scopes[scope])
        {
            string type_str;
            switch (entry.type)
            {
            case SymbolType::GLOBAL_VARIABLE:
                type_str = "[global variable]";
                break;
            case SymbolType::LOCAL_VARIABLE:
                type_str = "[local variable]";
                break;
            case SymbolType::FORMAL_ARGUMENT:
                type_str = "[formal argument]";
                break;
            case SymbolType::USER_FUNCTION:
                type_str = "[user function]";
                break;
            case SymbolType::LIBRARY_FUNCTION:
                type_str = "[library function]";
                break;
            }
            cout << left
                 << setw(name_width) << ("\"" + entry.name + "\"")
                 << setw(type_width) << type_str
                 << setw(line_width) << ("(line " + to_string(entry.line) + ")")
                 << setw(scope_width) << ("(scope " + to_string(entry.scope) + ")")
                 << endl;
        }
    }
}

void SymbolTable::assignOffsets(int scope)
{
    if (scope == 0)
    {
        int globalOffset = 0;
        for (auto &entry : scopes[0])
        {
            if (entry.type == SymbolType::GLOBAL_VARIABLE)
            {
                entry.offset = globalOffset++;
            }
        }
    }
    int formalOffset = 0; // Starting offset for formal arguments
    int localOffset = 0;  // Starting offset for locals (after formals)
    for (auto &entry : scopes[scope])
    {
        if (entry.type == SymbolType::FORMAL_ARGUMENT)
        {
            entry.offset = formalOffset++;
        }
        else if (entry.type == SymbolType::LOCAL_VARIABLE)
        {
            entry.offset = formalOffset + localOffset++;
        }
    }
}

SymbolEntry *SymbolTable::lookup_from_scope(const string &name, int start_scope)
{
    for (int s = start_scope; s >= 0; --s)
    {
        for (auto &entry : scopes[s])
        {
            if (entry.name == name)
            {
                return &entry;
            }
        }
    }
    return nullptr;
}

const SymbolEntry *SymbolTable::lookup_from_scope(const string &name, int start_scope) const
{
    for (int s = start_scope; s >= 0; --s)
    {
        
        for (const auto &entry : scopes[s])
        {
            if (entry.name == name)
            {
               
                return &entry;
            }
        }
    }
    
    return nullptr;
}



const vector<SymbolEntry>& SymbolTable::getScopeSymbols(int scope) const {
    if (scope < 0 || scope >= static_cast<int>(scopes.size())) {
        throw out_of_range("Scope index out of range");
    }
    return scopes[scope];
}