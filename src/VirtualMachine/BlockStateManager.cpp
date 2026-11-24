#define NULL 0
#include "BlockStateManager.h"
#include "BlockArgs.h"

CBlockStateManager::CBlockStateManager(void) : m_bTestFlag(false), m_bSkipBlockFlag(false)
{
    m_bArgBlock = false;
    m_bSkipArgBlockPush = false;
    // placeholder argument block
    m_pArguments = new BlockArgs();
}

CBlockStateManager::~CBlockStateManager(void)
{
    if (m_pArguments) delete m_pArguments;
}

void CBlockStateManager::StartArgBlock(char *pCmdLine)
{
    m_ArgumentStack.Push(m_pArguments);
    m_pArguments = new BlockArgs();
    if (pCmdLine)
    {
        m_pArguments->Cmd_TokenizeString(pCmdLine);
    }
    m_ArgBlockStack.Push(m_bArgBlock);
    m_bArgBlock = true;
    m_bSkipArgBlockPush = true;
}

void CBlockStateManager::StartBlock(void)
{
    m_TestFlagStack.Push(m_bTestFlag);
    // argh-- the different block starters need to talk to one another
    if (m_bSkipArgBlockPush)
    {
        m_bSkipArgBlockPush = false;
    }
    else
    {
        m_ArgBlockStack.Push(m_bArgBlock);
        m_bArgBlock = false;
    }
}

void CBlockStateManager::EndBlock(void)
{
    m_TestFlagStack.Pop(m_bTestFlag); 

    if (m_bArgBlock)
    {
        delete m_pArguments;
        m_pArguments = NULL;

        if (m_ArgumentStack.Count() > 0)
            m_ArgumentStack.Pop(m_pArguments);
    }
    m_ArgBlockStack.Pop(m_bArgBlock);
}




int CBlockStateManager::Cmd_Argc(void)
{
    if (m_pArguments)
        return m_pArguments->Cmd_Argc();
    else
        return 0;
}

char * CBlockStateManager::Cmd_Argv(int arg)
{
    if (m_pArguments)
        return m_pArguments->Cmd_Argv(arg);
    else
        return NULL;
}


char * CBlockStateManager::Cmd_Args(void)
{
    if (m_pArguments)
        return m_pArguments->Cmd_Args();
    else
        return NULL;
}
