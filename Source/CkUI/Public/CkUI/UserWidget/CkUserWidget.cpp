#include "CkUserWidget.h"

#include "Ckui/CkUI_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkUI/CkUI_Log.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UserWidget_UE::
    BindToActor(AActor* InActor)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InActor), TEXT("Failed to BIND UserWidget [{}] to an Actor because the Actor is INVALID"), this)
    { return; }

    TArray<UCk_UserWidget_UE*> BindWidgets;

    const auto& DoBindWidgetToActor = [&](UCk_UserWidget_UE* InWidget) -> void
    {
        if (ck::Is_NOT_Valid(InWidget))
        { return; }

        if (InWidget->DoGet_IsAlreadyBoundTo(InActor))
        { return; }

        InWidget->DoBindToActor(InActor);
        BindWidgets.Add(InWidget);
    };

    DoBindWidgetToActor(this);

    UCk_Utils_UI_UE::ForEachWidgetAndChildren_IncludingUserWidgets(this, [&](UWidget* InWidget) -> bool
    {
        auto* DerivedWidget = Cast<UCk_UserWidget_UE>(InWidget);

        if (ck::Is_NOT_Valid(DerivedWidget))
        { return false; }

        if (NOT DerivedWidget->Get_InheritBindActorFromParent())
        { return true; }

        DoBindWidgetToActor(DerivedWidget);
        return false;
    });

    for (auto& Widget : BindWidgets)
    {
        CK_ENSURE_IF_NOT(ck::IsValid(Widget),
            TEXT("Widget [{}] became invalid during binding process of [{}]!"),
            Widget,
            this)
        { continue; }

        Widget->DoBindToActor_BP(InActor);
    }
}

auto
    UCk_UserWidget_UE::
    UnbindFromActor(AActor* InActor)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InActor), TEXT("Failed to UNBIND UserWidget [{}] from an Actor because the Actor is INVALID"), this)
    { return; }

    TArray<UCk_UserWidget_UE*> UnbindWidgets;

    const auto& DoUnbindWidgetFromActor = [&](UCk_UserWidget_UE* InWidget) -> void
    {
        if (ck::Is_NOT_Valid(InWidget))
        { return; }

        if (InWidget->DoGet_IsAlreadyBoundTo(nullptr))
        { return; }

        InWidget->DoUnbindFromActor_BP(InActor);
        UnbindWidgets.Add(InWidget);
    };

    DoUnbindWidgetFromActor(this);

    UCk_Utils_UI_UE::ForEachWidgetAndChildren_IncludingUserWidgets(this, [&](UWidget* InWidget)
    {
        auto* DerivedWidget = Cast<UCk_UserWidget_UE>(InWidget);

        if (ck::Is_NOT_Valid(DerivedWidget))
        { return false; }

        if (NOT DerivedWidget->Get_InheritBindActorFromParent())
        { return true; }

        DoUnbindWidgetFromActor(DerivedWidget);
        return false;
    });

    for (auto& Widget : UnbindWidgets)
    {
        CK_ENSURE_IF_NOT(ck::IsValid(Widget),
            TEXT("Widget [{}] became invalid during unbinding process of [{}]!"),
            Widget,
            this)
        { continue; }

        Widget->DoUnbindFromActor(InActor);
    }
}

auto
    UCk_UserWidget_UE::
    DoGet_IsAlreadyBoundTo(const AActor* InActor) const
    -> bool
{
    if (Get_IsDefaultBindActor())
    { return false; }

    if (const auto& CurrentBindActor = Get_BindActor(); CurrentBindActor == InActor)
    { return true; }

    return false;
}

auto
    UCk_UserWidget_UE::
    DoBindToActor(AActor* InActor)
    -> void
{
    DoClearDefaultBindActor();

    const auto& PrevBindActor = Get_BindActor().Get();
    CK_ENSURE_IF_NOT(ck::Is_NOT_Valid(PrevBindActor),
        TEXT("Widget [{}] cannot bind since _BindActor is [{}]!"),
        this,
        PrevBindActor)
    { return; }

    _BindActor = InActor;
}

auto
    UCk_UserWidget_UE::
    DoBindToActor_BP(AActor* InActor)
    -> void
{
    DoClearDefaultBindActor();

    const auto& currentBindActor = Get_BindActor().Get();
    CK_ENSURE_IF_NOT(ck::IsValid(currentBindActor),
        TEXT("Widget [{}] should be bound at this point but BindActor is [{}]!"),
        this,
        currentBindActor)
    { return; }

    OnBindToActor(InActor);
}

auto
    UCk_UserWidget_UE::
    DoUnbindFromActor_BP(AActor* InActor)
    -> void
{
    const auto& currentBindActor = Get_BindActor().Get();
    CK_ENSURE_IF_NOT(currentBindActor == InActor,
        TEXT("Widget [{}] cannot unbind since BindActor [{}] and InActor [{}] are not the same"),
        this,
        currentBindActor,
        InActor)
    { return; }

    OnUnbindFromActor(currentBindActor);
}

auto
    UCk_UserWidget_UE::
    DoUnbindFromActor(AActor* InActor)
    -> void
{
    const auto& CurrentBindActor = Get_BindActor().Get();
    CK_ENSURE_IF_NOT(CurrentBindActor == InActor,
        TEXT("Widget [{}] cannot unbind since BindActor [{}] and InActor [{}] are not the same"),
        this,
        CurrentBindActor,
        InActor)
    { return; }

    DoClearDefaultBindActor();
    _BindActor = nullptr;
}

auto
    UCk_UserWidget_UE::
    DoApplyDefaultBindActor()
    -> void
{
    if (ck::IsValid(Get_BindActor()))
    { return; }

    if (ck::Is_NOT_Valid(GetOwningPlayer()))
    { return; }

    if (ck::Is_NOT_Valid(GetOwningPlayer()->GetPawn()))
    { return; }

    _IsDefaultBindActor = true;
    _BindActor = GetOwningPlayer()->GetPawn();
}

auto
    UCk_UserWidget_UE::
    DoClearDefaultBindActor()
    -> void
{
    if (NOT Get_IsDefaultBindActor())
    { return; }

    _IsDefaultBindActor = false;
    _BindActor = nullptr;
}

auto
    UCk_UserWidget_UE::
    NativeDestruct()
    -> void
{
    if (NOT _DoNotDestroyDuringTransitions)
    {
        Super::NativeDestruct();
    }
}

// --------------------------------------------------------------------------------------------------------------------

