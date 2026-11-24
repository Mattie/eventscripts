#include "Script.h"
#include "Block.h"
#include "ctype.h"
#include "string.h"
#include "malloc.h"
#include "../es_debug.h"

CScript::CScript(const char * szName)
: m_iNumBlocks(0)
, m_iNumLines(0)
{
    m_szName = strdup(szName);
    m_pBlocks = NULL;
    m_pNextScript = NULL;
    m_pPrevScript = NULL;
    m_bEnabled = true;
}

CScript::~CScript(void)
{
    if (m_szName) free(m_szName);
    CScript *pTemp = this;
    CScript * pNext = pTemp->m_pNextScript;
    if (pNext)
    {
        pNext->m_pPrevScript = pTemp->m_pPrevScript;
    }
    CScript * pPrev = pTemp->m_pPrevScript;
    if (pPrev)
    {
        pPrev->m_pNextScript = pTemp->m_pNextScript;
    }
}

void CScript::IncludeBlock(CBlock *pBlock)
{
    CBlock *pTemp = m_pBlocks;
    if (!pTemp)
    {
        m_pBlocks = pBlock;
        return;
    }

    while (pTemp->m_pNextBlock)
    {
        pTemp = pTemp->m_pNextBlock;
    }
    pTemp->m_pNextBlock = pBlock;
    return;
}
// CreateBlocks(CUtlBuffer * pBuffer, const char *szFunctionName = NULL)
const char* CScript::CreateBlocks(CUtlBuffer * pBuffer, bool bEvent, const char *szFunctionName = NULL)
{
    CBlock *pNewBlock = NULL;
    char szBuffer[MAX_LINE_LENGTH];
    m_iNumBlocks++;
    if (szFunctionName)
    {
        char szName[MAX_BLOCK_NAME];
        sprintf(szName, "%s/%s", m_szName, szFunctionName);
        pNewBlock = new CBlock(szName, this, bEvent);
    }
    else
    {
        // assign a name based on the block#s in the script
        char szName[MAX_BLOCK_NAME];
        sprintf(szName, "%s.b%d", m_szName, m_iNumBlocks);
        pNewBlock = new CBlock(szName, this);
    }

    // read a line in
    pBuffer->GetLine(szBuffer, MAX_LINE_LENGTH);
    char * pLine = szBuffer;
    m_iNumLines++;
    DxMsg(6, __FUNCTION__, __LINE__, "CreateBlocks# %s\n", pLine);
    while (pLine[0]!='\0')
    {
        while(*pLine==' ' || *pLine=='\t') pLine++;
        if (!(*pLine=='/' && *(pLine+1)=='/'))
        {
            // if we find a } we know we're done with the block, wrap it up
            if (*pLine=='}') break;;
            if (*pLine=='{') {
                const char * pBlockName = NULL;
                pBlockName = CreateBlocks(pBuffer, false);
                if (pBlockName)
                {
                    char szCommand[MAX_LINE_LENGTH];
                    sprintf(szCommand, "es__db %s\n", pBlockName);
                    pNewBlock->AddLine(szCommand, m_iNumLines);
                }
            }
            else
            {
                char szCommand[MAX_LINE_LENGTH];
                sprintf(szCommand, "%s\n", pLine);
                pNewBlock->AddLine(szCommand, m_iNumLines);
            }
        }
        pBuffer->GetLine(szBuffer, MAX_LINE_LENGTH);
        pLine = szBuffer;
        m_iNumLines++;
        DxMsg(6, __FUNCTION__, __LINE__, "CreateBlocks# %s\n", pLine);
    }
    if (pNewBlock)
    {
        pNewBlock->Close();
        IncludeBlock(pNewBlock);
        return pNewBlock->GetName();
    }
    else
        return NULL;
}

// parse the file
// TODO: grab the script attributes
// loop through functions file
//    CreateFunctionAndBlocks()
/*
                    KeyValues *pEventKey = pNewKey->FindKey(szEventName, true);
                    int i =1;
                    char szName[199];
                    sprintf(szName, "es__startblock %s/%s\n", szScriptFile, szEventName);
                    pEventKey->SetString("0", szName);
                    pBuffer->Scanf("%s\n", szBuffer);
                    int nBlockCount = 0;
                    if (0!=strcmp(szBuffer, "{\n")) {
                        Msg("Error parsing %s, missing open brace after event name\n", szScriptFile);
                        return;
                    }
                    else {
                        nBlockCount++;
                        pBuffer->Scanf("%s\n", szBuffer);
                    }
                    while (!filesystem->EndOfFile(fFile) && nBlockCount>0)
                    {
                        i++;
                        char * pTrim = trim(szBuffer);
                        char szValue[8025];
                        
                        if (*pTrim=='{' )
                        {
                            nBlockCount++;
                            sprintf(szValue, "es__startblock %s/%s.%d\n", szScriptFile, szEventName, i);
                        }
                        else if (*pTrim=='}')
                        {
                            nBlockCount--;
                            strcpy(szValue, "es__endblock\n");
                        }
                        else {
                            strcpy(szValue, pTrim);
                            strcat(szValue, "\n");
                        }
                        sprintf(szName, "%d", i);
                        pNewKey->SetString(szName, szValue);
                        D1Msg("%s wrote: %s\n", szEventName, szValue);


*/


char * CScript::trim(char *szToTrim)
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

bool CScript::LoadFromBuffer(CUtlBuffer * pBuffer)
{
    int i = 0;
    //while not end of file
    char szBuffer[8500];
    pBuffer->GetLine(szBuffer, 8500);
    m_iNumLines++;
    while (szBuffer[0]!='\0')
    {
        // read a line
        // TODO: read attributes

        // if line begins with event
        int nLoc = 0;
        bool bEvent = false;
        if (strncmp(szBuffer, "event ", sizeof("event"))==0)
        {
            bEvent = true;
        }
        if (bEvent || strncmp(szBuffer, "block ", sizeof("block"))==0)
        {
			
            //   parse the event name 
            char * pFind = 0;
            if (pFind = strstr(szBuffer, " "))
            {
                char szFunctionName[100];
                while (isspace(*pFind)) pFind++;
                strcpy(szFunctionName, pFind);
                pBuffer->GetLine(szBuffer, 8500); // throw away the open block.
                m_iNumLines++;
                CreateBlocks(pBuffer, bEvent, trim(szFunctionName));
            }
        }
        else
        {
            // bad stuff happened.
        }
        pBuffer->GetLine(szBuffer, 8500);
        m_iNumLines++;
    }
    return true;
}
