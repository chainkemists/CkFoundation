// Copyright 2025 CkFoundation. All Rights Reserved.

#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <CommonInputTypeEnum.h>
#include <InputCoreTypes.h>
#include <Kismet/BlueprintFunctionLibrary.h>
#include <Styling/SlateBrush.h>

#include "CkKeyIcon_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class APlayerController;
class UCommonInputBaseControllerData;
class UInputAction;

// --------------------------------------------------------------------------------------------------------------------

/**
 * Resolves a Slate brush for an FKey / UInputAction using the controller data
 * registered in Project Settings (UCommonInputSettings::PlatformInput).
 *
 * These are thin wrappers around CommonUI's own helpers (GetIconForEnhancedInputAction,
 * UCommonInputPlatformSettings::TryGetInputBrush) exposed for Blueprint/AngelScript
 * callers and framed with the Ck naming conventions.
 *
 * For a ready-to-use UMG widget that also auto-refreshes on device / remap events,
 * prefer UCk_InputActionWidget_UE (CkUI) — it subclasses UCommonActionWidget.
 */
UCLASS(NotBlueprintable)
class CKINPUT_API UCk_Utils_KeyIcon_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_KeyIcon_UE);

public:
    /**
     * Resolve the brush for an FKey using the current input device state.
     * @return  Brush from the matching controller data asset; empty brush on miss.
     */
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Input|KeyIcon",
        DisplayName = "[Ck][KeyIcon] Get Brush For Key")
    static FSlateBrush
    Get_BrushForKey(
        const APlayerController* InPlayerController,
        FKey InKey);

    /**
     * Resolve the brush for an Enhanced Input UInputAction, respecting remaps
     * and the currently active input device.
     */
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Input|KeyIcon",
        DisplayName = "[Ck][KeyIcon] Get Brush For InputAction")
    static FSlateBrush
    Get_BrushForInputAction(
        const APlayerController* InPlayerController,
        const UInputAction* InInputAction);

    /**
     * Returns the UCommonInputBaseControllerData registered in project settings
     * whose InputType (and GamepadName, if Gamepad) matches the current subsystem
     * state. Returns nullptr on mismatch.
     */
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Input|KeyIcon",
        DisplayName = "[Ck][KeyIcon] Get Active Controller Data")
    static const UCommonInputBaseControllerData*
    Get_ActiveControllerData(
        const APlayerController* InPlayerController);
};

// --------------------------------------------------------------------------------------------------------------------
