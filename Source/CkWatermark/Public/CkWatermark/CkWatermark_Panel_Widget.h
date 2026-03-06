#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Enums/CkEnums.h"
#include "CkWatermark/CkWatermark_Widget.h"

#include <Components/Widget.h>
#include <GameplayTagContainer.h>

#include "CkWatermark_Panel_Widget.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// Screen-corner anchor for a stat group.

UENUM(BlueprintType)
enum class ECk_Watermark_GroupAnchor : uint8
{
    TopLeft      UMETA(DisplayName = "Top Left"),
    TopCenter    UMETA(DisplayName = "Top Center"),
    TopRight     UMETA(DisplayName = "Top Right"),
    BottomLeft   UMETA(DisplayName = "Bottom Left"),
    BottomCenter UMETA(DisplayName = "Bottom Center"),
    BottomRight  UMETA(DisplayName = "Bottom Right"),
};

// Placement settings for one stat group (anchor + distance from the screen edge).
USTRUCT(BlueprintType)
struct CKWATERMARK_API FCk_Watermark_GroupPlacement
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
    ECk_Watermark_GroupAnchor Anchor = ECk_Watermark_GroupAnchor::BottomRight;

    // Distance from the nearest screen edge (Left, Top, Right, Bottom).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
    FMargin EdgePadding = FMargin(8.f);
};

// --------------------------------------------------------------------------------------------------------------------
// Fully code-driven watermark panel. No Blueprint layout required.
//
// Place this widget in a full-screen overlay slot in your HUD Widget Blueprint
// (or any UMG canvas that fills the viewport). All stat groups are positioned
// internally via Slate — nothing else needs to be done in UMG.
//
// Display modes
//   Hidden   — nothing visible.
//   Regular  — main stats + build config visible.
//   Detailed — everything visible, including ECS ticking group rows.
//
// Call Request_SetDisplayPolicy() from HUD code to switch modes at runtime.

UCLASS(BlueprintType, Blueprintable,
       meta = (DisplayName = "Ck Watermark Panel",
               Category    = "CkFoundation|Watermark"))
class CKWATERMARK_API UCkWatermark_Panel_UWidget_UE : public UWidget
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCkWatermark_Panel_UWidget_UE);

public:
    UCkWatermark_Panel_UWidget_UE();

    UFUNCTION(BlueprintCallable, Category = "Watermark")
    void
    Request_SetDisplayPolicy(
        ECk_Watermark_DisplayPolicy InNewPolicy);

private:
    // ---- Placement -----------------------------------------------------------
    // Bottom-right by default: Ensures / RAM / VRAM (row 1), Ping / ServerFPS / FPS / Time / Frame (row 2).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Watermark|Placement",
              meta = (AllowPrivateAccess = true))
    FCk_Watermark_GroupPlacement _StatsGroupPlacement;

    // Top-right by default (Detailed mode only): one row per ECS ticking group tag.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Watermark|Placement",
              meta = (AllowPrivateAccess = true))
    FCk_Watermark_GroupPlacement _EcsGroupsPlacement;

    // Bottom-left by default: CPU Brand, OS Version, Cores, Network Type, ECS debug settings, Build Config.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Watermark|Placement",
              meta = (AllowPrivateAccess = true))
    FCk_Watermark_GroupPlacement _InfoGroupPlacement;

    // ---- ECS -----------------------------------------------------------------
    // Tags listed here each get one stat row in Detailed mode.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Watermark|ECS",
              meta = (AllowPrivateAccess = true, Categories = "EcsWorldTickingGroup"))
    TArray<FGameplayTag> _DetailedEcsGroups;

    // Whether ECS stats read client-side or server-side timing data.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Watermark|ECS",
              meta = (AllowPrivateAccess = true))
    ECk_ClientServer _EcsStatSource = ECk_ClientServer::Client;

    // ---- State ---------------------------------------------------------------
    ECk_Watermark_DisplayPolicy _CurrentDisplayPolicy = ECk_Watermark_DisplayPolicy::Regular;

    // ---- Replication stats cache (mutable — updated lazily in paint lambdas) --
    mutable int32  _CachedRepActorCount     = 0;
    mutable int32  _CachedRepComponentCount = 0;
    mutable int32  _CachedRepObjectCount    = 0;
    mutable double _LastRepRefreshTime      = 0.0;

public:
    CK_PROPERTY_GET(_DetailedEcsGroups);
    CK_PROPERTY_GET(_EcsStatSource);

protected:
    auto RebuildWidget()             -> TSharedRef<SWidget> override;
    auto ReleaseSlateResources(bool) -> void                override;
    auto SynchronizeProperties()     -> void                override;
#if WITH_EDITOR
    auto GetPaletteCategory()        -> const FText         override;
#endif

private:
    auto DoRefreshReplicationCacheIfNeeded() const -> void;

    TSharedPtr<SWidget> _SlateRoot;
};

// --------------------------------------------------------------------------------------------------------------------
