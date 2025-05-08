#include "CkInput_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Input_UE::
    WasInputChordJustPressed(
        APlayerController* InPlayerController,
        const FInputChord& InInputChord)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InPlayerController), TEXT("Invalid Player Controller"))
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InInputChord), TEXT("Invalid Input Chord"))
    { return {}; }

    if (InInputChord.NeedsAlt() && NOT InPlayerController->IsInputKeyDown(EKeys::LeftAlt) && NOT InPlayerController->IsInputKeyDown(EKeys::RightAlt))
    { return {}; }

    if (InInputChord.NeedsShift() && NOT InPlayerController->IsInputKeyDown(EKeys::LeftShift) && NOT InPlayerController->IsInputKeyDown(EKeys::RightShift))
    { return {}; }

    if (InInputChord.NeedsCommand() && NOT InPlayerController->IsInputKeyDown(EKeys::LeftCommand) && NOT InPlayerController->IsInputKeyDown(EKeys::RightCommand))
    { return {}; }

    if (InInputChord.NeedsControl() && NOT InPlayerController->IsInputKeyDown(EKeys::LeftControl) && NOT InPlayerController->IsInputKeyDown(EKeys::RightControl))
    { return {}; }

    return InPlayerController->WasInputKeyJustPressed(InInputChord.Key);
}

auto
    UCk_Utils_Input_UE::
    WasInputKeyJustPressed(
        APlayerController* InPlayerController,
        const FKey&        InInputKey)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InPlayerController), TEXT("Invalid Player Controller"))
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InInputKey), TEXT("Invalid Input Key"))
    { return {}; }

    return InPlayerController->WasInputKeyJustPressed(InInputKey);
}

auto
    UCk_Utils_Input_UE::
    WasInputKeyJustPressed_WithCustomModifier(
        APlayerController* InPlayerController,
        const FKey& InInputKey,
        const FKey& InCustomModiferKey)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InPlayerController), TEXT("Invalid Player Controller"))
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InInputKey), TEXT("Invalid Input Key"))
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InCustomModiferKey), TEXT("Invalid Custom Modifier Key"))
    { return {}; }

    CK_ENSURE_IF_NOT(InInputKey != InCustomModiferKey, TEXT("Input Key and Custom Modifier Key cannot be the same"))
    { return {}; }

    return InPlayerController->WasInputKeyJustPressed(InInputKey) && InPlayerController->IsInputKeyDown(InCustomModiferKey);
}

// --------------------------------------------------------------------------------------------------------------------

