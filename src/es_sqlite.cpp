#include "es_strutils.h"
#include "es_convar.h"
#include "es_cmddef.h"
#include "utlhash.h"
#include "eiface.h"
#include "KeyValues.h"
#include "es_keygroups.h"

extern IVEngineServer * engine;
extern ConVar scriptdir_cvar;
extern ConVar datadir_cvar;

#include <sqlite3.h>

// use a hash table to store the name of console variables
struct DbHandleHashData {
    char * name;
    sqlite3 *value;
};

bool DbHandleHashDataCompare(const DbHandleHashData &p1, const DbHandleHashData &p2)
{
    return (FStrEq(p1.name, p2.name));
}
unsigned int DbHandleHashKeyFunc(const DbHandleHashData &p)
{
    char *str = p.name;

    unsigned int hash = 5381;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return hash;
}
CUtlHash<DbHandleHashData> g_DbHandleHashTable(65536,0,0,DbHandleHashDataCompare, DbHandleHashKeyFunc);

inline sqlite3 * FindDbHandle(const char * name)
{
    if (name==NULL)
    {
        return NULL;
    }
    DbHandleHashData pFind;
    pFind.name = (char*)name;
    pFind.value = NULL;
    UtlHashHandle_t hKey = g_DbHandleHashTable.Find(pFind);
    if (hKey != g_DbHandleHashTable.InvalidHandle())
    {
        return g_DbHandleHashTable.Element(hKey).value;
    }
    else
    {
        return NULL;
    }
}

bool RemoveDbHandle(const char * name)
{
    if (name==NULL)
    {
        return NULL;
    }
    DbHandleHashData pFind;
    pFind.name = (char*)name;
    pFind.value = NULL;
    UtlHashHandle_t hKey = g_DbHandleHashTable.Find(pFind);
    if (hKey != g_DbHandleHashTable.InvalidHandle())
    {
        g_DbHandleHashTable.Remove(hKey);
        return true;
    }
    else
    {
        return false;;
    }
    
}

KeyValues * g_pSql = NULL;
static int callback(void *NotUsed, int argc, char **argv, char **azColName){
  int i;
  KeyValues * pKey = NULL;
  if (g_pSql) pKey = g_pSql->CreateNewKey();
  for(i=0; i<argc; i++){
    if (pKey)
    {
        pKey->SetString(azColName[i], argv[i] ? argv[i] : "");
    }
    DxMsg(1, __FUNCTION__, __LINE__, "SQL query returned: %s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
  }
  return 0;
}

char * g_pSqlString = NULL;
static int callback_string(void *NotUsed, int argc, char **argv, char **azColName){
    free(g_pSqlString);
    g_pSqlString = strdup(argv[0] ? argv[0] : "");
    DxMsg(1, __FUNCTION__, __LINE__, "SQL query returned: %s = %s\n", azColName[0], argv[0] ? argv[0] : "NULL");
  return 0;
}

#define SQLDB_LOAD_FORMAT "%s/%s/%s/es_%s.sqldb"

// RETURNVAL
// es_sql 
ES_CON_COMMAND2(sql,
               "Local database support\n",
               3,
               "es_xsql open <db> [dbdir]\nes_xsql close <db>\n\nes_xsql queryvalue <db> <single-result-var> \"<SQL-string>\"\nes_xsql query <db> [result-keygroup] \"<SQL-string>\"\n")
    int rc = 0;
    char *zErrMsg = 0;
    const char *command = args.Arg(1);
    const char *databasename = args.Arg(2);
    char szGameDir[100];
    engine->GetGameDir(szGameDir, 100);

    if (FStrEq(command, "query"))
    {
        if (nCount>2)
        {
            const char * query = NULL;
            if (nCount>4)
            {
              query = args.Arg(4);
            }
            else
            {
                query = args.Arg(3);
            }
            sqlite3 * db = FindDbHandle(databasename);
            if (db)
            {
                if (nCount>4)
                {
                    const char * szKeyName = args.Arg(3);
                    g_pSql = GetKeyGroup(szKeyName, true);
                }
                else
                {
                    g_pSql = NULL;
                }
                rc = sqlite3_exec(db, query, callback, 0, &zErrMsg);
                if( rc!=SQLITE_OK ){
                    ErrMsg(0, "SQL error: %s\nYour query was: %s\n\t", zErrMsg, query);
                    ErrVar("SQL error");
                    sqlite3_free(zErrMsg);
                }            
            }
            else
            {
                ErrMsg(0, "Specified database does not exist. Use es_sql open to open a database.\n");
                ErrVar("Invalid database");
            }
        }
    }
    else if (FStrEq(command, "queryvalue"))
    {
        CHANGE_FIRST_ARG_INDEX_FOR_N_RETURNVALS(1)
        if (nCount>4)
        {
            const char * query = args.Arg(nFirstArg+4);
            sqlite3 * db = FindDbHandle(databasename);
            if (db)
            {
                g_pSqlString = NULL;
                rc = sqlite3_exec(db, query, callback_string, 0, &zErrMsg);
                if( rc!=SQLITE_OK ){
                    ErrMsg(0, "SQL error: %s\nYour query was: %s\n\t", zErrMsg, query);
                    ErrVar("SQL error");
                    sqlite3_free(zErrMsg);
                }
                RETURNVAL(args.Arg(3), g_pSqlString, "s");
            }
            else
            {
                ErrMsg(0, "Specified database does not exist. Use es_sql open to open a database.\n");
                ErrVar("Invalid database");
            }
        }
    }
    else if (FStrEq(command, "open"))
    {
        const char * szFilePattern = NULL;
        char szFullPath[MAX_COMMAND_STRING];
        if (nCount<4)
        {
            const char *szScriptDir = strlen(scriptdir_cvar.GetString()) > 0 ? scriptdir_cvar.GetString() : ".";
            sprintf(szFullPath, SQLDB_LOAD_FORMAT, szGameDir, datadir_cvar.GetString(), szScriptDir, databasename);
            DxMsg(1, __FUNCTION__, __LINE__, "es_sql open1 %s\n", szFullPath);
        }
        else
        {
             szFilePattern = args.Arg(3);
            if (*szFilePattern=='|')
            {
                sprintf(szFullPath, SQLDB_LOAD_FORMAT, szGameDir, "addons/eventscripts", szFilePattern+1, databasename);
                DxMsg(1, __FUNCTION__, __LINE__, "es_sql open2 %s\n", szFullPath);
            }
            else if (FStrEq(szFilePattern, ":memory:"))
            {
                strcpy(szFullPath, szFilePattern);
                DxMsg(1, __FUNCTION__, __LINE__, "es_sql open3 %s\n", szFullPath);
            }
            else
            {
                sprintf(szFullPath, SQLDB_LOAD_FORMAT, szGameDir, datadir_cvar.GetString(), szFilePattern, databasename);
                DxMsg(1, __FUNCTION__, __LINE__, "es_sql open4 %s\n", szFullPath);
            }
        }
        sqlite3 * db = NULL;
        rc = sqlite3_open(szFullPath, &db);
        if( rc ){
            DxMsg(0, __FUNCTION__, __LINE__, "Can't open database %s: %s\n", szFullPath, sqlite3_errmsg(db));
            ErrVar("Open failed");
            sqlite3_close(db);
            return;
        }
        DxMsg(1, __FUNCTION__, __LINE__, "es_sql open: %s database opened as %s.\n", szFullPath, databasename);
        DbHandleHashData *pNew = new DbHandleHashData();
        if (pNew)
        {
            pNew->name = strlwr(strdup(databasename));
            pNew->value = db;
            g_DbHandleHashTable.Insert(*pNew);
        }
        else
        {
            DxMsg(0, __FUNCTION__, __LINE__, "es_sql open: Out of memory.\n");
            ErrVar("Out of memory");
            return;
        }

    }
    else if (FStrEq(command, "close"))
    {
        sqlite3 * db = FindDbHandle(databasename);
        if (db)
        {
          sqlite3_close(db);
          RemoveDbHandle(databasename);
          DxMsg(1, __FUNCTION__, __LINE__, "es_sql close: %s database closed.\n", databasename);
        }
    }
}

ES_CON_COMMAND2(dosql, 
               "Does some SQL.\n",
               3,
               "<db> <query>")
  sqlite3 *db;
  char *zErrMsg = 0;
  int rc;

  rc = sqlite3_open(args.Arg(1), &db);
  if( rc ){
    ErrMsg(0, "Can't open database: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return;
  }
  rc = sqlite3_exec(db, args.Arg(2), callback, 0, &zErrMsg);
  if( rc!=SQLITE_OK ){
    ErrMsg(0, "SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
  }
  sqlite3_close(db);
  return;

}


// RETURNVAL_UNSUPPORTED
ES_CON_COMMAND2(formatqv, 
                "Allows you to format a string by filling in a list of strings into a format string.",
                3,
                "<variable> <format-string> [var1] [var2] [...] [varN]")
        //              0           1             2            3       4      ...     N
    const int nTokenStart = 2;          // offset into Cmd_Argv() one ahead of where tokens begin 
    char szFormatted[MAX_FORMATTED];    // to store the formatted string we're building.
    szFormatted[0]='\0';
    char szArgument[MAX_FORMATTED];     // to store the semi-expanded string each time we loop
    strcpy(szArgument, args.Arg(2)); // starts as the full format string

    // Using an algorithm that shro hates because it's so wildly
    // inefficient. I like it because there's so little pointer
    // arithmetic.
    // Loop 1-9 doing a string replace in the format string for %1, %2, etc.
    for (int i = 1;i<=(args.ArgC()-nTokenStart)-1;i++)
    {
        char szParam[5];
        sprintf(szParam, "%%%d", i);
        ConVar *pVarN = GetVar(args.Arg(nTokenStart + i), false);
        if (!pVarN) 
        {
            ErrMsg(0,"ERROR: variable %s does not exist.\n", args.Arg(nTokenStart+i));
            ErrVar("Variable does not exist");
            return;
        }
        char *pReplaceWith = sqlite3_mprintf("%q", pVarN->GetString());
        if (pReplaceWith) {
            V_StrSubst(szArgument, szParam, pReplaceWith, szFormatted, MAX_FORMATTED);
            sqlite3_free((void*)pReplaceWith);
        }
        else
        {
            ErrMsg(0,"ERROR: You did not provide enough parameters for the string you specified.\n");
            ErrVar("Not enough arguments.");
        }
        strcpy(szArgument, szFormatted);
    }

    // All double-percents should resolve last into a single %
    V_StrSubst(szArgument, "%%", "%", szFormatted, MAX_FORMATTED);

    SetConVar(args.Arg(1), szFormatted, true);
}


