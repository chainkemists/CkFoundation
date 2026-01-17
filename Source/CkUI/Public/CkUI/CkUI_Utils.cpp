// Copyright 2025 CkFoundation. All Rights Reserved.

#include "CkUI/CkUI_Utils.h"

#include "CommonActivatableWidget.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkUI/Subsystem/CkUI_Subsystem.h"

#include <Blueprint/UserWidget.h>
#include <Blueprint/WidgetTree.h>
#include <Blueprint/WidgetLayoutLibrary.h>
#include <CommonInputSubsystem.h>
#include <Components/NamedSlot.h>
#include <Components/PanelWidget.h>
#include <Framework/Application/SlateApplication.h>
#include <GameFramework/PlayerController.h>
#include <UObject/UObjectIterator.h>
#include <Widgets/CommonActivatableWidgetContainer.h>

// --------------------------------------------------------------------------------------------------------------------
// Static Member Initialization
// --------------------------------------------------------------------------------------------------------------------

int32 UCk_Utils_UI_UE::InputSuspensionCounter = 0;

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

    TArray<UWidget*> Widgets;
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
// Input Suspension
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_UI_UE::
    SuspendInput(
        const APlayerController* InPlayerController,
        FName InReason)
    -> FCk_UI_InputSuspensionToken
{
    if (ck::Is_NOT_Valid(InPlayerController))
    { return {}; }

    return SuspendInput(InPlayerController->GetLocalPlayer(), InReason);
}

auto
    UCk_Utils_UI_UE::
    SuspendInput(
        const ULocalPlayer* InLocalPlayer,
        FName InReason)
    -> FCk_UI_InputSuspensionToken
{
    if (ck::Is_NOT_Valid(InLocalPlayer))
    { return {}; }

    auto* MutableLocalPlayer = const_cast<ULocalPlayer*>(InLocalPlayer);
    auto* Subsystem = MutableLocalPlayer->GetSubsystem<UCk_UI_Subsystem_UE>();

    if (ck::Is_NOT_Valid(Subsystem))
    { return {}; }

    return Subsystem->SuspendInput(InReason);
}

auto
    UCk_Utils_UI_UE::
    ResumeInput(
        const APlayerController* InPlayerController,
        FCk_UI_InputSuspensionToken& InHandle)
    -> void
{
    if (ck::Is_NOT_Valid(InPlayerController))
    { return; }

    ResumeInput(InPlayerController->GetLocalPlayer(), InHandle);
}

auto
    UCk_Utils_UI_UE::
    ResumeInput(
        const ULocalPlayer* InLocalPlayer,
        FCk_UI_InputSuspensionToken& InHandle)
    -> void
{
    if (NOT InHandle.IsValid())
    { return; }

    if (ck::Is_NOT_Valid(InLocalPlayer))
    { return; }

    auto* MutableLocalPlayer = const_cast<ULocalPlayer*>(InLocalPlayer);
    auto* Subsystem = MutableLocalPlayer->GetSubsystem<UCk_UI_Subsystem_UE>();

    if (ck::Is_NOT_Valid(Subsystem))
    { return; }

    Subsystem->ResumeInput(InHandle);
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