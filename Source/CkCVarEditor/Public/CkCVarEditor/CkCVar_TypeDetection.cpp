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

    // 2. Fallback: query IConsoleManager for engine console objects
    auto* ConsoleObject = IConsoleManager::Get().FindConsoleObject(*InCVarName.ToString());
    if (ConsoleObject == nullptr)
    {
        return {};
    }

    // Check if it's a command (not a variable)
    if (ConsoleObject->AsCommand() != nullptr && ConsoleObject->AsVariable() == nullptr)
    {
        return ECk_CVarType::Command;
    }

    auto* CVar = ConsoleObject->AsVariable();
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
