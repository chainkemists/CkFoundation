// Copyright 2025 CkFoundation. All Rights Reserved.

#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/ContextReceiver/CkContextReceiver.h"
#include "CkUI/Interfaces/CkUI_Interfaces.h"
#include "CkUI/Types/CkUI_Types.h"

#include <CommonActivatableWidget.h>

#include "CkActivatableWidget.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType, Blueprintable, meta = (DisableNativeTick))
class CKUI_API UCk_ActivatableWidget_UE
    : public UCommonActivatableWidget
    , public ICk_UI_LayerParticipant
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_ActivatableWidget_UE);

    // ----------------------------------------------------------------------------------------------------------------
    // ICk_UI_LayerParticipant Implementation
    // ----------------------------------------------------------------------------------------------------------------

public:
    auto OnPrePushToLayer_Implementation(FGameplayTag InLayerTag) -> void override;
    auto OnPostPushToLayer_Implementation(FGameplayTag InLayerTag) -> void override;
    auto OnPrePopFromLayer_Implementation(FGameplayTag InLayerTag) -> void override;
    auto OnPostPopFromLayer_Implementation(FGameplayTag InLayerTag) -> void override;

    // ----------------------------------------------------------------------------------------------------------------
    // Context
    // ----------------------------------------------------------------------------------------------------------------

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|UI|Context")
    FCk_Handle
    Get_ContextEntity() const;

protected:
    UFUNCTION(BlueprintNativeEvent,
              Category = "Ck|UI|Context")
    void
    OnValidContextInjected(
        const FCk_Handle& InContextEntity);

    UFUNCTION(BlueprintNativeEvent,
              Category = "Ck|UI|Context")
    void
    OnContextCleared();

private:
    UFUNCTION()
    void
    HandleContextInjected(
        FCk_Handle_ContextReceiver InContextReceiver,
        FCk_Handle InContextEntity);

    UFUNCTION()
    void
    HandleContextCleared(
        FCk_Handle_ContextReceiver InContextReceiver);

    // ----------------------------------------------------------------------------------------------------------------
    // Layer Tag Accessor
    // ----------------------------------------------------------------------------------------------------------------

public:
    UFUNCTION(BlueprintPure)
    FGameplayTag Get_CurrentLayerTag() const { return _CurrentLayerTag; }

#if WITH_EDITOR
    auto ValidateCompiledWidgetTree(const UWidgetTree& BlueprintWidgetTree, class IWidgetCompilerLog& CompileLog) const -> void override;
#endif

    // ----------------------------------------------------------------------------------------------------------------
    // UWidget Overrides
    // ----------------------------------------------------------------------------------------------------------------

protected:
#if WITH_EDITOR
    auto GetPaletteCategory() -> const FText override;
#endif

    auto NativeConstruct() -> void override;
    auto NativeDestruct() -> void override;
    auto NativeOnDeactivated() -> void override;

    // ----------------------------------------------------------------------------------------------------------------
    // Properties
    // ----------------------------------------------------------------------------------------------------------------

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Ck|UI|Context")
    FCk_Handle_ContextReceiver _ContextReceiver;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ck|UI|Context")
    bool _InheritContextFromParent = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ck|UI|Context")
    bool _ClearContextWhenDeactivated = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ck|UI|Lifecycle")
    bool _DoNotDestroyDuringTransitions = false;

    UPROPERTY(Transient, BlueprintReadOnly, Category = "Ck|UI|Lifecycle")
    FGameplayTag _CurrentLayerTag;

public:
    CK_PROPERTY_GET(_ContextReceiver);
    CK_PROPERTY_GET(_InheritContextFromParent);
    CK_PROPERTY_GET(_ClearContextWhenDeactivated);
    CK_PROPERTY_GET(_DoNotDestroyDuringTransitions);
};

// --------------------------------------------------------------------------------------------------------------------
