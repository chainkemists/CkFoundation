#include "CkCrowd_DebugSettings.h"

#include "CkCore/Object/CkObject_Utils.h"

#include "HAL/IConsoleManager.h"

// --------------------------------------------------------------------------------------------------------------------
//
// Static CVar storage. FAutoConsoleVariableRef registers the CVar with IConsoleManager at TU load
// time (DLL load → before any CDO is constructed), so the bidirectional sync below has a valid
// CVar to push to and read from. Default values mirror the UPROPERTY defaults; the
// PostInitProperties hydration overrides these with the persisted UPROPERTY values once the CDO
// is up.
//
// Each callback writes the new value back into the CDO's UPROPERTY and saves the config so a
// console-driven change persists for the next session — keeps console + Editor Preferences
// holding the same value at all times.
//
// --------------------------------------------------------------------------------------------------------------------

namespace ck_crowd_debug_settings_cvars
{
    static int32 GDrawAgentBody    = 0;
    static int32 GDrawSeparation   = 0;
    static int32 GDrawBreadcrumbs  = 0;
    static int32 GDrawPlannedPaths = 0;

    template <typename TFieldGetter, typename TFieldSetter>
    auto WriteToSettings(TFieldGetter&& InFieldGetter, TFieldSetter&& InFieldSetter, IConsoleVariable* InCVar) -> void
    {
        if (InCVar == nullptr)
        { return; }

        auto* Settings = GetMutableDefault<UCk_Crowd_DebugSettings_UE>();
        if (NOT IsValid(Settings))
        { return; }

        const auto NewValue = InCVar->GetInt() != 0;

        // Guard against ping-pong: PostInitProperties calls SetWithCurrentPriority on every CVar
        // for hydration, which fires this callback. If the value matches what's already in the
        // UPROPERTY, skip the SaveConfig (no actual change to persist).
        if (InFieldGetter(Settings) == NewValue)
        { return; }

        InFieldSetter(Settings, NewValue);
        Settings->SaveConfig();
    }

    static FAutoConsoleVariableRef CVarDrawAgentBody(
        TEXT("ck.Crowd.Debug.AgentBody"),
        GDrawAgentBody,
        TEXT("Draw a body capsule + forward-facing cone for every crowd agent.\n")
        TEXT("  0 = off (default)\n")
        TEXT("  1 = on — color comes from UCk_Utils_CrowdAgent_UE::Get_DebugColor; state\n")
        TEXT("        tags (PathPending → yellow blend, Asleep → desaturate) modulate it"),
        FConsoleVariableDelegate::CreateLambda([](IConsoleVariable* InCVar)
        {
            WriteToSettings(
                [](UCk_Crowd_DebugSettings_UE* InS) { return InS->Get_DrawAgentBody(); },
                [](UCk_Crowd_DebugSettings_UE* InS, bool InV) { InS->Set_DrawAgentBody(InV); },
                InCVar);
        }),
        ECVF_Cheat);

    static FAutoConsoleVariableRef CVarDrawSeparation(
        TEXT("ck.Crowd.Debug"),
        GDrawSeparation,
        TEXT("Draw crowd agent separation diagnostics in PIE.\n")
        TEXT("  0 = off (default)\n")
        TEXT("  1 = draw separation radius circle, force arrow, neighbor connections"),
        FConsoleVariableDelegate::CreateLambda([](IConsoleVariable* InCVar)
        {
            WriteToSettings(
                [](UCk_Crowd_DebugSettings_UE* InS) { return InS->Get_DrawSeparation(); },
                [](UCk_Crowd_DebugSettings_UE* InS, bool InV) { InS->Set_DrawSeparation(InV); },
                InCVar);
        }),
        ECVF_Cheat);

    static FAutoConsoleVariableRef CVarDrawBreadcrumbs(
        TEXT("ck.Crowd.DrawBreadcrumbs"),
        GDrawBreadcrumbs,
        TEXT("Draw breadcrumb trails for every recorder-tracked crowd agent.\n")
        TEXT("  0 = off (default — but the debugger-selected agent always draws)\n")
        TEXT("  1 = on — every tracked agent's path renders at agent body height"),
        FConsoleVariableDelegate::CreateLambda([](IConsoleVariable* InCVar)
        {
            WriteToSettings(
                [](UCk_Crowd_DebugSettings_UE* InS) { return InS->Get_DrawBreadcrumbs(); },
                [](UCk_Crowd_DebugSettings_UE* InS, bool InV) { InS->Set_DrawBreadcrumbs(InV); },
                InCVar);
        }),
        ECVF_Cheat);

    static FAutoConsoleVariableRef CVarDrawPlannedPaths(
        TEXT("ck.Crowd.DrawPlannedPaths"),
        GDrawPlannedPaths,
        TEXT("Draw planned-path waypoints for every crowd agent with a path result.\n")
        TEXT("  0 = off (default — but the debugger-selected agent always draws)\n")
        TEXT("  1 = on — every agent's planned waypoints render at agent body height"),
        FConsoleVariableDelegate::CreateLambda([](IConsoleVariable* InCVar)
        {
            WriteToSettings(
                [](UCk_Crowd_DebugSettings_UE* InS) { return InS->Get_DrawPlannedPaths(); },
                [](UCk_Crowd_DebugSettings_UE* InS, bool InV) { InS->Set_DrawPlannedPaths(InV); },
                InCVar);
        }),
        ECVF_Cheat);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Crowd_DebugSettings_UE::
    PostInitProperties()
    -> void
{
    Super::PostInitProperties();

    if (NOT IsTemplate())
    { return; }

    // Hydrate the static CVars from the persisted UPROPERTY values. Setting via
    // ECVF_SetByProjectSetting tells UE this came from settings (not console), so a later
    // console `set` still overrides cleanly. Suppresses the change callback (we'd otherwise
    // ping-pong: callback writes the same value back to the UPROPERTY, triggers SaveConfig).
    if (auto* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("ck.Crowd.Debug.AgentBody")))
    { CVar->SetWithCurrentPriority(_DrawAgentBody    ? 1 : 0); }
    if (auto* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("ck.Crowd.Debug")))
    { CVar->SetWithCurrentPriority(_DrawSeparation   ? 1 : 0); }
    if (auto* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("ck.Crowd.DrawBreadcrumbs")))
    { CVar->SetWithCurrentPriority(_DrawBreadcrumbs  ? 1 : 0); }
    if (auto* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("ck.Crowd.DrawPlannedPaths")))
    { CVar->SetWithCurrentPriority(_DrawPlannedPaths ? 1 : 0); }
}

#if WITH_EDITOR
auto
    UCk_Crowd_DebugSettings_UE::
    PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
    -> void
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    // Push the edited value into the matching CVar so processors that read the CVar (and the
    // debugger checkboxes that resolve through IConsoleManager) reflect the change immediately
    // without requiring an editor restart.
    const auto Name = PropertyChangedEvent.GetPropertyName();
    if (Name == GET_MEMBER_NAME_CHECKED(UCk_Crowd_DebugSettings_UE, _DrawAgentBody))
    {
        if (auto* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("ck.Crowd.Debug.AgentBody")))
        { CVar->SetWithCurrentPriority(_DrawAgentBody ? 1 : 0); }
    }
    else if (Name == GET_MEMBER_NAME_CHECKED(UCk_Crowd_DebugSettings_UE, _DrawSeparation))
    {
        if (auto* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("ck.Crowd.Debug")))
        { CVar->SetWithCurrentPriority(_DrawSeparation ? 1 : 0); }
    }
    else if (Name == GET_MEMBER_NAME_CHECKED(UCk_Crowd_DebugSettings_UE, _DrawBreadcrumbs))
    {
        if (auto* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("ck.Crowd.DrawBreadcrumbs")))
        { CVar->SetWithCurrentPriority(_DrawBreadcrumbs ? 1 : 0); }
    }
    else if (Name == GET_MEMBER_NAME_CHECKED(UCk_Crowd_DebugSettings_UE, _DrawPlannedPaths))
    {
        if (auto* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("ck.Crowd.DrawPlannedPaths")))
        { CVar->SetWithCurrentPriority(_DrawPlannedPaths ? 1 : 0); }
    }
}
#endif

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Crowd_DebugSettings_UE::
    Get_DrawAgentBody()
    -> bool
{
    const auto* Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Crowd_DebugSettings_UE>();
    if (ck::Is_NOT_Valid(Settings))
    { return false; }
    return Settings->Get_DrawAgentBody();
}

auto
    UCk_Utils_Crowd_DebugSettings_UE::
    Get_DrawSeparation()
    -> bool
{
    const auto* Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Crowd_DebugSettings_UE>();
    if (ck::Is_NOT_Valid(Settings))
    { return false; }
    return Settings->Get_DrawSeparation();
}

auto
    UCk_Utils_Crowd_DebugSettings_UE::
    Get_DrawBreadcrumbs()
    -> bool
{
    const auto* Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Crowd_DebugSettings_UE>();
    if (ck::Is_NOT_Valid(Settings))
    { return false; }
    return Settings->Get_DrawBreadcrumbs();
}

auto
    UCk_Utils_Crowd_DebugSettings_UE::
    Get_DrawPlannedPaths()
    -> bool
{
    const auto* Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Crowd_DebugSettings_UE>();
    if (ck::Is_NOT_Valid(Settings))
    { return false; }
    return Settings->Get_DrawPlannedPaths();
}

// --------------------------------------------------------------------------------------------------------------------
