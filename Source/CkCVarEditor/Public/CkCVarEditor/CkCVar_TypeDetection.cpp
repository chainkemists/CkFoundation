#include "CkCVar_TypeDetection.h"

#include "CkCVar/Settings/CkCVar_Settings.h"

#include <HAL/IConsoleManager.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::cvar::
    DetectCVarType(
        FName InCVarName)
    -> TOptional<ECk_CVarType>
{
    if (InCVarName == NAME_None)
    {
        return {};
    }

    // 1. Check our persistent settings registry first (has explicit type info)
    if (const auto Type = UCk_CVar_Settings_UE::Get()->GetType(InCVarName))
    {
        return Type;
    }

    // 2. Fallback: query IConsoleManager for engine CVars
    auto* CVar = IConsoleManager::Get().FindConsoleVariable(*InCVarName.ToString());
    if (CVar == nullptr)
    {
        return {};
    }

    if (CVar->IsVariableBool())
    {
        return ECk_CVarType::Bool;
    }

    if (CVar->IsVariableInt())
    {
        return ECk_CVarType::Int32;
    }

    if (CVar->IsVariableFloat())
    {
        return ECk_CVarType::Float;
    }

    if (CVar->IsVariableString())
    {
        return ECk_CVarType::String;
    }

    // Default to string if type cannot be determined
    return ECk_CVarType::String;
}

// --------------------------------------------------------------------------------------------------------------------
