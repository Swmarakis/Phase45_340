#ifndef AL_HPP
#define AL_HPP

#include <string>
#include <variant>
#include <iostream>
#include <sstream>
#include <iomanip>

using namespace std;

/* Enumerated class for token categories */
enum class Category
{
    /* Keywords */
    KEYWORD_IF = 1,
    KEYWORD_ELSE,
    KEYWORD_WHILE,
    KEYWORD_FOR,
    KEYWORD_FUNCTION,
    KEYWORD_RETURN,
    KEYWORD_BREAK,
    KEYWORD_CONTINUE,
    KEYWORD_AND,
    KEYWORD_NOT,
    KEYWORD_OR,
    KEYWORD_LOCAL,
    KEYWORD_TRUE,
    KEYWORD_FALSE,
    KEYWORD_NIL,
    /* Operators */
    OPERATOR_ASSIGN,
    OPERATOR_PLUS,
    OPERATOR_MINUS,
    OPERATOR_MULTIPLY,
    OPERATOR_DIVIDE,
    OPERATOR_MODULO,
    OPERATOR_EQUALS,
    OPERATOR_NOT_EQUALS,
    OPERATOR_INCREMENT,
    OPERATOR_DECREMENT,
    OPERATOR_GREATER,
    OPERATOR_LESS,
    OPERATOR_GREATER_EQUAL,
    OPERATOR_LESS_EQUAL,
    /* Punctuation */
    PUNCTUATION_LBRACE,
    PUNCTUATION_RBRACE,
    PUNCTUATION_LBRACKET,
    PUNCTUATION_RBRACKET,
    PUNCTUATION_LPAREN,
    PUNCTUATION_RPAREN,
    PUNCTUATION_SEMICOLON,
    PUNCTUATION_COMMA,
    PUNCTUATION_COLON,
    PUNCTUATION_COLONCOLON,
    PUNCTUATION_DOT,
    PUNCTUATION_DOTDOT,
    /* Constants and Identifiers */
    INTCONST,
    REALCONST,
    STRINGCONST,
    IDENT,
    /* Comments */
    COMMENT_LINE,
    COMMENT_BLOCK,
    COMMENT_NESTED,
    /* Escape Sequences */
    ESCAPE_NEWLINE,
    ESCAPE_TAB,
    /* Undefined */
    UNDEFINED
};

/* Struct to hold token info */
struct alpha_token_t
{
    int line_number;                    /* The token line number */
    int token_number;                   /* Token sequence number */
    string content;                     /* Token's text string */
    Category category;                  /* Token enum category */
    variant<int, double, string> value; /* Token value (int, double, or string) */
};

class Node
{
public:
    std::string content; // Assuming Node stores a string like $1->content

    // Constructor that takes a string
    Node(const std::string &content) : content(content) {}

    // Default constructor (if needed)
    Node() = default;

    // Destructor (if you manage dynamic memory)
    ~Node() = default;
};

/* Functions for string representations */
string getCategoryString(const alpha_token_t &token);
string getAttributeString(const alpha_token_t &token);

#endif