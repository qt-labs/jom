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

#include "makefilelinereader.h"
#include "helperfunctions.h"
#include <QTextCodec>
#include <QDebug>

namespace NMakeFile {

MakefileLineReader::MakefileLineReader(const QString& filename)
:   m_file(filename),
    m_nLineBufferSize(m_nInitialLineBufferSize),
    m_nLineNumber(0)
{
    m_lineBuffer = reinterpret_cast<char*>( malloc(m_nLineBufferSize) );
}

MakefileLineReader::~MakefileLineReader()
{
    close();
    free(m_lineBuffer);
}

bool MakefileLineReader::open()
{
    if (!m_file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    // check BOM
    enum FileEncoding { FCLatin1, FCUTF8, FCUTF16 };
    FileEncoding fileEncoding = FCLatin1;
    QByteArray buf = m_file.peek(3);
    if (buf.startsWith("\xFF\xFE"))
        fileEncoding = FCUTF16;
    else if (buf.startsWith("\xEF\xBB\xBF"))
        fileEncoding = FCUTF8;

    if (fileEncoding == FCLatin1) {
        m_readLineImpl = &NMakeFile::MakefileLineReader::readLine_impl_local8bit;
    } else {
        m_readLineImpl = &NMakeFile::MakefileLineReader::readLine_impl_unicode;
        m_textStream.setCodec(fileEncoding == FCUTF8 ? "UTF-8" : "UTF-16");
        m_textStream.setAutoDetectUnicode(false);
        m_textStream.setDevice(&m_file);
    }

    return true;
}

void MakefileLineReader::close()
{
    m_file.close();
}

void MakefileLineReader::growLineBuffer(size_t nGrow)
{
    //fprintf(stderr, "realloc %d -> %d\n", m_nLineBufferSize, m_nLineBufferSize + nGrow);
    m_nLineBufferSize += nGrow;
    m_lineBuffer = reinterpret_cast<char*>(realloc(m_lineBuffer, m_nLineBufferSize));
}

/**
 * This function reads lines from a makefile and
 *    - ignores all lines that start with #
 *    - combines multi-lines (lines with \ at the end) into a single long line
 *    - handles the ^ escape character for \ and \n at the end
 */
MakefileLine MakefileLineReader::readLine(bool bInlineFileMode)
{
    if (bInlineFileMode) {
        m_nLineNumber++;
        return MakefileLine{ QString::fromLatin1(m_file.readLine()) };
    }

    return (this->*m_readLineImpl)();
}

/**
 * readLine implementation optimized for 8 bit files.
 */
MakefileLine MakefileLineReader::readLine_impl_local8bit()
{
    size_t bytesRead;
    do {
        m_nLineNumber++;
        const qint64 n = m_file.readLine(m_lineBuffer, m_nLineBufferSize - 1);
        if (n <= 0)
            return {};

        bytesRead = n;
        while (m_lineBuffer[bytesRead - 1] != '\n') {
            if (m_file.atEnd()) {
                // The file didn't end with a newline.
                // Code below relies on having a trailing newline.
                // We're imitating it by increasing the string length.
                if (bytesRead >= (m_nLineBufferSize - 2))
                    growLineBuffer(1);
                ++bytesRead;
                break;
            }

            growLineBuffer(m_nLineBufferGrowSize);
            int moreBytesRead = m_file.readLine(m_lineBuffer + bytesRead, m_nLineBufferSize - 1 - bytesRead);
            if (moreBytesRead <= 0)
                break;
            bytesRead += moreBytesRead;
        }

    } while (m_lineBuffer[0] == '#');
    m_lineBuffer[bytesRead] = '\0';

    MakefileLine line;
    char* buf = m_lineBuffer;
    int bufLength = static_cast<int>(bytesRead);
    if (bufLength >= 2 && buf[bufLength - 2] == '\\') {
        if (bufLength >= 3 && buf[bufLength - 3] == '^') {
            buf[bufLength - 3] = '\\';      // replace "^\\\n" -> "\\\\\n"
            bufLength -= 2;                 // remove "\\\n"
        } else if (bufLength >= 3 && buf[bufLength - 3] == '\\') {
            bufLength--;    // remove trailing \n
        } else {
            bufLength -= 2; // remove "\\\n"
            line.continuation = LineContinuationType::Backslash;
        }
    } else if (bufLength >= 2 && buf[bufLength - 2] == '^') {
        bufLength -= 2;
        line.continuation = LineContinuationType::Caret;
    } else {
        bufLength--;    // remove trailing \n
    }

    line.content = QString::fromLatin1(buf, bufLength);
    return line;
}

/**
 * readLine implementation for unicode files.
 * Much slower than the 8 bit version.
 */
MakefileLine MakefileLineReader::readLine_impl_unicode()
{
    MakefileLine line;
    QString str;
    do {
        m_nLineNumber++;
        str = m_textStream.readLine();
    } while (str.startsWith(QLatin1Char('#')));

    if (str.isNull())
        return line;

    if (str.endsWith(QLatin1String("^\\"))) {
        str.remove(str.length() - 2, 1);
    } else if (str.endsWith(QLatin1String("\\\\"))) {
        // Do nothing. Double backslash at the end is not altered.
    } else if (str.endsWith(QLatin1Char('\\'))) {
        str.chop(1);
        line.continuation = LineContinuationType::Backslash;
    } else if (str.endsWith(QLatin1Char('^'))) {
        str.chop(1);
        line.continuation = LineContinuationType::Caret;
    }

    line.content = str;
    return line;
}

} // namespace NMakeFile
