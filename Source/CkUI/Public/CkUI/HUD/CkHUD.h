// Copyright 2025 CkFoundation. All Rights Reserved.

#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle.h"

#include <GameFramework/HUD.h>

#include "CkHUD.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_UI_LayoutConfigAsset_UE;
class UCk_UI_PrimaryGameLayout_UE;
struct FStreamableHandle;

// --------------------------------------------------------------------------------------------------------------------

/**
 * Base HUD class that integrates with the CkUI layer system.
 *
 * The HUD owns the layer configuration asset and drives UI initialization
 * through the UI subsystem. Override OnLayoutReady to respond when the
 * layout and all starting widgets have been created.
 *
 * Usage:
 * 1. Create a subclass and set LayoutConfigAsset in the editor
 * 2. Override OnLayoutReady_Event or bind to OnLayoutReady for post-init logic
 * 3. The HUD automatically calls AddPlayer/RemovePlayer on the subsystem
 */
UCLASS(Abstract, BlueprintType)
class CKUI_API ACk_HUD_UE : public AHUD
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(ACk_HUD_UE);

    ACk_HUD_UE();

    /** Refreshes the rebuilt ECS context on the retained primary layout and active Game-layer HUD. */
    void Refresh_Context(const FCk_Handle& InContextEntity);

    // ----------------------------------------------------------------------------------------------------------------

public:
    /** Native delegate called when layout is ready (after starting widgets loaded). */
    DECLARE_MULTICAST_DELEGATE_OneParam(FOnLayoutReady, UCk_UI_PrimaryGameLayout_UE*);
    FOnLayoutReady OnLayoutReady;

    /** Blueprint delegate called when layout is ready. */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLayoutReady_BP, UCk_UI_PrimaryGameLayout_UE*, InLayout);

    UPROPERTY(BlueprintAssignable, Category = "Ck|UI")
    FOnLayoutReady_BP OnLayoutReady_BP;

    /** Native delegate called when layout is destroyed. */
    DECLARE_MULTICAST_DELEGATE(FOnLayoutDestroyed);
    FOnLayoutDestroyed OnLayoutDestroyed;

    /** Blueprint delegate called when layout is destroyed. */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLayoutDestroyed_BP);

    UPROPERTY(BlueprintAssignable, Category = "Ck|UI")
    FOnLayoutDestroyed_BP OnLayoutDestroyed_BP;

    // ----------------------------------------------------------------------------------------------------------------

protected:
    /** Called when the layout is ready. Override in Blueprint to handle initialization. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Ck|UI",
        DisplayName = "On Layout Ready")
    void OnLayoutReady_Event(UCk_UI_PrimaryGameLayout_UE* InLayout);

    /** Called when the layout is destroyed. Override in Blueprint to handle cleanup. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Ck|UI",
        DisplayName = "On Layout Destroyed")
    void OnLayoutDestroyed_Event();

    // ----------------------------------------------------------------------------------------------------------------

protected:
    auto BeginPlay() -> void override;
    auto EndPlay(const EEndPlayReason::Type InEndPlayReason) -> void override;
    auto Get_Layout() const -> UCk_UI_PrimaryGameLayout_UE*;

    // ----------------------------------------------------------------------------------------------------------------

private:
    auto DoInitializeUI() -> void;
    auto HandleLayoutConfigLoaded() -> void;
    auto DoShutdownUI() -> void;
    auto HandlePrimaryGameLayoutCreated() -> void;
    auto HandlePrimaryGameLayoutDestroyed() -> void;

    // ----------------------------------------------------------------------------------------------------------------

private:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ck|UI",
        meta = (AllowPrivateAccess = true))
    TSoftObjectPtr<UCk_UI_LayoutConfigAsset_UE> _LayoutConfigAsset;

    TSharedPtr<FStreamableHandle> _LayoutConfigLoadHandle;

public:
    CK_PROPERTY_GET(_LayoutConfigAsset);
};

// --------------------------------------------------------------------------------------------------------------------
