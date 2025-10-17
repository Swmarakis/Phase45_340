#include "al.hpp"

string getCategoryString(const alpha_token_t &token)
{
    switch (token.category)
    {
    case Category::KEYWORD_IF:
    case Category::KEYWORD_ELSE:
    case Category::KEYWORD_WHILE:
    case Category::KEYWORD_FOR:
    case Category::KEYWORD_FUNCTION:
    case Category::KEYWORD_RETURN:
    case Category::KEYWORD_BREAK:
    case Category::KEYWORD_CONTINUE:
    case Category::KEYWORD_AND:
    case Category::KEYWORD_NOT:
    case Category::KEYWORD_OR:
    case Category::KEYWORD_LOCAL:
    case Category::KEYWORD_TRUE:
    case Category::KEYWORD_FALSE:
    case Category::KEYWORD_NIL:
        return "KEYWORD";

    case Category::OPERATOR_ASSIGN:
    case Category::OPERATOR_PLUS:
    case Category::OPERATOR_MINUS:
    case Category::OPERATOR_MULTIPLY:
    case Category::OPERATOR_DIVIDE:
    case Category::OPERATOR_MODULO:
    case Category::OPERATOR_EQUALS:
    case Category::OPERATOR_NOT_EQUALS:
    case Category::OPERATOR_INCREMENT:
    case Category::OPERATOR_DECREMENT:
    case Category::OPERATOR_GREATER:
    case Category::OPERATOR_LESS:
    case Category::OPERATOR_GREATER_EQUAL:
    case Category::OPERATOR_LESS_EQUAL:
        return "OPERATOR";

    case Category::PUNCTUATION_LBRACE:
    case Category::PUNCTUATION_RBRACE:
    case Category::PUNCTUATION_LBRACKET:
    case Category::PUNCTUATION_RBRACKET:
    case Category::PUNCTUATION_LPAREN:
    case Category::PUNCTUATION_RPAREN:
    case Category::PUNCTUATION_SEMICOLON:
    case Category::PUNCTUATION_COMMA:
    case Category::PUNCTUATION_COLON:
    case Category::PUNCTUATION_COLONCOLON:
    case Category::PUNCTUATION_DOT:
    case Category::PUNCTUATION_DOTDOT:
        return "PUNCTUATION";

    case Category::INTCONST:
    case Category::REALCONST:
        return "CONST";

    case Category::STRINGCONST:
        return "STRING";

    case Category::IDENT:
        return "ID";

    case Category::COMMENT_LINE:
    case Category::COMMENT_BLOCK:
        return "COMMENT";

    case Category::COMMENT_NESTED:
        return "NESTED_COMMENT";

    case Category::ESCAPE_NEWLINE:
        return "ESCAPE_NEWLINE";

    case Category::ESCAPE_TAB:
        return "ESCAPE_TAB";

    case Category::UNDEFINED:
        return "UNDEFINED";

    default:
        return "UNKNOWN";
    }
}

string getAttributeString(const alpha_token_t &token)
{
    ostringstream oss;
    switch (token.category)
    {
    case Category::KEYWORD_IF:
        return "IF <-enumerated";
    case Category::KEYWORD_ELSE:
        return "ELSE <-enumerated";
    case Category::KEYWORD_WHILE:
        return "WHILE <-enumerated";
    case Category::KEYWORD_FOR:
        return "FOR <-enumerated";
    case Category::KEYWORD_FUNCTION:
        return "FUNCTION <-enumerated";
    case Category::KEYWORD_RETURN:
        return "RETURN <-enumerated";
    case Category::KEYWORD_BREAK:
        return "BREAK <-enumerated";
    case Category::KEYWORD_CONTINUE:
        return "CONTINUE <-enumerated";
    case Category::KEYWORD_AND:
        return "AND <-enumerated";
    case Category::KEYWORD_NOT:
        return "NOT <-enumerated";
    case Category::KEYWORD_OR:
        return "OR <-enumerated";
    case Category::KEYWORD_LOCAL:
        return "LOCAL <-enumerated";
    case Category::KEYWORD_TRUE:
        return "TRUE <-enumerated";
    case Category::KEYWORD_FALSE:
        return "FALSE <-enumerated";
    case Category::KEYWORD_NIL:
        return "NIL <-enumerated";

    case Category::OPERATOR_ASSIGN:
        return "ASSIGN <-enumerated";
    case Category::OPERATOR_PLUS:
        return "PLUS <-enumerated";
    case Category::OPERATOR_MINUS:
        return "MINUS <-enumerated";
    case Category::OPERATOR_MULTIPLY:
        return "MULTIPLY <-enumerated";
    case Category::OPERATOR_DIVIDE:
        return "DIVIDE <-enumerated";
    case Category::OPERATOR_MODULO:
        return "MODULO <-enumerated";
    case Category::OPERATOR_EQUALS:
        return "EQUALS <-enumerated";
    case Category::OPERATOR_NOT_EQUALS:
        return "NOT_EQUALS <-enumerated";
    case Category::OPERATOR_INCREMENT:
        return "INCREMENT <-enumerated";
    case Category::OPERATOR_DECREMENT:
        return "DECREMENT <-enumerated";
    case Category::OPERATOR_GREATER:
        return "GREATER <-enumerated";
    case Category::OPERATOR_LESS:
        return "LESS <-enumerated";
    case Category::OPERATOR_GREATER_EQUAL:
        return "GREATER_EQUAL <-enumerated";
    case Category::OPERATOR_LESS_EQUAL:
        return "LESS_EQUAL <-enumerated";

    case Category::PUNCTUATION_LBRACE:
        return "LEFT_BRACE <-enumerated";
    case Category::PUNCTUATION_RBRACE:
        return "RIGHT_BRACE <-enumerated";
    case Category::PUNCTUATION_LBRACKET:
        return "LEFT_BRACKET <-enumerated";
    case Category::PUNCTUATION_RBRACKET:
        return "RIGHT_BRACKET <-enumerated";
    case Category::PUNCTUATION_LPAREN:
        return "LEFT_PAREN <-enumerated";
    case Category::PUNCTUATION_RPAREN:
        return "RIGHT_PAREN <-enumerated";
    case Category::PUNCTUATION_SEMICOLON:
        return "SEMICOLON <-enumerated";
    case Category::PUNCTUATION_COMMA:
        return "COMMA <-enumerated";
    case Category::PUNCTUATION_COLON:
        return "COLON <-enumerated";
    case Category::PUNCTUATION_COLONCOLON:
        return "COLONCOLON <-enumerated";
    case Category::PUNCTUATION_DOT:
        return "DOT <-enumerated";
    case Category::PUNCTUATION_DOTDOT:
        return "DOTDOT <-enumerated";

    case Category::INTCONST:
        oss << get<int>(token.value);
        return oss.str() + " <-integer";

    case Category::REALCONST:
        oss << fixed << setprecision(2) << get<double>(token.value);
        return oss.str() + " <-real";

    case Category::STRINGCONST:
        return "\"" + token.content + "\" <-char*";

    case Category::IDENT:
        return "\"" + token.content + "\" <-char*";

    case Category::COMMENT_LINE:
        return "LINE_COMMENT <-enumerated";

    case Category::COMMENT_BLOCK:
    case Category::COMMENT_NESTED:
        return "BLOCK_COMMENT <-enumerated";

    case Category::ESCAPE_NEWLINE:
        return "\"\\n\" <-escape";

    case Category::ESCAPE_TAB:
        return "\"\\t\" <-escape";

    case Category::UNDEFINED:
        return "UNDEFINED";

    default:
        return "UNKNOWN";
    }
}