#pragma once

// --------------------------------------------------------------------------------------------------------------------
// CK_REGISTER_LOG_FUNCTIONS_WITH_AS expands to AS_FORCE_LINK / FAngelscriptBinds usage, but
// CkLog_Utils.h does not pull in the AS headers — every CkCrowd .cpp that expands the macro
// includes this header to get them.
// --------------------------------------------------------------------------------------------------------------------

#if WITH_ANGELSCRIPT_CK
#include "AngelscriptBinds.h"
#endif
