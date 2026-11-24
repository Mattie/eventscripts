#ifndef ES_STRUTILS_H
#define ES_STRUTILS_H

#ifdef _WIN32
#pragma once
#endif

//=========================================================================
// Includes
//=========================================================================
#include "strtools.h"

//=========================================================================
// Removes escape characters from a string.
//=========================================================================
void unescape(char * s, const char * t);

//=========================================================================
// Case-sensitively compares two strings and returns true if they match.
//=========================================================================
inline bool FStrEq(const char *sz1, const char *sz2)
{
    if (sz1 && sz2)
	    return(Q_stricmp(sz1, sz2) == 0);
    else
        return false;
}

//=========================================================================
// Case-insensitively compares two strings and returns true if they match.
//=========================================================================
inline bool FStruEq(const char *sz1, const char *sz2)
{
    if (sz1 && sz2)
	    return(Q_strcasecmp(sz1, sz2) == 0);
    else
        return false;
}

//=========================================================================
// String in-place whitespace trim
//=========================================================================
inline char * trim(char *szToTrim)
{
    char *pEnd = szToTrim + strlen(szToTrim)-1;
    while (*pEnd==' ' || *pEnd=='\t' || *pEnd=='\n' || *pEnd=='\r')
    {
        *pEnd='\0';
        pEnd--;
    }
    pEnd = szToTrim;
    while (*pEnd==' ' || *pEnd=='\t' || *pEnd=='\n' || *pEnd=='\r')
    {
        pEnd++;
    }
    return pEnd;
}

//=========================================================================
// String in-place whitespace trim
//=========================================================================
inline char * TrimEol(char *szToTrim)
{
    char *pEnd = szToTrim + strlen(szToTrim)-1;
    while (*pEnd=='\n')
    {
        *pEnd='\0';
        pEnd--;
    }
    pEnd = szToTrim;
    while (*pEnd==' ' || *pEnd=='\t' || *pEnd=='\n' || *pEnd=='\r')
    {
        pEnd++;
    }
    return pEnd;
}

//=========================================================================
// String quote stripping from the string
//=========================================================================
inline char * StripAllQuotes(char * szDest, const char *szSrc)
{
    if (!szDest || !szSrc)
        return NULL;

    const char * s = szSrc; 
    char * d = szDest;
    while(*s)
    {
        if (*s!='\"')
        {
            *d=*s;
            d++;
        }
        s++;
    }
    *d=*s;
    return szDest;
}

//=========================================================================
// String replace.
//=========================================================================
char *str_replace(const char *subject, const char *search, const char *replace, char *replaced, int size);

#ifndef ORANGEBOX
#define V_StrSubst Q_StrSubst
#endif

#endif // ES_STRUTILS_H

