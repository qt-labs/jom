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

#include "makefilelinereader.h"

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
    return m_file.open(QIODevice::ReadOnly | QIODevice::Text);
}

void MakefileLineReader::close()
{
    m_file.close();
}

// this is potentially slow
void MakefileLineReader::removeFirstCharacter(char* buf, char* endPtr)
{
    char* tmp = buf;
    while (*tmp && tmp != endPtr) {
        *tmp = *(tmp+1);
        tmp++;
    }
}

void MakefileLineReader::addBufferToLine(QString& line, char* buf, int bufLength)
{
    // filter in-line comments
    char* tmp = buf;
    for (; *tmp; ++tmp) {
        if (*tmp == '#') {
            if (tmp[-1] != '^') {
                *tmp = '\0';
                bufLength = tmp - buf;
                break;
            } else {
                removeFirstCharacter(tmp-1, buf + bufLength - 1);
                bufLength--;
            }
        }
    }

    line.append(QString::fromLatin1(buf, bufLength));
}

QString MakefileLineReader::readLine()
{
    QString line;
    bool multiLineAppendix = false;
    bool endOfLineReached = false;
    qint64 bytesRead;
    do {
        do {
            m_nLineNumber++;
            bytesRead = m_file.readLine(m_lineBuffer, m_nLineBufferSize - 1);
            if (bytesRead <= 0)
                return QString();

            while (m_lineBuffer[bytesRead-1] != '\n') {
                //fprintf(stderr, "realloc %d -> %d\n", m_nLineBufferSize, m_nLineBufferSize + m_nLineBufferGrowSize);
                m_nLineBufferSize += m_nLineBufferGrowSize;
                m_lineBuffer = reinterpret_cast<char*>(realloc(m_lineBuffer, m_nLineBufferSize));
                int moreBytesRead = m_file.readLine(m_lineBuffer + bytesRead, m_nLineBufferSize - 1 - bytesRead);
                if (moreBytesRead <= 0)
                    break;
                bytesRead += moreBytesRead;
            }

        } while (m_lineBuffer[0] == '#');
        m_lineBuffer[bytesRead] = '\0';

        char* buf = m_lineBuffer;
        int bufLength = bytesRead;
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
                addBufferToLine(line, buf, bufLength);
                endOfLineReached = true;
            } else {
                bufLength -= 2; // remove "\\\n"
                addBufferToLine(line, buf, bufLength);
                multiLineAppendix = true;
            }
        } else if (bufLength >= 2 && buf[bufLength - 2] == '^') {
            bufLength--;
            buf[bufLength-1] = '\n';
            addBufferToLine(line, buf, bufLength);
            multiLineAppendix = true;
        } else {
            bufLength--;    // remove trailing \n
            addBufferToLine(line, buf, bufLength);
            endOfLineReached = true;
        }
    } while (!endOfLineReached);

    // trim whitespace from the right
    int idx = line.length() - 1;
    while (idx > 0 && (line.at(idx) == QLatin1Char(' ') || line.at(idx) == QLatin1Char('\t')))
        --idx;
    line.truncate(idx+1);

    return line;
}

} // namespace NMakeFile
