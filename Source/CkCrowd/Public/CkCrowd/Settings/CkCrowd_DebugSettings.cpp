#include "CkCrowd_DebugSettings.h"

#include "CkCore/Object/CkObject_Utils.h"

#include "HAL/IConsoleManager.h"

// --------------------------------------------------------------------------------------------------------------------
// File-scope CVar storage: FAutoConsoleVariableRef registers with IConsoleManager at TU load, before
// any CDO exists, so PostInitProperties' hydration always has a CVar to push into.
// --------------------------------------------------------------------------------------------------------------------

namespace ck_crowd_debug_settings_cvars
{
    static int32 GDrawAgentBody     = 0;
    static int32 GDrawSeparation    = 0;
    static int32 GDrawBreadcrumbs   = 0;
    static int32 GDrawPlannedPaths  = 0;
    static int32 GDrawPathTrouble   = 1;
    static int32 GDrawNavProjection = 0;
    static int32 GDrawAgentRings    = 0;

    template <typename TFieldGetter, typename TFieldSetter>
    auto WriteToSettings(TFieldGetter&& InFieldGetter, TFieldSetter&& InFieldSetter, IConsoleVariable* InCVar) -> void
    {
        if (InCVar == nullptr)
        { return; }

        auto* Settings = GetMutableDefault<UCk_Crowd_DebugSettings_UE>();
        if (NOT IsValid(Settings))
        { return; }

        const auto NewValue = InCVar->GetInt() != 0;

        // Ping-pong guard: PostInitProperties hydrates via SetWithCurrentPriority, which fires this
        // callback; if the UPROPERTY already holds the value there is nothing to persist.
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

    static FAutoConsoleVariableRef CVarDrawPathTrouble(
        TEXT("ck.Crowd.DrawPathTrouble"),
        GDrawPathTrouble,
        TEXT("Draw path-trouble diagnostics in the game world.\n")
        TEXT("  0 = off\n")
        TEXT("  1 = on (default) — marker, sidewalk/Unreal-nav status, attempted goal, dashed line, and distance"),
        FConsoleVariableDelegate::CreateLambda([](IConsoleVariable* InCVar)
        {
            WriteToSettings(
                [](UCk_Crowd_DebugSettings_UE* InS) { return InS->Get_DrawPathTrouble(); },
                [](UCk_Crowd_DebugSettings_UE* InS, bool InV) { InS->Set_DrawPathTrouble(InV); },
                InCVar);
        }),
        ECVF_Cheat);

    static FAutoConsoleVariableRef CVarDrawNavProjection(
        TEXT("ck.Crowd.DrawNavProjection"),
        GDrawNavProjection,
        TEXT("Draw a navmesh-projection marker (green/red circle on the floor) under every crowd agent.\n")
        TEXT("  0 = off (default — synchronous ProjectPointToNavigation per agent per tick is the\n")
        TEXT("        single most expensive Crowd debug viz at scale)\n")
        TEXT("  1 = on"),
        FConsoleVariableDelegate::CreateLambda([](IConsoleVariable* InCVar)
        {
            WriteToSettings(
                [](UCk_Crowd_DebugSettings_UE* InS) { return InS->Get_DrawNavProjection(); },
                [](UCk_Crowd_DebugSettings_UE* InS, bool InV) { InS->Set_DrawNavProjection(InV); },
                InCVar);
        }),
        ECVF_Cheat);

    static FAutoConsoleVariableRef CVarDrawAgentRings(
        TEXT("ck.Crowd.DrawAgentRings"),
        GDrawAgentRings,
        TEXT("Draw the orbit-diagnosis rings for crowd agents (arrival / predicted-orbit / turn-radius / velocity).\n")
        TEXT("  0 = off (default — but the debugger-selected agent always draws)\n")
        TEXT("  1 = on — every agent draws its arrival ring, predicted-orbit ring, turn-radius circle, velocity"),
        FConsoleVariableDelegate::CreateLambda([](IConsoleVariable* InCVar)
        {
            WriteToSettings(
                [](UCk_Crowd_DebugSettings_UE* InS) { return InS->Get_DrawAgentRings(); },
                [](UCk_Crowd_DebugSettings_UE* InS, bool InV) { InS->Set_DrawAgentRings(InV); },
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

    // Load-time hydration: push the persisted UPROPERTY values into the CVars. SetWithCurrentPriority
    // keeps the settings-tier priority, so a later console `set` still overrides cleanly.
    if (auto* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("ck.Crowd.Debug.AgentBody")))
    { CVar->SetWithCurrentPriority(_DrawAgentBody    ? 1 : 0); }
    if (auto* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("ck.Crowd.Debug")))
    { CVar->SetWithCurrentPriority(_DrawSeparation   ? 1 : 0); }
    if (auto* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("ck.Crowd.DrawBreadcrumbs")))
    { CVar->SetWithCurrentPriority(_DrawBreadcrumbs  ? 1 : 0); }
    if (auto* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("ck.Crowd.DrawPlannedPaths")))
    { CVar->SetWithCurrentPriority(_DrawPlannedPaths ? 1 : 0); }
    if (auto* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("ck.Crowd.DrawPathTrouble")))
    { CVar->Set(_DrawPathTrouble ? 1 : 0, ECVF_SetByGameSetting); }
    if (auto* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("ck.Crowd.DrawNavProjection")))
    { CVar->SetWithCurrentPriority(_DrawNavProjection ? 1 : 0); }
    if (auto* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("ck.Crowd.DrawAgentRings")))
    { CVar->SetWithCurrentPriority(_DrawAgentRings ? 1 : 0); }
}

#if WITH_EDITOR
auto
    UCk_Crowd_DebugSettings_UE::
    PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
    -> void
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    // Push the edited value into the matching CVar so CVar readers (draw processors, the debugger
    // checkboxes resolving through IConsoleManager) reflect it without an editor restart.
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
    else if (Name == GET_MEMBER_NAME_CHECKED(UCk_Crowd_DebugSettings_UE, _DrawPathTrouble))
    {
        if (auto* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("ck.Crowd.DrawPathTrouble")))
        {
            CVar->SetWithCurrentPriority(
                _DrawPathTrouble ? 1 : 0,
                NAME_None,
                ECVF_SetByConsole,
                ECVF_SetByScalability);
        }
    }
    else if (Name == GET_MEMBER_NAME_CHECKED(UCk_Crowd_DebugSettings_UE, _DrawNavProjection))
    {
        if (auto* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("ck.Crowd.DrawNavProjection")))
        { CVar->SetWithCurrentPriority(_DrawNavProjection ? 1 : 0); }
    }
    else if (Name == GET_MEMBER_NAME_CHECKED(UCk_Crowd_DebugSettings_UE, _DrawAgentRings))
    {
        if (auto* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("ck.Crowd.DrawAgentRings")))
        { CVar->SetWithCurrentPriority(_DrawAgentRings ? 1 : 0); }
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

auto
    UCk_Utils_Crowd_DebugSettings_UE::
    Get_DrawPathTrouble()
    -> bool
{
    const auto* Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Crowd_DebugSettings_UE>();
    if (ck::Is_NOT_Valid(Settings))
    { return false; }
    return Settings->Get_DrawPathTrouble();
}

auto
    UCk_Utils_Crowd_DebugSettings_UE::
    Get_DrawNavProjection()
    -> bool
{
    const auto* Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Crowd_DebugSettings_UE>();
    if (ck::Is_NOT_Valid(Settings))
    { return false; }
    return Settings->Get_DrawNavProjection();
}

auto
    UCk_Utils_Crowd_DebugSettings_UE::
    Get_DrawAgentRings()
    -> bool
{
    const auto* Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Crowd_DebugSettings_UE>();
    if (ck::Is_NOT_Valid(Settings))
    { return false; }
    return Settings->Get_DrawAgentRings();
}

// --------------------------------------------------------------------------------------------------------------------
