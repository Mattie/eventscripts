#ifdef WIN32
#include "es_convar.h"

//=========================================================================
// Benchmark console command.
//=========================================================================
// Kept for scripts that still call cbench.
CON_COMMAND(cbench, "Do some benchmarks for C++")
{
#ifdef ORANGEBOX
	(void)args;
#endif
}

#endif // WIN32
