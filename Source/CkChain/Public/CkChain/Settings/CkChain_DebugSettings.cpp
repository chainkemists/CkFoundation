#include "CkChain_DebugSettings.h"

#include "CkCore/Object/CkObject_Utils.h"
#include "HAL/IConsoleManager.h"

namespace ck_chain_debug_settings_cvars
{
    int32 GDrawHistory = 0;
    int32 GDrawLinkTargets = 0;

    template <typename TFieldGetter, typename TFieldSetter>
    auto
    WriteToSettings(TFieldGetter&& InFieldGetter, TFieldSetter&& InFieldSetter, IConsoleVariable* InCVar)
        -> void
    {
        if (InCVar == nullptr)
        { return; }

        auto* Settings = GetMutableDefault<UCk_Chain_DebugSettings_UE>();
        if (ck::Is_NOT_Valid(Settings))
        { return; }

        const auto Value = InCVar->GetInt() != 0;
        if (InFieldGetter(Settings) == Value)
        { return; }

        InFieldSetter(Settings, Value);
        Settings->SaveConfig();
    }

    FAutoConsoleVariableRef CVarDrawHistory(
        TEXT("ck.Chain.DrawHistory"), GDrawHistory,
        TEXT("Draw chain history samples and polyline. 0 = off (default), 1 = on."),
        FConsoleVariableDelegate::CreateLambda([](IConsoleVariable* InCVar)
        {
            WriteToSettings(
                [](UCk_Chain_DebugSettings_UE* InSettings) { return InSettings->Get_DrawHistory(); },
                [](UCk_Chain_DebugSettings_UE* InSettings, bool InValue) { InSettings->Set_DrawHistory(InValue); },
                InCVar);
        }), ECVF_Cheat);

    FAutoConsoleVariableRef CVarDrawLinkTargets(
        TEXT("ck.Chain.DrawLinkTargets"), GDrawLinkTargets,
        TEXT("Draw chain link target arrows and held links. 0 = off (default), 1 = on."),
        FConsoleVariableDelegate::CreateLambda([](IConsoleVariable* InCVar)
        {
            WriteToSettings(
                [](UCk_Chain_DebugSettings_UE* InSettings) { return InSettings->Get_DrawLinkTargets(); },
                [](UCk_Chain_DebugSettings_UE* InSettings, bool InValue) { InSettings->Set_DrawLinkTargets(InValue); },
                InCVar);
        }), ECVF_Cheat);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Chain_DebugSettings_UE::
    PostInitProperties()
    -> void
{
    Super::PostInitProperties();
    if (NOT IsTemplate())
    { return; }

    if (auto* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("ck.Chain.DrawHistory")))
    { CVar->Set(_DrawHistory ? 1 : 0, static_cast<EConsoleVariableFlags>(CVar->GetFlags() & ECVF_SetByMask)); }
    if (auto* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("ck.Chain.DrawLinkTargets")))
    { CVar->Set(_DrawLinkTargets ? 1 : 0, static_cast<EConsoleVariableFlags>(CVar->GetFlags() & ECVF_SetByMask)); }
}

#if WITH_EDITOR
auto
    UCk_Chain_DebugSettings_UE::
    PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
    -> void
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    const auto Name = PropertyChangedEvent.GetPropertyName();
    if (Name == GET_MEMBER_NAME_CHECKED(UCk_Chain_DebugSettings_UE, _DrawHistory))
    {
        if (auto* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("ck.Chain.DrawHistory")))
        { CVar->Set(_DrawHistory ? 1 : 0, static_cast<EConsoleVariableFlags>(CVar->GetFlags() & ECVF_SetByMask)); }
    }
    else if (Name == GET_MEMBER_NAME_CHECKED(UCk_Chain_DebugSettings_UE, _DrawLinkTargets))
    {
        if (auto* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("ck.Chain.DrawLinkTargets")))
        { CVar->Set(_DrawLinkTargets ? 1 : 0, static_cast<EConsoleVariableFlags>(CVar->GetFlags() & ECVF_SetByMask)); }
    }
}
#endif

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Chain_DebugSettings_UE::
    Get_DrawHistory()
    -> bool
{
    const auto* Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Chain_DebugSettings_UE>();
    return ck::IsValid(Settings) && Settings->Get_DrawHistory();
}

auto
    UCk_Utils_Chain_DebugSettings_UE::
    Get_DrawLinkTargets()
    -> bool
{
    const auto* Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Chain_DebugSettings_UE>();
    return ck::IsValid(Settings) && Settings->Get_DrawLinkTargets();
}
