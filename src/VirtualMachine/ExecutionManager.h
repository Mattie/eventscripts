#pragma once

#include "utlstack.h"
#include "utlhash.h"
#include "ScriptManager.h"
#include "KeyValues.h"
#include "BlockStateManager.h"

#define MAX_COMMAND_STRING 2048

//    // holds functions in memory
//    // also knows how to handle actual ServerExecute sorts of things.
//    ExecutionManager
//        instructionpointer
//        LinkedList Scripts
//        HashTable Blocks

struct VMInstruction {
    char * szCommand;
    int nIP;
    char * szLineNum;
};

class CExecutionManager
{
public:
    CExecutionManager(CScriptManager *pScriptMgr, CBlockStateManager *pBlockMgr);
    ~CExecutionManager(void);
    char * CreateCmdStringForAllFunctions(const char * szFunctionName);
    bool AnyBlockExistsForEvent(const char *szEventName);
    bool ExecBlock(const char *pBlockName, VMInstruction *pOut, char *szCommandLine = NULL);
    bool ExecEvent(unsigned int nBlockHandle, VMInstruction *pOut);
    bool ExecNextInstruction(void *pInstruction, VMInstruction *pOutNext, bool bSkipBlock = false);
private:
    //KeyValues *pNextStatement;
    //CUtlStack<KeyValues *> CallStack;
    CScriptManager *m_pScriptMgr;
    CBlockStateManager *m_pBlockMgr;
};



