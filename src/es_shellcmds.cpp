#include "es_cmddef.h"
#include "es_convar.h"
#include "es_util.h"
#include "es_strutils.h"
#include "muParser/muParser.h"
#include "VirtualMachine/VirtualMachine.h"

CVirtualMachine g_ShellVM;
VMInstruction g_LatestInstruction;

void ImmediateExecuteCommand(char *szCommand);
void ExecuteCommandNow(char *szCommand);
extern bool g_bManagedServerCmdExecution;



bool doblock_esshell(const char *pTrim, char*szCommandLine = NULL)
{
    if (g_ShellVM.BlockExists(pTrim))
    {
        VMInstruction Next;
        if (g_ShellVM.ExecBlock(pTrim, &Next, szCommandLine))
        {
            DxMsg(5, __FUNCTION__, __LINE__, "doblock preparing:\n   NextIP: 0x%x\n      Cmd: %s", Next.nIP, Next.szCommand);
            if (g_bManagedServerCmdExecution)
            {
                unsigned int nInstruction = Next.nIP;
                Msg("[D08SYNC] block enter name=%s\n", pTrim);
                ExecuteCommandNow(Next.szCommand);
                while (nInstruction != 0)
                {
                    VMInstruction Current;
                    if (!g_ShellVM.ExecNextInstruction((void *)nInstruction, &Current))
                    {
                        break;
                    }
                    ExecuteCommandNow(Current.szCommand);
                    nInstruction = Current.nIP;
                }
                Msg("[D08SYNC] block leave name=%s\n", pTrim);
                return true;
            }
            if (Next.nIP != 0)
            {
                char szCommand[MAX_COMMAND_STRING];
                sprintf(szCommand, "%s;es__ip %d\n", Next.szCommand,Next.nIP);
                ImmediateExecuteCommand(szCommand);
            }
            else
            {
                DxMsg(2, __FUNCTION__, __LINE__,"Null instruction pointer provided for block: %s.\n", pTrim);
				ImmediateExecuteCommand(Next.szCommand);
            }
            return true;
        }
        else
        {
            DxMsg(2, __FUNCTION__, __LINE__,"Block was refused: %s.\n", pTrim);
        }
    }
    return false;
}



// muParser integration
using namespace mu;
extern mu::Parser g_MathParser;

enum CompareOperation {
    none,
    equalto,
    notequalto,
    greaterthan,
    lessthan,
    notlessthan,
    notgreaterthan,
    in,
    notin
};

bool do_cond_test(const CCommand &args, float fToken1, CompareOperation cop, float fToken2)
{
    // currently we support only four operators 
    if (cop == none)
    {
        DxMsg(0, __FUNCTION__, __LINE__,"%s: Unsupported comparison operator. Your line was:\n  %s\n", args.Arg(0), args.ArgS());
        ErrVar("Syntax error");
        return false;
    }
    if(
       (cop == equalto        && fToken1 == fToken2) || 
       (cop == notequalto     && fToken1 != fToken2) ||
       (cop == greaterthan    && fToken1 >  fToken2) || 
       (cop == lessthan       && fToken1 <  fToken2) || 
       (cop == notgreaterthan && fToken1 <= fToken2) || 
       (cop == notlessthan    && fToken1 >= fToken2) ||
       (cop == in             && fToken1 == fToken2) ||
       (cop == notin          && fToken1 != fToken2)
      )
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool do_cond_test(const CCommand &args, const char *szToken1, CompareOperation cop, const char *szToken2)
{
    // currently we support only four operators 
    if (cop == none)
    {
        DxMsg(0, __FUNCTION__, __LINE__,"%s: Unsupported comparison operator. Your line was:\n  %s\n", args.Arg(0), args.ArgS());
        ErrVar("Syntax error");
        return false;
    }
    if(
       (cop == equalto     &&  FStrEq(szToken1, szToken2)) || 
       (cop == notequalto  && !FStrEq(szToken1, szToken2)) ||
       (cop == greaterthan && (atof(szToken1) > atof(szToken2))) || 
       (cop == lessthan    && (atof(szToken1) < atof(szToken2))) || 
       (cop == notgreaterthan && (atof(szToken1) <= atof(szToken2))) || 
       (cop == notlessthan    && (atof(szToken1) >= atof(szToken2))) ||
       (cop == in             && Q_stristr(szToken2,szToken1)) ||
       (cop == notin          && !Q_stristr(szToken2,szToken1))
      )
    {
        return true;
    }
    else
    {
        return false;
    }
}





// CON_COMMAND for es_xif -- checks for proper syntax
// conditional function that supports different types but same operators, etc
void ifthen(const CCommand &args, const char *szToken1, const char *pOpString, const char *szToken2, char *szPredicate, bool bBlockBased = false)
{
    CompareOperation cop = none;
    char * szCommand = szPredicate;

    if (FStruEq(pOpString, "equalto") || FStruEq(pOpString, "=") || FStruEq(pOpString, "=="))   {
        cop = equalto;
        goto do_cond_test;
    }
    if (FStruEq(pOpString, "notequalto") || FStruEq(pOpString, "!=") || FStruEq(pOpString, "!=")) {
        cop = notequalto;
        goto do_cond_test;
    }
    if (FStruEq(pOpString, "greaterthan") || FStruEq(pOpString, ">")) {
        cop = greaterthan;
        goto do_cond_test;
    }
    if (FStruEq(pOpString, "lessthan") || FStruEq(pOpString, "<")) {
        cop = lessthan;
        goto do_cond_test;
    }

    if (FStruEq(pOpString, "notlessthan") || FStruEq(pOpString, ">=") || FStruEq(pOpString, "=>")) {
        cop = notlessthan;
        goto do_cond_test;
    }

    if (FStruEq(pOpString, "notgreaterthan") || FStruEq(pOpString, "=<") || FStruEq(pOpString, "<=")) {
        cop = notgreaterthan;
        goto do_cond_test;
    }
    if (FStruEq(pOpString, "in") || FStruEq(pOpString, ":")) {
        cop = in;
        goto do_cond_test;
    }

    if (FStruEq(pOpString, "notin") || FStruEq(pOpString, "!:")) {
        cop = notin;
        goto do_cond_test;
    }


do_cond_test:
    if (do_cond_test(args, szToken1, cop, szToken2))
    {
        g_ShellVM.SetConditionalFlag(true);
        if (!bBlockBased)
        {
            // Just toss it on the stack
            ImmediateExecuteCommand(szCommand);
            return;
        }
        else
        {
            DxMsg(4, __FUNCTION__, __LINE__,"setting skip flag\n");
            // signal that the next block needs to run
            g_ShellVM.SetSkipBlockFlag(false);
        }
    }
    else
    {
        g_ShellVM.SetConditionalFlag(false);
        if (bBlockBased)
        {
            // skip the next block
            g_ShellVM.SetSkipBlockFlag(true);
        }
    }
}

void do_cond_predicate(char *szCommand, bool bBlockBased)
{
        g_ShellVM.SetConditionalFlag(true);
        if (!bBlockBased)
        {
            // Just toss it on the stack
            ImmediateExecuteCommand(szCommand);
            return;
        }
        else
        {
            DxMsg(4, __FUNCTION__, __LINE__,"setting skip flag\n");
            // signal that the next block needs to run
            g_ShellVM.SetSkipBlockFlag(false);
        }
}

void fail_cond_predicate(bool bBlockBased)
{
        g_ShellVM.SetConditionalFlag(false);
        if (bBlockBased)
        {
            // skip the next block
            g_ShellVM.SetSkipBlockFlag(true);
        }
}

void ifx_parse(const CCommand &args, const char *szCondition, char * szPredicate, bool bBlockBased = true)
{
  using namespace mu;
  char * szCommand = szPredicate;
  try
  {
    g_MathParser.SetExpr(args.Arg(3));
    float fresult = g_MathParser.Eval();
    if (do_cond_test(args, fresult, notequalto, 0.0f))
    {
        do_cond_predicate(szPredicate, bBlockBased);
    }
    else
    {
        fail_cond_predicate(bBlockBased);
    }

  }
  catch (Parser::exception_type &e)
  {
      DxMsg(0, __FUNCTION__, __LINE__, "Exception: %s!\n", e.GetMsg().c_str());
      //DxMsg(0, __FUNCTION__, __LINE__, "Exception: %d!\n", (int)e.GetPos());
  }
  //ParseResetVariables();
}

void ifx_bool(bool bFailOnZero, const char *szVariable, char * szPredicate, bool bBlockBased = true)
{
    ConVar *cvar = FindVar(szVariable);
    if (cvar)
    {
        // 0 0.00000          -- false .
        // 0 0.00000 0.00000  -- false
        // 0 0.00000 0        -- false .
        // 1 1.00000 1.00000  -- true
        // 2 2.10000 2.10000  -- true
        // 0 0.00000 Blah     -- true
        const char *pSubject = cvar->GetString();
            char *stopstring = NULL; 
            char *stopstring2 = NULL; 
            long _n = strtol(pSubject, &stopstring, 10);
            float _f = strtod(pSubject, &stopstring2);
        bool bNumeric = (FStrEq("", stopstring) || FStrEq("", stopstring2));
        bool bStringEmpty = !(*pSubject);
        bool bZeroFloat = !_f;
        bool bSucceed = true;

        if (bStringEmpty || (bNumeric && bZeroFloat))
        {
            // it really is zero! if bFailOnZero we should fail.
            bSucceed = !bFailOnZero;
        }
        else
        {
            // it is non-zero! if bFailOnZero we should succeed.
            bSucceed = bFailOnZero;
        }
        if (bSucceed)
        {
            do_cond_predicate(szPredicate, bBlockBased);
            return;
        }
        else
        {
            fail_cond_predicate(bBlockBased);
            return;
        }
    }
    fail_cond_predicate(bBlockBased);
}


CON_COMMAND( ifx,"   ifx <mode>(<arguments>) do\n Never supports expansion of variables.\n\n")
{
    if (ARGC() < 4)
    {
        DxMsg(0, __FUNCTION__, __LINE__, "%s: Syntax error.\n\t%s", ARGV(0), ARGS());
        ErrVar("Syntax error");
        return;
    }
    else if (FStruEq(ARGV(1), "parse"))
    {
		PARSE_ARGS_FROM_ENGINE();
		ifx_parse(args, ARGV(3), NULL, true);
    }
    else if (FStruEq(ARGV(1), "true"))
    {
        ifx_bool(true, ARGV(3), NULL, true);
    }
    else if (FStruEq(ARGV(1), "false"))
    {
        ifx_bool(false, ARGV(3), NULL, true);
    }
    else
    {
        DxMsg(0, __FUNCTION__, __LINE__, "%s: Syntax error. Unsupported mode: %s\n\t%s", ARGV(0),ARGV(1), ARGS());
        ErrVar("Syntax error");
        return;
    }
}


ES_CON_COMMAND5(if, if, "Allows you to conditionalize a script line.", 7, "(<token1> <operator> <token2>) <then/do> [typical script line]\n")

    // es_xif (<token1> equalto <token2>) then <any typical script line>
    //   0    1   2       3        4    5   6  7+     
    char szCommand[MAX_COMMAND_STRING];
    *szCommand = 0;
    bool bBlockBased = false;
 
    // Only supports equalto and notequalto
    if(args.Arg(1)[0]!='(')
    {
        DxMsg(0, __FUNCTION__, __LINE__,"es_xif: Format is:\n  es_xif (<foo> equalto <bar>) then <any typical script line>\n2 Your line was: %s\n", ARGS());    
        ErrVar("Syntax error");
        return;
    }
    const char * pPrePred = args.Arg(6);
    if(args.Arg(5)[0]!=')')
    {
        DxMsg(0, __FUNCTION__, __LINE__,"es_xif: Format is:\n  es_xif (<foo> equalto <bar>) then <any typical script line>\n3 Your line was: %s\n", args.ArgS());
        ErrVar("Syntax error");
        return;
    }

    if(!FStrEq(pPrePred, "then") && !FStrEq(pPrePred, "do"))
    {
        DxMsg(0, __FUNCTION__, __LINE__,"es_xif: Format is:\n  es_xif (<foo> equalto <bar>) then <any typical script line>\n4 Your line was: %s\n", args.ArgS());

        ErrVar("Syntax error");
        return;
    }

    const char *pEnd = NULL;
    char *pSubset = NULL;
    if(FStrEq(pPrePred, "then"))
    {
        pSubset = ") then ";
    }
    if(FStrEq(pPrePred, "do"))
    {
        bBlockBased = true;
        pSubset = ") do";
    }
    pEnd = strstr(args.ArgS(), pSubset);

    if(!pEnd)
    {
        DxMsg(0, __FUNCTION__, __LINE__,"es_xif: Couldn't find then portion. Format is:\n  es_xif (<foo> equalto <bar>) then <any typical script line>\n5 Your line was: %s\n", args.ArgS());
        ErrVar("Syntax error");
        return; 
    }
    pEnd+=strlen(pSubset); 
    if (!bBlockBased)
    {
        strcpy(szCommand, pEnd); 
        strcat(szCommand, "\n");
    }
    const char * pOpString = args.Arg(3);
    ifthen(args, args.Arg(2), pOpString, args.Arg(4), szCommand, bBlockBased);
}


CON_COMMAND( es_xelse,"   es_xelse <any typical script line>\nExecutes the line if the previous 'if' call returned false.\n Does NOT support expansion of EventScripts variables.\n")
{
    if (ARGC() > 1 && FStruEq(ARGV(1), "do"))
    {
        if (!g_ShellVM.GetConditionalFlag())
        {
            g_ShellVM.SetSkipBlockFlag(false);
        }
        else
        {
            g_ShellVM.SetSkipBlockFlag(true);
        }
    }
    else
    {
        if (!g_ShellVM.GetConditionalFlag())
        {
            if (ARGC() > 1)
            {
                char szPreparseCommand[MAX_COMMAND_STRING];
                strcpy(szPreparseCommand, ARGS());
                strcat(szPreparseCommand, "\n");
                ImmediateExecuteCommand(szPreparseCommand);
            }
        }
    }
}

static void es_else(DISPATCH_PARAM); 
static ConCommand elsef_command( "else", es_else, "   else <any typical script line>\nExecutes the line if the previous 'if' call returned false.\n");
static void es_else(DISPATCH_PARAM)
{
    char szPreparseCommand[MAX_COMMAND_STRING];
    strcpy(szPreparseCommand, "es es_xelse");
    if (ARGC() > 1 && FStruEq(ARGV(1), "do"))
    {
        if (!g_ShellVM.GetConditionalFlag())
        {
            g_ShellVM.SetSkipBlockFlag(false);
        }
        else
        {
            g_ShellVM.SetSkipBlockFlag(true);
        }
    }
    else
    {
        if (ARGC() > 1 )
        {   strcat(szPreparseCommand, " ");
            strcat(szPreparseCommand, ARGS());
        }
        strcat(szPreparseCommand, "\n");

        DxMsg(2, __FUNCTION__, __LINE__,"es_else executing: %s\n", szPreparseCommand);

        ImmediateExecuteCommand(szPreparseCommand);
    }
}


// ***********************************
// ** Begin Cr3V3TT3's Math Code
// ***********************************

//using namespace std;
static bool IsInt(const char* s){ // This is for testing if the char s is an int value
    if (strchr(s,'.'))
    {
        return false; // is there is no alpha char but a "." the char s could be a float so not an int
    }
    if (s[0] == '\0') 
    {
        return false;  // empty string is not a number
    }
    char *stopstring = NULL;
    long n = strtol(s, &stopstring, 10);
    if (FStrEq("", stopstring))
    {
        return true;
    }
    else
    {
        return false;
    }
}
static bool IsFloat(const char* s){
    if (strchr(s,'.')) {
        if (s[0] == '\0') 
        {
            return false;  // empty string is not a number
        }
        char *stopstring = NULL;
        double n = strtod(s, &stopstring);
        if (FStrEq("", stopstring))
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

// Purpose for INT cvars
static void SetVar_I(ConVar* cvar, int value) {
        cvar->SetValue(value);
        DxMsg(1, __FUNCTION__, __LINE__,"%s = \"%ld\"\n",cvar->GetName(), cvar->GetInt());
    }
static void AddVar_I(ConVar* cvar, int value)  {
        int n = value + cvar->GetInt();
        cvar->SetValue(n);
        DxMsg(1, __FUNCTION__, __LINE__,"%s = \"%ld\"\n",cvar->GetName(), cvar->GetInt());
    }
static void RemoveVar_I(ConVar* cvar, int value){
    int var = cvar->GetInt();
    var -= value;
    cvar->SetValue(var);
    DxMsg(1, __FUNCTION__, __LINE__,"%s = \"%ld\"\n",cvar->GetName(), cvar->GetInt());
}
static void MultiplyVar_I(ConVar* cvar, int value){
    int n = value * cvar->GetInt();
    cvar->SetValue(n);
    DxMsg(1, __FUNCTION__, __LINE__,"%s = \"%ld\"\n",cvar->GetName(), cvar->GetInt());
}
static void DivideVar_I(ConVar* cvar, int value){
    float var = cvar->GetFloat(); // Setted to float because a divide make float many times
    var /= value;
    cvar->SetValue(var);
    DxMsg(1, __FUNCTION__, __LINE__,"%s = \"%lf\"\n",cvar->GetName(), cvar->GetFloat());
}
static void PowerVar_I(ConVar* cvar, int value){
    int n = pow((float)cvar->GetInt(), value) ;
    cvar->SetValue(n);
    DxMsg(1, __FUNCTION__, __LINE__,"%s = \"%ld\"\n",cvar->GetName(), cvar->GetInt());
}
static void RootVar_I(ConVar* cvar, int value){
    int n = pow(cvar->GetInt(), (1.0f/value));
    cvar->SetValue(n);
    DxMsg(1, __FUNCTION__, __LINE__,"%s = \"%ld\"\n",cvar->GetName(), cvar->GetInt());
}
static void AbsVar_I(ConVar* cvar){
    int f = (int)cvar->GetInt();
    cvar->SetValue(abs(f));
    DxMsg(1, __FUNCTION__, __LINE__,"%s = \"%lf\"\n",cvar->GetName(), cvar->GetFloat());
}

// Purpose for float cvars
static void SetVar_F(ConVar* cvar, float value){
    cvar->SetValue(value);
    DxMsg(1, __FUNCTION__, __LINE__,"%s = \"%lf\"\n",cvar->GetName(), cvar->GetFloat());
}
static void AddVar_F(ConVar* cvar, float value){
    float f = value + cvar->GetFloat();
    cvar->SetValue(f);
    DxMsg(1, __FUNCTION__, __LINE__,"%s = \"%lf\"\n",cvar->GetName(), cvar->GetFloat());
}
static void RemoveVar_F(ConVar* cvar, float value){
    float var = cvar->GetFloat();
    var -= value;cvar->SetValue(var);
    DxMsg(1, __FUNCTION__, __LINE__,"%s = \"%lf\"\n",cvar->GetName(), cvar->GetFloat());
}
static void MultiplyVar_F(ConVar* cvar, float value){
    float f = value * (float)cvar->GetFloat();
    cvar->SetValue(f);
    DxMsg(1, __FUNCTION__, __LINE__,"%s = \"%lf\"\n",cvar->GetName(), cvar->GetFloat());
}
static void DivideVar_F(ConVar* cvar, float value){
    float var = cvar->GetFloat();
    var /= value;
    cvar->SetValue(var);
    DxMsg(1, __FUNCTION__, __LINE__,"%s = \"%lf\"\n",cvar->GetName(), cvar->GetFloat());
}
static void PowerVar_F(ConVar* cvar, float value){
    float f = pow((float)cvar->GetFloat(), value);
    cvar->SetValue(f);
    DxMsg(1, __FUNCTION__, __LINE__,"%s = \"%lf\"\n",cvar->GetName(), cvar->GetFloat());
}
static void RootVar_F(ConVar* cvar, float value){
    float f = pow((float)cvar->GetFloat(), (1.0f/value));
    cvar->SetValue(f);
    DxMsg(1, __FUNCTION__, __LINE__,"%s = \"%lf\"\n",cvar->GetName(), cvar->GetFloat());
}
static void AbsVar_F(ConVar* cvar){
    float f = (float)cvar->GetFloat();
    cvar->SetValue(abs(f));
    DxMsg(1, __FUNCTION__, __LINE__,"%s = \"%lf\"\n",cvar->GetName(), cvar->GetFloat());
}
/*
// Purpose for string cvars
static void SetVar_C(ConVar* cvar, const char * value){ //
    cvar->SetValue(value);
    DxMsg(0, __FUNCTION__, __LINE__,"%s = \"%s\"\n",cvar->GetName(), cvar->GetString());
}
static void AddVar_C(ConVar* cvar, const char * value){ //
    std::string strDest=cvar->GetString();
    strDest += value;
    cvar->SetValue(strDest.c_str());
    DxMsg(0, __FUNCTION__, __LINE__,"%s = \"%s\"\n",cvar->GetName(), cvar->GetString());
}
static void RemoveVar_C(ConVar* cvar, const char * value){ //
    std::string strDest=cvar->GetString();
    int iTermLen = strlen(value);
    int iPos = 0;
    while((iPos = strDest.find(value)) != std::string::npos)
        strDest.replace(strDest.begin() + iPos, strDest.begin() + iPos + iTermLen, "");
    cvar->SetValue(strDest.c_str());
    DxMsg(0, __FUNCTION__, __LINE__,"%s = \"%s\"\n",cvar->GetName(), cvar->GetString());
}*/


static void ReplaceVar_C(ConVar* cvar, const char * value1, const char * value2){ //
    char szFormatted[MAX_COMMAND_STRING];
	const char *d = cvar->GetString();
    if (!V_StrSubst(d, value1, value2, szFormatted, MAX_COMMAND_STRING))
    {
        ErrMsg(0,"ERROR: resulting replaced string too long. Copied as much as we could.\n");
        ErrVar("Resulting string too long.");
    }
    cvar->SetValue(szFormatted);
    DxMsg(1, __FUNCTION__, __LINE__,"%s = \"%s\"\n",cvar->GetName(), cvar->GetString());
}/*
static void LCaseVar_C(ConVar* cvar){ //
    std::string strDest=cvar->GetString();
    transform(strDest.begin(), strDest.end(), strDest.begin(),tolower);
    cvar->SetValue(strDest.c_str());
    DxMsg(0, __FUNCTION__, __LINE__,"%s = \"%s\"\n",cvar->GetName(), cvar->GetString());
}
static void UCaseVar_C(ConVar* cvar){ //
    std::string strDest=cvar->GetString();
    transform(strDest.begin(), strDest.end(), strDest.begin(),toupper);
    cvar->SetValue(strDest.c_str());
    DxMsg(0, __FUNCTION__, __LINE__,"%s = \"%s\"\n",cvar->GetName(), cvar->GetString());
}
*/

// This function takes a slice out of pStr and stores it in pOut.
// It follows the Python slice convention:
// Negative numbers wrap around the string (-1 references the last character).
// Large numbers are clamped to the end of the string.
// VSTDLIB_INTERFACE void Q_StrSlice( const char *pStr, int firstChar, int lastCharNonInclusive, char *pOut, int outSize );

static void CutVar_C(ConVar* cvar, int where, int howlong, ConVar* output)
{
    char szOutput[MAX_FORMATTED];
    Q_StrSlice(cvar->GetString(), where, howlong, szOutput, MAX_FORMATTED);
    output->SetValue(szOutput);
    DxMsg(1, __FUNCTION__, __LINE__,"%s = \"%s\"\n",output->GetName(), output->GetString());
	//ErrMsg(0, "This function is no longer operational! ALERT\n");
}

// RETURNVAL_UNSUPPORTED
CON_COMMAND( es_xmath, "EventScripts math functions, courtesy of Cr3V3TT3")
{
    // es_math <var> <command> <param1> <param2> <param3> <param4>
    if (FStrEq(ARGV(1),"")){
        DxMsg(0, __FUNCTION__, __LINE__,"Specify a var !\n"); // If i made a findvar with a non specified arg that cause crashs
        DxMsg(0, __FUNCTION__, __LINE__,"Syntax: es_xmath <variable> <operator> <value1> [value2]\n");
        ErrVar("Not enough arguments.");
        return;
    }
    ConVar* cvar = FindVar( ARGV(1) ); 
    if (!cvar){
        DxMsg(0, __FUNCTION__, __LINE__,"The var \"%s\" could not be found\n", ARGV(1)); 
        return; // If it doesn t find the var
    }

	if (!CanChange(cvar)){
		return;
	}

    if (ARGC() == 3) {
            if (FStrEq(ARGV(2),"sin")){
                cvar->SetValue((float)sin(cvar->GetFloat()));
                return;
            }
            if (FStrEq(ARGV(2),"cos")){
                cvar->SetValue((float) cos(cvar->GetFloat()));
                return;
            }
            if (FStrEq(ARGV(2),"tan")){
                cvar->SetValue((float)tan(cvar->GetFloat()));
                return;
            }
            // Arctangent of 5.000000 is 78.690068 degrees
            if (FStrEq(ARGV(2),"atan")){
                cvar->SetValue((float)atan(cvar->GetFloat()));
                return;
            }
            // Arcsine of 0.500000 is 30.000000 degrees
            if (FStrEq(ARGV(2),"asin")){
                cvar->SetValue((float)asin(cvar->GetFloat()));
                return;
            }
            // Arccosine of 0.500000 is 60.000000 degrees
            if (FStrEq(ARGV(2),"acos")){
                cvar->SetValue((float)acos(cvar->GetFloat()));
                return;
            }
            if (FStrEq(ARGV(2),"float")){
                float blah = cvar->GetFloat();
                 char szNewValue[80];
                sprintf(szNewValue, "%10.10f", blah);
                cvar->SetValue(0.001f);
                cvar->SetValue(szNewValue);
                return;
            }
            if (FStrEq(ARGV(2),"int")){
                float blah = cvar->GetInt();
                cvar->SetValue(0);
                cvar->SetValue(blah);
                return;
            }
    }
    

    if (IsInt(cvar->GetString())){ // if the var is an int use int command *Var_I
        if (ARGC() == 3){
            if (FStrEq(ARGV(2),"abs")){
                // If arg 2 is "root" then do the command root
                // es_math <myvar> root <new value>
                // eg : es_math myvar root 2
                // myvar = 2
                AbsVar_I(cvar);
                return;
            }
            else{
                DxMsg(0, __FUNCTION__, __LINE__,"Bad operation\n");
                ErrVar("Invalid operation");
                // The operation that the user use is not unknow (bad version) or just a bad syntax
                return;
            }
        }
        else if (ARGC() == 4){
            if (FStrEq(ARGV(2),"add")||FStrEq(ARGV(2),"+")){
                // If arg 2 is "add" or "+" then do the command for increment
                // es_math <myvar> add <value>
                // es_math <myvar> + <value>
                // eg : es_setinfo myvar 40 ;es_math myvar + 50
                // myvar = 90
                AddVar_I(cvar, atoi(ARGV(3))); 
                return;
            }
            else if (FStrEq(ARGV(2),"subtract")||FStrEq(ARGV(2),"-")){
                // If arg 2 is "remove" or "-" then do the command for decrease
                // es_math <myvar> remove <value>
                // es_math <myvar> - <value>
                // eg : es_setinfo myvar 40 ;es_math myvar - 10
                // myvar = 30
                RemoveVar_I(cvar, atoi(ARGV(3)));
                return;
            }
            else if (FStrEq(ARGV(2),"multiply")||FStrEq(ARGV(2),"*")){
                // If arg 2 is "multiply" or "*" then do the command for multiply
                // es_math <myvar> mutliply <value>
                // es_math <myvar> * <value>
                // eg : es_setinfo myvar 10 ;es_math myvar * 3
                // myvar = 30
                MultiplyVar_I(cvar, atoi(ARGV(3)));
                return;
            }
            else if (FStrEq(ARGV(2),"divide")||FStrEq(ARGV(2),"/")){
                // If arg 2 is "divide" or "-" then do the command for divide
                // es_math <myvar> divide <value>
                // es_math <myvar> / <value>
                // eg : es_setinfo myvar 40 ;es_math myvar / 20
                // myvar = 2.000000
                // PS : Divides always return float for now
                // TODO : Made something like a check to know if the divide dont return
                //            int and not float
                DivideVar_I(cvar, atoi(ARGV(3)));
                return;
            }
            else if (FStrEq(ARGV(2),"set")||FStrEq(ARGV(2),"=")){
                // If arg 2 is "set" or "=" then do the command set
                // es_math <myvar> set <new value>
                // es_math <myvar> = <new value>
                // eg : es_math myvar = 50
                // myvar = 50
                SetVar_I(cvar, atoi(ARGV(3)));
                return;
            }
            else if (FStrEq(ARGV(2),"power")||FStrEq(ARGV(2),"^")){
                // If arg 2 is "power" or "^" then do the command power
                // es_math <myvar> power <new value>
                // es_math <myvar> ^ <new value>
                // eg : es_math myvar ^ 2
                // myvar = 4
                PowerVar_I(cvar, atoi(ARGV(3)));
                return;
            }
            else if (FStrEq(ARGV(2),"root")){
                // If arg 2 is "root" then do the command root
                // es_math <myvar> root <new value>
                // eg : es_math myvar root 2
                // myvar = 2
                RootVar_I(cvar, atoi(ARGV(3)));
                return;
            }
            else{
                DxMsg(0, __FUNCTION__, __LINE__,"Bad operation\n");
                ErrVar("Invalid operation");
                // The operation that the user use is not unknow (bad version) or just a bad syntax
                return;
            }
        }
        else{
            DxMsg(0, __FUNCTION__, __LINE__,"Incorrect number of parameter\n");
            ErrVar("Incorrect number of parameters");
            return;
        }
    }
    else if (IsFloat(cvar->GetString())){
        if (ARGC() == 3){
            if (FStrEq(ARGV(2),"abs")){
                // If arg 2 is "root" then do the command root
                // es_math <myvar> root <new value>
                // eg : es_math myvar root 2
                // myvar = 2
                AbsVar_F(cvar);
                return;
            }
            else{
                DxMsg(0, __FUNCTION__, __LINE__,"Bad operation\n");
                ErrVar("Invalid operation");
                // The operation that the user use is not unknow (bad version) or just a bad syntax
                return;
            }
        }
        if (ARGC() == 4){
            if (FStrEq(ARGV(2),"add")||FStrEq(ARGV(2),"+")){
                AddVar_F(cvar, atof(ARGV(3)));
                return;
            }
            else if (FStrEq(ARGV(2),"subtract")||FStrEq(ARGV(2),"-")){
                RemoveVar_F(cvar, atof(ARGV(3)));
                return;
            }
            else if (FStrEq(ARGV(2),"multiply")||FStrEq(ARGV(2),"*")){
                MultiplyVar_F(cvar, atof(ARGV(3)));
                return;
            }
            else if (FStrEq(ARGV(2),"divide")||FStrEq(ARGV(2),"/")){
                DivideVar_F(cvar, atof(ARGV(3)));
                return;
            }
            else if (FStrEq(ARGV(2),"set")||FStrEq(ARGV(2),"=")){
                SetVar_F(cvar, atof(ARGV(3)));
                return;
            }
            else if (FStrEq(ARGV(2),"power")||FStrEq(ARGV(2),"^")){
                PowerVar_F(cvar, atoi(ARGV(3)));
                return;
            }
            else if (FStrEq(ARGV(2),"root")){
                RootVar_F(cvar, atoi(ARGV(3)));
                return;
            }
            else{
                DxMsg(0, __FUNCTION__, __LINE__,"Bad operation\n");
                ErrVar("Bad operation");
                return;
            }
        }
        else {
            DxMsg(0, __FUNCTION__, __LINE__,"Incorrect number of parameter\n"); 
            ErrVar("Incorrect number of parameters");
        }
    }
    else
    {
		DxMsg(0, __FUNCTION__, __LINE__,"Math function called on a non-numeric value: %s, %s\n", cvar->GetName(), cvar->GetString());
        ErrVar("Math function called on a non-numeric value.");
    }
} 


// RETURNVAL_UNSUPPORTED
CON_COMMAND( es_xstring, "EventScripts string functions, experimental support originally written by Cr3V3TT3")
{
    // es_string <var> <command> <param1> <param2> <param3> <param4>
    if (FStrEq(ARGV(1),"")){
        DxMsg(0, __FUNCTION__, __LINE__,"Specify a var !\n"); // If i made a findvar with a non specified arg that cause crashs
        DxMsg(0, __FUNCTION__, __LINE__,"Syntax: es_xstring <variable> <operator> <value1> [value2]\n");
        ErrVar("Not enough arguments.");
        return;
    }
    ConVar* cvar = FindVar( ARGV(1) );
    if (!cvar){
        DxMsg(0, __FUNCTION__, __LINE__,"The var \"%s\" could not be found\n", ARGV(1)); 
        return; // If it doesn t find the var
    }
	if (!CanChange(cvar)){
		return;
	}
    if (ARGC() == 4) {
        if (FStrEq(ARGV(2),"replace")){
            // If arg 2 is "replace" then do the command to replace
            // es_math <myvar> replace <the char to search> <the char to replace>
            // eg : es_setinfo myvar "Hi, are you there?" ;es_math myvar replace "there" "french"
            // myvar = "Hi, are you french?"
            ReplaceVar_C(cvar, ARGV(3), "");
            return;
        }else if (FStrEq(ARGV(2),"replacev")){
            // If arg 2 is "replace" then do the command to replace
            // es_math <myvar> replace <the char to search> <the char to replace>
            // eg : es_setinfo myvar "Hi, are you there?" ;es_math myvar replace "there" "french"
            // myvar = "Hi, are you french?"
            ConVar *pOne = GetVar(ARGV(3));
            if (pOne)
                ReplaceVar_C(cvar,pOne->GetString(), "");
            return;
        }
    }else if (ARGC() == 5){
        if (FStrEq(ARGV(2),"replace")){
            // If arg 2 is "replace" then do the command to replace
            // es_math <myvar> replace <the char to search> <the char to replace>
            // eg : es_setinfo myvar "Hi, are you there?" ;es_math myvar replace "there" "french"
            // myvar = "Hi, are you french?"
            ReplaceVar_C(cvar, ARGV(3), ARGV(4));
            return;
        }
        else if (FStrEq(ARGV(2),"replacev")){
            // If arg 2 is "replace" then do the command to replace
            // es_math <myvar> replace <the char to search> <the char to replace>
            // eg : es_setinfo myvar "Hi, are you there?" ;es_math myvar replace "there" "french"
            // myvar = "Hi, are you french?"
            ConVar *pOne = GetVar(ARGV(3));
            ConVar *pTwo = GetVar(ARGV(4));
            if (pOne && pTwo)
                ReplaceVar_C(cvar,pOne->GetString(), pTwo->GetString());
            return;
        }
        else if (FStrEq(ARGV(2),"section"))
        {
                // If arg 2 is "cut" then do the command to cut
                // es_math <myvar> cut <start char> <number of char>
                // eg : es_setinfo myvar "Hi there, are you there?" ;es_math myvar cut 0 8
                // myvar = "Hi there"
                // PS : if the output is not given (4 params) then use the same var
                CutVar_C(cvar, atoi(ARGV(3)), atoi(ARGV(4)), cvar);
                return;
        }
        else {
            DxMsg(0, __FUNCTION__, __LINE__,"Incorrect number of parameter or bad operation\n");
            ErrVar("Incorrect number of parameters");
            return;
        }
    }
    else {
        DxMsg(0, __FUNCTION__, __LINE__,"Incorrect number of parameters.\n");
        ErrVar("Incorrect number of parameters");
    }
}

CON_COMMAND( es_string, "EventScripts string functions, courtesy of Cr3V3TT3. (Supported operators: replace and section.) Expands EventScripts variables for its parameters when found.")
{
    if (ARGC() < 3)
    {
        DxMsg(0, __FUNCTION__, __LINE__,"Syntax: es_string <variable> <operator> <value1> [value2]\n");
        ErrVar("Not enough arguments.");
        return;
    }
    char szPreparseCommand[MAX_COMMAND_STRING];
    strcpy(szPreparseCommand, "es es_xstring ");
    strcat(szPreparseCommand, ARGS());
    strcat(szPreparseCommand, "\n");

    DxMsg(1, __FUNCTION__, __LINE__,"es_string executing: %s\n", szPreparseCommand); 
    ImmediateExecuteCommand(szPreparseCommand);
}

CON_COMMAND( es_math, "EventScripts math functions, courtesy of Cr3V3TT3. Expands EventScripts variables for its parameters when found.")
{
    if (ARGC() < 3)
    {
        DxMsg(0, __FUNCTION__, __LINE__,"Syntax: es_math <variable> <operator> <value1> [value2]\n");
        ErrVar("Not enough arguments.");
        return;
    }
    char szPreparseCommand[MAX_COMMAND_STRING];
    strcpy(szPreparseCommand, "es es_xmath ");
    strcat(szPreparseCommand, ARGS());
    strcat(szPreparseCommand, "\n");

    DxMsg(2, __FUNCTION__, __LINE__,"es_math executing: %s\n", szPreparseCommand);
    ImmediateExecuteCommand(szPreparseCommand);
}

// ***********************************
// ** End Cr3V3TT3's Math Code
// ***********************************


// ***********************************
// ** Begin Cr3V3TT3's es_xtoken Code
// ***********************************
// RETURNVAL_UNSUPPORTED

CON_COMMAND( es_xtoken, "Set a variable to a particular token in the string.\n Syntax: es_xtoken <variable> <string> [token#] [tokenchar]")
{
    if ((ARGC() <= 5)&&(ARGC() >= 3))
    {
        int iSeps = 0;
        const char* cSeps = "";
        if (ARGC() == 5)
        {
            iSeps = strlen(ARGV(4));
            cSeps = ARGV(4);
        }
        if (ARGC() == 4)
        {
            iSeps = 1;
            cSeps = " ";
        }

        DxMsg(2, __FUNCTION__, __LINE__,"es_token, cSeps: \"%s\", iSeps: %d\n", cSeps, iSeps);

        CUtlVector<char*, CUtlMemory<char*> > foo;

        //Q_SplitString2(ARGV(2), ((const char**) &cSeps), iSeps, foo );
        //  Should work :/ valve problem? or recast problem from ** to * ? that was the first time for me to see **

        Q_SplitString(ARGV(2), cSeps, foo );

        if (FStrEq(ARGV(1),""))
        {
            DxMsg(0, __FUNCTION__, __LINE__,"Specify a var !\n"); // If i made a findvar with a non specified arg that cause crashs
            ErrVar("Not enough arguments.");
			foo.PurgeAndDeleteElements();
            return;
        }
        ConVar* cvar = NULL;
        if (FStrEq(ARGV(3),"0")) 
        {
            cvar = SetConVar( ARGV(1), foo.Count());
        }
        else
        {
            if (atoi(ARGV(3))-1 >= foo.Count() || atoi(ARGV(3))-1 < 0)
            {
                DxMsg(0, __FUNCTION__, __LINE__,"The specified token number %d doesn't exist\n", ARGV(3));
                ErrVar("Specified token doesn't exist.");
				foo.PurgeAndDeleteElements();
                return; 
            }
            cvar = SetConVar( ARGV(1), foo.Element(atoi(ARGV(3))-1));
        }
        if (!cvar)
        {
            DxMsg(0, __FUNCTION__, __LINE__,"The var \"%s\" could not be set.\n", ARGV(1)); 
            foo.PurgeAndDeleteElements();
            return; // If it doesn t find the var
        }
        else
        {
            DxMsg(1, __FUNCTION__, __LINE__,"%s = \"%s\"\n",cvar->GetName(), cvar->GetString());
        }
        foo.PurgeAndDeleteElements();
    }
    else
    {
        DxMsg(0, __FUNCTION__, __LINE__,"Incorrect number of parameters\n    Syntax: es_xtoken <variable> <string> <token#> [tokenchars]\n");
        ErrVar("Not enough arguments.");
        return;
    }
}

CON_COMMAND( es_token, "Set a variable to a particular token in the string.\n Syntax: es_token <variable> <string> [token#] [tokenchar]")
{
    if (ARGC() < 3)
    {
        DxMsg(0, __FUNCTION__, __LINE__,"Syntax: es_token <variable> <string> [token#] [tokenchar]\n");
        ErrVar("Not enough arguments.");
        return;
    } 
    char szPreparseCommand[MAX_COMMAND_STRING];
    strcpy(szPreparseCommand, "es es_xtoken "); 
    strcat(szPreparseCommand, ARGS());
    strcat(szPreparseCommand, "\n");

    DxMsg(2, __FUNCTION__, __LINE__,"es_token executing: %s", szPreparseCommand);
    ImmediateExecuteCommand(szPreparseCommand);
}

// ***********************************
// ** End Cr3V3TT3's es_xtoken Code
// ***********************************

// ***********************************
// ** Begin Cr3V3TT3's es_rand Code
// ***********************************
// RETURNVAL_UNSUPPORTED
CON_COMMAND( es_xrand, "Syntax : es_rand <cvar> <minimum> <maximum>\n" )
{
    if (ARGC() != 4)
    {
        DxMsg(0, __FUNCTION__, __LINE__,"Syntax: es_xrand <cvar> <minimum> <maximum>\n");
        ErrVar("Not enough arguments.");
        return;
    }
    if (FStrEq(ARGV(1),""))
    {
        DxMsg(0, __FUNCTION__, __LINE__,"Specify a var !\n"); // If i made a findvar with a non specified arg that cause crashs
        ErrVar("Not enough arguments.");
        return;
    }
	int start = atoi(ARGV(2));
	int finish = atoi(ARGV(3));
	int i = start;
	if (start != finish)
	{
		i = (rand() % (finish-start+1))+start;
	}
    // set i as a random int between arg2 and arg3
    ConVar* cvar = SetConVar( ARGV(1), i );
    if (!cvar)
    {
        DxMsg(0, __FUNCTION__, __LINE__,"The var \"%s\" could not be set\n", ARGV(1)); 
        return; // If it doesn t find the var
    }
    else {
        DxMsg(1, __FUNCTION__, __LINE__,"%s = \"%s\"\n",cvar->GetName(), cvar->GetString());
    }
}

CON_COMMAND( es_rand, "Place a random value between <minimum> and <maximum> to <cvar>.")
{
    if (ARGC() < 4)
    {
        DxMsg(0, __FUNCTION__, __LINE__,"Syntax: es_rand <cvar> <minimum> <maximum>\n");
        ErrVar("Not enough arguments.");
        return;
    }
    char szPreparseCommand[MAX_COMMAND_STRING];
    strcpy(szPreparseCommand, "es es_xrand ");
    strcat(szPreparseCommand, ARGS());
    strcat(szPreparseCommand, "\n");
    DxMsg(1, __FUNCTION__, __LINE__,"es_rand executing: %s", szPreparseCommand);
    ImmediateExecuteCommand(szPreparseCommand);
}
// ***********************************
// ** End Cr3V3TT3's es_rand Code
// ***********************************



