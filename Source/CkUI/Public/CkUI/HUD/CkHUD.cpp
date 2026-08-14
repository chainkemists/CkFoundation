// Copyright 2025 CkFoundation. All Rights Reserved.

#include "CkUI/HUD/CkHUD.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"
#include "../../../CkUI_Log.h" // module-root header (CkUI's log lives beside its Build.cs, not under Public/)
#include "CkUI/CkUI_GameplayTags.h"
#include "CkUI/Layout/CkUI_LayoutConfigAsset.h"
#include "CkUI/Layout/CkUI_LayerStack.h"
#include "CkUI/Layout/CkUI_Layout_Subsystem.h"
#include "CkUI/Layout/CkUI_PrimaryGameLayout.h"
#include "CkEcs/ContextReceiver/CkContextReceiver_Utils.h"

#include <Engine/AssetManager.h>
#include <Engine/StreamableManager.h>
#include <GameFramework/PlayerController.h>

// --------------------------------------------------------------------------------------------------------------------

ACk_HUD_UE::ACk_HUD_UE()
{
    PrimaryActorTick.bCanEverTick = false;
}

void
    ACk_HUD_UE::
    Refresh_Context(
        const FCk_Handle& InContextEntity)
{
    auto* const Layout = Get_Layout();

    const auto LayoutIsValid = ck::IsValid(Layout);
    CK_ENSURE_IF_NOT(LayoutIsValid, TEXT("HUD [{}] cannot refresh context without a primary layout"), this)
    { return; }

    const auto ContextIsValid = ck::IsValid(InContextEntity);
    CK_ENSURE_IF_NOT(ContextIsValid, TEXT("HUD [{}] cannot refresh an invalid context"), this)
    { return; }

    auto* const GameLayer = Layout->Get_Layer(TAG_UI_Layer_Game);
    const auto GameLayerIsValid = ck::IsValid(GameLayer);
    CK_ENSURE_IF_NOT(GameLayerIsValid, TEXT("HUD [{}] cannot refresh context without the Game layer"), this)
    { return; }

    auto* const ActiveGameWidget = GameLayer->GetActiveWidget();
    const auto ActiveGameWidgetIsValid = ck::IsValid(ActiveGameWidget);
    CK_ENSURE_IF_NOT(ActiveGameWidgetIsValid,
        TEXT("HUD [{}] cannot refresh context without an active Game-layer widget"), this)
    { return; }

    const auto LayoutHasContextReceiver =
        UCk_Utils_ContextReceiver_UE::HasContextReceiverPropertyOnObject(Layout);
    CK_ENSURE_IF_NOT(LayoutHasContextReceiver,
        TEXT("HUD [{}] cannot refresh a primary layout without a context receiver"), this)
    { return; }

    const auto GameWidgetHasContextReceiver =
        UCk_Utils_ContextReceiver_UE::HasContextReceiverPropertyOnObject(ActiveGameWidget);
    CK_ENSURE_IF_NOT(GameWidgetHasContextReceiver,
        TEXT("HUD [{}] cannot refresh an active Game-layer widget without a context receiver"), this)
    { return; }

    const auto LayoutResult = UCk_Utils_ContextReceiver_UE::RefreshContextIntoObject(Layout, InContextEntity);
    const auto LayoutRefreshed = LayoutResult == ECk_ContextReceiver_InjectResult::Success;
    CK_ENSURE_IF_NOT(LayoutRefreshed, TEXT("HUD [{}] failed to refresh primary-layout context with result [{}]"),
        this, LayoutResult)
    { return; }

    const auto GameWidgetResult =
        UCk_Utils_ContextReceiver_UE::RefreshContextIntoObject(ActiveGameWidget, InContextEntity);
    const auto GameWidgetRefreshed = GameWidgetResult == ECk_ContextReceiver_InjectResult::Success;
    CK_ENSURE_IF_NOT(GameWidgetRefreshed,
        TEXT("HUD [{}] failed to refresh active Game-layer widget context with result [{}]"), this, GameWidgetResult)
    { return; }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ACk_HUD_UE::
    BeginPlay()
    -> void
{
    Super::BeginPlay();
    DoInitializeUI();
}

auto
    ACk_HUD_UE::
    EndPlay(
        const EEndPlayReason::Type InEndPlayReason)
    -> void
{
    DoShutdownUI();
    Super::EndPlay(InEndPlayReason);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ACk_HUD_UE::
    Get_Layout() const
    -> UCk_UI_PrimaryGameLayout_UE*
{
    const auto* PlayerController = GetOwningPlayerController();

    if (ck::Is_NOT_Valid(PlayerController))
    { return nullptr; }

    const auto* LocalPlayer = PlayerController->GetLocalPlayer();

    if (ck::Is_NOT_Valid(LocalPlayer))
    { return nullptr; }

    const auto* Subsystem = LocalPlayer->GetSubsystem<UCk_UI_Layout_Subsystem_UE>();

    CK_ENSURE_IF_NOT(ck::IsValid(Subsystem),
        TEXT("HUD [{}] could not retrieve UI Layout Subsystem for LocalPlayer"), this)
    { return nullptr; }

    return Subsystem->Get_Layout();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ACk_HUD_UE::
    DoInitializeUI()
    -> void
{
    CK_ENSURE_IF_NOT(_LayoutConfigAsset.IsNull() == false,
        TEXT("HUD [{}] requires a LayoutConfigAsset to initialize UI"), this)
    { return; }

    _LayoutConfigLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
        _LayoutConfigAsset.ToSoftObjectPath(),
        FStreamableDelegate::CreateUObject(this, &ThisClass::HandleLayoutConfigLoaded));
}

auto
    ACk_HUD_UE::
    HandleLayoutConfigLoaded()
    -> void
{
    _LayoutConfigLoadHandle.Reset();

    auto* LayoutConfig = _LayoutConfigAsset.Get();

    CK_ENSURE_IF_NOT(ck::IsValid(LayoutConfig),
        TEXT("HUD [{}] failed to load its LayoutConfigAsset"), this)
    { return; }

    const auto* PlayerController = GetOwningPlayerController();

    CK_ENSURE_IF_NOT(ck::IsValid(PlayerController),
        TEXT("HUD [{}] has no owning PlayerController during UI initialization"), this)
    { return; }

    const auto* LocalPlayer = PlayerController->GetLocalPlayer();

    CK_ENSURE_IF_NOT(ck::IsValid(LocalPlayer),
        TEXT("HUD [{}] PlayerController has no LocalPlayer during UI initialization"), this)
    { return; }

    auto* Subsystem = LocalPlayer->GetSubsystem<UCk_UI_Layout_Subsystem_UE>();

    CK_ENSURE_IF_NOT(ck::IsValid(Subsystem),
        TEXT("HUD [{}] could not retrieve UI Layout Subsystem for LocalPlayer"), this)
    { return; }

    if (Subsystem->Has_Layout())
    {
        ck::ui::Warning(TEXT("HUD [{}] found a surviving PrimaryGameLayout [{}] from a previous world — "
            "destroying it and rebuilding so this world's HUD binds fresh"),
            this, Subsystem->Get_Layout());
        Subsystem->DestroyLayout();
    }

    ck::ui::Verbose(TEXT("HUD [{}] creating PrimaryGameLayout from config [{}]"), this, LayoutConfig);

    Subsystem->OnLayoutCreated.AddUObject(this, &ThisClass::HandlePrimaryGameLayoutCreated);
    Subsystem->OnLayoutDestroyed.AddUObject(this, &ThisClass::HandlePrimaryGameLayoutDestroyed);
    Subsystem->CreateLayout(LayoutConfig);
}

auto
    ACk_HUD_UE::
    DoShutdownUI()
    -> void
{
    if (_LayoutConfigLoadHandle.IsValid())
    {
        _LayoutConfigLoadHandle->CancelHandle();
        _LayoutConfigLoadHandle.Reset();
    }

    const auto* PlayerController = GetOwningPlayerController();

    if (ck::Is_NOT_Valid(PlayerController))
    {
        ck::ui::Verbose(TEXT("HUD [{}] shutdown: owning PlayerController already gone — layout teardown "
            "deferred to the next world's HUD"), this);
        return;
    }

    const auto* LocalPlayer = PlayerController->GetLocalPlayer();

    if (ck::Is_NOT_Valid(LocalPlayer))
    {
        ck::ui::Verbose(TEXT("HUD [{}] shutdown: no LocalPlayer — layout teardown deferred to the next "
            "world's HUD"), this);
        return;
    }

    auto* Subsystem = LocalPlayer->GetSubsystem<UCk_UI_Layout_Subsystem_UE>();

    if (ck::Is_NOT_Valid(Subsystem))
    { return; }

    Subsystem->OnLayoutCreated.RemoveAll(this);
    Subsystem->OnLayoutDestroyed.RemoveAll(this);

    ck::ui::Verbose(TEXT("HUD [{}] shutdown: destroying PrimaryGameLayout [{}]"), this, Subsystem->Get_Layout());
    Subsystem->DestroyLayout();
}

auto
    ACk_HUD_UE::
    HandlePrimaryGameLayoutCreated()
    -> void
{
    auto* Layout = Get_Layout();

    OnLayoutReady.Broadcast(Layout);
    OnLayoutReady_BP.Broadcast(Layout);
    OnLayoutReady_Event(Layout);
}

auto
    ACk_HUD_UE::
    HandlePrimaryGameLayoutDestroyed()
    -> void
{
    OnLayoutDestroyed.Broadcast();
    OnLayoutDestroyed_BP.Broadcast();
    OnLayoutDestroyed_Event();
}

// --------------------------------------------------------------------------------------------------------------------
