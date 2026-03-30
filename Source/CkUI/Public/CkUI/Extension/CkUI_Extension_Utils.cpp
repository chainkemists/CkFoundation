// Copyright 2025 CkFoundation. All Rights Reserved.

#include "CkUI/Extension/CkUI_Extension_Utils.h"

#include "CkCore/Validation/CkIsValid.h"
#include "CkUI/Extension/CkUI_Extension_Subsystem.h"

#include <GameFramework/PlayerController.h>

// --------------------------------------------------------------------------------------------------------------------
// Subsystem Access
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_UI_Extension_UE::
    Get_ExtensionSubsystem(
        const APlayerController* InPlayerController)
    -> UCk_UI_Extension_Subsystem_UE*
{
    if (ck::Is_NOT_Valid(InPlayerController))
    { return nullptr; }

    return Get_ExtensionSubsystem(InPlayerController->GetLocalPlayer());
}

auto
    UCk_Utils_UI_Extension_UE::
    Get_ExtensionSubsystem(
        const ULocalPlayer* InLocalPlayer)
    -> UCk_UI_Extension_Subsystem_UE*
{
    if (ck::Is_NOT_Valid(InLocalPlayer))
    { return nullptr; }

    return InLocalPlayer->GetSubsystem<UCk_UI_Extension_Subsystem_UE>();
}

// --------------------------------------------------------------------------------------------------------------------
// Extension Registration
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_UI_Extension_UE::
    RegisterExtension(
        const APlayerController* InPlayerController,
        FGameplayTag InExtensionPointTag,
        TSubclassOf<UUserWidget> InWidgetClass,
        int32 InPriority)
    -> FCk_UI_ExtensionHandle
{
    auto* Subsystem = Get_ExtensionSubsystem(InPlayerController);

    if (ck::Is_NOT_Valid(Subsystem))
    { return {}; }

    return Subsystem->RegisterExtension(InExtensionPointTag, InWidgetClass, InPriority);
}

auto
    UCk_Utils_UI_Extension_UE::
    RegisterExtension_Instance(
        const APlayerController* InPlayerController,
        FGameplayTag InExtensionPointTag,
        UUserWidget* InWidgetInstance,
        int32 InPriority)
    -> FCk_UI_ExtensionHandle
{
    auto* Subsystem = Get_ExtensionSubsystem(InPlayerController);

    if (ck::Is_NOT_Valid(Subsystem))
    { return {}; }

    return Subsystem->RegisterExtension(InExtensionPointTag, InWidgetInstance, InPriority);
}

auto
    UCk_Utils_UI_Extension_UE::
    UnregisterExtension(
        FCk_UI_ExtensionHandle& InHandle)
    -> void
{
    InHandle.Unregister();
}

auto
    UCk_Utils_UI_Extension_UE::
    TryUnregisterExtensionByWidget(
        UUserWidget* InWidget)
    -> bool
{
    if (ck::Is_NOT_Valid(InWidget))
    { return false; }

    const auto* LocalPlayer = InWidget->GetOwningLocalPlayer();

    if (ck::Is_NOT_Valid(LocalPlayer))
    { return false; }

    auto* Subsystem = LocalPlayer->GetSubsystem<UCk_UI_Extension_Subsystem_UE>();

    if (ck::Is_NOT_Valid(Subsystem))
    { return false; }

    return Subsystem->TryUnregisterExtensionByWidget(InWidget);
}

auto
    UCk_Utils_UI_Extension_UE::
    IsValid_Extension(
        const FCk_UI_ExtensionHandle& InHandle)
    -> bool
{
    return ck::IsValid(InHandle);
}

auto
    UCk_Utils_UI_Extension_UE::
    HasExtensionsAtPoint(
        const APlayerController* InPlayerController,
        FGameplayTag InExtensionPointTag)
    -> bool
{
    auto* Subsystem = Get_ExtensionSubsystem(InPlayerController);

    if (ck::Is_NOT_Valid(Subsystem))
    { return false; }

    return Subsystem->HasExtensionsAtPoint(InExtensionPointTag);
}

auto
    UCk_Utils_UI_Extension_UE::
    ClearExtensionsAtPoint(
        const APlayerController* InPlayerController,
        FGameplayTag InExtensionPointTag)
    -> void
{
    auto* Subsystem = Get_ExtensionSubsystem(InPlayerController);

    if (ck::Is_NOT_Valid(Subsystem))
    { return; }

    Subsystem->ClearExtensionsAtPoint(InExtensionPointTag);
}

auto
    UCk_Utils_UI_Extension_UE::
    UnregisterExtensionPoint(
        FCk_UI_ExtensionPointHandle& InHandle)
    -> void
{
    InHandle.Unregister();
}

auto
    UCk_Utils_UI_Extension_UE::
    IsValid_ExtensionPoint(
        const FCk_UI_ExtensionPointHandle& InHandle)
    -> bool
{
    return ck::IsValid(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------