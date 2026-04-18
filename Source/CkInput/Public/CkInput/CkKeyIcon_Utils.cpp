// Copyright 2025 CkFoundation. All Rights Reserved.

#include "CkKeyIcon_Utils.h"

#include "CkInput/CkInput_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat.h"

#include <CommonInputBaseTypes.h>
#include <CommonInputSettings.h>
#include <CommonInputSubsystem.h>
#include <CommonUITypes.h>
#include <GameFramework/PlayerController.h>
#include <InputAction.h>

#if WITH_ANGELSCRIPT_CK
#include <AngelscriptBinds.h>
#include <AngelscriptManager.h>
#endif

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_KeyIcon_UE::
    Get_BrushForKey(
        const APlayerController* InPlayerController,
        FKey InKey)
    -> FSlateBrush
{
    auto* Subsystem = UCk_Utils_Input_UE::Get_CommonInputSubsystem(InPlayerController);
    if (ck::Is_NOT_Valid(Subsystem))
    { return FSlateBrush{}; }

    FSlateBrush Brush;
    const auto Found = UCommonInputPlatformSettings::Get()->TryGetInputBrush(
        Brush,
        InKey,
        Subsystem->GetCurrentInputType(),
        Subsystem->GetCurrentGamepadName());

    return Found ? Brush : FSlateBrush{};
}

auto
    UCk_Utils_KeyIcon_UE::
    Get_BrushForInputAction(
        const APlayerController* InPlayerController,
        const UInputAction* InInputAction)
    -> FSlateBrush
{
    auto* Subsystem = UCk_Utils_Input_UE::Get_CommonInputSubsystem(InPlayerController);
    if (ck::Is_NOT_Valid(Subsystem) || ck::Is_NOT_Valid(InInputAction))
    { return FSlateBrush{}; }

    return CommonUI::GetIconForEnhancedInputAction(Subsystem, InInputAction);
}

auto
    UCk_Utils_KeyIcon_UE::
    Get_ActiveControllerData(
        const APlayerController* InPlayerController)
    -> const UCommonInputBaseControllerData*
{
    auto* Subsystem = UCk_Utils_Input_UE::Get_CommonInputSubsystem(InPlayerController);
    if (ck::Is_NOT_Valid(Subsystem))
    { return nullptr; }

    const auto* PlatformSettings = UCommonInputPlatformSettings::Get();
    CK_ENSURE_IF_NOT(ck::IsValid(PlatformSettings),
        TEXT("UCommonInputPlatformSettings CDO not available"))
    { return nullptr; }

    const auto InputType   = Subsystem->GetCurrentInputType();
    const auto GamepadName = Subsystem->GetCurrentGamepadName();

    for (const auto& SoftClass : PlatformSettings->GetControllerData())
    {
        const auto* LoadedClass = SoftClass.LoadSynchronous();
        if (ck::Is_NOT_Valid(LoadedClass))
        { continue; }

        const auto* Data = LoadedClass->GetDefaultObject<UCommonInputBaseControllerData>();
        if (ck::Is_NOT_Valid(Data))
        { continue; }

        if (Data->InputType != InputType)
        { continue; }

        if (InputType == ECommonInputType::Gamepad && Data->GamepadName != GamepadName)
        { continue; }

        return Data;
    }

    const auto InputTypeName = StaticEnum<ECommonInputType>()->GetNameStringByValue(static_cast<int64>(InputType));

    CK_ENSURE_IF_NOT(false,
        TEXT("No UCommonInputBaseControllerData registered for InputType=[{}] GamepadName=[{}]. "
             "Verify Project Settings -> CommonInput -> Platform Input has a controller data asset "
             "matching this device."),
        InputTypeName, GamepadName)
    { return nullptr; }

    return nullptr;
}

// --------------------------------------------------------------------------------------------------------------------
// Angelscript bindings — expose makers for CommonUI structs whose fields are
// UPROPERTY(EditAnywhere) without BlueprintRead*, which makes them unreachable
// from AngelScript via default property access.
// --------------------------------------------------------------------------------------------------------------------

#if WITH_ANGELSCRIPT_CK
AS_FORCE_LINK const FAngelscriptBinds::FBind Bind_CkKeyIcon_CommonInputMakers(
    FAngelscriptBinds::EOrder::Late, []
{
    FAngelscriptBinds::BindGlobalFunction(
        "FCommonInputKeyBrushConfiguration Make_CommonInputKeyBrushConfiguration(FKey InKey, const FSlateBrush& InKeyBrush)",
        [](FKey InKey, const FSlateBrush& InKeyBrush) -> FCommonInputKeyBrushConfiguration
        {
            auto Config = FCommonInputKeyBrushConfiguration{};
            Config.Key = InKey;
            Config.KeyBrush = InKeyBrush;
            return Config;
        });

    FAngelscriptBinds::BindGlobalFunction(
        "FCommonInputKeySetBrushConfiguration Make_CommonInputKeySetBrushConfiguration(const TArray<FKey>& InKeys, const FSlateBrush& InKeyBrush)",
        [](const TArray<FKey>& InKeys, const FSlateBrush& InKeyBrush) -> FCommonInputKeySetBrushConfiguration
        {
            auto Config = FCommonInputKeySetBrushConfiguration{};
            Config.Keys = InKeys;
            Config.KeyBrush = InKeyBrush;
            return Config;
        });
});
#endif

// --------------------------------------------------------------------------------------------------------------------
