#pragma once

#include "CkCVar/CkCVar_Data.h"

// --------------------------------------------------------------------------------------------------------------------

// Convenience wrapper — delegates to FCk_CVar_DelegateSignatureHolder::GetSignatureFunctionForType
// which lives in the CkCVar module alongside the delegate declarations.
namespace ck::cvar
{
    inline auto GetDelegateSignatureFunctionForType(
        ECk_CVarType InType) -> UFunction*
    {
        return FCk_CVar_DelegateSignatureHolder::GetSignatureFunctionForType(InType);
    }
}

// --------------------------------------------------------------------------------------------------------------------
