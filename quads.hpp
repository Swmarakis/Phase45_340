#ifndef QUADS_HPP
#define QUADS_HPP

#include <string>
#include <vector>
#include <map>

using namespace std;

struct Quad
{
    string op;           // Operation (e.g., "if_eq", "assign")
    string result;       // Result or destination
    string arg1;         // First argument
    string arg2;         // Second argument
    string target_label; // Symbolic label for jumps (e.g., "L0")
    int line;                 // Source line number
    bool ignored = false;     // Flag to ignore this quad
};

extern vector<Quad> quads;
extern map<string, size_t> label_to_quad;
extern int temp_counter;

string new_temp();
string new_label();
void emit(const string &op, const string &result = "",
          const string &arg1 = "", const string &arg2 = "",
          const string &target_label = "", int line = 1);
vector<size_t> merge(const vector<size_t> &a, const vector<size_t> &b);
void backpatch(const vector<size_t> &list, size_t label);
void print_quads();


//  REMOVE IF NOT USE 
bool is_simple_arithmetic(const string &left, const string &right);

int next_quad();
void patch_quad(int quad_num, string target);

#endif // QUADS_HPP