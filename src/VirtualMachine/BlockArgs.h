#ifndef BLOCKARGS_H
#define BLOCKARGS_H
#pragma once
#include "strtools.h"
#define MAX_ARGS 80
#ifndef ORANGEBOX
#include "characterset.h"
#else
typedef struct characterset_s
{
	char set[256];
} characterset_t;
#endif
#ifdef PYTHON_SUPPORT
#include "Python.h"
#endif

class BlockArgs
{
private:
    int			cmd_argc;
    char		*cmd_argv[MAX_ARGS];
    char		*cmd_null_string;
    char		*cmd_args;
    char		com_token[1024]; 
    bool        balloc;
protected:
	char        *cmd_string;

public:

    BlockArgs(void ) {
	    cmd_argc = 0;
	    cmd_args = NULL;
        cmd_null_string = "";
        cmd_string = NULL;
        balloc = false;
    }

    ~BlockArgs() {
	    int		i;
        // clear the args from the last string
	    for (i=0 ; i<cmd_argc ; i++)
	    {
		    delete[] cmd_argv[i];
	    }
    	if (balloc)
        {
            free(cmd_args);
        }
	    cmd_argc = 0;
	    cmd_args = NULL;
        free(cmd_string);
    }

    char *COM_Parse (char *data);
    int	Cmd_Argc (void) const;

    char *Cmd_Argv (int arg) const;
    char *Cmd_Args (void) const;
    // Parses the given string into command line tokens.
    void Cmd_TokenizeString (const char *input);
    static void InitializeCharacterSets();
    //const char* ParseFile( const char* pFileBytes, char* pToken, bool* pWasQuoted, characterset_t *pCharSet )
    char* ParseFile( char* pFileBytes, char* pToken, bool* pWasQuoted );
    const char* ParseFile( const char* pFileBytes, char* pToken, bool* pWasQuoted, characterset_t *pCharSet = NULL);
    static void CharacterSetBuild( characterset_t *pSetBuffer, const char *pszSetString );
#ifdef PYTHON_SUPPORT
    void Cmd_TokenizePython (char *cmdname, PyObject *args);
#endif
    
};

#ifndef ORANGEBOX
class CCommand : BlockArgs
{
public:
	CCommand() : BlockArgs() {
	};
	CCommand( int nArgC, const char **ppArgV );
	bool Tokenize( const char *pCommand, characterset_t *pBreakSet = NULL )
	{
		this->Cmd_TokenizeString(pCommand);
		return true;
	}
	void Reset();

	int ArgC() const {
		return this->Cmd_Argc();
	}
//	const char **ArgV() const;
	const char *ArgS() const {
		return this->Cmd_Args();
	}					// All args that occur after the 0th arg, in string form
	const char *GetCommandString() const		// The entire command in string form, including the 0th arg
	{
		return this->ArgC() ? this->cmd_string : "";
	}
	const char *operator[]( int nIndex ) const;	// Gets at arguments
	const char *Arg( int nIndex ) const {
		return this->Cmd_Argv(nIndex);
	}
		
		//;		// Gets at arguments
	
	// Helper functions to parse arguments to commands.
	const char* FindArg( const char *pName ) const;
	int FindArgInt( const char *pName, int nDefaultVal ) const;

	static int MaxCommandLength();
	static characterset_t* DefaultBreakSet();

};
#endif

#endif
