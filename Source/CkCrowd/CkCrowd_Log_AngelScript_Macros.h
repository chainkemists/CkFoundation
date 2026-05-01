#pragma once

// --------------------------------------------------------------------------------------------------------------------
// AngelScript binding includes for the CkCrowd log functions.
//
// CK_REGISTER_LOG_FUNCTIONS_WITH_AS expands to a binding registration block
// that uses AS_FORCE_LINK / FAngelscriptBinds. Those symbols live in the
// Angelscript module. CkLog_Utils.h declares the macro but doesn't pull in
// the AS headers — they must come from the consuming .cpp's include chain.
//
// This header centralises the AS-side include so every CkCrowd .cpp that
// expands the macro picks it up consistently.
// --------------------------------------------------------------------------------------------------------------------

#if WITH_ANGELSCRIPT_CK
#include "AngelscriptBinds.h"
#endif
