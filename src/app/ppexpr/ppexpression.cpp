/****************************************************************************
 **
 ** Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies).
 ** Contact: Qt Software Information (qt-info@nokia.com)
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
#include "ppexpression.h"
#include "NMakeExpressionLexer.h"
#include "NMakeExpressionParser.h"
#include "../preprocessor.h"
#include "../macrotable.h"

#include <QFile>
#include <QDebug>

namespace NMakeFile {

static inline void removeDoubleQuotes(QString& str)
{
    if (str.startsWith('"')) {
        str = str.mid(1, str.length() - 2);
        str.replace("\"\"", "\"");
    }
}

static int isMacroDefined_callback(void* caller, pANTLR3_STRING sz)
{
    if (caller) {
        Preprocessor* pp = reinterpret_cast<Preprocessor*>(caller);
        QString identifier = QString::fromUtf16((const ushort*)sz->chars);
        removeDoubleQuotes(identifier);
        return pp->macroTable()->isMacroDefined(identifier);
    }
    return 0;
}

static int isFileExisting_callback(void* caller, pANTLR3_STRING sz)
{
    QString filename = QString::fromUtf16((const ushort*)sz->chars);
    filename = filename.right(filename.length() - 6);
    int idx = filename.indexOf('(');
    if (idx != -1)
        filename = filename.right(filename.length() - idx - 1);
    idx = filename.lastIndexOf(')');
    if (idx != -1)
        filename = filename.left(idx);
    filename = filename.trimmed();
    removeDoubleQuotes(filename);
    return QFile::exists(filename);
}

PPExpression::PPExpression(NMakeFile::Preprocessor* pp)
:   m_preprocessor(pp)
{
}

PPExpression::~PPExpression()
{
}

int PPExpression::evaluate(const QString& expression)
{
    //qDebug() << "PPExpression::evaluate" << expression;
    const pANTLR3_UINT16 szExpression = (pANTLR3_UINT16)( expression.utf16() );
    pANTLR3_INPUT_STREAM inputStream = antlr3NewUCS2StringInPlaceStream(szExpression, expression.length(), 0);
    if (!inputStream)
        return 0;

    pNMakeExpressionLexer lxr = NMakeExpressionLexerNew(inputStream);
    if (!lxr)
        return 0;

    pANTLR3_COMMON_TOKEN_STREAM tstream = antlr3CommonTokenStreamSourceNew(ANTLR3_SIZE_HINT, TOKENSOURCE(lxr));
    if (!tstream)
        return 0;

    pNMakeExpressionParser psr = NMakeExpressionParserNew(tstream);
    if (!psr)
        return 0;

    int result = psr->expr(psr, m_preprocessor, &isMacroDefined_callback, &isFileExisting_callback);
    psr->free(psr);
    tstream->free(tstream);
    lxr->free(lxr);
    inputStream->free(inputStream);

    return result;
}

} // namespace NMakeFile
