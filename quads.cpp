#include "quads.hpp"
#include <iostream>
#include <sstream>
#include <set>
#include <algorithm>
#include <unordered_map>
#include <iomanip>

vector<Quad> quads;
map<string, size_t> label_to_quad;
vector<string> loop_incr_labels;
int temp_counter = 0;
int label_counter = 0;

// Define arithmetic operations
static const set<string> arithmetic_ops = {"add", "sub", "mul", "div", "mod", "uminus", "not"};

string new_temp()
{
    // Always return ^0 if we're at the top level statement
    if (temp_counter == 0)
    {
        temp_counter++;
        return "^0";
    }
    return "^" + to_string(temp_counter++);
}

string new_label()
{
    return "L" + to_string(label_counter++);
}

void emit(const string &op, const string &result,
          const string &arg1, const string &arg2,
          const string &target_label, int line)
{
    if (op == "label")
    {
        label_to_quad[target_label] = quads.size();
    }
    Quad q = {op, result, arg1, arg2, target_label, line};
    quads.push_back(q);
}

vector<size_t> merge(const vector<size_t> &a, const vector<size_t> &b)
{
    vector<size_t> res = a;
    res.insert(res.end(), b.begin(), b.end());
    return res;
}

void backpatch(const vector<size_t> &list, size_t label)
{
    for (size_t idx : list)
    {
        quads[idx].target_label = to_string(label);
    }
}

void print_quads() {
    vector<int> printed_numbers(quads.size(), 0);
    int counter = 1;
    for (size_t i = 0; i < quads.size(); ++i) {
        if (quads[i].op != "label") {
            printed_numbers[i] = counter++;
        }
    }

    unordered_map<string, int> label_to_printed;
    for (size_t i = 0; i < quads.size(); ++i) {
        if (quads[i].op == "label") {
            bool found = false;
            for (size_t j = i + 1; j < quads.size(); ++j) {
                if (printed_numbers[j] != 0) {
                    label_to_printed[quads[i].target_label] = printed_numbers[j];
                    found = true;
                    break;
                }
            }
            if (!found) {
                label_to_printed[quads[i].target_label] = counter;
            }
        }
    }

    // Set column widths
    const int num_width = 4;    // Quad number (e.g., "1:")
    const int op_width = 10;    // Operation (e.g., "assign")
    const int result_width = 5; // Result (e.g., "x")
    const int arg1_width = 7;   // Arg1 (e.g., "42")
    const int arg2_width = 7;   // Arg2 (e.g., "'true'")
    const int target_width = 5; // Target label (e.g., "5")
    const int line_width = 5;   // Line info (e.g., "[line 1]")

    // Print header with labels
    cout << left
              << setw(num_width) << "Num"
              << setw(op_width) << "Op"
              << setw(result_width) << "Res"
              << setw(arg1_width) << "Arg1"
              << setw(arg2_width) << "Arg2"
              << setw(target_width) << "Tgt"
              << setw(line_width) << "Line"
              << endl;

    // Print quads
    for (size_t i = 0; i < quads.size(); ++i) {
        if (quads[i].op != "label") {
            const Quad& q = quads[i];
            cout << left
                      << setw(num_width) << (to_string(printed_numbers[i]) + ":")
                      << setw(op_width) << q.op;
            
            if (q.op == "jump") {
                // Only target
                cout << setw(result_width) << ""
                          << setw(arg1_width) << ""
                          << setw(arg2_width) << "";
            } else if (q.op == "if_eq") {
                // arg1, arg2, target
                cout << setw(result_width) << ""
                          << setw(arg1_width) << q.arg1
                          << setw(arg2_width) << q.arg2;
            } else {
                // result, arg1, arg2
                cout << setw(result_width) << q.result
                          << setw(arg1_width) << q.arg1
                          << setw(arg2_width) << (q.arg2.empty() ? "" : q.arg2);
            }

            if (!q.target_label.empty()) {
                auto it = label_to_printed.find(q.target_label);
                string target = (it != label_to_printed.end()) ? to_string(it->second) : "UNDEFINED_LABEL";
                cout << setw(target_width) << target;
            } else {
                cout << setw(target_width) << "";
            }

            cout << setw(line_width) << ("[line " + to_string(q.line) + "]");
            if (q.ignored) {
                cout << " IGNORE QUAD";
            }
            cout << endl;
        }
    }
}

//  REMOVE IF NOT USE 
bool is_simple_arithmetic(const string &left, const string &right)
{
    // Check if both operands are numeric constants (not variables or temps)
    auto is_numeric = [](const string &s)
    {
        // Check if string is a numeric constant (e.g., "4", "5.2")
        if (s.empty())
            return false;
        return all_of(s.begin(), s.end(), [](char c)
                           { return isdigit(c) || c == '.'; });
    };

    return is_numeric(left) && is_numeric(right);
}



