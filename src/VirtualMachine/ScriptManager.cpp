#include "ScriptManager.h"
#include "../es_debug.h"



bool HashBDataCompare(const HashBData &p1, const HashBData &p2)
{
    return (0==strcmp(p1.name, p2.name));
}
unsigned int HashBKeyFunc(const HashBData &p)
{
    char *str = p.name;

    unsigned int hash = 5381;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return hash;
}

CScriptManager::CScriptManager(void)
{
    m_pScripts = NULL;
    m_pBlocks = new CUtlHash<HashBData>(65536,0,0,HashBDataCompare, HashBKeyFunc);
    m_pEvents = new CUtlHash<HashBData>(65536,0,0,HashBDataCompare, HashBKeyFunc);
}

CScriptManager::~CScriptManager(void)
{
    delete m_pBlocks;
    delete m_pEvents;
//  TODO: delete all of the scripts in memory
}

CBlock * CScriptManager::GetBlock(const char * szBlockName)
{
    DxMsg(5, __FUNCTION__, __LINE__, "GetBlock: \"%s\".\n", szBlockName);
    HashBData pNew;
    pNew.name = (char *)szBlockName;
    pNew.pBlock = NULL;

    
    UtlHashHandle_t hKey = m_pBlocks->Find(pNew);
    if (hKey != m_pBlocks->InvalidHandle())
    {
        DxMsg(3, __FUNCTION__, __LINE__, "Found: %s\n", szBlockName);
        return m_pBlocks->Element(hKey).pBlock;
    }
    else
    {
        DxMsg(1, __FUNCTION__, __LINE__, "GetBlock. Not found: %s\n", szBlockName);
        return NULL;
    }
}

unsigned int CScriptManager::GetEventHash(const char * szBlockName)
{
    DxMsg(5, __FUNCTION__, __LINE__, "GetEventHash: \"%s\".\n", szBlockName);
    HashBData pNew;
    pNew.name = (char *)szBlockName;
    pNew.pBlock = NULL;

    
    UtlHashHandle_t hKey = m_pBlocks->Find(pNew);
    if (hKey != m_pBlocks->InvalidHandle())
    {
        DxMsg(3, __FUNCTION__, __LINE__, "Found: %s\n", szBlockName);
        return (unsigned int)hKey;
    }
    else
    {
        DxMsg(1, __FUNCTION__, __LINE__, "GetEventHash. Not found: %s\n", szBlockName);
        return 0;
    }
}

CBlock * CScriptManager::GetEvent(unsigned int nBlockIndex)
{
    UtlHashHandle_t hKey = (UtlHashHandle_t)nBlockIndex;
    if (m_pBlocks->IsValidHandle(hKey))
    {
        DxMsg(3, __FUNCTION__, __LINE__, "GetEvent Found: %d\n", nBlockIndex);
        return m_pBlocks->Element(hKey).pBlock;
    }
    else
    {
        DxMsg(1, __FUNCTION__, __LINE__, "GetEvent. Not found: %d\n", nBlockIndex);
        return NULL;
    }
}

void CScriptManager::AddBlocks(CScript *pScript)
{
	DxMsg(4, __FUNCTION__, __LINE__, "AddBlocks: started\n");
    if (!pScript) return;
    CBlock *pTemp = pScript->m_pBlocks;
    if (!pTemp) return;
    while (pTemp)
    {
        // insert it into the overall block hashtable
        HashBData *pNew = new HashBData();
        pNew->name = strdup(pTemp->GetName());
        pNew->pBlock = pTemp;
        DxMsg(4, __FUNCTION__, __LINE__, "AddBlocks: %s\n", pNew->name);
        m_pBlocks->Insert(*pNew);

        // if it's an event we need to add it to the event hashtable
        if (pTemp->IsEventBlock())
        {
            DxMsg(4, __FUNCTION__, __LINE__, "Is an event block: %s\n", pTemp->m_pBaseName);
            // search for the event in the hashtable
            HashBData pEventBlocks;
            pEventBlocks.name = pTemp->m_pBaseName;
            pEventBlocks.pBlock = NULL;
            UtlHashHandle_t hKey = m_pEvents->Find(pEventBlocks);
            // is there already a bucket?
            if (hKey != m_pEvents->InvalidHandle())
            {
                // yes! put it in that bucket
                DxMsg(4, __FUNCTION__, __LINE__, "AddBlocks Event Bucket Found! %s\n", pTemp->m_pBaseName);
                CBlock *pBucket = m_pEvents->Element(hKey).pBlock;
                while (pBucket->m_pNextEventBlock)
                {
                    DxMsg(4, __FUNCTION__, __LINE__, "---Bucket: %s\n", pBucket->GetName());
                    pBucket = pBucket->m_pNextEventBlock;
                }
                DxMsg(4, __FUNCTION__, __LINE__, "---Bucket. %s\n", pBucket->GetName());

                pBucket->m_pNextEventBlock = pTemp;
                pTemp->m_pPrevEventBlock = pBucket;
                
            }
            else
            {
                // no! create the bucket
                HashBData *pNewEvent = new HashBData();
                pNewEvent->name = strdup(pTemp->m_pBaseName);
                pNewEvent->pBlock = pTemp;
                m_pEvents->Insert(*pNewEvent);
                DxMsg(3, __FUNCTION__, __LINE__, "AddEvents bucket created: %s - %d\n", pNewEvent->name, m_pEvents->Count());
                pTemp->m_pNextEventBlock = NULL;
                pTemp->m_pPrevEventBlock = NULL;
            }
            
        }
        pTemp = pTemp->m_pNextBlock;
    }

}

bool CScriptManager::AddScript(CScript *pScript)
{
	DxMsg(4, __FUNCTION__, __LINE__, "AddScript begins\n");
    if (!pScript) return false;

    
    if (!FindScript(pScript->GetName()))
    {
        CScript *pTemp = m_pScripts;
        if (!pTemp)
        {
            m_pScripts = pScript;
        }
        else
        {
            while (pTemp->m_pNextScript)
            {
                pTemp = pTemp->m_pNextScript;
            }
            pTemp->m_pNextScript = pScript;
            pScript->m_pPrevScript = pTemp;
        }
        AddBlocks(pScript);
        return true;
    }
    else
    {
        return false;
    }
}

CScript * CScriptManager::FindScript(const char *szName)
{
    if (!szName)
        return NULL;

    CScript *pTemp = m_pScripts;
    while (pTemp)
    {
        if (0==stricmp(pTemp->GetName(), szName))
        {
            return pTemp;
        }
        pTemp = pTemp->m_pNextScript;
    }
    return NULL;
}

void CScriptManager::SetScriptEnabled(const char * szName, bool bEnable)
{

    CScript *pTemp = FindScript(szName);
    if (pTemp)
    {
        if (bEnable)
            pTemp->Enable();
        else
            pTemp->Disable();
    }
}

bool CScriptManager::RemoveScript(const char * szScriptName)
{
    CScript *pTemp = FindScript(szScriptName);
    if (pTemp)
    {
        CBlock *pB = pTemp->m_pBlocks;
        while (pB)
        {
            HashBData pNew;
            pNew.name = (char *)pB->GetName();
            pNew.pBlock = NULL;
            // find it
            UtlHashHandle_t hKey = m_pBlocks->Find(pNew);
            if (hKey != m_pBlocks->InvalidHandle())
            {
                // remove it
                m_pBlocks->Remove(hKey);                
            }
            if (pB->IsEventBlock())
            {
                DxMsg(4, __FUNCTION__, __LINE__, "%s: is an event\n", pB->GetName());
                // remove it from the event queue
                if (pB->m_pPrevEventBlock)
                {
                    // the parent's next should be set our next so we can leave the chain.
                    pB->m_pPrevEventBlock->m_pNextEventBlock = pB->m_pNextEventBlock;
                    DxMsg(4, __FUNCTION__, __LINE__, "%s: the parent's (%s) next should be our next so we can leave the chain.\n", pB->GetName(), pB->m_pPrevEventBlock->GetName());
                    if (pB->m_pNextEventBlock)
                    {
                        // change our child's prev block to point to our former parent
                        pB->m_pNextEventBlock->m_pPrevEventBlock = pB->m_pPrevEventBlock;
                        DxMsg(3, __FUNCTION__, __LINE__, "%s: change our child's (%s) prev block to point to our former parent\n", pB->GetName(), pB->m_pNextEventBlock->GetName());
                    }
                } 
                else 
                {
                    DxMsg(4, __FUNCTION__, __LINE__, "%s: head event, we have to remove the hash entry\n", pB->GetName());
                    // head event, we have to remove the hash entry
                    HashBData pEventBlocks;
                    pEventBlocks.name = pB->m_pBaseName;
                    pEventBlocks.pBlock = NULL;
                    UtlHashHandle_t hKey2 = m_pEvents->Find(pEventBlocks);
                    // is there already a bucket?
                    if (hKey2 != m_pEvents->InvalidHandle())
                    {
                        // get rid of it
                        m_pEvents->Remove(hKey2);
                    }
                }
                // if we were the head, we have to move the head
                if (!pB->m_pPrevEventBlock && pB->m_pNextEventBlock)
                {
                    DxMsg(4, __FUNCTION__, __LINE__, "%s: if we were the head, we have to move the head\n", pB->GetName());
                    HashBData *pNewEvent = new HashBData();
                    pNewEvent->name = strdup(pB->m_pNextEventBlock->m_pBaseName);
                    pNewEvent->pBlock = pB->m_pNextEventBlock;
                    DxMsg(3, __FUNCTION__, __LINE__, "AddEvents: %s\n", pNewEvent->name);
                    m_pEvents->Insert(*pNewEvent);
                    pB->m_pNextEventBlock->m_pPrevEventBlock = NULL;
                }
            }
            // delete it
            CBlock *pNext = pB->m_pNextBlock;
            delete pB;
            // find the next
            pB = pNext;
        }
        if (m_pScripts == pTemp)
        {
            m_pScripts = m_pScripts->m_pNextScript;
        }
        delete pTemp;
        return true;
    }
    return false;
}
// Gets the first block in the linked list of events, NULL is no one registered
CBlock * CScriptManager::GetEventBlocks(const char * szEventName)
{
    // search for the event in the hashtable
    HashBData pEventBlocks;
    pEventBlocks.name = (char *)szEventName;
    pEventBlocks.pBlock = NULL;
    UtlHashHandle_t hKey = m_pEvents->Find(pEventBlocks);
    // is there already a bucket?
    if (hKey != m_pEvents->InvalidHandle())
    {
        return m_pEvents->Element(hKey).pBlock;
    }
    else
        return NULL;
}
