#include "CkKeyBinding_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkInput/Subsystem/CkKeyBinding_Subsystem.h"

#include <EnhancedInputSubsystems.h>
#include <InputAction.h>
#include <PlayerMappableKeySettings.h>
#include <UserSettings/EnhancedInputUserSettings.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_input_utils
{
    auto Get_EISubsystem(const APlayerController* InPlayerController) -> UEnhancedInputLocalPlayerSubsystem*;
}

namespace
{
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

    auto* Subsystem = ck_input_utils::Get_EISubsystem(InPlayerController);
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
    if (ck::Is_NOT_Valid(Profile))
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

auto
    UCk_Utils_KeyBinding_UE::
    Get_KeyForMapping(
        APlayerController* InPlayerController,
        FName InMappingName,
        EPlayerMappableKeySlot InSlot)
    -> FKey
{
    CK_ENSURE_IF_NOT(ck::IsValid(InPlayerController), TEXT("Invalid Player Controller"))
    { return {}; }

    auto* Settings = Get_InputUserSettings(InPlayerController);
    if (ck::Is_NOT_Valid(Settings))
    { return {}; }

    auto* Profile = Get_CurrentProfile(Settings);
    if (ck::Is_NOT_Valid(Profile, ck::IsValid_Policy_NullptrOnly{}))
    { return {}; }

    auto FindArgs = FMapPlayerKeyArgs{};
    FindArgs.MappingName = InMappingName;
    FindArgs.Slot = InSlot;

    if (const auto* Mapping = Profile->FindKeyMapping(FindArgs);
        ck::IsValid(Mapping, ck::IsValid_Policy_NullptrOnly{}))
    {
        return Mapping->GetCurrentKey();
    }

    return FKey{EKeys::Invalid};
}

auto
    UCk_Utils_KeyBinding_UE::
    Get_KeyForInputAction(
        APlayerController* InPlayerController,
        const UInputAction* InInputAction,
        EPlayerMappableKeySlot InSlot)
    -> FKey
{
    const auto MappingName = Get_MappingNameFromInputAction(InInputAction);
    if (MappingName.IsNone())
    { return FKey{EKeys::Invalid}; }

    return Get_KeyForMapping(InPlayerController, MappingName, InSlot);
}

auto
    UCk_Utils_KeyBinding_UE::
    Get_MappingNamesForKey(
        APlayerController* InPlayerController,
        FKey InKey)
    -> TArray<FName>
{
    CK_ENSURE_IF_NOT(ck::IsValid(InPlayerController), TEXT("Invalid Player Controller"))
    { return {}; }

    auto* Settings = Get_InputUserSettings(InPlayerController);
    if (ck::Is_NOT_Valid(Settings))
    { return {}; }

    auto* Profile = Get_CurrentProfile(Settings);
    if (ck::Is_NOT_Valid(Profile, ck::IsValid_Policy_NullptrOnly{}))
    { return {}; }

    auto Result = TArray<FName>{};
    Profile->GetMappingNamesForKey(InKey, Result);
    return Result;
}

auto
    UCk_Utils_KeyBinding_UE::
    Get_MappingNameFromInputAction(
        const UInputAction* InInputAction)
    -> FName
{
    CK_ENSURE_IF_NOT(ck::IsValid(InInputAction), TEXT("Invalid Input Action"))
    { return {}; }

    const auto& Settings = InInputAction->GetPlayerMappableKeySettings();
    if (ck::Is_NOT_Valid(Settings))
    { return {}; }

    return Settings->GetMappingName();
}

auto
    UCk_Utils_KeyBinding_UE::
    Get_MappableKeyInfoFromInputAction(
        const UInputAction* InInputAction,
        FCk_KeyBinding_MappableKeyInfo& OutInfo)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InInputAction), TEXT("Invalid Input Action"))
    { return false; }

    const auto& Settings = InInputAction->GetPlayerMappableKeySettings();
    if (ck::Is_NOT_Valid(Settings))
    { return false; }

    OutInfo = FCk_KeyBinding_MappableKeyInfo{Settings->GetMappingName()}
    .Set_DisplayName(Settings->DisplayName)
    .Set_DisplayCategory(Settings->DisplayCategory)
    .Set_Metadata(Settings->Metadata);

    return true;
}

auto
    UCk_Utils_KeyBinding_UE::
    Get_DidMappingKeyChange(
        APlayerController* InPlayerController,
        FName InMappingName,
        EPlayerMappableKeySlot InSlot,
        FKey InCachedKey,
        FKey& OutCurrentKey)
    -> bool
{
    OutCurrentKey = Get_KeyForMapping(InPlayerController, InMappingName, InSlot);
    return OutCurrentKey != InCachedKey;
}

auto
    UCk_Utils_KeyBinding_UE::
    BindTo_OnMappingKeyChanged(
        APlayerController* InPlayerController,
        FName MappingName,
        EPlayerMappableKeySlot Slot,
        FCk_OnMappingKeyChanged OnChanged)
    -> FCk_Handle_KeybindListener
{
    CK_ENSURE_IF_NOT(ck::IsValid(InPlayerController), TEXT("Invalid Player Controller"))
    { return {}; }

    const auto* LocalPlayer = InPlayerController->GetLocalPlayer();
    CK_ENSURE_IF_NOT(ck::IsValid(LocalPlayer), TEXT("Invalid Local Player"))
    { return {}; }

    auto* Subsystem = LocalPlayer->GetSubsystem<UCk_KeyBinding_Subsystem>();
    CK_ENSURE_IF_NOT(ck::IsValid(Subsystem), TEXT("KeyBinding Subsystem not found"))
    { return {}; }

    auto Handle = FCk_Handle_KeybindListener{MappingName, Slot, OnChanged};
    Subsystem->BindTo_MappingKeyChanged(Handle);
    return Handle;
}

auto
    UCk_Utils_KeyBinding_UE::
    UnbindFrom_OnMappingKeyChanged(
        APlayerController* InPlayerController,
        FCk_Handle_KeybindListener& InHandle)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InPlayerController), TEXT("Invalid Player Controller"))
    { return; }

    const auto* LocalPlayer = InPlayerController->GetLocalPlayer();
    if (ck::Is_NOT_Valid(LocalPlayer))
    { return; }

    auto* Subsystem = LocalPlayer->GetSubsystem<UCk_KeyBinding_Subsystem>();
    if (ck::Is_NOT_Valid(Subsystem))
    { return; }

    Subsystem->UnbindFrom_MappingKeyChanged(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_KeyBinding_UE::
    RemapKey(
        APlayerController* InPlayerController,
        FName InMappingName,
        EPlayerMappableKeySlot InSlot,
        FKey InNewKey,
        FGameplayTagContainer& OutFailureReason)
    -> bool
{
    OutFailureReason.Reset();

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

    Settings->MapPlayerKey(Args, OutFailureReason);
    return OutFailureReason.IsEmpty();
}

auto
    UCk_Utils_KeyBinding_UE::
    RemapKeys(
        APlayerController* InPlayerController,
        const TArray<FName>& InMappingNames,
        EPlayerMappableKeySlot InSlot,
        FKey InNewKey,
        FGameplayTagContainer& OutFailureReason)
    -> bool
{
    OutFailureReason.Reset();

    CK_ENSURE_IF_NOT(ck::IsValid(InPlayerController), TEXT("Invalid Player Controller"))
    { return {}; }

    auto* Settings = Get_InputUserSettings(InPlayerController);
    CK_ENSURE_IF_NOT(ck::IsValid(Settings), TEXT("Enhanced Input User Settings not found"))
    { return {}; }

    const auto LastIndex = InMappingNames.Num() - 1;

    for (auto i = 0; i < InMappingNames.Num(); ++i)
    {
        auto Args = FMapPlayerKeyArgs{};
        Args.MappingName = InMappingNames[i];
        Args.Slot = InSlot;
        Args.NewKey = InNewKey;
        Args.bCreateMatchingSlotIfNeeded = true;
        Args.bDeferOnSettingsChangedBroadcast = (i != LastIndex);

        auto FailureReason = FGameplayTagContainer{};
        Settings->MapPlayerKey(Args, FailureReason);
        OutFailureReason.AppendTags(FailureReason);
    }

    return OutFailureReason.IsEmpty();
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
    if (ck::Is_NOT_Valid(Profile))
    { return; }

    Profile->ResetToDefault();

    // ResetToDefault operates on the profile directly, bypassing the
    // UEnhancedInputUserSettings layer, so OnSettingsChanged is never
    // broadcast.  Fire it manually so the keybind watcher subsystem
    // (and any other listeners) pick up the changes.
    Settings->OnSettingsChanged.Broadcast(Settings);
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
        const TArray<FName>& InExcludeMappingNames,
        TArray<FCk_KeyBinding_ConflictInfo>& OutConflicts,
        ECk_KeyConflictScope InScope)
    -> bool
{
    OutConflicts.Empty();

    CK_ENSURE_IF_NOT(ck::IsValid(InPlayerController), TEXT("Invalid Player Controller"))
    { return {}; }

    auto* Settings = Get_InputUserSettings(InPlayerController);
    if (ck::Is_NOT_Valid(Settings))
    { return {}; }

    auto* Profile = Get_CurrentProfile(Settings);
    if (ck::Is_NOT_Valid(Profile, ck::IsValid_Policy_NullptrOnly{}))
    { return {}; }

    // Resolve the source category from the first excluded mapping when filtering by SameCategory
    auto SourceCategory = FText::GetEmpty();
    if (InScope == ECk_KeyConflictScope::SameCategory && NOT InExcludeMappingNames.IsEmpty())
    {
        if (const auto* SourceRow = Profile->FindKeyMappingRow(InExcludeMappingNames[0]);
            ck::IsValid(SourceRow, ck::IsValid_Policy_NullptrOnly{}))
        {
            for (const auto& Mapping : SourceRow->Mappings)
            {
                SourceCategory = Mapping.GetDisplayCategory();
                break;
            }
        }
    }

    auto ConflictingMappingNames = TArray<FName>{};
    Profile->GetMappingNamesForKey(InNewKey, ConflictingMappingNames);

    for (const auto& MappingName : ConflictingMappingNames)
    {
        if (InExcludeMappingNames.Contains(MappingName))
        { continue; }

        const auto* Row = Profile->FindKeyMappingRow(MappingName);
        if (ck::Is_NOT_Valid(Row, ck::IsValid_Policy_NullptrOnly{}))
        { continue; }

        for (const auto& Mapping : Row->Mappings)
        {
            if (Mapping.GetCurrentKey() != InNewKey)
            { continue; }

            if (InScope == ECk_KeyConflictScope::SameCategory
                && NOT Mapping.GetDisplayCategory().EqualTo(SourceCategory))
            { break; }

            OutConflicts.Emplace(FCk_KeyBinding_ConflictInfo{MappingName, Mapping.GetDisplayName(), Mapping.GetDisplayCategory(), Mapping.GetCurrentKey(), Mapping.GetSlot()});
            break;
        }
    }

    return NOT OutConflicts.IsEmpty();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_KeyBinding_UE::
    SwapKeys(
        APlayerController* InPlayerController,
        FName InMappingName,
        EPlayerMappableKeySlot InSlot,
        FKey InNewKey,
        FGameplayTagContainer& OutFailureReason)
    -> bool
{
    OutFailureReason.Reset();

    CK_ENSURE_IF_NOT(ck::IsValid(InPlayerController), TEXT("Invalid Player Controller"))
    { return {}; }

    auto* Settings = Get_InputUserSettings(InPlayerController);
    CK_ENSURE_IF_NOT(ck::IsValid(Settings), TEXT("Enhanced Input User Settings not found"))
    { return {}; }

    auto* Profile = Get_CurrentProfile(Settings);
    CK_ENSURE_IF_NOT(ck::IsValid(Profile), TEXT("No active key profile found"))
    { return {}; }

    // Find the current key of the mapping being rebound (becomes the swap target)
    auto OldKey = FKey{EKeys::Invalid};
    {
        auto FindArgs = FMapPlayerKeyArgs{};
        FindArgs.MappingName = InMappingName;
        FindArgs.Slot = InSlot;

        if (const auto* CurrentMapping = Profile->FindKeyMapping(FindArgs);
            ck::IsValid(CurrentMapping, ck::IsValid_Policy_NullptrOnly{}))
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
                OutFailureReason.AppendTags(FailureReason);
                break;
            }
        }
    }

    // Now assign InNewKey to the original mapping
    {
        auto Args = FMapPlayerKeyArgs{};
        Args.MappingName = InMappingName;
        Args.Slot = InSlot;
        Args.NewKey = InNewKey;
        Args.bCreateMatchingSlotIfNeeded = true;
        Args.bDeferOnSettingsChangedBroadcast = false;

        auto FailureReason = FGameplayTagContainer{};
        Settings->MapPlayerKey(Args, FailureReason);
        OutFailureReason.AppendTags(FailureReason);
    }

    return OutFailureReason.IsEmpty();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_KeyBinding_UE::
    UnbindConflictAndRemap(
        APlayerController* InPlayerController,
        FName InMappingName,
        EPlayerMappableKeySlot InSlot,
        FKey InNewKey,
        FGameplayTagContainer& OutFailureReason)
    -> bool
{
    OutFailureReason.Reset();

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
                OutFailureReason.AppendTags(FailureReason);
                break;
            }
        }
    }

    // Now assign InNewKey to the target mapping
    {
        auto Args = FMapPlayerKeyArgs{};
        Args.MappingName = InMappingName;
        Args.Slot = InSlot;
        Args.NewKey = InNewKey;
        Args.bCreateMatchingSlotIfNeeded = true;
        Args.bDeferOnSettingsChangedBroadcast = false;

        auto FailureReason = FGameplayTagContainer{};
        Settings->MapPlayerKey(Args, FailureReason);
        OutFailureReason.AppendTags(FailureReason);
    }

    return OutFailureReason.IsEmpty();
}

// --------------------------------------------------------------------------------------------------------------------
