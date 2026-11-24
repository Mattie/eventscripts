#include "Block.h"
#include "Script.h"
#include "strtools.h"
//#define NULL 0


extern inline void DxMsg(int nLevel, const char* szFunction, int nLine, const tchar *pszFormat, ...);

CBlock::CBlock(const char * szName, CScript *pFromScript, bool bEvent)
{
    DxMsg(5, __FUNCTION__, __LINE__, "Name: %s\n", szName);
    m_pBaseName = NULL;
    if (szName)
    {
        strcpy(m_szName, szName);
        m_pBaseName = strrchr(m_szName, '/')+1; 
        if (!m_pBaseName) m_pBaseName = m_szName;
        DxMsg(5, __FUNCTION__, __LINE__, "CBlock: Basename: %s\n", szName);

    }
    m_iLines = 0;
    m_bClosed = false;
    m_pScript = pFromScript;
    m_pNextBlock = NULL;
    m_pPrevBlock = NULL;
    m_pNextEventBlock = NULL;
    m_pPrevEventBlock = NULL;
    m_bEventBlock = bEvent;

    char szText[MAX_LINE_LENGTH];
    Q_snprintf(szText, MAX_LINE_LENGTH, "es__sb %s\n", m_szName);
    m_pCode = new CCodeLine(szText, pFromScript->m_iNumLines, this);
    m_pTail = m_pCode;
}

CBlock::~CBlock(void)
{
    // loop through and delete all code lines
    while (m_pCode)
    {
        CCodeLine *pTemp = m_pCode->m_pNextLine;
        delete m_pCode;
        m_pCode = pTemp;
    }
}


inline void CBlock::AppendLine(int nLine, const char *szLine)
{
    CCodeLine *pTemp = new CCodeLine(szLine, nLine, this);
    m_pTail->m_pNextLine = pTemp;
    m_pTail = pTemp;
    DxMsg(7, __FUNCTION__, __LINE__, "Added block line #%d to block %s:\n  %s", nLine, m_szName, szLine);
}

int CBlock::AddLine(const char * szLine, int nLineNum)
{
    if (m_bClosed || !szLine)
        return -1;
    // add a line to the block if it isn't closed yet
    m_iLines++;
    AppendLine(nLineNum, szLine);
    return m_iLines;
}

int CBlock::Close()
{
    AddLine("es__eb\n\n", -1);
    AddLine("\n", -1);
    m_bClosed = true;
    return m_iLines;
}

bool CBlock::IsEventBlock()
{
    return m_bEventBlock;
}



CCodeLine::CCodeLine(const char *szCode, int nLine, CBlock *pContainer)
{
    m_szCode = strdup(szCode);
    m_pContainingBlock = pContainer;
    m_pNextLine = NULL;
    m_nLine = nLine;
    
    char szFormat[MAX_LINE_LENGTH];
    if (m_pContainingBlock && m_pContainingBlock->m_pScript)
    {
        Q_snprintf(szFormat, MAX_LINE_LENGTH, LINE_NUMBER_STRING_FORMAT, m_pContainingBlock->m_pScript->GetName(), m_nLine);
    }
    else
    {
        Q_snprintf(szFormat, MAX_LINE_LENGTH, LINE_NUMBER_STRING_FORMAT, "unknown", m_nLine);
    }
    m_szLineNumberString = strdup(szFormat);

}
CCodeLine::~CCodeLine()
{
    free(m_szCode);
    free(m_szLineNumberString);
}
const char *CCodeLine::GetString()
{
    return m_szCode;
}
CCodeLine *CCodeLine::GetNextLine()
{
    return m_pNextLine;
}
const char *CCodeLine::GetLineNumberString()
{
    return m_szLineNumberString;
}
int CCodeLine::GetLineNumber()
{
    return m_nLine;
}
