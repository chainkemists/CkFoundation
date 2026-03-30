// Copyright 2025 CkFoundation. All Rights Reserved.

#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/ContextReceiver/CkContextReceiver.h"
#include "CkUI/Types/CkUI_Types.h"

#include <CommonUserWidget.h>

#include "CkUserWidget.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType, Blueprintable, meta = (DisableNativeTick))
class CKUI_API UCk_UserWidget_UE
    : public UCommonUserWidget
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_UserWidget_UE);

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
    // UWidget Overrides
    // ----------------------------------------------------------------------------------------------------------------

protected:
    auto NativeConstruct() -> void override;

#if WITH_EDITOR
    auto GetPaletteCategory() -> const FText override;
#endif

    auto NativeDestruct() -> void override;

    // ----------------------------------------------------------------------------------------------------------------
    // Properties
    // ----------------------------------------------------------------------------------------------------------------

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Ck|UI|Context")
    FCk_Handle_ContextReceiver _ContextReceiver;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Ck|UI|Context")
    bool _InheritContextFromParent = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Ck|UI|Lifecycle")
    bool _DoNotDestroyDuringTransitions = false;

public:
    CK_PROPERTY_GET(_ContextReceiver);
    CK_PROPERTY_GET(_InheritContextFromParent);
    CK_PROPERTY_GET(_DoNotDestroyDuringTransitions);
};

// --------------------------------------------------------------------------------------------------------------------
