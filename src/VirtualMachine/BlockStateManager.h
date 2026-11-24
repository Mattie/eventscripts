#ifndef BLOCKSTATEMGR_H
#define BLOCKSTATEMGR_H
#pragma once


#include "tier1/utlstack.h"
class BlockArgs;

class CBlockStateManager
{
private:
    bool m_bTestFlag;
    bool m_bSkipBlockFlag;
    bool m_bSkipArgBlockPush;
    bool m_bArgBlock;
    BlockArgs * m_pArguments;
    CUtlStack<bool> m_TestFlagStack;
    CUtlStack<BlockArgs *> m_ArgumentStack;
    CUtlStack<bool> m_ArgBlockStack;
    
public:

    CBlockStateManager(void);
    ~CBlockStateManager(void);
    void StartArgBlock(char *pCmdLine);
    void StartBlock();
    void EndBlock(void);
    inline void SetConditionalFlag(bool bTestOutcome);
    inline bool GetConditionalFlag(void);
    inline bool GetSkipBlockFlag(void);
    inline void SetSkipBlockFlag(bool bFlag);

    int Cmd_Argc(void);
    char * Cmd_Argv(int arg);
    char * Cmd_Args(void);
};


inline void CBlockStateManager::SetConditionalFlag(bool bTestOutcome)
{
    m_bTestFlag = bTestOutcome;
}

inline bool CBlockStateManager::GetConditionalFlag(void)
{
    return m_bTestFlag;
}   

inline bool CBlockStateManager::GetSkipBlockFlag(void)
{
    return m_bSkipBlockFlag;
}

inline void CBlockStateManager::SetSkipBlockFlag(bool bFlag)
{
    m_bSkipBlockFlag = bFlag;
    return;
}

#endif

