#pragma once
#include "utlstack.h"

class CConditionalStateManager
{
private:
    bool bTestFlag;
    CUtlStack<bool> TestFlagStack;

public:

    CConditionalStateManager(void);
    ~CConditionalStateManager(void);

    inline void StartConditionalBlock(void)
    {
        TestFlagStack.Push(bTestFlag);
    }

    inline void EndConditionalBlock(void)
    {
        TestFlagStack.Pop(bTestFlag);        
    }

    inline void SetConditionalFlag(bool bTestOutcome)
    {
        bTestFlag = bTestOutcome;
    }

    inline bool GetConditionalFlag(void)
    {
        return bTestFlag;
    }    

};
