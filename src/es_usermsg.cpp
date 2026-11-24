//=========================================================================
// Includes
//=========================================================================
#include "es_usermsg.h"
#include "es_cmddef.h"
#include "es_strutils.h"
#include "es_convar.h"
#include "MRecipientFilter.h"
#ifndef ORANGEBOX
#include "bitbuf.h"
#endif
// Singleton instance
CESUserMessages *g_pUserMessages;

//=========================================================================
// Constructor
//=========================================================================
CESUserMessages::CESUserMessages( IServerGameDLL *pGameDLL, IVEngineServer *pEngine, IPlayerInfoManager *pInfoManager )
{
	if( !pGameDLL || !pEngine || !pInfoManager ) 
	{
		Warning("[Eventscripts] The IServerGameDLL, IVEngineServer, and/or IPlayerInfoManager instance was NULL!\n");
		return;
	}

	m_pGameDLL		= pGameDLL;
	m_pEngine		= pEngine;
	m_pPlayerInfo	= pInfoManager;

	// Initialize the main usermessage KeyValue instance.
	m_pUserMessages = new KeyValues("usermessages");
}

//=========================================================================
// Destructor
//=========================================================================
CESUserMessages::~CESUserMessages()
{
	// We need to free memory upon shutdown.
	if( m_pUserMessages )
        m_pUserMessages->deleteThis();
}

//=========================================================================
// Returns the type of usermessage from the name.
//=========================================================================
int CESUserMessages::GetMessageType( const char *msgname )
{
	if( m_pGameDLL )
	{
		char    sz_mname[256] = "";
		int     sizereturn = 0;
		ConVar  *maxmsgvar = GetVar("eventscripts_maxmsg");

		if( !maxmsgvar )
		{
			DxMsg(0, __FUNCTION__, __LINE__, "[Eventscripts] Couldn't find the eventscripts_maxmsg variable.\n");
			return 0;
		}


		// We loop from 0 - whatever the value of eventscripts_maxmsg is.
		for( int x = 0; x < maxmsgvar->GetInt(); x++ )
		{
			// Check to see if this is even a valid usermessage. This also fills in the sz_mname string.
			if( m_pGameDLL->GetUserMessageInfo(x, sz_mname, 255, sizereturn) )
			{
				// If it's valid, compare the passed in name to the one found by GetUserMessageInfo.
				if( FStrEq(msgname, sz_mname) )
				{
					// If they match, return the index this user message is at.
					return x;
				}
			}
		}
	}

	// If we get here, someone made a no-no.
	return 0;
}

//=========================================================================
// Writes information into a usermessage string.
//=========================================================================
void CESUserMessages::WriteUserMessage( KeyValues *pKey, bf_write *pWrite )
{
	DxMsg(3, __FUNCTION__, __LINE__, "WriteUserMessage Started.\n");
	
	KeyValues *pTemp = pKey->GetFirstTrueSubKey();
	
	while( pTemp )
	{
		KeyValues *pValue = pTemp->GetFirstValue();
		
		if ( pValue )
		{
			const char *name =  pValue->GetName();

			DxMsg(3, __FUNCTION__, __LINE__, "WriteUserMessage: %s.\n", name);
			
			if (FStrEq(name,"string"))
			{
				char szMessage[MAX_COMMAND_STRING];
				unescape(szMessage, (char *)pValue->GetString());
				pWrite->WriteString(szMessage);
			}

			else if (FStrEq(name,"float"))
			{
				pWrite->WriteFloat(pValue->GetFloat());
			}

			else if (FStrEq(name,"short"))
			{
				pWrite->WriteShort(pValue->GetInt());
			}

			else if (FStrEq(name,"char"))
			{
				pWrite->WriteChar(pValue->GetInt());
			}

			else if (FStrEq(name,"byte"))
			{
				pWrite->WriteByte(pValue->GetInt());
			}

			else if (FStrEq(name,"long"))
			{
				pWrite->WriteLong(pValue->GetInt());
			}
		}
		pTemp = pTemp->GetNextTrueSubKey();
	}

	DxMsg(3, __FUNCTION__, __LINE__, "WriteUserMessage Done.\n");
}

//=========================================================================
// Sends a usermessage to a player based on their userid.
//=========================================================================
void CESUserMessages::SendUserMessageData( const char *key, int nUserid )
{
	if ( !m_pUserMessages )
	{
		DxMsg(0, __FUNCTION__, __LINE__, "Key does not exist, please create it: %s", key);
		return;
	}

	// Find the key that this usermessage was stored in.
	KeyValues *pDataKey = m_pUserMessages->FindKey(key);

	if ( pDataKey )
	{
		DxMsg(3, __FUNCTION__, __LINE__, "SendUserMessageData Started.\n");

		// Get the type of this message
		int nType = GetMessageType(pDataKey->GetString("type"));

		// Validity check.
		if ( nType == 0 )
		{
			DxMsg(0, __FUNCTION__, __LINE__, "Invalid UserMessage type: %s\n", pDataKey->GetString("type"));
			return;
		}

		// Create a list of userids which will see this message.
		MRecipientFilter filter;

		// Add a player userid to this list.
		filter.AddPlayer(nUserid, m_pPlayerInfo->GetGlobalVars()->maxClients , m_pPlayerInfo);

		// Create the usermessage instance.
		bf_write *pWrite;

		// Start the user message
		pWrite = UserMessageBegin(&filter, nType, NULL);

		// Write all the data stores inside the usermessage key that we got earlier.
		WriteUserMessage(pDataKey, pWrite);
		
		// End the usermessage.
		m_pEngine->MessageEnd();

		DxMsg(3, __FUNCTION__, __LINE__, "SendUserMessageData SENT: %d, %s\n", nUserid, pDataKey->GetString("type"));
	}

	// Looks like we couldn't find the key :(
	else
	{
		DxMsg(0, __FUNCTION__, __LINE__, "Key does not exist, please create it: %s", key);
		return;
	}
}

//=========================================================================
// Writes information to the requested usermessage based on it's type.
//=========================================================================
void CESUserMessages::WriteUserMessageData( const char *key, const char *type, const char *value )
{
	// Make sure the global user message tracker is valid.
	if ( !m_pUserMessages )
	{
		DxMsg(0, __FUNCTION__, __LINE__, "Key does not exist, please create it: %s", key);
		return;
	}

	// Find the requested usermessage in our list of usermessage types.
	KeyValues *pDataKey = m_pUserMessages->FindKey(key);

	// If we found it...
	if ( pDataKey )
	{
		DxMsg(3, __FUNCTION__, __LINE__, "WriteUserMessageData: %s = %s\n", type, value);

		// Create a new key for this message of type _type_.
        KeyValues *pValueKey = new KeyValues(type);
		pDataKey->AddSubKey(pValueKey);

		// Set the value of this usermessage.
		pValueKey->SetString(type, value);
	}

	// The usermessage was not found :(
	else
	{
		DxMsg(0, __FUNCTION__, __LINE__, "Key does not exist, please create it: %s", key);
		return;
	}
}

//=========================================================================
// This is the handler for es_usermsg. It parses all the parameters
// and performs the operations.
//=========================================================================
void CESUserMessages::UserMessageCommand( const CCommand &args, int nCount )
{
#if 0
	// YNH: Moved this to the constructor. Apparently, m_pUserMessages
	//	wasn't being initialized. m_pUserMessages->unused contained
	//	"hunter.mdl" o_O.
// 	if ( m_pUserMessages == NULL)
// 	{
// 		// If not create it.
// 		m_pUserMessages = new KeyValues("usermessages");
// 	}
#endif

	// If the first parameter passed into es_usermessage is 'create'
	if ( FStrEq(args.Arg(1), "create") )
	{
		// And the argument count is greater than 3...
		if ( nCount>3 )
		{
			// args.Arg(2) is the scripter defined instance of a usermessage.
			// Here, we try to see if that instance already exists.
			KeyValues *pDataKey = m_pUserMessages->FindKey(args.Arg(2), true);
			
			// If it does exist, we set the SDK-Type (i.e. SayText2, HudHint, etc) for the script-msg instance.
			if ( pDataKey )
			{
				pDataKey->SetString("type", args.Arg(3));
			}
			
			// We couldn't create this usermessage :(
			else
			{
				DxMsg(0, __FUNCTION__, __LINE__, "Couldn't create: %s\n", args.Arg(2));
				ErrVar("Internal error");
				return;
			}
		}

		// The script didn't provide enough parameters for this command. Oh well.
		else
		{
            DxMsg(0, __FUNCTION__, __LINE__, "Not enough parameters: %s\n", args.ArgS());
			ErrVar("Not enough parameters");
		}
	}

	// The script wants to delete a usermessage.
	else if ( FStrEq(args.Arg(1), "delete") )
	{
		// Did it provide enough parameters?
		// SYNTAX: 'es_usermsg delete usermessagename'
		//		Where usermessagename was the custom name used in the creation of the usermessage.
		if ( nCount > 2 )
		{
			// Find the data for the script-created usermessage instance.
			KeyValues *pTemp = m_pUserMessages->FindKey(args.Arg(2));
			
			// Is it valid?
			if (pTemp)
			{
				// We simply just delete this from the global user message list.
				m_pUserMessages->RemoveSubKey(pTemp);
				pTemp->deleteThis();

				DxMsg(1, __FUNCTION__, __LINE__, "Key deleted: %s\n", args.Arg(2)); // Yay! :D
			}
		}
	}

	// Script wants to send a user message.
	else if ( FStrEq(args.Arg(1), "send") )
	{
		// Did it provide enough arguments?
		if (nCount > 3)
		{
			// Call SendUserMessageData, and pass in:
			//		Name of the custom instance
			//		The userid to send it to.
			SendUserMessageData(args.Arg(2), atoi(args.Arg(3)));
		}
	}

	else
	{
		// We're being passed more than 4 arguments.
		if ( nCount > 4 )
		{
			// The user wants to write a usermessage. Call the function for that.
			if (FStrEq(args.Arg(1), "write"))
			{
				// Pass in:
				//		The name of the user message instance.
				//		The type of the value to be written (short, char, bool, etc).
				//		The value to be written.
				WriteUserMessageData(args.Arg(3), args.Arg(2), args.Arg(4));
			}

			//  writev just reads from a variable so it can get a string with quotes, etc
			else if ( FStrEq(args.Arg(1), "writev") )
			{
				// Get the convar
				ConVar *pOne = GetVar(args.Arg(4));
				if (pOne)
					// Write the convar's string value into this user message.
					WriteUserMessageData(args.Arg(3), args.Arg(2), pOne->GetString());
				else
					// If it's not valid, write '0' in place.
					WriteUserMessageData(args.Arg(3), args.Arg(2), "0");
			}

			// The passed in operation is invalid.
			else
			{
				DxMsg(0, __FUNCTION__, __LINE__, "Unknown user message command: %s\n", args.Arg(1));
				ErrVar("Invalid parameter");
				return;
			}
		}

		// The script didn't give us enough parameters.
		else
		{
            DxMsg(0, __FUNCTION__, __LINE__, "Not enough parameters: %s\n", args.ArgS());
			ErrVar("Not enough parameters");
		}
	}
	
#if 0
// YNH: I think this is the result of a mistaken copy + paste
//	during refactoring.
// 	DxMsg(3, __FUNCTION__, __LINE__, "]%s %s\n", args.Arg(0), args.ArgS());
// 	UserMessageCommand(args, nCount);
// 	DxMsg(3, __FUNCTION__, __LINE__, "]%s done.\n", args.Arg(0), args.ArgS());
#endif
}

//=========================================================================
// This is an abstract version of engine->UserMessageBegin.
//=========================================================================
bf_write *CESUserMessages::UserMessageBegin(IRecipientFilter * filter, int msg_type, const char * name)
{
#ifdef LEFT4DEAD
	return m_pEngine->UserMessageBegin( filter, msg_type, name);
#else
	return m_pEngine->UserMessageBegin( filter, msg_type);
#endif
}

/************************************************************************/
/* CONSOLE COMMAND DECLARATIONS                                         */
/************************************************************************/
ES_CON_COMMAND2(usermsg, "Create and send a usermsg to a client.", 2, "<operation> [data-type] <msg-name> [value]")
	DxMsg(3, __FUNCTION__, __LINE__, "]%s %s\n", args.Arg(0), args.ArgS());
	
	if( g_pUserMessages )
	{
		g_pUserMessages->UserMessageCommand(args, nCount);
		DxMsg(3, __FUNCTION__, __LINE__, "]%s done.\n", args.Arg(0), args.ArgS());
	}
}

