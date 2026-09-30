#include "CkRotateTowards_DebugSettings.h"

#include "CkCore/Object/CkObject_Utils.h"
#include "HAL/IConsoleManager.h"

namespace ck_rotate_towards_debug_settings_cvars
{
    int32 GDrawTargets = 0;

    template <typename TFieldGetter, typename TFieldSetter>
    auto
    WriteToSettings(TFieldGetter&& InFieldGetter, TFieldSetter&& InFieldSetter, IConsoleVariable* InCVar)
        -> void
    {
        if (InCVar == nullptr)
        { return; }

        auto* Settings = GetMutableDefault<UCk_RotateTowards_DebugSettings_UE>();
        if (ck::Is_NOT_Valid(Settings))
        { return; }

        const auto Value = InCVar->GetInt() != 0;
        if (InFieldGetter(Settings) == Value)
        { return; }

        InFieldSetter(Settings, Value);
        Settings->SaveConfig();
    }

    FAutoConsoleVariableRef CVarDrawTargets(
        TEXT("ck.RotateTowards.DrawTargets"), GDrawTargets,
        TEXT("Draw each rotate-towards entity's forward, desired forward, target line, rest line, yaw range edges and readout. 0 = off (default), 1 = on."),
        FConsoleVariableDelegate::CreateLambda([](IConsoleVariable* InCVar)
        {
            WriteToSettings(
                [](UCk_RotateTowards_DebugSettings_UE* InSettings) { return InSettings->Get_DrawTargets(); },
                [](UCk_RotateTowards_DebugSettings_UE* InSettings, bool InValue) { InSettings->Set_DrawTargets(InValue); },
                InCVar);
        }), ECVF_Cheat);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_RotateTowards_DebugSettings_UE::
    PostInitProperties()
    -> void
{
    Super::PostInitProperties();
    if (NOT IsTemplate())
    { return; }

    if (auto* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("ck.RotateTowards.DrawTargets")))
    { CVar->Set(_DrawTargets ? 1 : 0, static_cast<EConsoleVariableFlags>(CVar->GetFlags() & ECVF_SetByMask)); }
}

#if WITH_EDITOR
auto
    UCk_RotateTowards_DebugSettings_UE::
    PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
    -> void
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    const auto Name = PropertyChangedEvent.GetPropertyName();
    if (Name == GET_MEMBER_NAME_CHECKED(UCk_RotateTowards_DebugSettings_UE, _DrawTargets))
    {
        if (auto* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("ck.RotateTowards.DrawTargets")))
        { CVar->Set(_DrawTargets ? 1 : 0, static_cast<EConsoleVariableFlags>(CVar->GetFlags() & ECVF_SetByMask)); }
    }
}
#endif

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_RotateTowards_DebugSettings_UE::
    Get_DrawTargets()
    -> bool
{
    const auto* Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_RotateTowards_DebugSettings_UE>();
    return ck::IsValid(Settings) && Settings->Get_DrawTargets();
}
