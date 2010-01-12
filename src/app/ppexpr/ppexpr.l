%option 8bit noyywrap nounistd batch
/*%option bison-bridge*/
/*%option reentrant*/
%option warn
/*%option nodefault*/
%option outfile="ppexpr-lex.inc"
/*%option header-file="ppexpr-lex.h"*/

%{
#define YY_DECL int PPExprParser::yylex()
#define ECHO

static int isatty (int)
{
    return 0;
}
%}

%x STATE_STRING
%x STATE_DEFINED
%x STATE_EXIST
%x STATE_SHELLCOMMAND

%%

[ \t]+      /* eat whitespace */
[0-9]+      {
    yylval.num = atoi(yytext);
    return T_NUMBER;
}
\r?\n        return T_LINEFEED;
"&&"        return T_BOOL_AND;
"||"        return T_BOOL_OR;
"&"         return T_BIT_AND;
"|"         return T_BIT_OR;
"!="        return T_NOT_EQUAL;
"=="        return T_EQUAL;
"<"         return T_LESS_THAN;
">"         return T_GREATER_THAN;
"<="        return T_EQUAL_OR_LESS_THAN;
">="        return T_EQUAL_OR_GREATER_THAN;
"<<"        return T_SHIFT_LEFT;
">>"        return T_SHIFT_RIGHT;
"+"         return T_PLUS;
"-"         return T_MINUS;
"*"         return T_MULT;
"/"         return T_DIV;
"%"         return T_MOD;
"~"         return T_BIT_NOT;
"!"         return T_BOOL_NOT;
"("         return T_LEFT_PAREN;
")"         return T_RIGHT_PAREN;

\"          {
                BEGIN(STATE_STRING);
                yylval.str = new QByteArray();
            }

"["         {
                BEGIN(STATE_SHELLCOMMAND);
                yylval.str = new QByteArray();
            }

"DEFINED"   BEGIN(STATE_DEFINED);
"EXIST"     BEGIN(STATE_EXIST);

<STATE_DEFINED>{
    [:blank:]+     /* eat whitespace */
    "(".+")" {
        BEGIN(INITIAL);
        QByteArray macroName(yytext);
        int idx = macroName.indexOf('(');
        macroName.remove(0, idx + 1);
        idx = macroName.lastIndexOf(')');
        macroName.truncate(idx);
        macroName = macroName.trimmed();
        if (macroName.startsWith('\"'))
            macroName.remove(0, 1);
        if (macroName.endsWith('\"'))
            macroName.chop(1);
        yylval.num = m_macroTable->isMacroDefined(macroName) ? 1 : 0;
        return T_NUMBER;
    }
}

<STATE_EXIST>{
    [:blank:]+     /* eat whitespace */
    "(".+")" {
        BEGIN(INITIAL);
        QByteArray fileName(yytext);
        int idx = fileName.indexOf('(');
        fileName.remove(0, idx + 1);
        idx = fileName.lastIndexOf(')');
        fileName.truncate(idx);
        fileName = fileName.trimmed();
        if (fileName.startsWith('\"'))
            fileName.remove(0, 1);
        if (fileName.endsWith('\"'))
            fileName.chop(1);
        yylval.num = QFile::exists(fileName) ? 1 : 0;
        return T_NUMBER;
    }
}

"EXIST"[:blank:]*"(".*")" {
    QByteArray fileName(yytext);
    int idx = fileName.indexOf('(');
    fileName.remove(0, idx + 1);
    idx = fileName.lastIndexOf(')');
    fileName.truncate(idx);
    fileName = fileName.trimmed();
    if (fileName.startsWith('\"'))
        fileName.remove(0, 1);
    if (fileName.endsWith('\"'))
        fileName.chop(1);
    yylval.num = QFile::exists(fileName) ? 1 : 0;
    return T_NUMBER;
}

<STATE_STRING>{
    [^\n\"]+    yylval.str->append(yytext, yyleng);
    \"\"        yylval.str->append('\"');
    \"          {
                    BEGIN(INITIAL);
                    return T_STRING;
                }
}

<STATE_SHELLCOMMAND>{
    [^\n\x5b\x5d\x5e]+    yylval.str->append(yytext, yyleng);
    "^["        yylval.str->append('[');
    "^]"        yylval.str->append(']');
    "]"         {
                    BEGIN(INITIAL);
                    int exitCode = system(yylval.str->data());
                    delete yylval.str;
                    yylval.num = exitCode;
                    return T_NUMBER;
                }
}

%%

