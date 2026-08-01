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
#include <Components/Widget.h>
#include <Framework/Application/NavigationConfig.h>
#include <Framework/Application/SlateApplication.h>
#include <Framework/Application/SlateUser.h>
#include <GameFramework/PlayerController.h>
#include <GenericPlatform/GenericApplication.h>
#include <GenericPlatform/GenericWindow.h>
#include <GenericPlatform/ICursor.h>
#include <Layout/WidgetPath.h>
#include <UObject/UObjectIterator.h>
#include <Widgets/CommonActivatableWidgetContainer.h>

#if PLATFORM_MICROSOFT
// ICursor.h only forward-declares tagRECT; ICursor::Lock needs the complete type.
#include "Microsoft/WindowsHWrapper.h"
#endif

// --------------------------------------------------------------------------------------------------------------------

int32 UCk_Utils_UI_UE::InputSuspensionCounter = 0;

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
        [&](UWidget* InWidget) -> ECk_UI_ForEachWidgetResult
    {
        if (auto* const UserWidget = Cast<UUserWidget>(InWidget);
            ck::IsValid(UserWidget) && UserWidget->IsA(InClass))
        {
            Result.Add(UserWidget);
        }

        return ECk_UI_ForEachWidgetResult::Continue;
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

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_UI_UE::
    Request_SetNavigationConfig(
        const FCk_UI_NavigationConfig& InConfig)
    -> void
{
    if (NOT FSlateApplication::IsInitialized())
    { return; }

    // A fresh config rather than mutating the live one: the FNavigationConfig constructor is what
    // populates KeyEventRules / KeyActionRules, so building anew keeps the digital-rule defaults
    // intact instead of inheriting whatever a previous caller left behind.
    const auto NavigationConfig = MakeShared<FNavigationConfig>();

    NavigationConfig->bTabNavigation = InConfig.Get_TabNavigation();
    NavigationConfig->bKeyNavigation = InConfig.Get_KeyNavigation();
    NavigationConfig->bAnalogNavigation = InConfig.Get_AnalogNavigation();
    NavigationConfig->bIgnoreModifiersForNavigationActions = InConfig.Get_IgnoreModifiersForNavigationActions();
    NavigationConfig->AnalogNavigationHorizontalThreshold = InConfig.Get_AnalogNavigationHorizontalThreshold();
    NavigationConfig->AnalogNavigationVerticalThreshold = InConfig.Get_AnalogNavigationVerticalThreshold();
    NavigationConfig->AnalogHorizontalKey = InConfig.Get_AnalogHorizontalKey();
    NavigationConfig->AnalogVerticalKey = InConfig.Get_AnalogVerticalKey();

    FSlateApplication::Get().SetNavigationConfig(NavigationConfig);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_UI_UE::
    Get_NavigationConfig()
    -> FCk_UI_NavigationConfig
{
    auto Config = FCk_UI_NavigationConfig{};

    if (NOT FSlateApplication::IsInitialized())
    { return Config; }

    const auto& NavigationConfig = FSlateApplication::Get().GetNavigationConfig();

    Config.Set_TabNavigation(NavigationConfig->bTabNavigation);
    Config.Set_KeyNavigation(NavigationConfig->bKeyNavigation);
    Config.Set_AnalogNavigation(NavigationConfig->bAnalogNavigation);
    Config.Set_IgnoreModifiersForNavigationActions(NavigationConfig->bIgnoreModifiersForNavigationActions);
    Config.Set_AnalogNavigationHorizontalThreshold(NavigationConfig->AnalogNavigationHorizontalThreshold);
    Config.Set_AnalogNavigationVerticalThreshold(NavigationConfig->AnalogNavigationVerticalThreshold);
    Config.Set_AnalogHorizontalKey(NavigationConfig->AnalogHorizontalKey);
    Config.Set_AnalogVerticalKey(NavigationConfig->AnalogVerticalKey);

    return Config;
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
            CK_INVALID_ENUM(InNewMouseVisibility);
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

namespace ck_ui_cursor_lock
{
    auto
        Get_PlatformCursor()
        -> TSharedPtr<ICursor>
    {
        if (NOT FSlateApplication::IsInitialized())
        { return {}; }

        const auto PlatformApplication = FSlateApplication::Get().GetPlatformApplication();

        if (ck::Is_NOT_Valid(PlatformApplication))
        { return {}; }

        return PlatformApplication->Cursor;
    }
}

auto
    UCk_Utils_UI_UE::
    Request_LockCursorToWidget(
        UWidget* InWidget)
    -> ECk_UI_CursorLock_Result
{
    CK_ENSURE_IF_NOT(ck::IsValid(InWidget),
        TEXT("Cannot lock the cursor to an invalid Widget"))
    { return ECk_UI_CursorLock_Result::InvalidWidget; }

    const auto Cursor = ck_ui_cursor_lock::Get_PlatformCursor();

    if (ck::Is_NOT_Valid(Cursor))
    { return ECk_UI_CursorLock_Result::NoCursorAvailable; }

    const auto SlateWidget = InWidget->GetCachedWidget();

    if (ck::Is_NOT_Valid(SlateWidget))
    { return ECk_UI_CursorLock_Result::WidgetNotOnScreen; }

    auto WidgetPath = FWidgetPath{};

    if (NOT FSlateApplication::Get().GeneratePathToWidgetUnchecked(SlateWidget.ToSharedRef(), WidgetPath) ||
        NOT WidgetPath.IsValid())
    { return ECk_UI_CursorLock_Result::WidgetNotOnScreen; }

    const auto NativeWindow = WidgetPath.GetWindow()->GetNativeWindow();

    if (ck::Is_NOT_Valid(NativeWindow) || NOT NativeWindow->IsForegroundWindow())
    { return ECk_UI_CursorLock_Result::WindowNotForeground; }

    auto SlateClipRect = WidgetPath.Widgets.Last().Geometry.GetLayoutBoundingRect();

#if PLATFORM_DESKTOP
    const auto IsBorderlessGameWindow = NativeWindow->IsDefinitionValid() &&
        NativeWindow->GetDefinition().Type == EWindowType::GameWindow &&
        NOT NativeWindow->GetDefinition().HasOSWindowBorder;
    const auto ClipRectAdjustment = IsBorderlessGameWindow ? 0 : 1;
#else
    const auto ClipRectAdjustment = 0;
#endif

    // A fullscreen viewport whose resolution differs from the platform resolution offsets the hit-test.
    if (FSlateApplication::Get().GetTransformFullscreenMouseInput() && NOT GIsEditor &&
        NativeWindow->GetWindowMode() == EWindowMode::Fullscreen)
    {
        auto CachedDisplayMetrics = FDisplayMetrics{};
        FSlateApplication::Get().GetCachedDisplayMetrics(CachedDisplayMetrics);

        const auto DisplaySize = FVector2f{
            static_cast<float>(CachedDisplayMetrics.PrimaryDisplayWidth),
            static_cast<float>(CachedDisplayMetrics.PrimaryDisplayHeight)};
        const auto DisplayDistortion = SlateClipRect.GetSize() / DisplaySize;

        SlateClipRect.Left   /= DisplayDistortion.X;
        SlateClipRect.Top    /= DisplayDistortion.Y;
        SlateClipRect.Right  /= DisplayDistortion.X;
        SlateClipRect.Bottom /= DisplayDistortion.Y;
    }

    // Rounded inwards on every edge — half a pixel the other way lets the cursor escape the widget geometry.
    auto ClipRect = RECT{};
    ClipRect.left   = FMath::RoundToInt(SlateClipRect.Left + ClipRectAdjustment);
    ClipRect.top    = FMath::RoundToInt(SlateClipRect.Top + ClipRectAdjustment);
    ClipRect.right  = FMath::TruncToInt(SlateClipRect.Right - ClipRectAdjustment);
    ClipRect.bottom = FMath::TruncToInt(SlateClipRect.Bottom - ClipRectAdjustment);

    // One-shot: nothing re-locks on a layout change or auto-unlocks on teardown — re-call after a layout change,
    // and always pair with Request_UnlockCursor before the widget goes away.
    Cursor->Lock(&ClipRect);

    return ECk_UI_CursorLock_Result::Success;
}

auto
    UCk_Utils_UI_UE::
    Request_UnlockCursor()
    -> void
{
    const auto Cursor = ck_ui_cursor_lock::Get_PlatformCursor();

    if (ck::Is_NOT_Valid(Cursor))
    { return; }

    Cursor->Lock(nullptr);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_UI_UE::
    SuspendInput(
        const APlayerController* InPlayerController,
        FName InReason)
    -> FCk_Handle_InputSuspension
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
    -> FCk_Handle_InputSuspension
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
        FCk_Handle_InputSuspension& InSuspensionHandle)
    -> void
{
    if (ck::Is_NOT_Valid(InPlayerController))
    { return; }

    ResumeInput(InPlayerController->GetLocalPlayer(), InSuspensionHandle);
}

auto
    UCk_Utils_UI_UE::
    ResumeInput(
        const ULocalPlayer* InLocalPlayer,
        FCk_Handle_InputSuspension& InSuspensionHandle)
    -> void
{
    if (NOT InSuspensionHandle.IsValid())
    { return; }

    if (ck::Is_NOT_Valid(InLocalPlayer))
    { return; }

    auto* MutableLocalPlayer = const_cast<ULocalPlayer*>(InLocalPlayer);
    auto* Subsystem = MutableLocalPlayer->GetSubsystem<UCk_UI_Subsystem_UE>();

    if (ck::Is_NOT_Valid(Subsystem))
    { return; }

    Subsystem->ResumeInput(InSuspensionHandle);
}

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

#include "CkEcs/ContextReceiver/CkContextReceiver_Utils.h"
#include "CkUI/UserWidget/CkUserWidget.h"
#include "CkUI/UserWidget/CkActivatableWidget.h"

auto
    UCk_Utils_UI_UE::
    PropagateContextToChildWidgets(
        UWidget* InRootWidget,
        const FCk_Handle& InContextEntity)
    -> void
{
    ForEachWidgetAndChildren_IncludingUserWidgets(
        InRootWidget,
        [&InContextEntity](UWidget* InChildWidget) -> ECk_UI_ForEachWidgetResult
        {
            if (const auto* CkWidget = Cast<UCk_UserWidget_UE>(InChildWidget))
            {
                if (NOT CkWidget->Get_InheritContextFromParent())
                { return ECk_UI_ForEachWidgetResult::SkipSubtree; }

                UCk_Utils_ContextReceiver_UE::TryInjectContextIntoObject(InChildWidget, InContextEntity);
                return ECk_UI_ForEachWidgetResult::SkipSubtree;
            }

            if (const auto* CkActivatable = Cast<UCk_ActivatableWidget_UE>(InChildWidget))
            {
                if (NOT CkActivatable->Get_InheritContextFromParent())
                { return ECk_UI_ForEachWidgetResult::SkipSubtree; }

                UCk_Utils_ContextReceiver_UE::TryInjectContextIntoObject(InChildWidget, InContextEntity);
                return ECk_UI_ForEachWidgetResult::SkipSubtree;
            }

            UCk_Utils_ContextReceiver_UE::TryInjectContextIntoObject(InChildWidget, InContextEntity);
            return ECk_UI_ForEachWidgetResult::Continue;
        });
}

// --------------------------------------------------------------------------------------------------------------------