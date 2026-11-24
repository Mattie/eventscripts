#include "es_strutils.h"
#include "es_playerutil.h"
#include "es_cmddef.h"
#include "es_debug.h"
//#include "utlhash.h"
#include "eiface.h"
#include "KeyValues.h"
#include "Color.h"
#include "es_keygroups.h"
#ifndef snprintf
#define snprintf _snprintf
#endif

extern IServerPluginHelpers *helpers;
extern IServerPluginCallbacks *pPlugin;

Color ColorFromString(const char *str, bool &bFound) {
	bFound = true;
    if (!stricmp(str, "#red")) {
		return Color(255, 0, 0, 255);
    } else if (!stricmp(str, "#green")) {
		return Color(0, 255, 0, 255);
    } else if (!stricmp(str, "#blue")) {
		return Color(0, 0, 255, 255);
    } else if (!stricmp(str, "#orange")) {
		return Color(255, 130, 0, 255);
    } else if (!stricmp(str, "#purple") || !stricmp(str, "#violet")) {
		return Color(144, 0, 226, 255);
    } else if (!stricmp(str, "#pink")) {
		return Color(226, 0, 165, 255);
    } else if (!stricmp(str, "#cyan")) {
		return Color(0, 255, 255, 255);
    } else if (!stricmp(str, "#yellow")) {
		return Color(255, 255, 0, 255);
    } else if (!stricmp(str, "#blue")) {
		return Color(0, 0, 255, 255);
    } else if (!stricmp(str, "#white")) {
		return Color(255, 255, 255, 255);
    } else if (!stricmp(str, "#black")) {
		return Color(0, 0, 0, 255);
    } else if (!stricmp(str, "#darkgreen")) {
		return Color(0, 145, 0, 255);
    } else if (!stricmp(str, "#darkblue")) {
		return Color(0, 0, 145, 255);
    } else if (!stricmp(str, "#darkred")) {
		return Color(145, 0, 0, 255);
    } else if (!stricmp(str, "#gray") || !stricmp(str, "#grey")) {
		return Color(120, 120, 120, 255);
	}
	bFound = false;
	return Color(255, 255, 255, 255);
}

ES_CON_COMMAND2(toptext, 
               "Sends HUD message to one player.\n",
               3,
               "<userid> <duration> [color] <message>")
	int duration = atoi(args.Arg(2));
	if (duration < 1) {
		DxMsg(0, __FUNCTION__, __LINE__,"%s: The duration of your message was set too low\n", args.Arg(0));
		return;
	}
    int nUserid = atoi(args.Arg(1));
    if (nUserid < 1) return;
    edict_t *pPlayer =LookupEdictByUserid(nUserid);
	char msg[256];
	const char *pMsg = (char*)&msg;
    
	const char *start = args.ArgS() + strlen(args.Arg(1)) + strlen(args.Arg(2)) + 1;
	if (*start == '"') {
		strncpy(msg, start+2, min(sizeof(msg), strlen(start+2)));
	} else {
		strncpy(msg, start+1, min(sizeof(msg), strlen(start)));
	}
    int slen = strlen(msg);
	bool found;
	char color[32];
	int x = 0;
	for (x = 0; x < 32 && x < slen && msg[x] != ' '; x++) {
		color[x] = msg[x];
	}
	color[x] = 0;

	Color clr = ColorFromString(color, found);
	if (found) {
		if (strlen(msg) <= (strlen(color) + 1)) {
			DxMsg(0, __FUNCTION__, __LINE__,"You must send a message\n");
			return;
		}
		pMsg += strlen(color) + 1;
	}
    char szMessage[300];
    StripAllQuotes(szMessage, pMsg); 	

	KeyValues *kv = new KeyValues("msg");
	kv->SetString( "title", szMessage);
	kv->SetColor( "color", clr);
	kv->SetInt( "level", 5);
	kv->SetInt( "time", duration);
	
	helpers->CreateMessage(pPlayer, DIALOG_MSG, kv, pPlugin);
	kv->deleteThis();

	return;
}

void SendKeyMessage(int nUserid, DIALOG_TYPE nType, KeyValues *pKey)
{
	edict_t* p_target = LookupEdictByUserid(nUserid);
	if (!p_target)
	{
		ErrMsg(0,"Error: \"%s\" is an invalid userid.\n", nUserid);
		return;
	}
    if (!pKey)
    {
        ErrMsg(0, "Error: Invalid key for sending VGUI message.\n");
    }
    helpers->CreateMessage(p_target, nType, pKey, pPlugin);
}



ES_CON_COMMAND2(escmenu, 
               "Sends an ESC menu to a player.",
               6,
               "<time> <userid> <title> <text> <button1> [button...] [button8]")
	const int argc = nCount;

	// Did we provide a valid time?
	const char* p_arg1 = args.Arg(1);
	const int time = atoi(p_arg1);
	if ((time < 10) || (time > 200))
	{
		ErrMsg(0,"Error: \"%s\" is an invalid specifier for <time>.\nError: <time> must be at least 10 or at the most 200.\n", p_arg1);
		return;
	}

	// Did we provide a valid userid?
	const char* p_arg2 = args.Arg(2);
	const int userid = atoi(p_arg2);
	edict_t* p_target = LookupEdictByUserid(userid);
	if (!p_target)
	{
		ErrMsg(0,"Error: \"%s\" is an invalid userid.\n", p_arg2);
		return;
	}

	// Look for new lines.
	char* p_arg4 = strdup(args.Arg(4));
	for (int i = 0; p_arg4[i] != '\0'; i++)
	{
		if ((p_arg4[i] == '\\') && (p_arg4[i + 1] == 'n'))
		{
			p_arg4[i] = '\n';
			for (int j = i + 1; p_arg4[j] != '\0'; j++)
				p_arg4[j] = p_arg4[j + 1];
		}
	}

	// Create the menu.
	KeyValues* p_menu = new KeyValues("menu");
	p_menu->SetString("title", args.Arg(3));
	p_menu->SetInt("level", 0);
	p_menu->SetColor("color", Color(255, 255, 255, 255));
	p_menu->SetInt("time", time);
	p_menu->SetString("msg", p_arg4);
	free(p_arg4);

	// Add the buttons.
	for (int i = 1, argi = 5; i <= 8; i++, argi++)
	{
		if (argi > (argc - 1))
			break;

		char key[2], cmd[17];

		snprintf(key, sizeof(key), "%i", i);
		snprintf(cmd, sizeof(cmd), "menuselect %i;//\"", i);

		// Add the button.
		KeyValues* p_button = p_menu->FindKey(key, true);
		p_button->SetString("msg", args.Arg(argi));
		p_button->SetString("command", cmd);
	}

	helpers->CreateMessage(p_target, DIALOG_MENU, p_menu, pPlugin);
	p_menu->deleteThis();
}


ES_CON_COMMAND2(keygroupmsg, 
               "Sends a keygroup-based message to a player.",
               3,
               "<userid> <type> <keygroup>")

	KeyValues *kv = GetKeyGroup(args.Arg(3), FALSE);
    int nGuy = atoi(args.Arg(1));
    if (nGuy<=0)
    {
        ErrMsg(0, "Invalid userid: %s\n", args.Arg(1));
    }
    int nType = atoi(args.Arg(2));
    if (kv)
    {
        KeyValues *pKeyMsg = kv->FindKey("message");
        if (pKeyMsg)
        {
            SendKeyMessage(nGuy, (DIALOG_TYPE)nType, pKeyMsg);
        }
        else
        {
            ErrMsg(0, "Error: 'message' subkey was not found inside keygroup %s\n", args.Arg(3));
        }
    }
    else
    {
        ErrMsg(0, "Error: KeyGroup %s not found.\n", args.Arg(3));
    }
}
ES_CON_COMMAND2(esctextbox, 
               "Sends an ESC textbox to a player.",
               5,
               "<time> <userid> <title> <text>")
	// Did we provide a valid time?
	const char* p_arg1 = args.Arg(1);
	const int time = atoi(p_arg1);
	if ((time < 10) || (time > 200))
	{
		ErrMsg(0,"Error: \"%s\" is an invalid specifier for <time>.\nError: <time> must be at least 10 or at the most 200.\n", p_arg1);
		return;
	}

	// Did we provide a valid userid?
	const char* p_arg2 = args.Arg(2);
	const int userid = atoi(p_arg2);
	edict_t* p_target = LookupEdictByUserid(userid);
	if (!p_target)
	{
		ErrMsg(0,"Error: \"%s\" is an invalid userid.\n", p_arg2);
		return;
	}

	// Look for new lines and tab spaces.
	char* p_arg4 = strdup(args.Arg(4));
	for (int i = 0; p_arg4[i] != '\0'; i++)
	{
		if ((p_arg4[i] == '\\') && (p_arg4[i + 1] == 'n'))
		{
			p_arg4[i] = '\n';
			for (int j = i + 1; p_arg4[j] != '\0'; j++)
				p_arg4[j] = p_arg4[j + 1];
		}
	}
	
	// Create the text-box.
	KeyValues* p_textBox = new KeyValues("textBox");
	p_textBox->SetString("title", args.Arg(3));
	p_textBox->SetInt("level", 0);
	p_textBox->SetColor("color", Color(255, 255, 255, 255));
	p_textBox->SetInt("time", time);
	p_textBox->SetString("msg", p_arg4);
	free(p_arg4);
    helpers->CreateMessage(p_target, DIALOG_TEXT, p_textBox, pPlugin);
	p_textBox->deleteThis();
}

ES_CON_COMMAND2(escinputbox, 
               "Sends an ESC input box to a player.",
               6,
               "<time> <userid> <title> <text> <command>")
	// Did we provide a valid time?
	const char* p_arg1 = args.Arg(1);
	const int time = atoi(p_arg1);
	if ((time < 10) || (time > 200))
	{
		ErrMsg(0,"Error: \"%s\" is an invalid specifier for <time>.\nError: <time> must be at least 10 or at the most 200.\n", p_arg1);
		return;
	}

	// Did we provide a valid userid?
	const char* p_arg2 = args.Arg(2);
	const int userid = atoi(p_arg2);
	edict_t* p_target = LookupEdictByUserid(userid);
	if (!p_target)
	{
		ErrMsg(0,"Error: \"%s\" is an invalid userid.\n", p_arg2);
		return;
	}

	// Look for new lines.
	char* p_arg4 = strdup(args.Arg(4));
	for (int i = 0; p_arg4[i] != '\0'; i++)
	{
		if ((p_arg4[i] == '\\') && (p_arg4[i + 1] == 'n'))
		{
			p_arg4[i] = '\n';
			for (int j = i + 1; p_arg4[j] != '\0'; j++)
				p_arg4[j] = p_arg4[j + 1];
		}
	}

	// Create the entry-box.
	KeyValues* p_entryBox = new KeyValues("entryBox");
	p_entryBox->SetString("title", args.Arg(3));
	p_entryBox->SetInt("level", 0);
	p_entryBox->SetColor("color", Color(255, 255, 255, 255));
	p_entryBox->SetInt("time", time);
	p_entryBox->SetString("msg", p_arg4);
	const char* p_arg5 = args.Arg(5);
	char cmd[1024];
	snprintf(cmd, sizeof(cmd) - 1, "\"%s\"", p_arg5);
	p_entryBox->SetString("command", cmd);
	free(p_arg4);

	helpers->CreateMessage(p_target, DIALOG_ENTRY, p_entryBox, pPlugin);
	p_entryBox->deleteThis();
}

PY_COMMAND_VARARGS(sendkeypmsg,
                   "Sends a client message based on a KeyValues pointer. sendkeypmsg(userid,type,keyid)")
{
    KeyValues *pKey = NULL;
    int nUserid = 0;
    int nType = 0;
    if(!PyArg_ParseTuple(args, "iii", &nUserid, &nType, &pKey))
    {
        return NULL;
    }
    SendKeyMessage(nUserid, (DIALOG_TYPE)nType, pKey);
    Py_INCREF(g_pPyNone);
    return g_pPyNone;
}

