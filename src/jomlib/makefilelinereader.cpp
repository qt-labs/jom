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
QString MakefileLineReader::readLine(bool bInlineFileMode)
{
    if (bInlineFileMode) {
        m_nLineNumber++;
        return QString::fromLatin1(m_file.readLine());
    }

    return (this->*m_readLineImpl)();
}

/**
 * readLine implementation optimized for 8 bit files.
 */
QString MakefileLineReader::readLine_impl_local8bit()
{
    QString line;
    bool multiLineAppendix = false;
    bool endOfLineReached = false;
    size_t bytesRead;
    do {
        do {
            m_nLineNumber++;
            const qint64 n = m_file.readLine(m_lineBuffer, m_nLineBufferSize - 1);
            if (n <= 0)
                return QString();

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

        char* buf = m_lineBuffer;
        int bufLength = static_cast<int>(bytesRead);
        if (multiLineAppendix) {
            // skip leading whitespace characters
            while (*buf && (*buf == ' ' || *buf == '\t'))
                buf++;
            if (buf != m_lineBuffer) {
                buf--;          // make sure, we keep one whitespace
                *buf = ' ';     // convert possible tab to space
                bufLength -= buf - m_lineBuffer;
            }
        }

        if (bufLength >= 2 && buf[bufLength - 2] == '\\') {
            if (bufLength >= 3 && buf[bufLength - 3] == '^') {
                buf[bufLength - 3] = '\\';      // replace "^\\\n" -> "\\\\\n"
                bufLength -= 2;                 // remove "\\\n"
                line.append(QString::fromLatin1(buf, bufLength));
                endOfLineReached = true;
            } else {
                bufLength -= 2; // remove "\\\n"
                line.append(QString::fromLatin1(buf, bufLength));
                multiLineAppendix = true;
            }
        } else if (bufLength >= 2 && buf[bufLength - 2] == '^') {
            bufLength--;
            buf[bufLength-1] = '\n';
            line.append(QString::fromLatin1(buf, bufLength));
            multiLineAppendix = true;
        } else {
            bufLength--;    // remove trailing \n
            line.append(QString::fromLatin1(buf, bufLength));
            endOfLineReached = true;
        }
    } while (!endOfLineReached);

    // trim whitespace from the right
    int idx = line.length() - 1;
    while (idx > 0 && isSpaceOrTab(line.at(idx)))
        --idx;
    line.truncate(idx+1);

    return line;
}

/**
 * readLine implementation for unicode files.
 * Much slower than the 8 bit version.
 */
QString MakefileLineReader::readLine_impl_unicode()
{
    QString line;
    bool endOfLineReached = false;
    bool multilineAppendix = false;
    do {
        m_nLineNumber++;
        QString str = m_textStream.readLine();
        if (str.isNull())
            break;

        if (str.startsWith(QLatin1Char('#')))
            continue;

        if (multilineAppendix && (str.startsWith(QLatin1Char(' ')) || str.startsWith(QLatin1Char('\t')))) {
            str = str.trimmed();
            str.prepend(QLatin1Char(' '));
        }

        if (str.endsWith(QLatin1String("^\\"))) {
            str.remove(str.length() - 2, 1);
            endOfLineReached = true;
        } else if (str.endsWith(QLatin1Char('\\'))) {
            str.chop(1);
            multilineAppendix = true;
        } else if (str.endsWith(QLatin1Char('^'))) {
            str.chop(1);
            str.append(QLatin1Char('\n'));
            multilineAppendix = true;
        } else {
            endOfLineReached = true;
        }

        line.append(str);
    } while (!endOfLineReached);
    return line;
}

} // namespace NMakeFile
