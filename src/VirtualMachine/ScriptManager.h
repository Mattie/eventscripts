#pragma once

#include "utlhash.h"
#include "Script.h"
#include "Block.h"

//typedef bool (*CompareFunc_t)( Data const&, Data const& );
// use a hash table to store the name of console variables

struct HashBData {
    char * name;
    CBlock *pBlock;
};

//g_Scripts

class CScriptManager
{
public:
    CScriptManager(void);
    ~CScriptManager(void);
    bool AddScript(CScript *pScript);
    bool RemoveScript(const char * szScriptName);
    CScript * CScriptManager::FindScript(const char *szName);
    void SetScriptEnabled(const char * szName, bool bEnabled);
    CBlock * GetBlock(const char * szBlockName);
    CBlock * GetEvent(unsigned int nBlockIndex);
    unsigned int GetEventHash(const char * szBlockName);
    const CScript * GetScriptList(){
        return m_pScripts;
    }
private:
    CScript *m_pScripts;
    CUtlHash<HashBData> * m_pBlocks;
    CUtlHash<HashBData> * m_pEvents;
    void AddBlocks(CScript *pScript);

public:
    // Gets the first block in the linked list of events, NULL is no one registered
    CBlock * GetEventBlocks(const char * szEventName);
};
