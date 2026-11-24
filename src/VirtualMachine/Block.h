#ifndef BLOCK_H
#define BLOCK_H
#pragma once

#define MAX_BLOCK_NAME 512
#define MAX_LINE_LENGTH 3000
#define LINE_NUMBER_STRING_FORMAT "es_%s.txt"

class CScript;
class CCodeLine;

class CBlock
{
public:

    CBlock(const char *szName, CScript *pFromScript, bool bEvent=false);
    ~CBlock(void);
    
    // returns the name for the block
    const char *GetName() {
        return m_szName;
    }
    CCodeLine * GetCode()
    {
        return m_pCode;
    }
    bool IsEventBlock();
    int AddLine(const char * szLine, int nLineNum);
    int Close();
    // parent script
    CScript *m_pScript;
    CBlock * m_pNextBlock;
    CBlock * m_pPrevBlock;
    // linked list of event blocks, NULL if none.
    CBlock * m_pNextEventBlock;
    CBlock * m_pPrevEventBlock;
    // blocks final part of name (e.g. "player_hurt")
    char *m_pBaseName;
private:
    CCodeLine *m_pCode;
    CCodeLine *m_pTail;
    int m_iLines;
    bool m_bClosed;
    // whether the block is for an event
    bool m_bEventBlock;
    char m_szName[MAX_BLOCK_NAME];

    inline void AppendLine(int nLine, const char *szLine);
};


class CCodeLine
{
public:
    char *m_szCode;
    CBlock *m_pContainingBlock;
    CCodeLine * m_pNextLine;
    char *m_szLineNumberString;
    int m_nLine;
    CCodeLine(const char *szCode, int nLine, CBlock *pContainer);
    CCodeLine::~CCodeLine();
    const char *GetString();
    CCodeLine *GetNextLine();
    const char *GetLineNumberString();
    int GetLineNumber();
};


#endif
