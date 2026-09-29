#include "CkSway_DebugSettings.h"

#include "CkCore/Object/CkObject_Utils.h"
#include "HAL/IConsoleManager.h"

namespace ck_sway_debug_settings_cvars
{
    int32 GDrawOffsets = 0;

    template <typename TFieldGetter, typename TFieldSetter>
    auto
    WriteToSettings(TFieldGetter&& InFieldGetter, TFieldSetter&& InFieldSetter, IConsoleVariable* InCVar)
        -> void
    {
        if (InCVar == nullptr)
        { return; }

        auto* Settings = GetMutableDefault<UCk_Sway_DebugSettings_UE>();
        if (ck::Is_NOT_Valid(Settings))
        { return; }

        const auto Value = InCVar->GetInt() != 0;
        if (InFieldGetter(Settings) == Value)
        { return; }

        InFieldSetter(Settings, Value);
        Settings->SaveConfig();
    }

    FAutoConsoleVariableRef CVarDrawOffsets(
        TEXT("ck.Sway.DrawOffsets"), GDrawOffsets,
        TEXT("Draw sway parent axes, the parent-to-node line and the offset/stimulus readout. 0 = off (default), 1 = on."),
        FConsoleVariableDelegate::CreateLambda([](IConsoleVariable* InCVar)
        {
            WriteToSettings(
                [](UCk_Sway_DebugSettings_UE* InSettings) { return InSettings->Get_DrawOffsets(); },
                [](UCk_Sway_DebugSettings_UE* InSettings, bool InValue) { InSettings->Set_DrawOffsets(InValue); },
                InCVar);
        }), ECVF_Cheat);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Sway_DebugSettings_UE::
    PostInitProperties()
    -> void
{
    Super::PostInitProperties();
    if (NOT IsTemplate())
    { return; }

    if (auto* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("ck.Sway.DrawOffsets")))
    { CVar->Set(_DrawOffsets ? 1 : 0, static_cast<EConsoleVariableFlags>(CVar->GetFlags() & ECVF_SetByMask)); }
}

#if WITH_EDITOR
auto
    UCk_Sway_DebugSettings_UE::
    PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
    -> void
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    const auto Name = PropertyChangedEvent.GetPropertyName();
    if (Name == GET_MEMBER_NAME_CHECKED(UCk_Sway_DebugSettings_UE, _DrawOffsets))
    {
        if (auto* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("ck.Sway.DrawOffsets")))
        { CVar->Set(_DrawOffsets ? 1 : 0, static_cast<EConsoleVariableFlags>(CVar->GetFlags() & ECVF_SetByMask)); }
    }
}
#endif

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Sway_DebugSettings_UE::
    Get_DrawOffsets()
    -> bool
{
    const auto* Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Sway_DebugSettings_UE>();
    return ck::IsValid(Settings) && Settings->Get_DrawOffsets();
}
