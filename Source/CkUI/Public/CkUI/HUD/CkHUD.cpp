// Copyright 2025 CkFoundation. All Rights Reserved.

#include "CkUI/HUD/CkHUD.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"
#include "../../../CkUI_Log.h" // module-root header (CkUI's log lives beside its Build.cs, not under Public/)
#include "CkUI/Layout/CkUI_LayoutConfigAsset.h"
#include "CkUI/Layout/CkUI_Layout_Subsystem.h"
#include "CkUI/Layout/CkUI_PrimaryGameLayout.h"

#include <Engine/AssetManager.h>
#include <Engine/StreamableManager.h>
#include <GameFramework/PlayerController.h>

// --------------------------------------------------------------------------------------------------------------------

ACk_HUD_UE::ACk_HUD_UE()
{
    PrimaryActorTick.bCanEverTick = false;
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