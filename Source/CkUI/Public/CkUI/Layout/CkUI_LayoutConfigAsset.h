// Copyright 2025 CkFoundation. All Rights Reserved.

#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkUI/Extension/CkUI_Extension_Types.h"
#include "CkUI/Layout/CkUI_LayerStack.h"
#include "CkUI/Types/CkUI_Types.h"

#include <CommonActivatableWidget.h>
#include <Engine/DataAsset.h>
#include <GameplayTagContainer.h>
#include <Slate/SCommonAnimatedSwitcher.h>

#include "CkUI_LayoutConfigAsset.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_UI_LayerStack_UE;

// --------------------------------------------------------------------------------------------------------------------

/**
 * Configuration for a single UI layer within a layout.
 */
USTRUCT(BlueprintType)
struct CKUI_API FCk_UI_LayerConfig
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_UI_LayerConfig);

private:
    /** Tag identifying this layer. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true, Categories = "UI.Layer"))
    FGameplayTag _LayerTag;

    /** Widget class to use for this layer. If null, uses default UCk_UI_LayerStack_UE. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    TSubclassOf<UCk_UI_LayerStack_UE> _LayerWidgetClass = UCk_UI_LayerStack_UE::StaticClass();

    /** Priority for input mode resolution. Higher priority layers win. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true, ClampMin = 0, UIMin = 0))
    int32 _Priority = 0;

    /** Input mode when this layer has widgets. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = true))
    ECk_UI_InputMode _InputMode = ECk_UI_InputMode::GameOnly;

    /** The desired mouse capture behavior. Only applies to GameOnly and GameAndUI modes. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = true,
        EditCondition = "_InputMode != ECk_UI_InputMode::UIOnly"))
    EMouseCaptureMode _MouseCaptureMode = EMouseCaptureMode::CapturePermanently;

    /** Transition type when switching widgets. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Transition, meta = (AllowPrivateAccess = true))
    ECommonSwitcherTransition _TransitionType = ECommonSwitcherTransition::FadeOnly;

    /** Curve for transition animation. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Transition, meta = (AllowPrivateAccess = true))
    ETransitionCurve _TransitionCurve = ETransitionCurve::CubicInOut;

    /** Duration of transition in seconds. 0 for instant. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Transition, meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
    float _TransitionDuration = 0.0f;

    /** Widgets to automatically push when this layer is created. Loaded async. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true, DisplayName = "Starting Widgets", AllowAbstract = false))
    TArray<TSoftClassPtr<UCommonActivatableWidget>> _StartingWidgetClasses;

public:
    CK_PROPERTY_GET(_LayerTag);
    CK_PROPERTY_GET(_LayerWidgetClass);
    CK_PROPERTY(_Priority);
    CK_PROPERTY(_InputMode);
    CK_PROPERTY(_MouseCaptureMode);
    CK_PROPERTY(_TransitionType);
    CK_PROPERTY(_TransitionCurve);
    CK_PROPERTY(_TransitionDuration);
    CK_PROPERTY(_StartingWidgetClasses);

    CK_DEFINE_CONSTRUCTORS(FCk_UI_LayerConfig, _LayerTag, _LayerWidgetClass);
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * Data asset containing layout configuration.
 * Define your UI layers here and reference this asset when creating the primary game layout.
 */
UCLASS(DisplayName = "CkUI_LayoutConfig")
class CKUI_API UCk_UI_LayoutConfigAsset_UE : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_UI_LayoutConfigAsset_UE);

    // ----------------------------------------------------------------------------------------------------------------
    // Access
    // ----------------------------------------------------------------------------------------------------------------

public:
    TOptional<FCk_UI_LayerConfig> Get_LayerConfig(FGameplayTag InLayerTag) const;

    // ----------------------------------------------------------------------------------------------------------------
    // UPrimaryDataAsset Overrides
    // ----------------------------------------------------------------------------------------------------------------

public:
    virtual FPrimaryAssetId GetPrimaryAssetId() const override;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

    // ----------------------------------------------------------------------------------------------------------------
    // Properties
    // ----------------------------------------------------------------------------------------------------------------

private:
    UPROPERTY(EditAnywhere, Category = "Rendering", meta = (DisplayName = "Layout Z-Order", MinClamp = 0, UIMin = 0))
    int32 _LayoutZOrder = 10;
    /**
     * Input mode to use when no layers have active widgets.
     * Typically this should be GameOnly to return control to gameplay.
     */
    UPROPERTY(EditAnywhere, Category = "Default Input", meta = (DisplayName = "Default Input Mode"))
    ECk_UI_InputMode _DefaultInputMode = ECk_UI_InputMode::GameOnly;

    /**
     * Mouse capture mode to use when no layers have active widgets.
     * Only applies to GameOnly and GameAndUI modes.
     */
    UPROPERTY(EditAnywhere, Category = "Default Input", meta = (DisplayName = "Default Mouse Capture",
        EditCondition = "_DefaultInputMode != ECk_UI_InputMode::UIOnly"))
    EMouseCaptureMode _DefaultMouseCaptureMode = EMouseCaptureMode::CapturePermanently;

    /** Layer definitions. Layer Priority determines visual stacking (higher = on top). */
    UPROPERTY(EditAnywhere, Category = "Layers", meta = (TitleProperty = "P:{_Priority} | {_InputMode} | {_LayerTag}"))
    TArray<FCk_UI_LayerConfig> _LayerConfigs;

    /**
     * HUD elements to register as extensions.
     * These widgets will be automatically loaded and registered with the extension subsystem.
     */
    UPROPERTY(EditAnywhere, Category = "HUD Elements", meta = (TitleProperty = "{_SlotTag} -> {_WidgetClass}"))
    TArray<FCk_UI_HUDElementEntry> _HUDElements;

public:
    CK_PROPERTY_GET(_LayoutZOrder);
    CK_PROPERTY_GET(_DefaultInputMode);
    CK_PROPERTY_GET(_DefaultMouseCaptureMode);
    CK_PROPERTY_GET(_LayerConfigs);
    CK_PROPERTY_GET(_HUDElements);
};

// --------------------------------------------------------------------------------------------------------------------