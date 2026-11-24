#include "es_util.h"
#include "es_cmddef.h"
#include "es_convar.h"
#include "muParser/muParser.h"

// muParser integration
using namespace mu;
mu::Parser g_MathParser;

int g_nParserVariableCount = 0;
float NotParserCallback(float fVal)
{
  return !fVal;
}
void ParseResetVariables()
{
    g_MathParser.ClearVar();
}


//---------------------------------------------------------------------------
// Factory function for creating new parser variables
// This could as well be a function performing database queries.
float* ParseAddVariable(const char *a_szName)
{
  static float Zero = 0;

  ConVar * pVar = GetVar(a_szName);
#ifdef ALIENSWARM
    ConVar::CVValue_t * cvardata = &(pVar->GetRawValue());
#else
    ConVar *cvardata = pVar;
#endif
  if (pVar)
  {
    return &(cvardata->m_fValue);
  }
  else
  {
    return &Zero;
  }
}

void InitializeMathParse()
{
    // Define an infix operator
    g_MathParser.DefineInfixOprt("!", NotParserCallback);
    // Define the variable factory
    g_MathParser.SetVarFactory(ParseAddVariable);
    g_MathParser.EnableOptimizer();

}

// RETURNVAL_UNSUPPORTED
ES_CON_COMMAND2(mathparse,
               "Adds a say command that refers to a particular block.",
               2,
               "<var> <math-expression>")
  using namespace mu;

  try
  {
    ConVar *ctest = FindVar(args.Arg(1));
    if (!ctest){
        DxMsg(0, __FUNCTION__, __LINE__,"The var \"%s\" could not be set\n", args.Arg(1)); 
        ErrVar("Variable does not exist");
        return; // If it doesn t find the var
    }
	if (!CanChange(ctest)){
		return;
	}

    g_MathParser.SetExpr(args.Arg(2));
    double output = g_MathParser.Eval();
    // always want int if
    //    float and int are the same and non-zero (means the string is a number)
    //    float and int are zero and string 0.00000
    //
    // 0 0.00000 0.00000  -- int
    // 1 1.00000 1.00000  -- int
    // 2 2.10000 2.10000  -- float
    // 0 0.00000 Blah     -- string
    // 3 3.00000 3        -- int
    //if ((((float) ctest->GetInt() == ctest->GetFloat()) && ctest->GetInt() != 0 && strlen(ctest->GetString())>0) ||
    //    (ctest->GetInt() == 0 && ctest->GetFloat() == 0 && FStrEq("0.000000", ctest->GetString()))) 
    if (output - (float)(int)output != 0.0f)
    {
        ctest->SetValue((float)output);
    }
    else
    {
        char szTempFloat[512];
        // we convert int to string and use that.
        //ltoa(ctest->GetInt(), szTempFloat, 10);
        sprintf(szTempFloat, "%ld", (long)output);
        ctest->SetValue(szTempFloat);
    }
  }
  catch (Parser::exception_type &e)
  {
      DxMsg(0, __FUNCTION__, __LINE__, "Exception: %s!\n", e.GetMsg().c_str());
      //DxMsg(0, __FUNCTION__, __LINE__, "Exception: %d!\n", (int)e.GetPos());
  }
    //ParseResetVariables();
}


