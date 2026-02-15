#include "CkWatermarkStat_EcsDebugger_Widget.h"

#include "CkEcs/Settings/CkEcs_Settings.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkWatermarkStat_EcsDebugger_UWidget_UE::
    NativeGetStatName() const
    -> FText
{
    return FText::FromString(TEXT("ECS Debugger"));
}

auto
    UCkWatermarkStat_EcsDebugger_UWidget_UE::
    NativeGetStatValue() const
    -> FText
{
    switch (UCk_Utils_Ecs_Settings_UE::Get_HandleDebuggerBehavior())
    {
        case ECk_Ecs_HandleDebuggerBehavior::Disable:                     return FText::FromString(TEXT("Off"));
        case ECk_Ecs_HandleDebuggerBehavior::Enable:                      return FText::FromString(TEXT("On"));
        case ECk_Ecs_HandleDebuggerBehavior::EnableWithBlueprintDebugging: return FText::FromString(TEXT("On (BP)"));
        default:                                                            return FText::FromString(TEXT("---"));
    }
}

// --------------------------------------------------------------------------------------------------------------------
