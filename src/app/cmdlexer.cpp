#include "cmdlexer.h"

namespace NMakeFile {

void CmdLexer::setInput(const QString& input)
{
    this->input = input;
    tmp.clear();
    i = 0;
    quoteCount = 0;
    inQuote = false;
    escaped = false;
    parenthesisOpenFound = false;
    parenthesisCloseFound = false;
    ampersandCount = 0;
    pipeCount = 0;
    lessThanCount = 0;
    greaterThanCount = 0;
    splitHere = false;
}

QList<CmdLexer::Token> CmdLexer::lexTokens()
{
    QList<Token> tokens;
    Token t = lex();
    while (t.type != TEndOfInput) {
        tokens.append(t);
        t = lex();
    }
    return tokens;
}

CmdLexer::Token CmdLexer::lex()
{
    for (; i < input.size(); ++i) {
        if (splitHere) {
            splitHere = false;
            if (!tmp.isEmpty()) {
                Token t(tmp);
                tmp.clear();
                return t;
            }
        }

        const QChar& currentChar = input.at(i);
        if (currentChar == QLatin1Char('"')) {
            ++quoteCount;
            if (quoteCount == 3) {
                // third consecutive quote
                quoteCount = 0;
                tmp += currentChar;
            }
            continue;
        } else if (!inQuote && !escaped) {
            switch (currentChar.toAscii()) {
                case '&':
                    ++ampersandCount;
                    splitHere = true;
                    continue;
                case '|':
                    ++pipeCount;
                    splitHere = true;
                    continue;
                case '<':
                    ++lessThanCount;
                    splitHere = true;
                    continue;
                case '>':
                    ++greaterThanCount;
                    splitHere = true;
                    continue;
                case '(':
                    parenthesisOpenFound = true;
                    splitHere = true;
                    continue;
                case ')':
                    parenthesisCloseFound = true;
                    splitHere = true;
                    continue;
            }
        }
        if (quoteCount) {
            if (quoteCount == 1)
                inQuote = !inQuote;
            quoteCount = 0;
        }
        if (parenthesisOpenFound) {
            parenthesisOpenFound = false;
            return Token(TParenthesisOpen);
        } else if (parenthesisCloseFound) {
            parenthesisCloseFound = false;
            return Token(TParenthesisClose);
        }
        if (lessThanCount) {
            Token t;
            if (ampersandCount) {
                t.type = TRedirectInputHandle;
                ampersandCount = 0;
            } else {
                t.type = TRedirectInput;
            }
            lessThanCount = 0;
            return t;
        }
        if (greaterThanCount) {
            Token t;
            if (ampersandCount) {
                t.type = TRedirectOutputHandle;
                ampersandCount = 0;
            } else {
                t.type = (greaterThanCount == 1) ? TRedirectOutput : TRedirectOutputAppend;
            }
            greaterThanCount = 0;
            return t;
        }
        if (ampersandCount) {
            Token t;
            t.type = (ampersandCount == 1) ? TAmpersand : TDoubleAmpersand;
            ampersandCount = 0;
            return t;
        }
        if (pipeCount) {
            Token t;
            t.type = (pipeCount == 1) ? TPipe : TDoublePipe;
            pipeCount = 0;
            return t;
        }
        if (!inQuote && currentChar.isSpace()) {
            escaped = false;
            if (!tmp.isEmpty()) {
                Token t(tmp);
                tmp.clear();
                return t;
            }
        } else {
            if (!inQuote && !escaped && currentChar == QLatin1Char('^')) {
                escaped = true;
                continue;
            }
            tmp += currentChar;
            escaped = false;
        }
    }

    if (!tmp.isEmpty()) {
        Token t(tmp);
        tmp.clear();
        return t;
    }
    if (parenthesisCloseFound) {
        parenthesisCloseFound = false;
        return Token(TParenthesisClose);
    }

    return Token(TEndOfInput);
}

} // namespace NMakeFile
