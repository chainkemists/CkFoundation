#include "CkKeyBinding_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"

#include <EnhancedInputSubsystems.h>
#include <UserSettings/EnhancedInputUserSettings.h>

// --------------------------------------------------------------------------------------------------------------------

namespace
{
    auto Get_EISubsystem(const APlayerController* InPlayerController) -> UEnhancedInputLocalPlayerSubsystem*
    {
        if (ck::Is_NOT_Valid(InPlayerController))
        { return nullptr; }

        const auto* LocalPlayer = InPlayerController->GetLocalPlayer();
        if (ck::Is_NOT_Valid(LocalPlayer))
        { return nullptr; }

        return LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
    }

    auto Get_CurrentProfile(UEnhancedInputUserSettings* InSettings) -> UEnhancedPlayerMappableKeyProfile*
    {
        if (ck::Is_NOT_Valid(InSettings))
        { return nullptr; }

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION <= 5
        return InSettings->GetCurrentKeyProfile();
#else
        return InSettings->GetActiveKeyProfile();
#endif
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_KeyBinding_UE::
    Get_InputUserSettings(
        APlayerController* InPlayerController)
    -> UEnhancedInputUserSettings*
{
    CK_ENSURE_IF_NOT(ck::IsValid(InPlayerController), TEXT("Invalid Player Controller"))
    { return {}; }

    auto* Subsystem = Get_EISubsystem(InPlayerController);
    CK_ENSURE_IF_NOT(ck::IsValid(Subsystem), TEXT("Enhanced Input Local Player Subsystem not found"))
    { return {}; }

    return Subsystem->GetUserSettings();
}

auto
    UCk_Utils_KeyBinding_UE::
    Get_AllRemappableKeys(
        APlayerController* InPlayerController)
    -> TArray<FPlayerKeyMapping>
{
    CK_ENSURE_IF_NOT(ck::IsValid(InPlayerController), TEXT("Invalid Player Controller"))
    { return {}; }

    auto* Settings = Get_InputUserSettings(InPlayerController);
    if (ck::Is_NOT_Valid(Settings))
    { return {}; }

    auto* Profile = Get_CurrentProfile(Settings);
    if (ck::Is_NOT_Valid(Profile, ck::IsValid_Policy_NullptrOnly{}))
    { return {}; }

    auto Result = TArray<FPlayerKeyMapping>{};
    for (const auto& MappingRows = Profile->GetPlayerMappingRows();
        const auto& [MappingName, Row] : MappingRows)
    {
        for (const auto& Mapping : Row.Mappings)
        {
            Result.Add(Mapping);
        }
    }

    return Result;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_KeyBinding_UE::
    RemapKey(
        APlayerController* InPlayerController,
        FName InMappingName,
        EPlayerMappableKeySlot InSlot,
        FKey InNewKey)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InPlayerController), TEXT("Invalid Player Controller"))
    { return {}; }

    auto* Settings = Get_InputUserSettings(InPlayerController);
    CK_ENSURE_IF_NOT(ck::IsValid(Settings), TEXT("Enhanced Input User Settings not found"))
    { return {}; }

    auto Args = FMapPlayerKeyArgs{};
    Args.MappingName = InMappingName;
    Args.Slot = InSlot;
    Args.NewKey = InNewKey;
    Args.bCreateMatchingSlotIfNeeded = true;
    Args.bDeferOnSettingsChangedBroadcast = false;

    auto FailureReason = FGameplayTagContainer{};
    Settings->MapPlayerKey(Args, FailureReason);

    return FailureReason.IsEmpty();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_KeyBinding_UE::
    ResetMappingToDefault(
        APlayerController* InPlayerController,
        FName InMappingName)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InPlayerController), TEXT("Invalid Player Controller"))
    { return; }

    auto* Settings = Get_InputUserSettings(InPlayerController);
    if (ck::Is_NOT_Valid(Settings))
    { return; }

    auto Args = FMapPlayerKeyArgs{};
    Args.MappingName = InMappingName;
    Args.bDeferOnSettingsChangedBroadcast = false;

    auto FailureReason = FGameplayTagContainer{};
    Settings->ResetAllPlayerKeysInRow(Args, FailureReason);
}

auto
    UCk_Utils_KeyBinding_UE::
    ResetAllToDefaults(
        APlayerController* InPlayerController)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InPlayerController), TEXT("Invalid Player Controller"))
    { return; }

    auto* Settings = Get_InputUserSettings(InPlayerController);
    if (ck::Is_NOT_Valid(Settings))
    { return; }

    auto* Profile = Get_CurrentProfile(Settings);
    if (ck::Is_NOT_Valid(Profile, ck::IsValid_Policy_NullptrOnly{}))
    { return; }

    Profile->ResetToDefault();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_KeyBinding_UE::
    SaveKeyBindings(
        APlayerController* InPlayerController)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InPlayerController), TEXT("Invalid Player Controller"))
    { return; }

    auto* Settings = Get_InputUserSettings(InPlayerController);
    if (ck::Is_NOT_Valid(Settings))
    { return; }

    Settings->AsyncSaveSettings();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_KeyBinding_UE::
    Get_HasKeyConflicts(
        APlayerController* InPlayerController,
        FKey InNewKey,
        FName InExcludeMappingName,
        TArray<FText>& OutConflictingNames)
    -> bool
{
    OutConflictingNames.Empty();

    CK_ENSURE_IF_NOT(ck::IsValid(InPlayerController), TEXT("Invalid Player Controller"))
    { return {}; }

    auto* Settings = Get_InputUserSettings(InPlayerController);
    if (ck::Is_NOT_Valid(Settings))
    { return {}; }

    auto* Profile = Get_CurrentProfile(Settings);
    if (ck::Is_NOT_Valid(Profile, ck::IsValid_Policy_NullptrOnly{}))
    { return {}; }

    auto ConflictingMappingNames = TArray<FName>{};
    Profile->GetMappingNamesForKey(InNewKey, ConflictingMappingNames);

    for (const auto& MappingName : ConflictingMappingNames)
    {
        if (MappingName == InExcludeMappingName)
        { continue; }

        const auto* Row = Profile->FindKeyMappingRow(MappingName);
        if (ck::Is_NOT_Valid(Row, ck::IsValid_Policy_NullptrOnly{}))
        { continue; }

        for (const auto& Mapping : Row->Mappings)
        {
            if (Mapping.GetCurrentKey() == InNewKey)
            {
                OutConflictingNames.Add(Mapping.GetDisplayName());
                break;
            }
        }
    }

    return NOT OutConflictingNames.IsEmpty();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_KeyBinding_UE::
    SwapKeys(
        APlayerController* InPlayerController,
        FName InMappingName,
        EPlayerMappableKeySlot InSlot,
        FKey InNewKey)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InPlayerController), TEXT("Invalid Player Controller"))
    { return {}; }

    auto* Settings = Get_InputUserSettings(InPlayerController);
    CK_ENSURE_IF_NOT(ck::IsValid(Settings), TEXT("Enhanced Input User Settings not found"))
    { return {}; }

    auto* Profile = Get_CurrentProfile(Settings);
    CK_ENSURE_IF_NOT(ck::IsValid(Profile, ck::IsValid_Policy_NullptrOnly{}), TEXT("No active key profile found"))
    { return {}; }

    // Find the current key of the mapping being rebound (becomes the swap target)
    auto OldKey = FKey{EKeys::Invalid};
    {
        auto FindArgs = FMapPlayerKeyArgs{};
        FindArgs.MappingName = InMappingName;
        FindArgs.Slot = InSlot;

        const auto* CurrentMapping = Profile->FindKeyMapping(FindArgs);
        if (ck::IsValid(CurrentMapping, ck::IsValid_Policy_NullptrOnly{}))
        {
            OldKey = CurrentMapping->GetCurrentKey();
        }
    }

    // Find which mapping currently holds InNewKey and assign OldKey to it (the swap)
    auto ConflictingMappingNames = TArray<FName>{};
    Profile->GetMappingNamesForKey(InNewKey, ConflictingMappingNames);

    for (const auto& ConflictName : ConflictingMappingNames)
    {
        if (ConflictName == InMappingName)
        { continue; }

        const auto* Row = Profile->FindKeyMappingRow(ConflictName);
        if (ck::Is_NOT_Valid(Row, ck::IsValid_Policy_NullptrOnly{}))
        { continue; }

        for (const auto& Mapping : Row->Mappings)
        {
            if (Mapping.GetCurrentKey() == InNewKey)
            {
                auto SwapArgs = FMapPlayerKeyArgs{};
                SwapArgs.MappingName = ConflictName;
                SwapArgs.Slot = Mapping.GetSlot();
                SwapArgs.NewKey = OldKey;
                SwapArgs.bCreateMatchingSlotIfNeeded = false;
                SwapArgs.bDeferOnSettingsChangedBroadcast = true;

                auto FailureReason = FGameplayTagContainer{};
                Settings->MapPlayerKey(SwapArgs, FailureReason);
                break;
            }
        }
    }

    // Now assign InNewKey to the original mapping
    return RemapKey(InPlayerController, InMappingName, InSlot, InNewKey);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_KeyBinding_UE::
    UnbindConflictAndRemap(
        APlayerController* InPlayerController,
        FName InMappingName,
        EPlayerMappableKeySlot InSlot,
        FKey InNewKey)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InPlayerController), TEXT("Invalid Player Controller"))
    { return {}; }

    auto* Settings = Get_InputUserSettings(InPlayerController);
    CK_ENSURE_IF_NOT(ck::IsValid(Settings), TEXT("Enhanced Input User Settings not found"))
    { return {}; }

    auto* Profile = Get_CurrentProfile(Settings);
    CK_ENSURE_IF_NOT(ck::IsValid(Profile, ck::IsValid_Policy_NullptrOnly{}), TEXT("No active key profile found"))
    { return {}; }

    // Find and unbind all conflicting mappings that use InNewKey
    auto ConflictingMappingNames = TArray<FName>{};
    Profile->GetMappingNamesForKey(InNewKey, ConflictingMappingNames);

    for (const auto& ConflictName : ConflictingMappingNames)
    {
        if (ConflictName == InMappingName)
        { continue; }

        const auto* Row = Profile->FindKeyMappingRow(ConflictName);
        if (ck::Is_NOT_Valid(Row, ck::IsValid_Policy_NullptrOnly{}))
        { continue; }

        for (const auto& Mapping : Row->Mappings)
        {
            if (Mapping.GetCurrentKey() == InNewKey)
            {
                auto UnbindArgs = FMapPlayerKeyArgs{};
                UnbindArgs.MappingName = ConflictName;
                UnbindArgs.Slot = Mapping.GetSlot();
                UnbindArgs.bDeferOnSettingsChangedBroadcast = true;

                auto FailureReason = FGameplayTagContainer{};
                Settings->UnMapPlayerKey(UnbindArgs, FailureReason);
                break;
            }
        }
    }

    // Now assign InNewKey to the target mapping
    return RemapKey(InPlayerController, InMappingName, InSlot, InNewKey);
}

// --------------------------------------------------------------------------------------------------------------------
