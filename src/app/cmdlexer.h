#pragma once

#include <QtCore/QList>
#include <QtCore/QString>

namespace NMakeFile {

class CmdLexer
{
public:
    enum TokenType
    {
        TEndOfInput = 0,
        TArgument,
        TAmpersand,
        TDoubleAmpersand,
        TPipe,
        TDoublePipe,
        TParenthesisOpen,
        TParenthesisClose,
        TRedirectInput,         // <
        TRedirectOutput,        // >
        TRedirectOutputAppend,  // >>
        TRedirectInputHandle,   // <&
        TRedirectOutputHandle   // >&
    };

    struct Token
    {
        Token()
            : type(TArgument)
        {}

        explicit Token(const QString& str, TokenType t = TArgument)
            : type(t), value(str)
        {}

        explicit Token(TokenType t)
            : type(t)
        {
        }

        TokenType type;
        QString value;
    };

    void setInput(const QString& input);
    Token lex();
    QList<Token> lexTokens();

private:
    QString input;
    QString tmp;
    int i;
    uint quoteCount;
    bool inQuote;
    bool escaped;
    bool parenthesisOpenFound;
    bool parenthesisCloseFound;
    uint ampersandCount;
    uint pipeCount;
    uint lessThanCount;
    uint greaterThanCount;
    bool splitHere;
};

} // namespace NMakeFile
