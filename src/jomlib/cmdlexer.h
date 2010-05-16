/****************************************************************************
 **
 ** Copyright (C) 2008-2010 Nokia Corporation and/or its subsidiary(-ies).
 ** Contact: Nokia Corporation (qt-info@nokia.com)
 **
 ** This file is part of the jom project on Trolltech Labs.
 **
 ** This file may be used under the terms of the GNU General Public
 ** License version 2.0 or 3.0 as published by the Free Software Foundation
 ** and appearing in the file LICENSE.GPL included in the packaging of
 ** this file.  Please review the following information to ensure GNU
 ** General Public Licensing requirements will be met:
 ** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
 ** http://www.gnu.org/copyleft/gpl.html.
 **
 ** If you are unsure which license is appropriate for your use, please
 ** contact the sales department at qt-sales@nokia.com.
 **
 ** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 ** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 **
 ****************************************************************************/
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
