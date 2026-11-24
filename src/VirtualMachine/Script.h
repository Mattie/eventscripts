#ifndef SCRIPT_H
#define SCRIPT_H
#pragma once
#include "utlbuffer.h"
#define NULL 0

class CBlock;

class CScript
{
public:
    CScript(const char *szName);
    ~CScript(void);
    bool LoadFromBuffer(CUtlBuffer * pBuffer);
    const char * CreateBlocks(CUtlBuffer * pBuffer, bool bEvent, const char *szFunctionName);
    void IncludeBlock(CBlock *pBlock);
    CBlock *m_pBlocks;
    CScript * m_pNextScript;
    CScript * m_pPrevScript;
    const char * GetName() {
        return m_szName;
    }
    void Enable() {
        m_bEnabled = true;
    }
    void Disable() {
        m_bEnabled = false;
    }
    bool IsEnabled() {
        return m_bEnabled;
    }

private:
    char *m_szName;
    int m_iNumBlocks;
    bool m_bEnabled;

    char * trim(char *szToTrim);
public:
    int m_iNumLines;
};

#endif
