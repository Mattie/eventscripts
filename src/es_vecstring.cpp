#ifdef ORANGEBOX
#include "mathlib/vector.h"
#else
#include "vector.h"
#endif
#include "es_cmddef.h"
#include "es_convar.h"

#define MAX_VECTOR 256

char * AllocStringFromVector(Vector &vValue)
{
    char szVector[MAX_VECTOR];
    if (!vValue.IsValid())
    {
        strcpy(szVector, "0.0,0.0,0.0");
    }
    else
    {
        sprintf(szVector, "%f,%f,%f", vValue.x, vValue.y, vValue.z);
    }
    return strdup(szVector);
}

Vector * AllocVectorFromString(const char *szString)
{
    if (szString)
    {
        float x, y, z;
        if (3==sscanf(szString, "%f,%f,%f", &x, &y, &z))
        {
            return new Vector(x, y, z);
        }
        else
        {
            DxMsg(0, __FUNCTION__, __LINE__,"AllocVectorFromString: invalid string format\n");
            ErrVar("Incorrect arguments");
            return new Vector(0, 0, 0);
        }   
    }
    else
    {
        return new Vector(0, 0, 0);
    }
}

// delete Vector
Vector * CreateVectorFrom(Vector &A, Vector &B)
{
    float x = B.x - A.x;
    float y = B.y - A.y;
    float z = B.z - A.z;
    Vector *v = new Vector(x, y, z);
    return v;
}

// RETURNVAL
ES_CON_COMMAND2(createvectorfrompoints, 
               "Creates a vector-string that goes from point/vector A to point/vector B.\n",
               4,
               "<output-var> <vectorstring-A> <vectorstring-B>")
    int nFirstArg = bUseServerVariablesForReturn ? 0 : -1;
    nCount+=abs(nFirstArg);

    Vector *A = AllocVectorFromString(args.Arg(nFirstArg+2));
    Vector *B = AllocVectorFromString(args.Arg(nFirstArg+3));
    if (A && B)
    {
        Vector *v = CreateVectorFrom(*A, *B);
        if (v)
        {
            char *pv = AllocStringFromVector(*v);
            RETURNVAL(args.Arg(1), pv, "s");
            if (pv) free (pv);
            if (v) delete v;
        }
        else
        {
            DxMsg(0, __FUNCTION__, __LINE__,"Could not create new vector.\n");    
            ErrVar("Internal error");
        }
        delete A;
        delete B;
    }
    else
    {
        DxMsg(0, __FUNCTION__, __LINE__,"Could not create vectors from strings.\n");
        ErrVar("Internal error");
    }
}

// RETURNVAL
ES_CON_COMMAND2(createvectorstring, 
               "Creates a string form of three x y z variables representing a vector.\n",
               5,
               "<output-var> <x> <y> <z>")
    int nFirstArg = bUseServerVariablesForReturn ? 0 : -1;
    nCount+=abs(nFirstArg);

    float x = atof(args.Arg(nFirstArg+2));
    float y = atof(args.Arg(nFirstArg+3));
    float z = atof(args.Arg(nFirstArg+4));
    DxMsg(1, __FUNCTION__, __LINE__,"es_xcreatevectorstring: %f %f %f\n", x, y, z);
    Vector *v = new Vector(x, y, z);
    char *pv = AllocStringFromVector(*v);

    RETURNVAL(args.Arg(1), pv, "s");
    if (pv) free (pv);
    if (v) delete v;
}

// RETURNVAL x3
ES_CON_COMMAND2(splitvectorstring, 
               "Stores the vector's current x, y, and z as read from the vector in string form.\n",
               5,
               "<var x> <var y> <var z> <vectorstring>")
    int nFirstArg = bUseServerVariablesForReturn ? 0 : -3;
    nCount+=abs(nFirstArg);

    float d[4];
    memset(d, 0, sizeof(d));
    const char *vstring = args.Arg(nFirstArg+4);
    Vector *v = AllocVectorFromString(vstring);
    v->CopyToArray(d);
    DxMsg(2, __FUNCTION__, __LINE__, "splitvectorstring: %s, %d %d %d\n", vstring, d[0], d[1], d[2]);
    RETURNVAL3(args.Arg(1), d[0], "f", args.Arg(2), d[1], "f", args.Arg(3), d[2], "f");
    if (v) delete v;
}


