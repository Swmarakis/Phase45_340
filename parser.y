%{
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <utility>  
#include "al.hpp"
#include "symbol_table.hpp"
#include "quads.hpp"

extern int alpha_yylex(void*);
extern int yylineno;
extern char* yytext;
extern FILE* yyin;

int yyerror(const char* message);

// Wrapper for yylex to call alpha_yylex
int yylex(void* yylval) {
    return alpha_yylex(yylval);
}

// Scope and nesting counters
int anon_func_counter = 0;
unsigned int current_line = 1;
int function_nesting = 0;  
int loop_nesting = 0;     

// For collecting quads in for loops
bool collecting_quads = false;
vector<Quad> collected_quads;

// flag that shows is top level expr for temp values
bool in_top_level_expr = false;

// Label stacks for break/continue
vector<string> loop_exit_labels;
vector<string> loop_start_labels;

// Function end labels for return statements
vector<string> funcend_labels;

// Quad emission wrapper
void add_quad(const string& op, const string& result = "", 
              const string& arg1 = "", const string& arg2 = "", 
              const string& target_label = "") {
           
    if (collecting_quads) {
        Quad q = {op, result, arg1, arg2, target_label, yylineno};
        collected_quads.push_back(q);
    } else {
        emit(op, result, arg1, arg2, target_label, yylineno);
    }
}

void emit_label(const string& label) {
    add_quad("label", "", "", "", label);
}

%}

%define api.pure full
%define parse.trace
%expect 1

%union {
    alpha_token_t* token;    // Token from lexer
    Node* node;              // For lvalue and member nodes
    string* strValue;   // For expressions and constants
    pair<string, string>* labelPair; // For pairs of labels
    vector<string>* paramList; // For expression lists
    vector<pair<string, string>>* pairList; //For indexed and indexedelem
}

%token <token> IF ELSE WHILE FOR FUNCTION RETURN BREAK CONTINUE AND NOT OR LOCAL TRUE FALSE NIL
%token <token> ASSIGN PLUS MINUS MUL DIV MOD EQUALS NOT_EQUALS INCREMENT DECREMENT GREATER LESS
%token <token> GREATER_EQUAL LESS_EQUAL LEFT_BRACE RIGHT_BRACE LEFT_BRACKET RIGHT_BRACKET
%token <token> LEFT_PAREN RIGHT_PAREN SEMICOLON COMMA COLON COLONCOLON DOT DOTDOT
%token <token> INTCONST REALCONST STRINGCONST IDENT COMMENT_LINE COMMENT_BLOCK COMMENT_NESTED
%token <token> ESCAPE_NEWLINE ESCAPE_TAB UNDEFINED

%type <node> lvalue member
%type <strValue> expr assignexpr term primary const call objectdef funcprefix funcdef for_elist
%type <labelPair> func_labels indexedelem
%type <paramList> elist forstmt
%type <pairList> indexed 

%right ASSIGN
%left OR
%left AND
%nonassoc EQUALS NOT_EQUALS
%nonassoc GREATER GREATER_EQUAL LESS LESS_EQUAL
%nonassoc THEN
%nonassoc ELSE
%left PLUS MINUS
%left MUL DIV MOD
%right NOT INCREMENT DECREMENT UMINUS
%left DOT DOTDOT
%left LEFT_BRACKET RIGHT_BRACKET
%left LEFT_PAREN RIGHT_PAREN

%start program

%%

program : stmt_list
        ;

stmt_list : stmt stmt_list
          | /* empty */
          ;

stmt : expr SEMICOLON { 
        delete $1; //free expr
        temp_counter = 0;
        delete $2; 
    }
     | ifstmt
     | whilestmt
     | forstmt
     | returnstmt
     | BREAK SEMICOLON { 
         if (loop_nesting == 0) { 
             yyerror("break statement outside of loop"); 
         } else {
             add_quad("jump", "", "", "", loop_exit_labels.back());
         }
         delete $1; delete $2; 
     }
     | CONTINUE SEMICOLON { 
         if (loop_nesting == 0) { 
             yyerror("continue statement outside of loop"); 
         } else {
             add_quad("jump", "", "", "", loop_start_labels.back());
         }
         delete $1; delete $2; 
     }
     | block
     | funcdef
     | SEMICOLON { delete $1; }
     | COMMENT_LINE { delete $1; }
     | COMMENT_BLOCK { delete $1; }
     | COMMENT_NESTED { delete $1; }
     | UNDEFINED { 
         cerr << "\033[31mError\033[0m : Undefined token at line " << $1->line_number << endl; 
         delete $1; 
     }
     ;

expr : assignexpr { $$ = $1; }
     | expr PLUS expr { 
         string temp = new_temp();
         add_quad("add", temp, *$1, *$3, "");
         $$ = new string(temp);
         delete $1; delete $2; delete $3;
     }
     | expr MINUS expr { 
         string temp = new_temp();
         add_quad("sub", temp, *$1, *$3, "");
         $$ = new string(temp);
         delete $1; delete $2; delete $3;
     }
     | expr MUL expr { 
         string temp = new_temp();
         add_quad("mul", temp, *$1, *$3, "");
         $$ = new string(temp);
         delete $1; delete $2; delete $3;
     }
     | expr DIV expr { 
         string temp = new_temp();
         add_quad("div", temp, *$1, *$3, "");
         $$ = new string(temp);
         delete $1; delete $2; delete $3;
     }
     | expr MOD expr { 
         string temp = new_temp();
         add_quad("mod", temp, *$1, *$3, "");
         $$ = new string(temp);
         delete $1; delete $2; delete $3;
     }
    | expr GREATER expr { 
       string temp = new_temp();
       string label_true = new_label();
       string label_false = new_label();
       string label_after = new_label();
       
       add_quad("if_greater", "", *$1, *$3, label_true);
       add_quad("jump", "", "", "", label_false);
       add_quad("label", "", "", "", label_true);
       add_quad("assign", temp, "'true'", "");
       add_quad("jump", "", "", "", label_after);
       add_quad("label", "", "", "", label_false);
       add_quad("assign", temp, "'false'", "");
       add_quad("label", "", "", "", label_after);
       
       $$ = new string(temp);
       delete $1; delete $2; delete $3;
   }
   | expr GREATER_EQUAL expr { 
       string temp = new_temp();
       string label_true = new_label();
       string label_false = new_label();
       string label_after = new_label();
       
       add_quad("if_greatereq", "", *$1, *$3, label_true);
       add_quad("jump", "", "", "", label_false);
       add_quad("label", "", "", "", label_true);
       add_quad("assign", temp, "'true'", "");
       add_quad("jump", "", "", "", label_after);
       add_quad("label", "", "", "", label_false);
       add_quad("assign", temp, "'false'", "");
       add_quad("label", "", "", "", label_after);
       
       $$ = new string(temp);
       delete $1; delete $2; delete $3;
   }
   | expr LESS expr { 
       string temp = new_temp();
       string label_true = new_label();
       string label_false = new_label();
       string label_after = new_label();
       
       add_quad("if_less", "", *$1, *$3, label_true);
       add_quad("jump", "", "", "", label_false);
       add_quad("label", "", "", "", label_true);
       add_quad("assign", temp, "'true'", "");
       add_quad("jump", "", "", "", label_after);
       add_quad("label", "", "", "", label_false);
       add_quad("assign", temp, "'false'", "");
       add_quad("label", "", "", "", label_after);
       
       $$ = new string(temp);
       delete $1; delete $2; delete $3;
   }
   | expr LESS_EQUAL expr { 
       string temp = new_temp();
       string label_true = new_label();
       string label_false = new_label();
       string label_after = new_label();
       
       add_quad("if_lesseq", "", *$1, *$3, label_true);
       add_quad("jump", "", "", "", label_false);
       add_quad("label", "", "", "", label_true);
       add_quad("assign", temp, "'true'", "");
       add_quad("jump", "", "", "", label_after);
       add_quad("label", "", "", "", label_false);
       add_quad("assign", temp, "'false'", "");
       add_quad("label", "", "", "", label_after);
       
       $$ = new string(temp);
       delete $1; delete $2; delete $3;
   }
   | expr EQUALS expr { 
       string temp = new_temp();
       string label_true = new_label();
       string label_false = new_label();
       string label_after = new_label();
       
       add_quad("if_eq", "", *$1, *$3, label_true);
       add_quad("jump", "", "", "", label_false);
       add_quad("label", "", "", "", label_true);
       add_quad("assign", temp, "'true'", "");
       add_quad("jump", "", "", "", label_after);
       add_quad("label", "", "", "", label_false);
       add_quad("assign", temp, "'false'", "");
       add_quad("label", "", "", "", label_after);
       
       $$ = new string(temp);
       delete $1; delete $2; delete $3;
   }
   | expr NOT_EQUALS expr { 
       string temp = new_temp();
       string label_true = new_label();
       string label_false = new_label();
       string label_after = new_label();
       
       add_quad("if_noteq", "", *$1, *$3, label_true);
       add_quad("jump", "", "", "", label_false);
       add_quad("label", "", "", "", label_true);
       add_quad("assign", temp, "'true'", "");
       add_quad("jump", "", "", "", label_after);
       add_quad("label", "", "", "", label_false);
       add_quad("assign", temp, "'false'", "");
       add_quad("label", "", "", "", label_after);
       
       $$ = new string(temp);
       delete $1; delete $2; delete $3;
   }
| expr AND expr { 
    string temp = new_temp();
    string label_true1 = new_label();
    string label_true2 = new_label();
    string label_false = new_label();
    string label_end = new_label();
    
    // Check if operands are literals (start with a quote)
    bool is_literal1 = (*$1)[0] == '\'';
    bool is_literal2 = (*$3)[0] == '\'';
    bool both_literals = is_literal1 && is_literal2;
    bool expr1_value = both_literals ? (*$1 == "'true'") : false;
    bool expr2_value = both_literals ? (*$3 == "'true'") : false;
    bool and_result = both_literals ? (expr1_value && expr2_value) : false;
    
    add_quad("if_eq", "", *$1, "'true'", label_true1);
    add_quad("jump", "", "", "", label_false);
    add_quad("label", "", "", "", label_true1);
    add_quad("if_eq", "", *$3, "'true'", label_true2);
    add_quad("jump", "", "", "", label_false);
    add_quad("label", "", "", "", label_true2);
    
    // True branch quads
    size_t true_assign_quad = quads.size();
    add_quad("assign", temp, "'true'", "");
    size_t true_jump_quad = quads.size();
    add_quad("jump", "", "", "", label_end);
    
    add_quad("label", "", "", "", label_false);
    add_quad("assign", temp, "'false'", "");
    add_quad("label", "", "", "", label_end);
    
    // Mark true branch quads as ignored if result is false
    if (both_literals && !and_result) {
        quads[true_assign_quad].ignored = true;
        quads[true_jump_quad].ignored = true;
    }
    
    $$ = new string(temp);
    delete $1; delete $2; delete $3;
}
| expr OR expr { 
    string temp = new_temp();          // Create a temporary variable
    string label_true = new_label();   // Label for true result
    string label_check_second = new_label(); // Label to check second operand
    string label_false = new_label();  // Label for false result
    string label_end = new_label();    // Label for end of expression

    // Check if operands are literals (start with a quote)
    bool is_literal1 = (*$1)[0] == '\'';
    bool is_literal2 = (*$3)[0] == '\'';
    bool both_literals = is_literal1 && is_literal2;

    // Emit quads for the OR operation
    add_quad("if_eq", "", *$1, "'true'", label_true);  // If first operand is true, jump to true branch
    add_quad("jump", "", "", "", label_check_second);  // Else, check second operand
    add_quad("label", "", "", "", label_check_second); // Label to check second operand
    add_quad("if_eq", "", *$3, "'true'", label_true);  // If second operand is true, jump to true branch
    add_quad("jump", "", "", "", label_false);         // Else, jump to false branch
    add_quad("label", "", "", "", label_true);         // Label for true result
    add_quad("assign", temp, "'true'", "");            // Assign true to temp
    add_quad("jump", "", "", "", label_end);           // Jump to end
    add_quad("label", "", "", "", label_false);        // Label for false result
    add_quad("assign", temp, "'false'", "");           // Assign false to temp (may be ignored)
    add_quad("label", "", "", "", label_end);          // Label for end
    
    // Mark false branch as ignored if both operands are literals and result is true
    if (both_literals) {
        bool val1 = (*$1 == "'true'");  // Evaluate first operand
        bool val2 = (*$3 == "'true'");  // Evaluate second operand
        if (val1 || val2) {             // If result is true
            size_t false_assign_idx = quads.size() - 2;  // Index of "assign temp 'false'"
            quads[false_assign_idx].ignored = true;      // Mark false assign as ignored
        }
    }
    
    $$ = new string(temp);  // Set result to temp variable
    delete $1; delete $2; delete $3;  // Clean up memory
}
     | term { $$ = $1; }
     ;

term : LEFT_PAREN expr RIGHT_PAREN { $$ = $2; delete $1; delete $3; }
     | MINUS expr %prec UMINUS { 
         string temp = new_temp();
         add_quad("uminus", temp, *$2, "", "");
         $$ = new string(temp);
         delete $2;
     }
    | NOT expr {
            string temp = new_temp();
            string label_false = new_label();
            string label_end = new_label();

            // Quad 1: Check if expr == 'true', jump to false case
            add_quad("if_eq", "", *$2, "'true'", label_false);
            // Quad 2: Jump to quad 3 (though numbered, we use labels)
            add_quad("jump", "", "", "", "L_next");  // Temporary label, resolved later
            // Quad 3: Assign 'true' to temp (if expr != 'true')
            add_quad("label", "", "", "", "L_next");
            add_quad("assign", temp, "'true'", "");
            // Quad 4: Jump to end
            add_quad("jump", "", "", "", label_end);
            // Quad 5: False case, assign 'false' to temp
            add_quad("label", "", "", "", label_false);
            add_quad("assign", temp, "'false'", "");
            // Quad 6: End label (assignment to x follows in assignexpr)
            add_quad("label", "", "", "", label_end);

            $$ = new string(temp);
            delete $2;
    }
    /* Pre-increment: ++x */
    | INCREMENT lvalue { 
        SymbolEntry* entry = symbol_table.lookup($2->content);
            if (entry && (entry->type == SymbolType::USER_FUNCTION || 
                  entry->type == SymbolType::LIBRARY_FUNCTION)) {
                cerr << "\033[31mError\033[0m: Using " 
                  << (entry->type == SymbolType::USER_FUNCTION ? "ProgramFunc" : "LibFunc") 
                  << " as an lvalue" << endl;
                YYERROR;
            }
        string temp = new_temp();  // e.g., "^0"
        add_quad("add", $2->content, $2->content, "1", "");
        add_quad("assign", temp, $2->content, "");
        $$ = new string(temp);
        delete $1;
        delete $2;
    }
    /* Post-increment: x++ */
    | lvalue INCREMENT { 
        SymbolEntry* entry = symbol_table.lookup($1->content);
        if (entry && (entry->type == SymbolType::USER_FUNCTION || 
                  entry->type == SymbolType::LIBRARY_FUNCTION)) {
            cerr << "\033[31mError\033[0m: Using " 
                  << (entry->type == SymbolType::USER_FUNCTION ? "ProgramFunc" : "LibFunc") 
                  << " as an lvalue" << endl;
                YYERROR;
        }
        string temp = new_temp();  // e.g., "^0"
        add_quad("assign", temp, $1->content, "");
        add_quad("add", $1->content, $1->content, "1", "");
        $$ = new string(temp);
        delete $2;
        delete $1;
    }
    /* Pre-decrement: --x */
    | DECREMENT lvalue { 
        SymbolEntry* entry = symbol_table.lookup($2->content);
        if (entry && (entry->type == SymbolType::USER_FUNCTION || 
                      entry->type == SymbolType::LIBRARY_FUNCTION)) {
            cerr << "\033[31mError\033[0m: Using " 
                      << (entry->type == SymbolType::USER_FUNCTION ? "ProgramFunc" : "LibFunc") 
                      << " as an lvalue" << endl;
            YYERROR;
        }
        string temp = new_temp();  // e.g., "^0"
        add_quad("sub", $2->content, $2->content, "1", "");
        add_quad("assign", temp, $2->content, "");
        $$ = new string(temp);
        delete $1;
        delete $2;
    }
    /* Post-decrement: x-- */
    | lvalue DECREMENT { 
        SymbolEntry* entry = symbol_table.lookup($1->content);
        if (entry && (entry->type == SymbolType::USER_FUNCTION || 
                      entry->type == SymbolType::LIBRARY_FUNCTION)) {
            cerr << "\033[31mError\033[0m: Using " 
                      << (entry->type == SymbolType::USER_FUNCTION ? "ProgramFunc" : "LibFunc") 
                      << " as an lvalue" << endl;
            YYERROR;
        }
        string temp = new_temp();  // e.g., "^0"
        add_quad("assign", temp, $1->content, "");
        add_quad("sub", $1->content, $1->content, "1", "");
        $$ = new string(temp);
        delete $2;
        delete $1;
    }
    | primary { $$ = $1; }
    ;

assignexpr : lvalue ASSIGN expr { 
               SymbolEntry* entry = symbol_table.lookup($1->content);
               if (entry && (entry->type == SymbolType::USER_FUNCTION || 
                             entry->type == SymbolType::LIBRARY_FUNCTION)) {
                   cerr << "\033[31mError\033[0m: Using " 
                             << (entry->type == SymbolType::USER_FUNCTION ? "ProgramFunc" : "LibFunc") 
                             << " as an lvalue" << endl;
                   YYERROR;
               }
               add_quad("assign", $1->content, *$3, "");
               string temp = new_temp();
               add_quad("assign", temp, $1->content, "");
               $$ = new string(temp);
               delete $2; delete $3;
           }
           ;

primary : lvalue { $$ = new string($1->content); }
        | call { $$ = $1; }
        | objectdef { $$ = $1; }
        | const { $$ = $1; }
        ;

lvalue : IDENT { 
           SymbolEntry* entry = symbol_table.lookup($1->content);
           if (!entry) {
               if (function_nesting == 0) {
                   if (symbol_table.get_current_scope() == 0) {
                       symbol_table.insert($1->content, SymbolType::GLOBAL_VARIABLE, yylineno);
                   } else {
                       symbol_table.insert($1->content, SymbolType::LOCAL_VARIABLE, yylineno);
                   }
               } else {
                   symbol_table.insert($1->content, SymbolType::LOCAL_VARIABLE, yylineno);
               }
           }
           $$ = new Node($1->content);
           delete $1;
       }
       | LOCAL IDENT { 
           SymbolEntry* entry = symbol_table.lookup($2->content, symbol_table.get_current_scope());
           if (!entry) {
               if (symbol_table.is_library_function($2->content)) {
                   yyerror("Cannot shadow library function");
               } else {
                   symbol_table.insert($2->content, SymbolType::LOCAL_VARIABLE, yylineno);
               }
           }
           $$ = new Node($2->content);
           delete $1; delete $2;
       }
       | COLONCOLON IDENT { 
           SymbolEntry* entry = symbol_table.lookup_global($2->content);
           if (!entry) {
               string error_msg = "ERROR: No global variable '::" + string($2->content) + "' exists";
               yyerror(error_msg.c_str());
               YYERROR;
           }
           $$ = new Node($2->content);
           delete $1; delete $2;
       }
       | member { $$ = $1; }
       ;

member : lvalue DOT IDENT { 
           string temp = new_temp();
           add_quad("tablegetelem", temp, $1->content, "\"" + string($3->content) + "\"");
           $$ = new Node(temp);
           delete $2; delete $3;
       }
       | lvalue LEFT_BRACKET expr RIGHT_BRACKET { 
           string temp = new_temp();
           add_quad("tablegetelem", temp, $1->content, *$3);
           $$ = new Node(temp);
           delete $2; delete $4; delete $3;
       }
       | call DOT IDENT { 
           string temp = new_temp();
           add_quad("tablegetelem", temp, *$1, "\"" + string($3->content) + "\"");
           $$ = new Node(temp);
           delete $2; delete $3;
       }
       | call LEFT_BRACKET expr RIGHT_BRACKET { 
           string temp = new_temp();
           add_quad("tablegetelem", temp, *$1, *$3);
           $$ = new Node(temp);
           delete $2; delete $4; delete $3;
       }
       ;

call : primary LEFT_PAREN elist RIGHT_PAREN {
           for (auto it = $3->rbegin(); it != $3->rend(); ++it) {
               add_quad("param", "", *it, "");
           }
           add_quad("call", "", *$1, "");
           string temp = new_temp();
           add_quad("getretval", temp, "", "");
           $$ = new string(temp);
           delete $1;
           delete $2;
           delete $3;
           delete $4;
       }
     | lvalue DOTDOT IDENT LEFT_PAREN elist RIGHT_PAREN {
           string method_temp = new_temp();
           add_quad("tablegetelem", method_temp, $1->content, "\"" + string($3->content) + "\"");
           for (auto it = $5->rbegin(); it != $5->rend(); ++it) {
               add_quad("param", "", *it, "");
           }
           add_quad("param", "", $1->content, "");
           add_quad("call", "", method_temp, "");
           string temp = new_temp();
           add_quad("getretval", temp, "", "");
           $$ = new string(temp);
           delete $1;
           delete $2;
           delete $3;
           delete $4;
           delete $5;
           delete $6;
       }
     | LEFT_PAREN funcdef RIGHT_PAREN LEFT_PAREN elist RIGHT_PAREN {
           for (auto it = $5->rbegin(); it != $5->rend(); ++it) {
               add_quad("param", "", *it, "");
           }
           add_quad("call", "", *$2, "");
           string temp = new_temp();
           add_quad("getretval", temp, "", "");
           $$ = new string(temp);
           delete $1;
           delete $2;
           delete $3;
           delete $4;
           delete $5;
           delete $6;
       }
     ;

elist : expr {
          $$ = new vector<string>{*$1};
          delete $1;
      }
      | elist COMMA expr {
          $1->push_back(*$3);
          $$ = $1;
          delete $2;
          delete $3;
      }
      | /* empty */ {
          $$ = new vector<string>();
      };

objectdef : LEFT_BRACKET elist RIGHT_BRACKET { 
              string temp = new_temp();
              add_quad("tablecreate", temp, "", "");
              int index = 0;
              for (const auto& elem : *$2) {
                  add_quad("tablesetelem", temp, to_string(index), elem);
                  index++;
              }
              $$ = new string(temp);
              delete $1;
              delete $2;
              delete $3;
          }
          | LEFT_BRACKET indexed RIGHT_BRACKET { 
              string temp = new_temp();
              add_quad("tablecreate", temp, "", "");
              for (const auto& pair : *$2) {
                  add_quad("tablesetelem", temp, pair.first, pair.second);
              }
              $$ = new string(temp);
              delete $1;
              delete $2;
              delete $3;
          }
          ;

indexed : indexedelem { 
            $$ = new vector<pair<string, string>>();
            $$->push_back(*$1);
            delete $1;
        }
        | indexed COMMA indexedelem { 
            $1->push_back(*$3);
            $$ = $1;
            delete $2;
            delete $3;
        }
        ;

indexedelem : LEFT_BRACE expr COLON expr RIGHT_BRACE { 
                $$ = new pair<string, string>(*$2, *$4);
                delete $1;
                delete $2;
                delete $3;
                delete $4;
                delete $5;
            }
            ;

func_labels : /* empty */ { 
                  string L_after = new_label();
                  add_quad("jump", "", "", "", L_after);
                  string L_end = new_label();
                  funcend_labels.push_back(L_end);
                  $$ = new pair<string, string>(L_after, L_end);
              }
              ;

funcdef : func_labels FUNCTION funcprefix LEFT_PAREN { 
              symbol_table.increase_scope(); 
              SymbolEntry* func_entry = symbol_table.lookup(*$3); 
              if (func_entry && func_entry->type == SymbolType::USER_FUNCTION) {
                  func_entry->body_scope = symbol_table.get_current_scope();
              }
          } idlist RIGHT_PAREN { 
              function_nesting++; 
          } funcblock { 
              symbol_table.assignOffsets(symbol_table.get_current_scope());
              function_nesting--;
              pair<string, string>* labels = $1; // Get labels from func_labels
              add_quad("label", "", "", "", labels->second); // Place L_end label
              add_quad("funcend", *$3, "", ""); // Fix: Use *$3 (function name) in result
              funcend_labels.pop_back();
              symbol_table.decrease_scope();
              add_quad("label", "", "", "", labels->first); // Place L_after label
              delete labels; // Clean up
              $$ = $3; // Pass funcprefix's value
              delete $2; delete $4; delete $7;
          }
          ;

funcblock : LEFT_BRACE stmt_list RIGHT_BRACE { 
              delete $1; delete $3;
          }
          ;

block : LEFT_BRACE { symbol_table.increase_scope(); } stmt_list RIGHT_BRACE { 
          symbol_table.decrease_scope();
          delete $1; delete $4;
      }
      ;

funcprefix : IDENT { 
                if (symbol_table.is_library_function($1->content)) {
                    yyerror("Cannot redefine library function");
                } else {
                    symbol_table.insert($1->content, SymbolType::USER_FUNCTION, yylineno);
                    add_quad("funcstart", $1->content, "", "");  // Function name in result
                }
                $$ = new string($1->content);
                delete $1;
            }
           | /* anonymous function */ { 
                char anon_name[20];
                sprintf(anon_name, "$%d", anon_func_counter++);
                symbol_table.insert(anon_name, SymbolType::USER_FUNCTION, yylineno);
                add_quad("funcstart", anon_name, "", "");  // Anonymous name in result
                $$ = new string(anon_name);
            }
           ;

const : INTCONST { $$ = new string(to_string(get<int>($1->value))); delete $1; }
      | REALCONST { $$ = new string(to_string(get<double>($1->value))); delete $1; }
      | STRINGCONST { $$ = new string("\"" + string($1->content) + "\""); delete $1; }
      | NIL { $$ = new string("nil"); delete $1; }
      | TRUE { $$ = new string("'true'"); delete $1; }
      | FALSE { $$ = new string("'false'"); delete $1; }
      ;

idlist : IDENT { 
           symbol_table.insert($1->content, SymbolType::FORMAL_ARGUMENT, yylineno);
           delete $1;
       }
       | idlist COMMA IDENT { 
           symbol_table.insert($3->content, SymbolType::FORMAL_ARGUMENT, yylineno);
           delete $2; delete $3;
       }
       | /* empty */
       ;

ifstmt : IF LEFT_PAREN expr RIGHT_PAREN { 
           string label_then = new_label();
           string label_else = new_label();
           string label_end = new_label();
           
           add_quad("if_eq", "", *$3, "'true'", label_then);
           add_quad("jump", "", "", "", label_else);
           add_quad("label", "", "", "", label_then);
           
           // Store labels for later use
           $<paramList>$ = new vector<string>();
           $<paramList>$->push_back(label_else);  // else label
           $<paramList>$->push_back(label_end);   // end label
       } stmt {
           // After the if-body, jump to end and place else label
           vector<string>* labels = $<paramList>5;
           add_quad("jump", "", "", "", labels->at(1)); // Jump to end
           add_quad("label", "", "", "", labels->at(0)); // Place else label
           
           $<paramList>$ = labels; // Pass labels to next part
       } else_part {
           // Place final end label only if there was an else part
           vector<string>* labels = $<paramList>7;
           add_quad("label", "", "", "", labels->at(1)); // Place end label
           
           delete labels;
           delete $1; delete $2; delete $4; delete $3;
       }
       ;

else_part : ELSE stmt {
              delete $1;
          }
          | /* empty */ {
              // No else part - still need the end label for proper control flow
          }
          ;

whilestmt : WHILE { 
              // Initialize labels
              $<paramList>$ = new vector<string>();
              $<paramList>$->push_back(new_label()); // label_start (condition)
              $<paramList>$->push_back(new_label()); // label_body
              $<paramList>$->push_back(new_label()); // label_exit
              
              loop_nesting++;
              // Push labels for break/continue
              loop_start_labels.push_back($<paramList>$->at(0)); // continue jumps to start
              loop_exit_labels.push_back($<paramList>$->at(2));  // break jumps to exit
              
              // Emit start label
              add_quad("label", "", "", "", $<paramList>$->at(0)); // Loop start point
          } LEFT_PAREN expr RIGHT_PAREN { 
              // Access labels
              vector<string>* labels = $<paramList>2;
              
              // Emit condition check
              add_quad("if_eq", "", *$4, "'true'", labels->at(1)); // If true, jump to body
              add_quad("jump", "", "", "", labels->at(2)); // If false, jump to exit
              
              // Emit body label
              add_quad("label", "", "", "", labels->at(1)); // Body label
          } stmt { 
              // Access labels
              vector<string>* labels = $<paramList>2;
              
              // After body, loop back to start for next condition check
              add_quad("jump", "", "", "", labels->at(0)); // Jump to start
              
              // Emit exit label
              add_quad("label", "", "", "", labels->at(2)); // Exit label
              
              // Clean up
              loop_start_labels.pop_back();
              loop_exit_labels.pop_back();
              loop_nesting--;
              
              delete $1; delete $3; delete $5; delete $4;
              delete labels;
          };

for_elist : expr { $$ = $1; }
          | for_elist COMMA expr { $$ = $3; delete $2; }
          | /* empty */ { $$ = nullptr; }
          ;

forstmt : FOR LEFT_PAREN for_elist SEMICOLON expr SEMICOLON { 
              // Initialize labels
              $<paramList>$ = new vector<string>(); 
              $<paramList>$->push_back(new_label()); // label_cond
              $<paramList>$->push_back(new_label()); // label_body  
              $<paramList>$->push_back(new_label()); // label_incr
              $<paramList>$->push_back(new_label()); // label_exit
              
              loop_nesting++;
              loop_start_labels.push_back($<paramList>$->at(2)); // label_incr for continue
              loop_exit_labels.push_back($<paramList>$->at(3));  // label_exit for break
              
              add_quad("label", "", "", "", $<paramList>$->at(0)); // Condition label
              add_quad("if_eq", "", *$5, "'true'", $<paramList>$->at(1)); // If true, jump to body
              add_quad("jump", "", "", "", $<paramList>$->at(3)); // If false, exit
              
              collecting_quads = true;
          } for_elist RIGHT_PAREN { 
              collecting_quads = false;
              
              // Access labels from $<paramList>7
              vector<string>* labels = $<paramList>7;
              
              add_quad("label", "", "", "", labels->at(2)); // Increment label
              for (const auto& q : collected_quads) {
                  emit(q.op, q.result, q.arg1, q.arg2, q.target_label, q.line);
              }
              collected_quads.clear();
              add_quad("jump", "", "", "", labels->at(0)); // Jump to condition
              add_quad("label", "", "", "", labels->at(1)); // Body label
              
              // Clean up second action symbols
              if ($8) delete $8; // Second for_elist
              delete $9; // RIGHT_PAREN
          } stmt { 
              // Use labels from $<paramList>7 directly
              add_quad("jump", "", "", "", $<paramList>7->at(2)); // Jump to increment
              add_quad("label", "", "", "", $<paramList>7->at(3)); // Exit label
              
              loop_start_labels.pop_back();
              loop_exit_labels.pop_back();
              loop_nesting--;
              
              // Clean up remaining tokens and labels
              delete $1; delete $2; delete $4; delete $6; // FOR, LEFT_PAREN, SEMICOLON x2
              if ($3) delete $3; // First for_elist
              if ($5) delete $5; // expr
              delete $<paramList>7; // Label vector
          }
          ;

returnstmt : RETURN expr SEMICOLON { 
               if (function_nesting == 0) { 
                   yyerror("return statement outside of function");
                   return -1; 
               }
               add_quad("return", "", *$2, "");
               if (!funcend_labels.empty()) {
                   add_quad("jump", "", "", "", funcend_labels.back());
               }
               delete $1; delete $3; delete $2;
           }
           | RETURN SEMICOLON { 
               if (function_nesting == 0) { 
                   yyerror("return statement outside of function"); 
                   return -1; 
               }
               add_quad("return", "", "", "");
               if (!funcend_labels.empty()) {
                   add_quad("jump", "", "", "", funcend_labels.back());
               }
               delete $1; delete $2;
           }
           ;

%%

int yyerror(const char* message) {
    cerr << "\033[31mError\033[0m at line " << yylineno << ": " << message << endl;
    return 1;
}