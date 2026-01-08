// Copyright 2025 CkFoundation. All Rights Reserved.

#include "CkUI/CkUI_Utils.h"

#include "CommonActivatableWidget.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"
#include "CkUI/Subsystem/CkUI_Subsystem.h"
#include "CkUI/Layout/CkUI_Layout.h"
#include "CkUI/Layer/CkUI_LayerStack.h"

#include <CommonInputSubsystem.h>
#include <Blueprint/UserWidget.h>
#include <Blueprint/WidgetTree.h>
#include <Components/PanelWidget.h>
#include <Components/NamedSlot.h>
#include <Blueprint/WidgetLayoutLibrary.h>
#include <Framework/Application/SlateApplication.h>
#include <GameFramework/PlayerController.h>
#include <UObject/UObjectIterator.h>
#include <Widgets/CommonActivatableWidgetContainer.h>

// --------------------------------------------------------------------------------------------------------------------
// Static Member Initialization
// --------------------------------------------------------------------------------------------------------------------

int32 UCk_Utils_UI_UE::InputSuspensionCounter = 0;

// --------------------------------------------------------------------------------------------------------------------
// Subsystem & Layout Access
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_UI_UE::
    Get_UISubsystem(
        const APlayerController* InPlayerController)
    -> UCk_UI_Subsystem_UE*
{
    if (ck::Is_NOT_Valid(InPlayerController))
    { return nullptr; }

    const auto* LocalPlayer = InPlayerController->GetLocalPlayer();
    return Get_UISubsystem(LocalPlayer);
}

auto
    UCk_Utils_UI_UE::
    Get_UISubsystem(
        const ULocalPlayer* InLocalPlayer)
    -> UCk_UI_Subsystem_UE*
{
    if (ck::Is_NOT_Valid(InLocalPlayer))
    { return nullptr; }

    return InLocalPlayer->GetSubsystem<UCk_UI_Subsystem_UE>();
}

auto
    UCk_Utils_UI_UE::
    Get_Layout(
        const APlayerController* InPlayerController)
    -> UCk_UI_Layout_UE*
{
    const auto* Subsystem = Get_UISubsystem(InPlayerController);

    if (ck::Is_NOT_Valid(Subsystem))
    { return nullptr; }

    return Subsystem->TryGet_CurrentLayout();
}

auto
    UCk_Utils_UI_UE::
    Get_Layer(
        const APlayerController* InPlayerController,
        FGameplayTag InLayerTag)
    -> UCk_UI_LayerStack_UE*
{
    const auto* Layout = Get_Layout(InPlayerController);

    if (ck::Is_NOT_Valid(Layout))
    { return nullptr; }

    return Layout->Get_Layer(InLayerTag);
}

// --------------------------------------------------------------------------------------------------------------------
// Layer Operations
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_UI_UE::
    PushWidgetToLayer(
        const APlayerController* InPlayerController,
        FGameplayTag InLayerTag,
        TSubclassOf<UCommonActivatableWidget> InWidgetClass)
    -> UCommonActivatableWidget*
{
    auto* Subsystem = Get_UISubsystem(InPlayerController);

    if (ck::Is_NOT_Valid(Subsystem))
    { return nullptr; }

    return Subsystem->PushWidgetToLayer(InLayerTag, InWidgetClass);
}

auto
    UCk_Utils_UI_UE::
    PushWidgetToLayer_Soft(
        const APlayerController* InPlayerController,
        FGameplayTag InLayerTag,
        TSoftClassPtr<UCommonActivatableWidget> InWidgetClass,
        FCk_Delegate_UI_OnWidgetReady InOnWidgetReady)
    -> void
{
    auto* Subsystem = Get_UISubsystem(InPlayerController);

    if (ck::Is_NOT_Valid(Subsystem))
    {
        InOnWidgetReady.ExecuteIfBound(nullptr);
        return;
    }

    Subsystem->PushWidgetToLayer_Soft(InLayerTag, InWidgetClass, InOnWidgetReady);
}

auto
    UCk_Utils_UI_UE::
    PushWidgetInstanceToLayer(
        const APlayerController* InPlayerController,
        FGameplayTag InLayerTag,
        UCommonActivatableWidget* InWidget)
    -> UCommonActivatableWidget*
{
    auto* Subsystem = Get_UISubsystem(InPlayerController);

    if (ck::Is_NOT_Valid(Subsystem))
    { return nullptr; }

    return Subsystem->PushWidgetInstanceToLayer(InLayerTag, InWidget);
}

auto
    UCk_Utils_UI_UE::
    PopWidgetFromLayer(
        const APlayerController* InPlayerController,
        FGameplayTag InLayerTag)
    -> UCommonActivatableWidget*
{
    auto* Subsystem = Get_UISubsystem(InPlayerController);

    if (ck::Is_NOT_Valid(Subsystem))
    { return nullptr; }

    return Subsystem->PopWidgetFromLayer(InLayerTag);
}

auto
    UCk_Utils_UI_UE::
    ClearLayer(
        const APlayerController* InPlayerController,
        FGameplayTag InLayerTag)
    -> void
{
    auto* Subsystem = Get_UISubsystem(InPlayerController);

    if (ck::Is_NOT_Valid(Subsystem))
    { return; }

    Subsystem->ClearLayer(InLayerTag);
}

auto
    UCk_Utils_UI_UE::
    RemoveWidget(
        const APlayerController* InPlayerController,
        UCommonActivatableWidget* InWidget)
    -> bool
{
    auto* Subsystem = Get_UISubsystem(InPlayerController);

    if (ck::Is_NOT_Valid(Subsystem))
    { return false; }

    return Subsystem->RemoveWidget(InWidget);
}

auto
    UCk_Utils_UI_UE::
    RemoveWidgetSelf(
        UCommonActivatableWidget* InWidget)
    -> bool
{
    if (ck::Is_NOT_Valid(InWidget))
    { return false; }

    const auto* OwningPlayer = InWidget->GetOwningPlayer();
    return RemoveWidget(OwningPlayer, InWidget);
}

// --------------------------------------------------------------------------------------------------------------------
// Named Slot Operations
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_UI_UE::
    FindNamedSlot(
        UUserWidget* InSourceWidget,
        FName InNamedSlotName,
        ECk_UI_NamedSlot_SearchRecursive InIsRecursive)
    -> UNamedSlot*
{
    CK_ENSURE_IF_NOT(ck::IsValid(InSourceWidget),
        TEXT("Widget source for NamedSlot [{}] is not valid"), InNamedSlotName)
    { return nullptr; }

    CK_ENSURE_IF_NOT(ck::IsValid(InNamedSlotName),
        TEXT("Named Slot name is not valid. Trying to get NamedSlot from widget [{}]"), InSourceWidget)
    { return nullptr; }

    auto* RootWidgetTree = InSourceWidget->WidgetTree.Get();

    CK_ENSURE_IF_NOT(ck::IsValid(RootWidgetTree),
        TEXT("Widget tree of widget [{}] is not valid"), InSourceWidget)
    { return nullptr; }

    auto Widgets = TArray<UWidget*>{};
    RootWidgetTree->GetAllWidgets(Widgets);

    const auto* FoundNamedSlot = Widgets.FindByPredicate([&](UWidget* Widget)
    {
        return Widget->GetFName() == InNamedSlotName && ck::IsValid(Cast<UNamedSlot>(Widget));
    });

    if (ck::IsValid(FoundNamedSlot, ck::IsValid_Policy_NullptrOnly{}) &&
        ck::IsValid(Cast<UNamedSlot>(*FoundNamedSlot)))
    { return Cast<UNamedSlot>(*FoundNamedSlot); }

    if (InIsRecursive == ECk_UI_NamedSlot_SearchRecursive::NonRecursive)
    { return nullptr; }

    for (const auto Widget : Widgets)
    {
        if (const auto UserWidget = Cast<UUserWidget>(Widget);
            ck::IsValid(UserWidget))
        {
            if (const auto NamedSlot = FindNamedSlot(UserWidget, InNamedSlotName, InIsRecursive))
            { return NamedSlot; }
        }

        if (const auto ContainerWidget = Cast<UCommonActivatableWidgetContainerBase>(Widget);
            ck::IsValid(ContainerWidget))
        {
            if (const auto& MaybeValidActiveWidget = ContainerWidget->GetActiveWidget();
                ck::IsValid(MaybeValidActiveWidget))
            {
                if (const auto NamedSlot = FindNamedSlot(MaybeValidActiveWidget, InNamedSlotName, InIsRecursive))
                { return NamedSlot; }
            }
        }
    }

    return nullptr;
}

auto
    UCk_Utils_UI_UE::
    IsNamedSlotOccupied(
        UNamedSlot* InNamedSlot)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InNamedSlot),
        TEXT("Named Slot is not valid"))
    { return false; }

    return InNamedSlot->GetAllChildren().Num() > 0;
}

auto
    UCk_Utils_UI_UE::
    InsertWidgetToNamedSlot(
        UNamedSlot* InNamedSlot,
        UUserWidget* InInsertedWidget)
    -> UPanelSlot*
{
    CK_ENSURE_IF_NOT(ck::IsValid(InNamedSlot),
        TEXT("Named Slot Name is not valid, trying to insert widget [{}]"), InInsertedWidget)
    { return nullptr; }

    CK_ENSURE_IF_NOT(ck::IsValid(InInsertedWidget),
        TEXT("Widget to be inserted into NamedSlot [{}] is not valid"), InNamedSlot)
    { return nullptr; }

    return InNamedSlot->AddChild(InInsertedWidget);
}

auto
    UCk_Utils_UI_UE::
    FindNamedSlotAndInsertWidget(
        UUserWidget* InSourceWidget,
        UUserWidget* InInsertedWidget,
        FName InNamedSlotName,
        ECk_UI_NamedSlot_SearchRecursive InIsRecursive,
        ECk_UI_NamedSlot_EnsureSlotIsFound InEnsureSlotIsFound)
    -> UPanelSlot*
{
    if (auto NamedSlot = FindNamedSlot(InSourceWidget, InNamedSlotName, InIsRecursive);
        ck::IsValid(NamedSlot))
    {
        CK_ENSURE_IF_NOT(NOT IsNamedSlotOccupied(NamedSlot),
            TEXT("Named Slot [{}] is not available for inserting a widget, already has widget [{}] present"),
            NamedSlot,
            NamedSlot->GetAllChildren().Num() > 0 ? NamedSlot->GetAllChildren()[0] : nullptr)
        { return nullptr; }

        return InsertWidgetToNamedSlot(NamedSlot, InInsertedWidget);
    }

    CK_ENSURE_IF_NOT(InEnsureSlotIsFound != ECk_UI_NamedSlot_EnsureSlotIsFound::EnsureSlotIsFound,
        TEXT("Widget [{}] does not contain named slot [{}]"),
        InSourceWidget,
        InNamedSlotName)
    {}

    return nullptr;
}

// --------------------------------------------------------------------------------------------------------------------
// Focus Management
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_UI_UE::
    Get_WidgetsOfClassInHierarchy(
        UUserWidget* InSourceWidget,
        TSubclassOf<UUserWidget> InClass)
    -> TArray<UUserWidget*>
{
    if (ck::Is_NOT_Valid(InSourceWidget))
    { return {}; }

    if (ck::Is_NOT_Valid(InClass))
    { return {}; }

    auto Result = TArray<UUserWidget*>{};

    ForEachWidgetAndChildren_IncludingUserWidgets(InSourceWidget,
        [&](UWidget* InWidget) -> bool
    {
        if (auto* const UserWidget = Cast<UUserWidget>(InWidget);
            ck::IsValid(UserWidget) && UserWidget->IsA(InClass))
        {
            Result.Add(UserWidget);
        }

        return false; // continue traversal
    });

    return Result;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_UI_UE::
    Get_CurrentlyFocusedWidget(
        int32 InUserIndex)
    -> UWidget*
{
    const auto FocusedSlateWidget = FSlateApplication::Get().GetUserFocusedWidget(InUserIndex);

    if (ck::Is_NOT_Valid(FocusedSlateWidget))
    { return nullptr; }

    for (TObjectIterator<UWidget> Itr; Itr; ++Itr)
    {
        if (auto* CandidateUMGWidget = *Itr;
            CandidateUMGWidget->GetCachedWidget() == FocusedSlateWidget)
        { return CandidateUMGWidget; }
    }

    return nullptr;
}

auto
    UCk_Utils_UI_UE::
    SetInputModeAndMouseVisibility(
        APlayerController* InPlayerController,
        ECk_UI_NewInputMode InNewInputMode,
        ECk_UI_NewMouseVisibility InNewMouseVisibility,
        bool InResetCursorToCenter)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InPlayerController), TEXT("Invalid PlayerController passed to SetInputModeAndMouseVisibility"))
    { return; }

    switch (InNewInputMode)
    {
        case ECk_UI_NewInputMode::DontChange:
        {
            break;
        }
        case ECk_UI_NewInputMode::GameOnly:
        {
            const auto InputMode = FInputModeGameOnly();
            InPlayerController->SetInputMode(InputMode);
            break;
        }
        case ECk_UI_NewInputMode::UIOnly:
        {
            const auto InputMode = FInputModeUIOnly();
            InPlayerController->SetInputMode(InputMode);
            break;
        }
        case ECk_UI_NewInputMode::GameAndUI:
        {
            const auto InputMode = FInputModeGameAndUI();
            InPlayerController->SetInputMode(InputMode);
            break;
        }
        default:
        {
            CK_INVALID_ENUM(InNewInputMode);
            break;
        }
    }

    switch (InNewMouseVisibility)
    {
        case ECk_UI_NewMouseVisibility::DontChange:
        {
            break;
        }
        case ECk_UI_NewMouseVisibility::Show:
        {
            InPlayerController->SetShowMouseCursor(true);
            break;
        }
        case ECk_UI_NewMouseVisibility::Hide:
        {
            InPlayerController->SetShowMouseCursor(false);
            break;
        }
        default:
        {
            CK_INVALID_ENUM(InNewInputMode);
            break;
        }
    }

    if (InResetCursorToCenter)
    {
        const auto& Size = UWidgetLayoutLibrary::GetViewportSize(InPlayerController);
        const auto X = FMath::TruncToInt(Size.X / 2);
        const auto Y = FMath::TruncToInt(Size.Y / 2);
        InPlayerController->SetMouseLocation(X, Y);
    }
}

// --------------------------------------------------------------------------------------------------------------------
// Context Injection (Routes through Subsystem)
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_UI_UE::
    InjectContext(
        UUserWidget* InWidget,
        const FCk_UI_Context& InContext)
    -> void
{
    if (ck::Is_NOT_Valid(InWidget))
    { return; }

    auto* Subsystem = Get_UISubsystem(InWidget->GetOwningPlayer());

    if (ck::Is_NOT_Valid(Subsystem))
    { return; }

    Subsystem->InjectContextToWidget(InWidget, InContext);
}

auto
    UCk_Utils_UI_UE::
    InjectEntityContext(
        UUserWidget* InWidget,
        FCk_Handle InEntity)
    -> void
{
    InjectContext(InWidget, FCk_UI_Context::MakeFromEntity(InEntity));
}

auto
    UCk_Utils_UI_UE::
    InjectActorContext(
        UUserWidget* InWidget,
        AActor* InActor)
    -> void
{
    InjectContext(InWidget, FCk_UI_Context::MakeFromActor(InActor));
}

auto
    UCk_Utils_UI_UE::
    ClearContext(
        UUserWidget* InWidget)
    -> void
{
    if (ck::Is_NOT_Valid(InWidget))
    { return; }

    auto* Subsystem = Get_UISubsystem(InWidget->GetOwningPlayer());

    if (ck::Is_NOT_Valid(Subsystem))
    { return; }

    Subsystem->ClearContextFromWidget(InWidget);
}

// --------------------------------------------------------------------------------------------------------------------
// Context Query (From Subsystem Registry)
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_UI_UE::
    Get_ContextForWidget(
        const UUserWidget* InWidget)
    -> FCk_UI_Context
{
    if (ck::Is_NOT_Valid(InWidget))
    { return {}; }

    const auto* Subsystem = Get_UISubsystem(InWidget->GetOwningPlayer());

    if (ck::Is_NOT_Valid(Subsystem))
    { return {}; }

    return Subsystem->Get_ContextForWidget(InWidget);
}

auto
    UCk_Utils_UI_UE::
    Has_ValidContextForWidget(
        const UUserWidget* InWidget)
    -> bool
{
    return Get_ContextForWidget(InWidget).IsValid();
}

auto
    UCk_Utils_UI_UE::
    Get_ContextEntity(
        const UUserWidget* InWidget)
    -> FCk_Handle
{
    return Get_ContextForWidget(InWidget).Get_Entity();
}

auto
    UCk_Utils_UI_UE::
    Get_ContextActor(
        const UUserWidget* InWidget)
    -> AActor*
{
    return Get_ContextForWidget(InWidget).Get_Actor().Get();
}

auto
    UCk_Utils_UI_UE::
    Get_ContextPayload(
        const UUserWidget* InWidget)
    -> UObject*
{
    return Get_ContextForWidget(InWidget).Get_Payload().Get();
}

// --------------------------------------------------------------------------------------------------------------------
// Context Creation Helpers
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_UI_UE::
    MakeContext_Entity(
        FCk_Handle InEntity)
    -> FCk_UI_Context
{
    return FCk_UI_Context::MakeFromEntity(InEntity);
}

auto
    UCk_Utils_UI_UE::
    MakeContext_Actor(
        AActor* InActor)
    -> FCk_UI_Context
{
    return FCk_UI_Context::MakeFromActor(InActor);
}

auto
    UCk_Utils_UI_UE::
    MakeContext_Object(
        UObject* InObject)
    -> FCk_UI_Context
{
    return FCk_UI_Context::MakeFromObject(InObject);
}

auto
    UCk_Utils_UI_UE::
    MakeContext_EntityAndActor(
        FCk_Handle InEntity,
        AActor* InActor)
    -> FCk_UI_Context
{
    return FCk_UI_Context::MakeFromEntityAndActor(InEntity, InActor);
}

// --------------------------------------------------------------------------------------------------------------------
// Input Operations
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_UI_UE::
    Get_EffectiveInputMode(
        const APlayerController* InPlayerController)
    -> ECk_UI_InputMode
{
    const auto* Subsystem = Get_UISubsystem(InPlayerController);

    if (ck::Is_NOT_Valid(Subsystem))
    { return ECk_UI_InputMode::GameOnly; }

    return Subsystem->Get_EffectiveInputMode();
}

// --------------------------------------------------------------------------------------------------------------------
// Input Suspension
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_UI_UE::
    SuspendInput(
        const APlayerController* InPlayerController,
        FName InReason)
    -> FName
{
    if (ck::Is_NOT_Valid(InPlayerController))
    { return NAME_None; }

    return SuspendInput(InPlayerController->GetLocalPlayer(), InReason);
}

auto
    UCk_Utils_UI_UE::
    SuspendInput(
        const ULocalPlayer* InLocalPlayer,
        FName InReason)
    -> FName
{
    if (ck::Is_NOT_Valid(InLocalPlayer))
    { return NAME_None; }

    auto* CommonInputSubsystem = UCommonInputSubsystem::Get(InLocalPlayer);

    if (ck::Is_NOT_Valid(CommonInputSubsystem))
    { return NAME_None; }

    InputSuspensionCounter++;

    auto SuspendToken = InReason;
    SuspendToken.SetNumber(InputSuspensionCounter);

    CommonInputSubsystem->SetInputTypeFilter(ECommonInputType::MouseAndKeyboard, SuspendToken, true);
    CommonInputSubsystem->SetInputTypeFilter(ECommonInputType::Gamepad, SuspendToken, true);
    CommonInputSubsystem->SetInputTypeFilter(ECommonInputType::Touch, SuspendToken, true);

    return SuspendToken;
}

auto
    UCk_Utils_UI_UE::
    ResumeInput(
        const APlayerController* InPlayerController,
        FName InSuspendToken)
    -> void
{
    if (ck::Is_NOT_Valid(InPlayerController))
    { return; }

    ResumeInput(InPlayerController->GetLocalPlayer(), InSuspendToken);
}

auto
    UCk_Utils_UI_UE::
    ResumeInput(
        const ULocalPlayer* InLocalPlayer,
        FName InSuspendToken)
    -> void
{
    if (InSuspendToken == NAME_None)
    { return; }

    if (ck::Is_NOT_Valid(InLocalPlayer))
    { return; }

    auto* CommonInputSubsystem = UCommonInputSubsystem::Get(InLocalPlayer);

    if (ck::Is_NOT_Valid(CommonInputSubsystem))
    { return; }

    CommonInputSubsystem->SetInputTypeFilter(ECommonInputType::MouseAndKeyboard, InSuspendToken, false);
    CommonInputSubsystem->SetInputTypeFilter(ECommonInputType::Gamepad, InSuspendToken, false);
    CommonInputSubsystem->SetInputTypeFilter(ECommonInputType::Touch, InSuspendToken, false);
}

// --------------------------------------------------------------------------------------------------------------------
// Input Type Queries
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_UI_UE::
    Get_OwningPlayerInputType(
        const UUserWidget* InWidget)
    -> ECommonInputType
{
    if (ck::Is_NOT_Valid(InWidget))
    { return ECommonInputType::MouseAndKeyboard; }

    const auto* LocalPlayer = InWidget->GetOwningLocalPlayer();

    if (ck::Is_NOT_Valid(LocalPlayer))
    { return ECommonInputType::MouseAndKeyboard; }

    const auto* InputSubsystem = LocalPlayer->GetSubsystem<UCommonInputSubsystem>();

    if (ck::Is_NOT_Valid(InputSubsystem))
    { return ECommonInputType::MouseAndKeyboard; }

    return InputSubsystem->GetCurrentInputType();
}

auto
    UCk_Utils_UI_UE::
    IsOwningPlayerUsingGamepad(
        const UUserWidget* InWidget)
    -> bool
{
    return Get_OwningPlayerInputType(InWidget) == ECommonInputType::Gamepad;
}

auto
    UCk_Utils_UI_UE::
    IsOwningPlayerUsingTouch(
        const UUserWidget* InWidget)
    -> bool
{
    return Get_OwningPlayerInputType(InWidget) == ECommonInputType::Touch;
}

auto
    UCk_Utils_UI_UE::
    IsOwningPlayerUsingMouseAndKeyboard(
        const UUserWidget* InWidget)
    -> bool
{
    return Get_OwningPlayerInputType(InWidget) == ECommonInputType::MouseAndKeyboard;
}

// --------------------------------------------------------------------------------------------------------------------