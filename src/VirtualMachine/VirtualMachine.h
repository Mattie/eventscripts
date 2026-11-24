#pragma once

#include "BlockStateManager.h"
#include "ExecutionManager.h"
// CVirtualMachine
//    registers
//      KeyValue *pLastReturn
//      pBlockenvironment
//    conditional-stack
//    block-stack
//    bool GetTestFlag()
//    bool GetLastReturn()
//    void SetLastReturn(KeyValue *pLastReturn)

//    TraceStateManager
//      szCurFuncname
//      TraceStack
//
//    ConditionalStateManager
//       bool bLastTest
//       ConditionalStack
//       
//    BlockStateManager
//       TBD

//    // holds functions in memory
//    // also knows how to handle actual ServerExecute sorts of things.
//    ExecutionManager
//        instructionpointer
//        LinkedList Scripts
//        HashTable Blocks
//
//    Script
//        Attributes
//        LinkedList of blocks
//    
//    Block
//        KeyValue *Lines





class CVirtualMachine
{
public:
    CVirtualMachine(void);
    ~CVirtualMachine(void);
private:
    CBlockStateManager m_BlockMgr;
    CExecutionManager * m_pExecMgr;
    CScriptManager m_ScriptMgr;
    //CTraceStateManager TraceMgr;
public:
    int Cmd_Argc(void) {
        return m_BlockMgr.Cmd_Argc();
    }

    char * Cmd_Argv(int arg) {
        return m_BlockMgr.Cmd_Argv(arg);
    }
    char * Cmd_Args(void) {
        return m_BlockMgr.Cmd_Args();
    }
    void StartArgBlock(char *szCommand)
    {
        return m_BlockMgr.StartArgBlock(szCommand);
    }
    void StartBlock()
    {
        return m_BlockMgr.StartBlock();
    }

    void EndBlock(void)
    {
        return m_BlockMgr.EndBlock();
    }

    void SetConditionalFlag(bool bTestOutcome)
    {
        return m_BlockMgr.SetConditionalFlag(bTestOutcome);
    }

    bool GetConditionalFlag(void)
    {
        return m_BlockMgr.GetConditionalFlag();
    }

    void SetSkipBlockFlag(bool bNextBlockIsConditional)
    {
        return m_BlockMgr.SetSkipBlockFlag(bNextBlockIsConditional);    
    }

    bool GetSkipBlockFlag(void)
    {
        return m_BlockMgr.GetSkipBlockFlag();    
    }

    const CScript * GetScriptList(void)
    {
        return m_ScriptMgr.GetScriptList();
    }

    bool CVirtualMachine::RemoveScript(const char * szName);
    // AddScript()

    void EnableScript(const char *szName)
    {
        m_ScriptMgr.SetScriptEnabled(szName, true);
    }

    void DisableScript(const char *szName)
    {
        m_ScriptMgr.SetScriptEnabled(szName, false);
    }

    bool AddScript(CScript * pNewScript);

    // You have to delete this yourself or it will leak
    char * CreateCmdStringForAllFunctions(const char * szFunctionName)    {
        if (m_pExecMgr)
            return m_pExecMgr->CreateCmdStringForAllFunctions(szFunctionName);
        else
            return NULL;
    }
    bool AnyBlockExistsForEvent(const char *szEventName)    {
        if (m_pExecMgr)
            return m_pExecMgr->AnyBlockExistsForEvent(szEventName);
        else
            return false;
    }
    bool ScriptExists(const char *szScriptName)    {
        return m_ScriptMgr.FindScript(szScriptName) == NULL ? false : true;
    }

    bool BlockExists(const char *szBlockName)    {
        return m_ScriptMgr.GetBlock(szBlockName) == NULL ? false : true;
    }


    bool ExecBlock(const char *pBlockName, VMInstruction *pOut, char *szCommandLine= NULL);
    bool ExecEvent(unsigned int nBlock, VMInstruction *pOut);
    bool ExecNextInstruction(void *pInstruction, VMInstruction *pOutNext);
    
};



// iff (1) 
// pushcond
//    iff (1)
//    pushcond
//    popcond
//    else
//    pushcond
//    popcond
// popcond

