#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Validation/CkIsValid.h"

#include <Blueprint/UserWidget.h>
#include <Blueprint/WidgetTree.h>

#include "CkUserWidget.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType, Blueprintable, meta = (DisableNativeTick))
class CKUI_API UCk_UserWidget_UE : public UUserWidget
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

public:
    template <typename T_Predicate>
    auto ForEachWidget_IncludingUserWidgets(T_Predicate InPred) -> void;

    template <typename T_Predicate>
    static auto ForEachWidgetAndChildren_IncludingUserWidgets(UWidget* InWidget, T_Predicate InPred) -> void;

protected:
    auto DoGet_IsAlreadyBoundTo(const AActor* InActor) const -> bool;
    auto DoBindToActor(AActor* InActor) -> void;
    auto DoBindToActor_BP(AActor* InActor) -> void;
    auto DoUnbindFromActor_BP(AActor* InActor) -> void;
    auto DoUnbindFromActor(AActor* InActor) -> void;

    auto DoApplyDefaultBindActor() -> void;
    auto DoClearDefaultBindActor() -> void;

protected:
    virtual void NativeDestruct() override;

protected:
    UPROPERTY(Transient, BlueprintReadOnly,
              Category = "UCk_UserWidget_UE")
    TWeakObjectPtr<AActor> _BindActor;

    UPROPERTY(Transient, BlueprintReadOnly,
              Category = "UCk_UserWidget_UE")
    bool _IsDefaultBindActor = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "UCk_UserWidget_UE")
    bool _InheritBindActorFromParent = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "UCk_UserWidget_UE")
    bool _DoNotDestroyDuringTransitions = false;

public:
    CK_PROPERTY_GET(_BindActor);
    CK_PROPERTY_GET(_IsDefaultBindActor);
    CK_PROPERTY_GET(_InheritBindActorFromParent);
    CK_PROPERTY_GET(_DoNotDestroyDuringTransitions);
};

// --------------------------------------------------------------------------------------------------------------------
// Definitions

template <typename T_Predicate>
auto
    UCk_UserWidget_UE::
    ForEachWidget_IncludingUserWidgets(T_Predicate InPred)
    -> void
{
    if (auto* RootWidget = GetRootWidget(); ck::IsValid(RootWidget))
    { ForEachWidgetAndChildren_IncludingUserWidgets(RootWidget, InPred); }
}

template <typename Predicate>
auto
    UCk_UserWidget_UE::
    ForEachWidgetAndChildren_IncludingUserWidgets(UWidget* InWidget, Predicate Pred)
    -> void
{
    if (ck::Is_NOT_Valid(InWidget))
    { return; }

    if (Pred(InWidget))
    { return; }

    if (const auto* UserWidget = Cast<UUserWidget>(InWidget); ck::IsValid(UserWidget))
    {
        const auto* WidgetTree = UserWidget->WidgetTree.Get();

        if (ck::IsValid(WidgetTree))
        {
            ForEachWidgetAndChildren_IncludingUserWidgets(WidgetTree->RootWidget, Pred);
        }
    }

    if (const auto* NamedSlotHost = Cast<INamedSlotInterface>(InWidget); ck::IsValid(NamedSlotHost, ck::IsValid_Policy_NullptrOnly{}))
    {
        TArray<FName> SlotNames;
        NamedSlotHost->GetSlotNames(SlotNames);

        for (const auto& SlotName : SlotNames)
        {
            auto* SlotContent = NamedSlotHost->GetContentForSlot(SlotName);

            if (ck::Is_NOT_Valid(SlotContent))
            { continue; }

            ForEachWidgetAndChildren_IncludingUserWidgets(SlotContent, Pred);
        }
    }

    if (const auto* PanelParent = Cast<UPanelWidget>(InWidget); ck::IsValid(PanelParent, ck::IsValid_Policy_NullptrOnly{}))
    {
        for (auto ChildIndex = 0; ChildIndex < PanelParent->GetChildrenCount(); ++ChildIndex)
        {
            auto* ChildWidget = PanelParent->GetChildAt(ChildIndex);

            if (ck::Is_NOT_Valid(ChildWidget))
            { continue; }

            ForEachWidgetAndChildren_IncludingUserWidgets(ChildWidget, Pred);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
