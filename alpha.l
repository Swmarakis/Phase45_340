%{
#include <iostream>
#include <string>
#include <vector>
#include "al.hpp"
#include "parser.tab.hpp"  // Include Bison-generated token definitions

#define YY_DECL int alpha_yylex(void* ylval)

int line_number = 1;
int token_number = 1;
int comment_nesting = 0;
int comment_start_line;
std::vector<int> comment_starts;

std::string raw_content;      
std::string interpreted_value;
int string_start_line;        

using namespace std;
%}

%option noyywrap
%option yylineno
%x BLOCK_COMMENT
%x STRING_STATE

DIGIT       [0-9]
LETTER      [a-zA-Z]
INTEGER     {DIGIT}+
REAL        {DIGIT}+\.{DIGIT}+
WHITESPACE  [ \t]+
IDENT       {LETTER}({LETTER}|{DIGIT}|_)*

%%

{WHITESPACE}    { /* Ignore whitespace */ }
\n              { 
    line_number++; 
    }

"if"    {
    alpha_token_t* token = new alpha_token_t;
    token->line_number = line_number;
    token->token_number = token_number++;
    token->content = yytext;
    token->category = Category::KEYWORD_IF;
    token->value = string();
    static_cast<YYSTYPE*>(ylval)->token = token;
    return IF;
}

"else"    {
    alpha_token_t* token = new alpha_token_t;
    token->line_number = line_number;
    token->token_number = token_number++;
    token->content = yytext;
    token->category = Category::KEYWORD_ELSE;
    token->value = string();
    static_cast<YYSTYPE*>(ylval)->token = token;
    return ELSE;
}

"while"    {
    alpha_token_t* token = new alpha_token_t;
    token->line_number = line_number;
    token->token_number = token_number++;
    token->content = yytext;
    token->category = Category::KEYWORD_WHILE;
    token->value = string();
    static_cast<YYSTYPE*>(ylval)->token = token;
    return WHILE;
}

"for"    {
    alpha_token_t* token = new alpha_token_t;
    token->line_number = line_number;
    token->token_number = token_number++;
    token->content = yytext;
    token->category = Category::KEYWORD_FOR;
    token->value = string();
    static_cast<YYSTYPE*>(ylval)->token = token;
    return FOR;
}

"function"    {
    alpha_token_t* token = new alpha_token_t;
    token->line_number = line_number;
    token->token_number = token_number++;
    token->content = yytext;
    token->category = Category::KEYWORD_FUNCTION;
    token->value = string();
    static_cast<YYSTYPE*>(ylval)->token = token;
    return FUNCTION;
}

"return"    {
    alpha_token_t* token = new alpha_token_t;
    token->line_number = line_number;
    token->token_number = token_number++;
    token->content = yytext;
    token->category = Category::KEYWORD_RETURN;
    token->value = string();
    static_cast<YYSTYPE*>(ylval)->token = token;
    return RETURN;
}

"break"    {
    alpha_token_t* token = new alpha_token_t;
    token->line_number = line_number;
    token->token_number = token_number++;
    token->content = yytext;
    token->category = Category::KEYWORD_BREAK;
    token->value = string();
    static_cast<YYSTYPE*>(ylval)->token = token;
    return BREAK;
}

"continue"    {
    alpha_token_t* token = new alpha_token_t;
    token->line_number = line_number;
    token->token_number = token_number++;
    token->content = yytext;
    token->category = Category::KEYWORD_CONTINUE;
    token->value = string();
    static_cast<YYSTYPE*>(ylval)->token = token;
    return CONTINUE;
}

"and"    {
    alpha_token_t* token = new alpha_token_t;
    token->line_number = line_number;
    token->token_number = token_number++;
    token->content = yytext;
    token->category = Category::KEYWORD_AND;
    token->value = string();
    static_cast<YYSTYPE*>(ylval)->token = token;
    return AND;
}

"not"    {
    alpha_token_t* token = new alpha_token_t;
    token->line_number = line_number;
    token->token_number = token_number++;
    token->content = yytext;
    token->category = Category::KEYWORD_NOT;
    token->value = string();
    static_cast<YYSTYPE*>(ylval)->token = token;
    return NOT;
}

"or"    {
    alpha_token_t* token = new alpha_token_t;
    token->line_number = line_number;
    token->token_number = token_number++;
    token->content = yytext;
    token->category = Category::KEYWORD_OR;
    token->value = string();
    static_cast<YYSTYPE*>(ylval)->token = token;
    return OR;
}

"local"    {
    alpha_token_t* token = new alpha_token_t;
    token->line_number = line_number;
    token->token_number = token_number++;
    token->content = yytext;
    token->category = Category::KEYWORD_LOCAL;
    token->value = string();
    static_cast<YYSTYPE*>(ylval)->token = token;
    return LOCAL;
}

"true"    {
    alpha_token_t* token = new alpha_token_t;
    token->line_number = line_number;
    token->token_number = token_number++;
    token->content = yytext;
    token->category = Category::KEYWORD_TRUE;
    token->value = string();
    static_cast<YYSTYPE*>(ylval)->token = token;
    return TRUE;
}

"false"    {
    alpha_token_t* token = new alpha_token_t;
    token->line_number = line_number;
    token->token_number = token_number++;
    token->content = yytext;
    token->category = Category::KEYWORD_FALSE;
    token->value = string();
    static_cast<YYSTYPE*>(ylval)->token = token;
    return FALSE;
}

"nil"    {
    alpha_token_t* token = new alpha_token_t;
    token->line_number = line_number;
    token->token_number = token_number++;
    token->content = yytext;
    token->category = Category::KEYWORD_NIL;
    token->value = string();
    static_cast<YYSTYPE*>(ylval)->token = token;
    return NIL;
}

"=="    {
    alpha_token_t* token = new alpha_token_t;
    token->line_number = line_number;
    token->token_number = token_number++;
    token->content = yytext;
    token->category = Category::OPERATOR_EQUALS;
    token->value = string();
    static_cast<YYSTYPE*>(ylval)->token = token;
    return EQUALS;
}

"!="    {
    alpha_token_t* token = new alpha_token_t;
    token->line_number = line_number;
    token->token_number = token_number++;
    token->content = yytext;
    token->category = Category::OPERATOR_NOT_EQUALS;
    token->value = string();
    static_cast<YYSTYPE*>(ylval)->token = token;
    return NOT_EQUALS;
}

"++"    {
    alpha_token_t* token = new alpha_token_t;
    token->line_number = line_number;
    token->token_number = token_number++;
    token->content = yytext;
    token->category = Category::OPERATOR_INCREMENT;
    token->value = string();
    static_cast<YYSTYPE*>(ylval)->token = token;
    return INCREMENT;
}

"--"    {
    alpha_token_t* token = new alpha_token_t;
    token->line_number = line_number;
    token->token_number = token_number++;
    token->content = yytext;
    token->category = Category::OPERATOR_DECREMENT;
    token->value = string();
    static_cast<YYSTYPE*>(ylval)->token = token;
    return DECREMENT;
}

">="    {
    alpha_token_t* token = new alpha_token_t;
    token->line_number = line_number;
    token->token_number = token_number++;
    token->content = yytext;
    token->category = Category::OPERATOR_GREATER_EQUAL;
    token->value = string();
    static_cast<YYSTYPE*>(ylval)->token = token;
    return GREATER_EQUAL;
}

"<="    {
    alpha_token_t* token = new alpha_token_t;
    token->line_number = line_number;
    token->token_number = token_number++;
    token->content = yytext;
    token->category = Category::OPERATOR_LESS_EQUAL;
    token->value = string();
    static_cast<YYSTYPE*>(ylval)->token = token;
    return LESS_EQUAL;
}

"="    {
    alpha_token_t* token = new alpha_token_t;
    token->line_number = line_number;
    token->token_number = token_number++;
    token->content = yytext;
    token->category = Category::OPERATOR_ASSIGN;
    token->value = string();
    static_cast<YYSTYPE*>(ylval)->token = token;
    return ASSIGN;
}

"+"    {
    alpha_token_t* token = new alpha_token_t;
    token->line_number = line_number;
    token->token_number = token_number++;
    token->content = yytext;
    token->category = Category::OPERATOR_PLUS;
    token->value = string();
    static_cast<YYSTYPE*>(ylval)->token = token;
    return PLUS;
}

"-"    {
    alpha_token_t* token = new alpha_token_t;
    token->line_number = line_number;
    token->token_number = token_number++;
    token->content = yytext;
    token->category = Category::OPERATOR_MINUS;
    token->value = string();
    static_cast<YYSTYPE*>(ylval)->token = token;
    return MINUS;
}

"*"    {
    alpha_token_t* token = new alpha_token_t;
    token->line_number = line_number;
    token->token_number = token_number++;
    token->content = yytext;
    token->category = Category::OPERATOR_MULTIPLY;
    token->value = string();
    static_cast<YYSTYPE*>(ylval)->token = token;
    return MUL;
}

"/"    {
    alpha_token_t* token = new alpha_token_t;
    token->line_number = line_number;
    token->token_number = token_number++;
    token->content = yytext;
    token->category = Category::OPERATOR_DIVIDE;
    token->value = string();
    static_cast<YYSTYPE*>(ylval)->token = token;
    return DIV;
}

"%"    {
    alpha_token_t* token = new alpha_token_t;
    token->line_number = line_number;
    token->token_number = token_number++;
    token->content = yytext;
    token->category = Category::OPERATOR_MODULO;
    token->value = string();
    static_cast<YYSTYPE*>(ylval)->token = token;
    return MOD;
}

">"    {
    alpha_token_t* token = new alpha_token_t;
    token->line_number = line_number;
    token->token_number = token_number++;
    token->content = yytext;
    token->category = Category::OPERATOR_GREATER;
    token->value = string();
    static_cast<YYSTYPE*>(ylval)->token = token;
    return GREATER;
}

"<"    {
    alpha_token_t* token = new alpha_token_t;
    token->line_number = line_number;
    token->token_number = token_number++;
    token->content = yytext;
    token->category = Category::OPERATOR_LESS;
    token->value = string();
    static_cast<YYSTYPE*>(ylval)->token = token;
    return LESS;
}

"{"    {
    alpha_token_t* token = new alpha_token_t;
    token->line_number = line_number;
    token->token_number = token_number++;
    token->content = yytext;
    token->category = Category::PUNCTUATION_LBRACE;
    token->value = string();
    static_cast<YYSTYPE*>(ylval)->token = token;
    return LEFT_BRACE;
}

"}"    {
    alpha_token_t* token = new alpha_token_t;
    token->line_number = line_number;
    token->token_number = token_number++;
    token->content = yytext;
    token->category = Category::PUNCTUATION_RBRACE;
    token->value = string();
    static_cast<YYSTYPE*>(ylval)->token = token;
    return RIGHT_BRACE;
}

"["    {
    alpha_token_t* token = new alpha_token_t;
    token->line_number = line_number;
    token->token_number = token_number++;
    token->content = yytext;
    token->category = Category::PUNCTUATION_LBRACKET;
    token->value = string();
    static_cast<YYSTYPE*>(ylval)->token = token;
    return LEFT_BRACKET;
}

"]"    {
    alpha_token_t* token = new alpha_token_t;
    token->line_number = line_number;
    token->token_number = token_number++;
    token->content = yytext;
    token->category = Category::PUNCTUATION_RBRACKET;
    token->value = string();
    static_cast<YYSTYPE*>(ylval)->token = token;
    return RIGHT_BRACKET;
}

"("    {
    alpha_token_t* token = new alpha_token_t;
    token->line_number = line_number;
    token->token_number = token_number++;
    token->content = yytext;
    token->category = Category::PUNCTUATION_LPAREN;
    token->value = string();
    static_cast<YYSTYPE*>(ylval)->token = token;
    return LEFT_PAREN;
}

")"    {
    alpha_token_t* token = new alpha_token_t;
    token->line_number = line_number;
    token->token_number = token_number++;
    token->content = yytext;
    token->category = Category::PUNCTUATION_RPAREN;
    token->value = string();
    static_cast<YYSTYPE*>(ylval)->token = token;
    return RIGHT_PAREN;
}

";"    {
    alpha_token_t* token = new alpha_token_t;
    token->line_number = line_number;
    token->token_number = token_number++;
    token->content = yytext;
    token->category = Category::PUNCTUATION_SEMICOLON;
    token->value = string();
    static_cast<YYSTYPE*>(ylval)->token = token;
    return SEMICOLON;
}

","    {
    alpha_token_t* token = new alpha_token_t;
    token->line_number = line_number;
    token->token_number = token_number++;
    token->content = yytext;
    token->category = Category::PUNCTUATION_COMMA;
    token->value = string();
    static_cast<YYSTYPE*>(ylval)->token = token;
    return COMMA;
}

":"    {
    alpha_token_t* token = new alpha_token_t;
    token->line_number = line_number;
    token->token_number = token_number++;
    token->content = yytext;
    token->category = Category::PUNCTUATION_COLON;
    token->value = string();
    static_cast<YYSTYPE*>(ylval)->token = token;
    return COLON;
}

"::"    {
    alpha_token_t* token = new alpha_token_t;
    token->line_number = line_number;
    token->token_number = token_number++;
    token->content = yytext;
    token->category = Category::PUNCTUATION_COLONCOLON;
    token->value = string();
    static_cast<YYSTYPE*>(ylval)->token = token;
    return COLONCOLON;
}

"."    {
    alpha_token_t* token = new alpha_token_t;
    token->line_number = line_number;
    token->token_number = token_number++;
    token->content = yytext;
    token->category = Category::PUNCTUATION_DOT;
    token->value = string();
    static_cast<YYSTYPE*>(ylval)->token = token;
    return DOT;
}

".."    {
    alpha_token_t* token = new alpha_token_t;
    token->line_number = line_number;
    token->token_number = token_number++;
    token->content = yytext;
    token->category = Category::PUNCTUATION_DOTDOT;
    token->value = string();
    static_cast<YYSTYPE*>(ylval)->token = token;
    return DOTDOT;
}

{INTEGER}    {
    alpha_token_t* token = new alpha_token_t;
    token->line_number = line_number;
    token->token_number = token_number++;
    token->content = yytext;
    token->category = Category::INTCONST;
    token->value = stoi(yytext);
    static_cast<YYSTYPE*>(ylval)->token = token;
    return INTCONST;
}

{REAL}    {
    alpha_token_t* token = new alpha_token_t;
    token->line_number = line_number;
    token->token_number = token_number++;
    token->content = yytext;
    token->category = Category::REALCONST;
    token->value = stod(yytext);
    static_cast<YYSTYPE*>(ylval)->token = token;
    return REALCONST;
}

{IDENT}    {
    alpha_token_t* token = new alpha_token_t;
    token->line_number = line_number;
    token->token_number = token_number++;
    token->content = yytext;
    token->category = Category::IDENT;
    token->value = string(yytext);
    static_cast<YYSTYPE*>(ylval)->token = token;
    return IDENT;
}

"//"    {
    int start_line = line_number;
    char c;
    while ((c = yyinput()) != '\n' && c != EOF);
    if (c == '\n') line_number++;
    alpha_token_t* token = new alpha_token_t;
    token->line_number = start_line;
    token->token_number = token_number++;
    token->content = "//";
    token->category = Category::COMMENT_LINE;
    token->value = "";
    static_cast<YYSTYPE*>(ylval)->token = token;
    return COMMENT_LINE;
}

"/*"    {
    comment_start_line = line_number;
    comment_starts.push_back(line_number);
    comment_nesting = 1;
    BEGIN(BLOCK_COMMENT);
}

<BLOCK_COMMENT>"/*"    {
    comment_starts.push_back(line_number);
    comment_nesting++;
}

<BLOCK_COMMENT>"*/"    {
    if (!comment_starts.empty()) {
        int start = comment_starts.back();
        comment_starts.pop_back();
        alpha_token_t* token = new alpha_token_t;
        token->line_number = comment_start_line;
        token->token_number = token_number++;
        token->content = to_string(start) + " - " + to_string(line_number);
        if (comment_nesting > 1) {
            token->category = Category::COMMENT_NESTED;
            comment_nesting--;
            static_cast<YYSTYPE*>(ylval)->token = token;
            return COMMENT_NESTED;
        } else {
            token->category = Category::COMMENT_BLOCK;
            comment_nesting--;
            BEGIN(INITIAL);
            static_cast<YYSTYPE*>(ylval)->token = token;
            return COMMENT_BLOCK;
        }
    }
}

<BLOCK_COMMENT>.    { /* Ignore other characters */ }

<BLOCK_COMMENT>\n   { line_number++; }

<BLOCK_COMMENT><<EOF>>    {
    if (comment_nesting > 0) {
        cerr << "Error: Unclosed block comment starting at line " << comment_start_line << endl;
    }
    BEGIN(INITIAL);
    return 0;
}

\\[nt]    {
    alpha_token_t* token = new alpha_token_t;
    token->line_number = line_number;
    token->token_number = token_number++;
    token->content = string(yytext);
    if (yytext[1] == 'n') {
        token->category = Category::ESCAPE_NEWLINE;
        token->value = string("\\n");
    } else {
        token->category = Category::ESCAPE_TAB;
        token->value = string("\\t");
    }
    static_cast<YYSTYPE*>(ylval)->token = token;
    return (yytext[1] == 'n' ? ESCAPE_NEWLINE : ESCAPE_TAB);
}

\"    {
    string_start_line = line_number;
    raw_content = "";
    interpreted_value = "";
    BEGIN(STRING_STATE);
}

<STRING_STATE>\\[nt\\\"]    {
    raw_content += yytext[0];
    raw_content += yytext[1];
    char c = yytext[1];
    if (c == 'n') interpreted_value += '\n';
    else if (c == 't') interpreted_value += '\t';
    else if (c == '\\') interpreted_value += '\\';
    else if (c == '"') interpreted_value += '"';
}

<STRING_STATE>\"    {
    alpha_token_t* token = new alpha_token_t;
    token->line_number = string_start_line;
    token->token_number = token_number++;
    token->content = raw_content;
    token->category = Category::STRINGCONST;
    token->value = interpreted_value;
    static_cast<YYSTYPE*>(ylval)->token = token;
    BEGIN(INITIAL);
    return STRINGCONST;
}

<STRING_STATE>.    {
    raw_content += yytext[0];
    interpreted_value += yytext[0];
}

<STRING_STATE>\n    {
    cerr << "Error: unclosed string starting at line " << string_start_line << endl;
    line_number++;
    BEGIN(INITIAL);
}

<STRING_STATE><<EOF>>    {
    cerr << "Error: unclosed string starting at line " << string_start_line << endl;
    BEGIN(INITIAL);
    return 0;
}

<<EOF>>    {
    if (comment_nesting > 0) {
        cerr << "Error: Unclosed block comment starting at line " << comment_start_line << endl;
    }
    return 0;
}

\r    { /* Ignore carriage return */ }

.    {
    alpha_token_t* token = new alpha_token_t;
    token->line_number = line_number;
    token->token_number = token_number++;
    token->content = string(1, yytext[0]);
    token->category = Category::UNDEFINED;
    token->value = string(1, yytext[0]);
    static_cast<YYSTYPE*>(ylval)->token = token;
    cerr << "Error: undefined character '" << yytext[0] << "' at line " << line_number << endl;
    return UNDEFINED;
}

%%

/* No yywrap function needed due to %option noyywrap */