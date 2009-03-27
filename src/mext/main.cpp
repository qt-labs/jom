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
#include "consoleoutprocess.h"

#include <QTime>
#include <QDebug>

int main(int argc, char* argv[])
{
    if (argc < 2) {
        printf("usage: mext <program> [argument 1] ... [argument n]\n");
        return 0;
    }

    QString program = QLatin1String("cmd");
    QStringList arguments;
    arguments.append("/c");
    for (int i=1; i < argc; ++i)
        arguments.append(QString::fromLocal8Bit(argv[i]));

    QTime time;
    ConsoleOutProcess process;
    process.start(program, arguments);
    if (!process.waitForStarted(-1)) {
        printf("Program could not be started.\n");
        return 128;
    }
    time.start();
    process.waitForFinished(-1);
    int elapsed = time.elapsed();
    printf("execution time: %d ms\n", elapsed);
    return 0;
}
