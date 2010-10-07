#include "helperfunctions.h"

/**
 * Splits the string, respects "foo bar" and "foo ""knuffi"" bar".
 */
QStringList splitCommandLine(QString commandLine)
{
    QString str;
    QStringList arguments;
    commandLine.append(QLatin1Char(' '));   // append artificial space
    bool escapedQuote = false;
    bool insideQuotes = false;
    for (int i=0; i < commandLine.count(); ++i) {
        if (commandLine.at(i).isSpace() && !insideQuotes) {
            escapedQuote = false;
            str = str.trimmed();
            if (!str.isEmpty()) {
                arguments.append(str);
                str.clear();
            }
        } else {
            if (commandLine.at(i) == QLatin1Char('"')) {
                if (escapedQuote)  {
                    str.append(QLatin1Char('"'));
                    escapedQuote = false;
                } else {
                    escapedQuote = true;
                }
                insideQuotes = !insideQuotes;
            } else {
                str.append(commandLine.at(i));
                escapedQuote = false;
            }
        }
    }
    return arguments;
}
