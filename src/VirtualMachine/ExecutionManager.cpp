#include "ExecutionManager.h"
#include "Script.h"

extern inline void DxMsg(int nLevel, const char* szFunction, int nLine, const tchar *pszFormat, ...);

CExecutionManager::CExecutionManager(CScriptManager *pScriptMgr, CBlockStateManager *pBlockMgr)
{
    m_pScriptMgr = pScriptMgr;
    m_pBlockMgr = pBlockMgr;
}

CExecutionManager::~CExecutionManager(void)
{
}

bool CExecutionManager::ExecBlock(const char *pBlockName, VMInstruction *pOut, char *szCommandLine)
{
    CBlock *pBlock = m_pScriptMgr->GetBlock(pBlockName);

    if (pBlock)
    {
        DxMsg(4, __FUNCTION__, __LINE__, "ExecBlock found block: %s;\n", pBlockName);
        CCodeLine *pCode = pBlock->GetCode();
        pOut->szCommand = (char *)pCode->GetString();
        pOut->nIP = (unsigned int)pCode->GetNextLine();
        pOut->szLineNum = (char *)pCode->GetLineNumberString();
        DxMsg(6, __FUNCTION__, __LINE__, "ExecBlock discovered:\n   NextIP: 0x%x\n      Cmd: %s", pOut->nIP, pOut->szCommand);
        if (szCommandLine)
        {
            m_pBlockMgr->StartArgBlock(szCommandLine);
        }
        return true;
    }
    return false;
}

bool CExecutionManager::ExecEvent(unsigned int nBlockHandle, VMInstruction *pOut)
{
    CBlock *pBlock = m_pScriptMgr->GetEvent(nBlockHandle);

    if (pBlock)
    {
        DxMsg(4, __FUNCTION__, __LINE__, "ExecEvent found block: %d;\n", nBlockHandle);
        CCodeLine *pCode = pBlock->GetCode();
        pOut->szCommand = (char *)pCode->GetString();
        pOut->nIP = (unsigned int)pCode->GetNextLine();
        pOut->szLineNum = (char *)pCode->GetLineNumberString();
        DxMsg(6, __FUNCTION__, __LINE__, "ExecEvent discovered:\n   NextIP: 0x%x\n      Cmd: %s", pOut->nIP, pOut->szCommand);
        return true;
    }
    return false;
}

bool CExecutionManager::ExecNextInstruction(void *pInstruction, VMInstruction *pOutNext, bool bSkipBlock)
{
    // return the line and a pointer to the next instruction (if any)
    if (pInstruction)
    {
        CCodeLine *pCode = (CCodeLine *) pInstruction;
        pOutNext->szCommand = (char *)pCode->GetString();
        pOutNext->nIP = (unsigned int)pCode->GetNextLine();
        pOutNext->szLineNum = (char *)pCode->GetLineNumberString();
        DxMsg(6, __FUNCTION__, __LINE__, "ExecNextInstruction discovered:\n   NextIP: 0x%x\n", pOutNext->nIP);
        if (pOutNext->nIP == 0)
        {
            return false;
        }
        else
		{
			DxMsg(6, __FUNCTION__, __LINE__, "      Cmd: %s", pOutNext->szCommand);
            return true;
		}
    }
    return false;
}


bool CExecutionManager::AnyBlockExistsForEvent(const char *szEventName)
{
    // loop through all scripts and look for functions of this name
    if (m_pScriptMgr)
    {
        DxMsg(3, __FUNCTION__, __LINE__, "Trying event: %s\n", szEventName);
        return NULL != m_pScriptMgr->GetEventBlocks(szEventName);
    }
    return false;
}

char * CExecutionManager::CreateCmdStringForAllFunctions(const char * szFunctionName)   
{
    if (m_pScriptMgr)
    {
        char szCommandString[MAX_COMMAND_STRING];
        szCommandString[0]='\0';
        CBlock *pEventBlocks = m_pScriptMgr->GetEventBlocks(szFunctionName);
        while (pEventBlocks)
        {
            if (pEventBlocks->m_pScript && pEventBlocks->m_pScript->IsEnabled())
            {
                char szLast[MAX_COMMAND_STRING];
                sprintf(szLast, "es__de %x;", m_pScriptMgr->GetEventHash(pEventBlocks->GetName()));
                strcat(szCommandString, szLast);
            }
            pEventBlocks = pEventBlocks->m_pNextEventBlock;
        }
        strcat(szCommandString, "\n");
        return strdup(szCommandString);
    }
    return NULL;
}
