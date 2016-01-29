/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of jom.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "ppexpr_grammar_p.h"
#include <QtCore>

namespace NMakeFile {
    class MacroTable;
}

class PPExprParser : protected ppexpr_grammar
{
public:
    PPExprParser();
    ~PPExprParser();

    bool parse(const char* str);

    int expressionValue()
    {
        return sym(1).num;
    }

    QByteArray errorMessage() const
    {
        return m_errorMessage;
    }

    void setMacroTable(NMakeFile::MacroTable* macroTable)
    {
        m_macroTable = macroTable;
    }

protected:
    struct Value
    {
#ifdef _DEBUG
        Value()
        : num(0)
        {}
#endif

        union {
            int num;
            QByteArray* str;
        };
    };

protected:
    int yylex();
    inline void reallocateStack();

    inline Value &sym(int index)
    { return sym_stack [tos + index - 1]; }

protected:
    void* yyInputBuffer;
    Value yylval;
    int tos;
    int stack_size;
    Value *sym_stack;
    int *state_stack;
    NMakeFile::MacroTable* m_macroTable;
    QByteArray m_errorMessage;
};
