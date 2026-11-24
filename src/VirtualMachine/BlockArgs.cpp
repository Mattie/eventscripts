#include "BlockArgs.h"
#ifdef PYTHON_SUPPORT
#include "Python.h"
#endif

// used for debug output
extern inline void DxMsg(int nLevel, const char* szFunction, int nLine, const tchar *pszFormat, ...);

void BlockArgs::CharacterSetBuild( characterset_t *pSetBuffer, const char *pszSetString )
{
	int i = 0;

	// Test our pointers
	if ( !pSetBuffer || !pszSetString )
		return;

	memset( pSetBuffer->set, 0, sizeof(pSetBuffer->set) );

	while ( pszSetString[i] )
	{
		pSetBuffer->set[ pszSetString[i] ] = 1;
		i++;
	}

}

#define IN_CHARACTERSET( SetBuffer, character )		((character < 256) ? (SetBuffer).set[(character)] : 0)

// wordbreak parsing set
static characterset_t	g_BreakSet, g_BreakSetIncludingColons;


void BlockArgs::InitializeCharacterSets()
{
	static bool s_CharacterSetInitialized = false;
	if (!s_CharacterSetInitialized)
	{
		CharacterSetBuild( &g_BreakSet, "{}()'" );
		CharacterSetBuild( &g_BreakSetIncludingColons, "{}()':" );
		s_CharacterSetInitialized = true;
	}
}


const char* BlockArgs::ParseFile( const char* pFileBytes, char* pToken, bool* pWasQuoted, characterset_t *pCharSet )
{
	if (pWasQuoted)
		*pWasQuoted = false;

	if (!pFileBytes)
		return 0;

	InitializeCharacterSets();

	// YWB:  Ignore colons as token separators in COM_Parse
	static bool com_ignorecolons = false;  
	characterset_t& breaks = pCharSet ? *pCharSet : (com_ignorecolons ? g_BreakSet : g_BreakSetIncludingColons);
	
	unsigned int c;
	int len = 0;
	pToken[0] = 0;
	
// skip whitespace
skipwhite:

    
	while ( (c = (unsigned int)*pFileBytes) <= ' ')
	{
		if (c == 0)
			return 0;                    // end of file;
		pFileBytes++;
	}

// skip // comments
	if (c=='/' && pFileBytes[1] == '/')
	{
		while (*pFileBytes && *pFileBytes != '\n')
			pFileBytes++;
		goto skipwhite;
	}
	
// skip c-style comments
	if (c=='/' && pFileBytes[1] == '*' )
	{
		// Skip "/*"
		pFileBytes += 2;
		while ( *pFileBytes  )
		{
			if ( *pFileBytes == '*' &&
				 pFileBytes[1] == '/' )
			{
				pFileBytes += 2;
				break;
			}

			pFileBytes++;
		}

		goto skipwhite;
	}

// handle quoted strings specially
	if (c == '\"')
	{
		if (pWasQuoted)
			*pWasQuoted = true;

		pFileBytes++;
		while (1)
		{
			c = *pFileBytes++;
			if (c=='\"' || !c)
			{
				pToken[len] = 0;
				return pFileBytes;
			}
			pToken[len] = c;
			len++;
		}
	}

// parse single characters
	if ( IN_CHARACTERSET( breaks, c ) )
	{
		pToken[len] = c;
		len++;
		pToken[len] = 0;
		return pFileBytes+1;
	}

// parse a regular word
	do
	{
		pToken[len] = c;
		pFileBytes++;
		len++;
		c = (unsigned int)*pFileBytes;
		if ( IN_CHARACTERSET( breaks, (unsigned int)c ) )
			break;
	} while ((unsigned int)c>(unsigned int)32);
	
	pToken[len] = 0;
	return pFileBytes;
}


char* BlockArgs::ParseFile( char* pFileBytes, char* pToken, bool* pWasQuoted )
{
	return (char*)this->ParseFile( (const char*)pFileBytes, pToken, pWasQuoted );
}


char *BlockArgs::COM_Parse (char *data)
{
    char *pBlah = this->ParseFile( data, com_token, NULL );
	return pBlah;
}

int	BlockArgs::Cmd_Argc (void)  const  {
	return cmd_argc;
}

char *BlockArgs::Cmd_Argv (int arg)  const  {
	if ( (unsigned)arg >= (unsigned)cmd_argc )
		return cmd_null_string;
	return cmd_argv[arg];	
}

char *BlockArgs::Cmd_Args (void) const {
	return cmd_args;
}

// Parses the given string into command line tokens.
void BlockArgs::Cmd_TokenizeString (const char *input)
{
    cmd_string = strdup(input);
    char * text = cmd_string;
	while (1)
	{
        // skip whitespace up to a /n
		while (*text && (unsigned)*text <= (unsigned)' ' && *text != '\n')  {
			text++;
		}
    	
		if (*text == '\n')    {	
            // a newline seperates commands in the buffer
			text++;
			break;
		}
		if (!*text)
			return;
		if (cmd_argc == 1)
			cmd_args = text;
		text = COM_Parse (text);
		if (!text)
			return;

		if (cmd_argc < MAX_ARGS)
		{
			cmd_argv[cmd_argc] = (char *)new char[ Q_strlen(com_token)+1 ];
			Q_strcpy (cmd_argv[cmd_argc], com_token);
			cmd_argc++;
		}
	}
    
}


#ifdef PYTHON_SUPPORT
// Takes a python argument tuple and fakes a command-line argument
void BlockArgs::Cmd_TokenizePython (char * cmdname, PyObject *args)
{
    char szCommandString[8000];
    szCommandString[0] = 0;

    // get argv(0)
    cmd_argv[cmd_argc] = (char *)new char[ Q_strlen(cmdname)+1 ];
    Q_strcpy (cmd_argv[cmd_argc], cmdname);
    cmd_argc++;
    // loop through argvs
    int nArg = 0;
	PyObject *a;
    int sizetup = PyTuple_GET_SIZE(	args);
    while (nArg < sizetup) //PyArg_UnpackTuple(args, cmdname, nArg, nArg, &a) && cmd_argc < MAX_ARGS)
    {
        a = PyTuple_GetItem(	args, nArg);
        //   Get the string version of it
        PyObject *s;
        s = PyObject_Str(a);
        if (s)
        {
            char *orig = PyString_AsString(s);
            if (orig)
            {
			    cmd_argv[cmd_argc] = (char *)new char[ Q_strlen(orig)+1 ];
			    Q_strcpy (cmd_argv[cmd_argc], orig);
                if (cmd_argc>1)
                    strcat(szCommandString, " ");
                strcat(szCommandString, cmd_argv[cmd_argc]);
			    cmd_argc++;
            }
            else
            {
                DxMsg(0, __FUNCTION__, __LINE__, "Cmd_TokenizePython: ERROR with parsing %s on arg %d", cmdname, nArg);
            }
            Py_XDECREF(s);
        }
        nArg++;
    }
    //cmd_string = 
    cmd_args = strdup(szCommandString);
}

#endif
