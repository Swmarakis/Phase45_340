#ifndef SYMBOL_TABLE_HPP
#define SYMBOL_TABLE_HPP

#include <string>
#include <vector>
#include <iostream>

using namespace std;

enum class SymbolType {
    GLOBAL_VARIABLE,
    LOCAL_VARIABLE,
    FORMAL_ARGUMENT,
    USER_FUNCTION,
    LIBRARY_FUNCTION
};

struct SymbolEntry {
    string name;
    SymbolType type;
    int line;
    int scope;
    int body_scope = -1;
    int offset;
    SymbolEntry(const string &n, SymbolType t, int l, int s)
        : name(n), type(t), line(l), scope(s) {}
};

class SymbolTable {
private:
    vector<vector<SymbolEntry>> scopes;
    int current_scope;

public:
    SymbolTable();
    void increase_scope();
    void decrease_scope();
    void insert(const string &name, SymbolType type, int line);
    bool is_library_function(const string &name);
    int get_current_scope() const { return current_scope; }
    void print() const;
    // Non-const version for parser (allows modification)
    SymbolEntry *lookup(const string &name);
    // Const version for TargetCodeGenerator (read-only)
    const SymbolEntry *lookup(const string &name) const;
    SymbolEntry *lookup(const string &name, int scope);
    SymbolEntry *lookup_global(const string &name);
    SymbolEntry *lookup_function(const string &name);
    void assignOffsets(int scope);
    // Non-const version
    SymbolEntry *lookup_from_scope(const string &name, int start_scope);
    // Const version
    const SymbolEntry *lookup_from_scope(const string &name, int start_scope) const;
    const vector<SymbolEntry>& getScopeSymbols(int scope) const;
    
    // Public accessors for scope information
    size_t get_num_scopes() const { return scopes.size(); }
    size_t get_scope_size(int scope) const {
        if (scope >= 0 && static_cast<size_t>(scope) < scopes.size()) {
            return scopes[scope].size();
        }
        return 0;
    }
};

extern SymbolTable symbol_table;

#endif