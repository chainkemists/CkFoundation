#include "CkInput_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat.h"

#include <EnhancedInputSubsystems.h>
#include <InputMappingContext.h>

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
        const FKey& InInputKey)
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

namespace ck_input_utils
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
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Input_UE::
    AddMappingContexts(
        APlayerController* InPlayerController,
        const TArray<FCk_MappingContextWithPriority>& InContexts,
        bool InClearPrevious)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InPlayerController), TEXT("Invalid Player Controller"))
    { return {}; }

    auto* Subsystem = ck_input_utils::Get_EISubsystem(InPlayerController);
    CK_ENSURE_IF_NOT(ck::IsValid(Subsystem), TEXT("Enhanced Input Local Player Subsystem not found"))
    { return {}; }

    if (InClearPrevious)
    {
        Subsystem->ClearAllMappings();
    }

    for (const auto& Entry : InContexts)
    {
        if (auto* Context = Entry.Get_MappingContext().LoadSynchronous();
            ck::IsValid(Context))
        {
            Subsystem->AddMappingContext(Context, Entry.Get_Priority());
        }
    }

    return true;
}

auto
    UCk_Utils_Input_UE::
    RemoveMappingContexts(
        APlayerController* InPlayerController,
        const TArray<TSoftObjectPtr<UInputMappingContext>>& InContexts)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InPlayerController), TEXT("Invalid Player Controller"))
    { return {}; }

    auto* Subsystem = ck_input_utils::Get_EISubsystem(InPlayerController);
    CK_ENSURE_IF_NOT(ck::IsValid(Subsystem), TEXT("Enhanced Input Local Player Subsystem not found"))
    { return {}; }

    for (const auto& SoftContext : InContexts)
    {
        if (auto* Context = SoftContext.Get();
            ck::IsValid(Context))
        {
            Subsystem->RemoveMappingContext(Context);
        }
    }

    return true;
}

auto
    UCk_Utils_Input_UE::
    SwapMappingContexts(
        APlayerController* InPlayerController,
        TSoftObjectPtr<UInputMappingContext> InPreviousContext,
        TSoftObjectPtr<UInputMappingContext> InNewContext,
        int32 InPriority,
        bool InUsePreviousPriority)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InPlayerController), TEXT("Invalid Player Controller"))
    { return {}; }

    auto* NewContext = InNewContext.LoadSynchronous();
    CK_ENSURE_IF_NOT(ck::IsValid(NewContext), TEXT("Invalid New Context"))
    { return {}; }

    auto* Subsystem = ck_input_utils::Get_EISubsystem(InPlayerController);
    CK_ENSURE_IF_NOT(ck::IsValid(Subsystem), TEXT("Enhanced Input Local Player Subsystem not found"))
    { return {}; }

    auto FinalPriority = InPriority;

    if (auto* PreviousContext = InPreviousContext.Get();
        ck::IsValid(PreviousContext))
    {
        if (InUsePreviousPriority)
        {
            auto PreviousPriority = int32{0};
            if (Subsystem->HasMappingContext(PreviousContext, PreviousPriority))
            {
                FinalPriority = PreviousPriority;
            }
        }

        Subsystem->RemoveMappingContext(PreviousContext);
    }

    Subsystem->AddMappingContext(NewContext, FinalPriority);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

