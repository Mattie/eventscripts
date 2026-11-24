#include "VirtualMachine.h"

CVirtualMachine::CVirtualMachine(void)
{
    m_pExecMgr = new CExecutionManager(&m_ScriptMgr, &m_BlockMgr);
}

CVirtualMachine::~CVirtualMachine(void)
{
    delete m_pExecMgr;
}

bool CVirtualMachine::RemoveScript(const char * szName)
{
    return m_ScriptMgr.RemoveScript(szName);
}

bool CVirtualMachine::AddScript(CScript * pNewScript)
{
    return m_ScriptMgr.AddScript(pNewScript);
}


bool CVirtualMachine::ExecBlock(const char *pNewScript, VMInstruction *pOut, char *szCmdLine)
{
    if (m_pExecMgr )
    {
        if (m_BlockMgr.GetSkipBlockFlag())
        {
            m_BlockMgr.SetSkipBlockFlag(false);
            // skip the block!
        }
        else
        {
            return m_pExecMgr->ExecBlock(pNewScript, pOut, szCmdLine); 
        }
    }

    return false;
}

bool CVirtualMachine::ExecEvent(unsigned int nBlock, VMInstruction *pOut)
{
    if (m_pExecMgr )
    {
       return m_pExecMgr->ExecEvent(nBlock, pOut); 
    }

    return false;
}

bool CVirtualMachine::ExecNextInstruction(void *pInstruction, VMInstruction *pOutNext)
{
    // return the line and a pointer to the next instruction (if any)
    if (m_pExecMgr)
        return m_pExecMgr->ExecNextInstruction(pInstruction, pOutNext); 
    else
        return false;
}

