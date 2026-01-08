// Copyright 2025 CkFoundation. All Rights Reserved.

#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkUI/Layer/CkUI_LayerStack.h"
#include "CkUI/Types/CkUI_Types.h"

#include <CommonActivatableWidget.h>
#include <Engine/DataAsset.h>
#include <GameplayTagContainer.h>
#include <Slate/SCommonAnimatedSwitcher.h>

#include "CkUI_LayerConfigAsset.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_UI_LayerStack_UE;

// --------------------------------------------------------------------------------------------------------------------

/**
 * Configuration for a single UI layer.
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

    /** The desired mouse behavior when the game gets input. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = true))
    EMouseCaptureMode _GameMouseCaptureMode = EMouseCaptureMode::CapturePermanently;

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
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true, DisplayName = "Starting Widgets"))
    TArray<TSoftClassPtr<UCommonActivatableWidget>> _StartingWidgetClasses;

public:
    CK_PROPERTY_GET(_LayerTag);
    CK_PROPERTY_GET(_LayerWidgetClass);
    CK_PROPERTY_GET(_Priority);
    CK_PROPERTY_GET(_InputMode);
    CK_PROPERTY_GET(_GameMouseCaptureMode);
    CK_PROPERTY_GET(_TransitionType);
    CK_PROPERTY_GET(_TransitionCurve);
    CK_PROPERTY_GET(_TransitionDuration);
    CK_PROPERTY_GET(_StartingWidgetClasses);
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * Data asset containing layer configuration.
 * Define your UI layers here and reference this asset in your HUD or game mode.
 */
UCLASS(DisplayName = "CkUI_LayerConfig")
class CKUI_API UCk_UI_LayerConfigAsset_UE : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_UI_LayerConfigAsset_UE);

    // ----------------------------------------------------------------------------------------------------------------
    // Access
    // ----------------------------------------------------------------------------------------------------------------

public:
    auto Get_LayerConfig(FGameplayTag InLayerTag) const -> TOptional<FCk_UI_LayerConfig>;

    // ----------------------------------------------------------------------------------------------------------------
    // UPrimaryDataAsset Overrides
    // ----------------------------------------------------------------------------------------------------------------

public:
    virtual auto GetPrimaryAssetId() const -> FPrimaryAssetId override;

#if WITH_EDITOR
    virtual auto IsDataValid(FDataValidationContext& Context) const -> EDataValidationResult override;
#endif

    // ----------------------------------------------------------------------------------------------------------------
    // Properties
    // ----------------------------------------------------------------------------------------------------------------

private:
    UPROPERTY(EditAnywhere, Category = "Layers", meta = (TitleProperty = "_LayerTag"))
    TArray<FCk_UI_LayerConfig> _LayerConfigs;

public:
    CK_PROPERTY_GET(_LayerConfigs);
};

// --------------------------------------------------------------------------------------------------------------------