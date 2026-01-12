// Copyright 2025 CkFoundation. All Rights Reserved.

#include "CkUI/HUD/CkHUD.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"
#include "CkUI/Layout/CkUI_Layout_Subsystem.h"
#include "CkUI/Layout/CkUI_PrimaryGameLayout.h"

#include <GameFramework/PlayerController.h>

// --------------------------------------------------------------------------------------------------------------------
// Constructor
// --------------------------------------------------------------------------------------------------------------------

ACk_HUD_UE::ACk_HUD_UE()
{
    PrimaryActorTick.bCanEverTick = false;
}

// --------------------------------------------------------------------------------------------------------------------
// AActor Overrides
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
// Accessors
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
// Internal
// --------------------------------------------------------------------------------------------------------------------

auto
    ACk_HUD_UE::
    DoInitializeUI()
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(_LayoutConfigAsset),
        TEXT("HUD [{}] requires a valid LayoutConfigAsset to initialize UI"), this)
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
    { return; }

    Subsystem->OnLayoutCreated.AddUObject(this, &ThisClass::HandlePrimaryGameLayoutCreated);
    Subsystem->OnLayoutDestroyed.AddUObject(this, &ThisClass::HandlePrimaryGameLayoutDestroyed);
    Subsystem->CreateLayout(_LayoutConfigAsset);
}

auto
    ACk_HUD_UE::
    DoShutdownUI()
    -> void
{
    const auto* PlayerController = GetOwningPlayerController();

    if (ck::Is_NOT_Valid(PlayerController))
    { return; }

    const auto* LocalPlayer = PlayerController->GetLocalPlayer();

    if (ck::Is_NOT_Valid(LocalPlayer))
    { return; }

    auto* Subsystem = LocalPlayer->GetSubsystem<UCk_UI_Layout_Subsystem_UE>();

    if (ck::Is_NOT_Valid(Subsystem))
    { return; }

    Subsystem->OnLayoutCreated.RemoveAll(this);
    Subsystem->OnLayoutDestroyed.RemoveAll(this);
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