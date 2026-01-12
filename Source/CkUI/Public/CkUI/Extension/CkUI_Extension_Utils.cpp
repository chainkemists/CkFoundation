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
    UnregisterExtension(
        FCk_UI_ExtensionHandle& InHandle)
    -> void
{
    InHandle.Unregister();
}

auto
    UCk_Utils_UI_Extension_UE::
    IsValid_Extension(
        FCk_UI_ExtensionHandle& InHandle)
    -> bool
{
    return InHandle.IsValid();
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
        FCk_UI_ExtensionPointHandle& InHandle)
    -> bool
{
    return InHandle.IsValid();
}

// --------------------------------------------------------------------------------------------------------------------