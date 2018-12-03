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

#ifndef MAKEFILELINEREADER_H
#define MAKEFILELINEREADER_H

#include <QtCore/QFile>
#include <QtCore/QTextStream>

namespace NMakeFile {

enum class LineContinuationType
{
    None,
    Backslash,
    Caret
};

struct MakefileLine
{
    QString content;
    LineContinuationType continuation = LineContinuationType::None;
};

inline bool isComplete(const MakefileLine &line)
{
    return line.continuation == LineContinuationType::None;
}

class MakefileLineReader {
public:
    MakefileLineReader(const QString& filename);
    ~MakefileLineReader();

    bool open();
    void close();
    MakefileLine readLine(bool bInlineFileMode);
    QString fileName() const { return m_file.fileName(); }
    uint lineNumber() const { return m_nLineNumber; }

private:
    void growLineBuffer(size_t nGrow);

    typedef MakefileLine (MakefileLineReader::*ReadLineImpl)();
    ReadLineImpl m_readLineImpl;
    MakefileLine readLine_impl_local8bit();
    MakefileLine readLine_impl_unicode();

private:
    QFile m_file;
    QTextStream m_textStream;
    static const size_t m_nInitialLineBufferSize = 6144;
    static const size_t m_nLineBufferGrowSize = 1024;
    size_t m_nLineBufferSize;
    char *m_lineBuffer;
    uint m_nLineNumber;
};

} // namespace NMakeFile

#endif // MAKEFILELINEREADER_H
