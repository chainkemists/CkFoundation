#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Validation/CkIsValid.h"

#include <CommonActivatableWidget.h>
#include <Blueprint/WidgetTree.h>

#include "CkUserWidget.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::widget_palette_categories
{
    const FText Default = NSLOCTEXT("CkUI", "WidgetPaletteCategory", "CkFoundation Plugin");
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType, Blueprintable, meta = (DisableNativeTick))
class CKUI_API UCk_UserWidget_UE : public UCommonActivatableWidget
{
    GENERATED_BODY()
public:
    CK_GENERATED_BODY(UCk_UserWidget_UE);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|UI|Widget")
    void BindToActor(AActor* InActor);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|UI|Widget")
    void UnbindFromActor(AActor* InActor);

    UFUNCTION(BlueprintImplementableEvent,
              Category = "Ck|UI|Widget")
    void OnBindToActor(AActor* InActor);

    UFUNCTION(BlueprintImplementableEvent,
              Category = "Ck|UI|Widget")
    void OnUnbindFromActor(AActor* InActor);

protected:
    auto DoGet_IsAlreadyBoundTo(const AActor* InActor) const -> bool;
    auto DoBindToActor(AActor* InActor) -> void;
    auto DoBindToActor_BP(AActor* InActor) -> void;
    auto DoUnbindFromActor_BP(AActor* InActor) -> void;
    auto DoUnbindFromActor(AActor* InActor) -> void;

protected:
#if WITH_EDITOR
    auto GetPaletteCategory() -> const FText override;
#endif

protected:
    virtual void NativeDestruct() override;
    virtual void NativeOnDeactivated() override;

protected:
    UPROPERTY(Transient, BlueprintReadOnly,
              Category = "UCk_UserWidget_UE")
    TWeakObjectPtr<AActor> _BindActor;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "UCk_UserWidget_UE")
    bool _InheritBindActorFromParent = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "UCk_UserWidget_UE")
    bool _DoNotDestroyDuringTransitions = false;

public:
    CK_PROPERTY_GET(_BindActor);
    CK_PROPERTY_GET(_InheritBindActorFromParent);
    CK_PROPERTY_GET(_DoNotDestroyDuringTransitions);
};

// --------------------------------------------------------------------------------------------------------------------