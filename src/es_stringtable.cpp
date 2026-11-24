#include "eiface.h"
#include "networkstringtabledefs.h"
#include "es_cmddef.h"
#include "es_debug.h"



extern INetworkStringTableContainer * networkstringtable;
extern IVEngineServer * engine;

void DumpStringTableItem(const char *table, const char *path) {
	INetworkStringTable *pDownloadablesTable = networkstringtable->FindTable(table);
	if (!pDownloadablesTable)
		return;
    int nIndex = pDownloadablesTable->FindStringIndex(path);
	if (nIndex != INVALID_STRING_INDEX) {
        DxMsg(0, __FUNCTION__, __LINE__, "Data:\n%s\n", pDownloadablesTable->GetString(nIndex));
        int size = 40;
        DxMsg(0, __FUNCTION__, __LINE__, "Data:\n%s\n", pDownloadablesTable->GetStringUserData(nIndex, &size));
//		bool save = engine->LockNetworkStringTables(false);
//		pDownloadablesTable->AddString(path, strlen(path)+1);
//		engine->LockNetworkStringTables(save);
	}
	return;
}

bool AddStringTableItem(const char *table, const char *path) {
	INetworkStringTable *pDownloadablesTable = networkstringtable->FindTable(table);
	if (!pDownloadablesTable)
		return false;
	if (pDownloadablesTable->FindStringIndex(path) == INVALID_STRING_INDEX) {
		bool save = engine->LockNetworkStringTables(false);
#ifdef ORANGEBOX
		pDownloadablesTable->AddString(false, path, strlen(path)+1);
#else
		pDownloadablesTable->AddString(path, strlen(path)+1);
#endif
		engine->LockNetworkStringTables(save);
	}
	return true;
}

ES_CON_COMMAND2(stringtable,
               "Update an entry in a stringtable",
               2,
               "<tablename> <string>")
    if (AddStringTableItem(args.Arg(1), args.Arg(2)))
    {
        DxMsg(1, __FUNCTION__, __LINE__, "Added string: %s to table %s\n", args.Arg(2), args.Arg(1));
    }
    else
    {
        DxMsg(0, __FUNCTION__, __LINE__, "Could not add strings: %s to table %s\n", args.Arg(2), args.Arg(1));
    }

}

ES_CON_COMMAND2(dumpstringtable,
               "Outputs a specific string table item",
               2,
               "<tablename> <string>")
    DumpStringTableItem(args.Arg(1), args.Arg(2));
}


/*// Bytewise
#define WRITE_BYTE		(MessageWriteByte)
#define WRITE_CHAR		(MessageWriteChar)
#define WRITE_SHORT		(MessageWriteShort)
#define WRITE_WORD		(MessageWriteWord)
#define WRITE_LONG		(MessageWriteLong)
#define WRITE_FLOAT		(MessageWriteFloat)
#define WRITE_ANGLE		(MessageWriteAngle)
#define WRITE_COORD		(MessageWriteCoord)
#define WRITE_VEC3COORD	(MessageWriteVec3Coord)
#define WRITE_VEC3NORMAL (MessageWriteVec3Normal)
#define WRITE_ANGLES	(MessageWriteAngles)
#define WRITE_STRING	(MessageWriteString)
#define WRITE_ENTITY	(MessageWriteEntity)*/
// usermsg create 
// usermsg write byte 
// usermsg write char
// usermsg write short
// usermsg write long
// usermsg write float
// usermsg write point
// usermsg write vector
// usermsg write string
// usermsg write entity
// usermsg send
// usermsg delete 

void dumpstringtablenames()
{
    int TableNum = networkstringtable->GetNumTables();
    for(int i =0;i<TableNum;i++)
    {
        INetworkStringTable * pTemp = networkstringtable->GetTable(i);
        if (pTemp)
        {
            DxMsg(0, __FUNCTION__, __LINE__, "StringTable: %s\n", pTemp->GetTableName());
            for (int j = 0;j<pTemp->GetNumStrings();j++)
            {
                DxMsg(0, __FUNCTION__, __LINE__, " --- \"%s\"\n", pTemp->GetString(j));
            }
        }
    }
}
CON_COMMAND(es_dumpstringtables, "Dump string tables.")
{
    dumpstringtablenames();
}

