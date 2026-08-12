#include "CkIntentMatcher_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkInput/CkInputLayer_Utils.h"

#include "CkIntent/CkIntentSampler_Utils.h"
#include "CkIntent/CkIntent_Log.h"

#include <HAL/IConsoleManager.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_intent_matcher_utils
{
    static bool RecordScanDiagnostics = false;
    static FAutoConsoleVariableRef CVarRecordScanDiagnostics(
        TEXT("ck.Intent.RecordScanDiagnostics"),
        RecordScanDiagnostics,
        TEXT("When true, every backward scan a matcher runs is recorded into that matcher's near-miss ring: the "
             "intent, the terminal frame, whether it matched, and for each step of the definition walked the "
             "frame it matched at or how many frames were examined before the walk gave up. Read it back with "
             "Get_ScanDiagnostics. Off by default — the ring is written on every scan attempt."),
        ECVF_Default);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(
    UCk_Utils_IntentMatcher_UE,
    FCk_Handle_IntentMatcher,
    ck::FFragment_IntentMatcher_Params,
    ck::FFragment_IntentMatcher_Current);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_IntentMatcher_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_IntentMatcher_ParamsData& InParams)
    -> FCk_Handle_IntentMatcher
{
    const auto HandleIsValid = ck::IsValid(InHandle);
    CK_ENSURE_IF_NOT(HandleIsValid,
        TEXT("Add: invalid Handle [{}] — cannot compose an IntentMatcher onto it"), InHandle)
    {}
    if (NOT HandleIsValid)
    { return {}; }

    const auto HandleIsAnInputLayer = UCk_Utils_InputLayer_UE::Has(InHandle);
    CK_ENSURE_IF_NOT(HandleIsAnInputLayer,
        TEXT("Add: Handle [{}] is not an InputLayer — a matcher off a layer would have no arbitration to answer "
             "to, and no way to tell an event it was allowed to see from one a layer above it consumed"), InHandle)
    {}
    if (NOT HandleIsAnInputLayer)
    { return {}; }

    const auto EntityHasNoMatcher = NOT Has(InHandle);
    CK_ENSURE_IF_NOT(EntityHasNoMatcher,
        TEXT("Add: Handle [{}] already carries an IntentMatcher — one layer runs one set, and a second matcher "
             "would register captures the first one believes it owns"), InHandle)
    {}
    if (NOT EntityHasNoMatcher)
    { return {}; }

    const auto DecayFramesArePositive = InParams.Get_LatchDecayFrames() > 0;
    CK_ENSURE_IF_NOT(DecayFramesArePositive,
        TEXT("Add: IntentMatcher declaration on [{}] asks for a latch decay of [{}] frames — a latch that expires "
             "the frame it is stamped cannot be polled at all, and one that never expires leaves a completion "
             "claimable for the rest of the session"), InHandle, InParams.Get_LatchDecayFrames())
    {}
    if (NOT DecayFramesArePositive)
    { return {}; }

    InHandle.Add<ck::FFragment_IntentMatcher_Params>(InParams);
    InHandle.Add<ck::FFragment_IntentMatcher_Current>();

    ck::intent::Verbose
    (
        TEXT("IntentMatcher composed onto InputLayer [{}] with capture behavior [{}] and no active set"),
        InHandle, InParams.Get_CaptureBehavior()
    );

    return CastChecked(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_IntentMatcher_UE::
    Get_IntentPhase(
        const FCk_Handle_IntentMatcher& InMatcher,
        const FGameplayTag& InIntentTag)
    -> ECk_Intent_Phase
{
    const auto Index = DoGet_IntentIndex_ByTag(InMatcher, InIntentTag);

    if (Index == INDEX_NONE)
    { return ECk_Intent_Phase::Idle; }

    return InMatcher.Get<ck::FFragment_IntentMatcher_Current>().Get_PhaseRows()[Index].Get_Phase();
}

auto
    UCk_Utils_IntentMatcher_UE::
    Get_IntentPhase_ByName(
        const FCk_Handle_IntentMatcher& InMatcher,
        FName InIntentName)
    -> ECk_Intent_Phase
{
    const auto Index = DoGet_IntentIndex_ByName(InMatcher, InIntentName);

    if (Index == INDEX_NONE)
    { return ECk_Intent_Phase::Idle; }

    return InMatcher.Get<ck::FFragment_IntentMatcher_Current>().Get_PhaseRows()[Index].Get_Phase();
}

auto
    UCk_Utils_IntentMatcher_UE::
    TryGet_CompletionFrame(
        const FCk_Handle_IntentMatcher& InMatcher,
        const FGameplayTag& InIntentTag)
    -> int32
{
    return DoGet_CompletionFrame(InMatcher, DoGet_IntentIndex_ByTag(InMatcher, InIntentTag));
}

auto
    UCk_Utils_IntentMatcher_UE::
    TryGet_CompletionFrame_ByName(
        const FCk_Handle_IntentMatcher& InMatcher,
        FName InIntentName)
    -> int32
{
    return DoGet_CompletionFrame(InMatcher, DoGet_IntentIndex_ByName(InMatcher, InIntentName));
}

auto
    UCk_Utils_IntentMatcher_UE::
    TryGet_ActivationFrame(
        const FCk_Handle_IntentMatcher& InMatcher,
        const FGameplayTag& InIntentTag)
    -> int32
{
    return DoGet_ActivationFrame(InMatcher, DoGet_IntentIndex_ByTag(InMatcher, InIntentTag));
}

auto
    UCk_Utils_IntentMatcher_UE::
    TryGet_ActivationFrame_ByName(
        const FCk_Handle_IntentMatcher& InMatcher,
        FName InIntentName)
    -> int32
{
    return DoGet_ActivationFrame(InMatcher, DoGet_IntentIndex_ByName(InMatcher, InIntentName));
}

auto
    UCk_Utils_IntentMatcher_UE::
    Get_IsClaimed(
        const FCk_Handle_IntentMatcher& InMatcher,
        const FGameplayTag& InIntentTag)
    -> bool
{
    return ck::IsValid(TryGet_ClaimedBy(InMatcher, InIntentTag));
}

auto
    UCk_Utils_IntentMatcher_UE::
    Get_IsClaimed_ByName(
        const FCk_Handle_IntentMatcher& InMatcher,
        FName InIntentName)
    -> bool
{
    return ck::IsValid(TryGet_ClaimedBy_ByName(InMatcher, InIntentName));
}

auto
    UCk_Utils_IntentMatcher_UE::
    TryGet_ClaimedBy(
        const FCk_Handle_IntentMatcher& InMatcher,
        const FGameplayTag& InIntentTag)
    -> FCk_Handle
{
    return DoGet_ClaimedBy(InMatcher, DoGet_IntentIndex_ByTag(InMatcher, InIntentTag));
}

auto
    UCk_Utils_IntentMatcher_UE::
    TryGet_ClaimedBy_ByName(
        const FCk_Handle_IntentMatcher& InMatcher,
        FName InIntentName)
    -> FCk_Handle
{
    return DoGet_ClaimedBy(InMatcher, DoGet_IntentIndex_ByName(InMatcher, InIntentName));
}

auto
    UCk_Utils_IntentMatcher_UE::
    Get_HasActiveSet(
        const FCk_Handle_IntentMatcher& InMatcher)
    -> bool
{
    return NOT InMatcher.Get<ck::FFragment_IntentMatcher_Current>().Get_ActiveSet().Get_IsEmpty();
}

auto
    UCk_Utils_IntentMatcher_UE::
    Get_ActiveIntentCount(
        const FCk_Handle_IntentMatcher& InMatcher)
    -> int32
{
    return InMatcher.Get<ck::FFragment_IntentMatcher_Current>().Get_ActiveSet().Get_Intents().Num();
}

auto
    UCk_Utils_IntentMatcher_UE::
    Get_ScanDiagnosticsEnabled()
    -> bool
{
    return ck_intent_matcher_utils::RecordScanDiagnostics;
}

auto
    UCk_Utils_IntentMatcher_UE::
    Get_ScanDiagnostics(
        const FCk_Handle_IntentMatcher& InMatcher)
    -> TArray<FCk_Intent_ScanDiagnostic>
{
    const auto& Current = InMatcher.Get<ck::FFragment_IntentMatcher_Current>();
    const auto& Entries = Current.Get_ScanDiagnostics();

    if (Entries.IsEmpty())
    { return {}; }

    auto Newest = TArray<FCk_Intent_ScanDiagnostic>{};
    Newest.Reserve(Current.Get_ScanDiagnosticsCount());

    // The modulus is the LIVE entry count rather than the declared capacity: while the ring is still filling,
    // the write index and the array length advance together, so both phases wrap on the same value.
    const auto Slots = Entries.Num();

    for (auto Offset = 0; Offset < Current.Get_ScanDiagnosticsCount(); ++Offset)
    {
        const auto Index = ((Current.Get_ScanDiagnosticsNextWrite() - 1 - Offset) % Slots + Slots) % Slots;
        Newest.Emplace(Entries[Index]);
    }

    return Newest;
}

auto
    UCk_Utils_IntentMatcher_UE::
    Get_RegisteredCaptureKeys(
        const FCk_Handle_IntentMatcher& InMatcher)
    -> TArray<FKey>
{
    auto Keys = TArray<FKey>{};

    for (const auto& Registered : InMatcher.Get<ck::FFragment_IntentMatcher_Current>().Get_RegisteredCaptures())
    {
        for (const auto& Key : Registered.Get_Keys())
        {
            if (NOT Key.IsValid())
            { continue; }

            Keys.AddUnique(Key);
        }
    }

    return Keys;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_IntentMatcher_UE::
    Request_SwapSet(
        FCk_Handle_IntentMatcher& InMatcher,
        const FCk_Request_IntentMatcher_SwapSet& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_IntentMatcher
{
    const auto MatcherIsValid = ck::IsValid(InMatcher);
    CK_ENSURE_IF_NOT(MatcherIsValid,
        TEXT("Request_SwapSet: invalid IntentMatcher handle [{}] — the set is dropped"), InMatcher)
    {}
    if (NOT MatcherIsValid)
    {
        InDelegate.ExecuteIfBound(InMatcher, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InMatcher;
    }

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InMatcher.AddOrGet<ck::FFragment_IntentMatcher_Requests>()._Requests.Emplace(InRequest);

    return InMatcher;
}

auto
    UCk_Utils_IntentMatcher_UE::
    Request_Claim(
        FCk_Handle_IntentMatcher& InMatcher,
        const FCk_Request_IntentMatcher_Claim& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_IntentMatcher
{
    const auto MatcherIsValid = ck::IsValid(InMatcher);
    CK_ENSURE_IF_NOT(MatcherIsValid,
        TEXT("Request_Claim: invalid IntentMatcher handle [{}] — there is no row to claim"), InMatcher)
    {}
    if (NOT MatcherIsValid)
    {
        InDelegate.ExecuteIfBound(InMatcher, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InMatcher;
    }

    const auto ClaimantIsValid = ck::IsValid(InRequest.Get_Claimant());
    CK_ENSURE_IF_NOT(ClaimantIsValid,
        TEXT("Request_Claim: invalid claimant on IntentMatcher [{}] — exclusivity is expressed as WHO holds the "
             "intent, so a claim with nobody to name cannot exclude anyone"), InMatcher)
    {}
    if (NOT ClaimantIsValid)
    {
        InDelegate.ExecuteIfBound(InMatcher, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InMatcher;
    }

    const auto Index = DoGet_IntentIndex_ByName(InMatcher, InRequest.Get_IntentName());

    if (Index == INDEX_NONE)
    {
        ck::intent::Verbose
        (
            TEXT("Request_Claim on IntentMatcher [{}] rejected: the active set carries no intent named [{}]"),
            InMatcher, InRequest.Get_IntentName()
        );

        InDelegate.ExecuteIfBound(InMatcher, ECk_Request_OperationResult::Failed);
        return InMatcher;
    }

    auto& Row = InMatcher.Get<ck::FFragment_IntentMatcher_Current>()._PhaseRows[Index];

    const auto PhaseIsClaimable = Row._Phase == ECk_Intent_Phase::Completed ||
                                  Row._Phase == ECk_Intent_Phase::Active;

    if (NOT PhaseIsClaimable)
    {
        ck::intent::Verbose
        (
            TEXT("Request_Claim on IntentMatcher [{}] rejected: intent [{}] is [{}], and only a completed or an "
                 "active intent is something to take ownership of"),
            InMatcher, InRequest.Get_IntentName(), Row._Phase
        );

        InDelegate.ExecuteIfBound(InMatcher, ECk_Request_OperationResult::Failed);
        return InMatcher;
    }

    if (ck::IsValid(Row._ClaimedBy))
    {
        // A repeat claim by the holder is not a second claim — the caller's intent already holds, which is what
        // Succeeded means. Re-stamping would move a claim frame nobody asked to move.
        const auto ClaimantAlreadyHoldsIt = Row._ClaimedBy == InRequest.Get_Claimant();

        InDelegate.ExecuteIfBound(InMatcher, ClaimantAlreadyHoldsIt
            ? ECk_Request_OperationResult::Succeeded
            : ECk_Request_OperationResult::Failed);

        return InMatcher;
    }

    Row._ClaimedBy = InRequest.Get_Claimant();
    Row._ClaimedFrame = DoGet_LatestRecordFrame(InMatcher);

    ck::intent::Verbose
    (
        TEXT("IntentMatcher [{}]: intent [{}] claimed by [{}] on record frame [{}]"),
        InMatcher, InRequest.Get_IntentName(), Row._ClaimedBy, Row._ClaimedFrame
    );

    // Fired AFTER the mutation, so a delegate that turns around and polls the matcher sees the claim it caused.
    InDelegate.ExecuteIfBound(InMatcher, ECk_Request_OperationResult::Succeeded);

    return InMatcher;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_IntentMatcher_UE::
    BindTo_OnIntentPhaseChanged(
        FCk_Handle_IntentMatcher& InMatcher,
        const FCk_Delegate_IntentMatcher_PhaseChanged& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_IntentMatcher
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnIntentPhaseChanged, InMatcher, InDelegate, InBindingPolicy, InPostFireBehavior);

    return InMatcher;
}

auto
    UCk_Utils_IntentMatcher_UE::
    UnbindFrom_OnIntentPhaseChanged(
        FCk_Handle_IntentMatcher& InMatcher,
        const FCk_Delegate_IntentMatcher_PhaseChanged& InDelegate)
    -> FCk_Handle_IntentMatcher
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnIntentPhaseChanged, InMatcher, InDelegate);

    return InMatcher;
}

auto
    UCk_Utils_IntentMatcher_UE::
    BindTo_OnIntentCompleted(
        FCk_Handle_IntentMatcher& InMatcher,
        const FCk_Delegate_IntentMatcher_Completed& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_IntentMatcher
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnIntentCompleted, InMatcher, InDelegate, InBindingPolicy, InPostFireBehavior);

    return InMatcher;
}

auto
    UCk_Utils_IntentMatcher_UE::
    UnbindFrom_OnIntentCompleted(
        FCk_Handle_IntentMatcher& InMatcher,
        const FCk_Delegate_IntentMatcher_Completed& InDelegate)
    -> FCk_Handle_IntentMatcher
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnIntentCompleted, InMatcher, InDelegate);

    return InMatcher;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_IntentMatcher_UE::
    DoGet_CompletionFrame(
        const FCk_Handle_IntentMatcher& InMatcher,
        int32 InIntentIndex)
    -> int32
{
    if (InIntentIndex == INDEX_NONE)
    { return INDEX_NONE; }

    const auto& Row = InMatcher.Get<ck::FFragment_IntentMatcher_Current>().Get_PhaseRows()[InIntentIndex];

    // The row carries one frame for whichever phase it is in, so a completion frame is only a completion frame
    // while the row says Completed.
    if (Row.Get_Phase() != ECk_Intent_Phase::Completed)
    { return INDEX_NONE; }

    return Row.Get_PhaseFrame();
}

auto
    UCk_Utils_IntentMatcher_UE::
    DoGet_ActivationFrame(
        const FCk_Handle_IntentMatcher& InMatcher,
        int32 InIntentIndex)
    -> int32
{
    if (InIntentIndex == INDEX_NONE)
    { return INDEX_NONE; }

    const auto& Row = InMatcher.Get<ck::FFragment_IntentMatcher_Current>().Get_PhaseRows()[InIntentIndex];

    // Gated for the same reason the completion frame is: the row carries one frame for whichever phase it is in,
    // and a level that has already been released names the frame it was released on, not the one it began at.
    if (Row.Get_Phase() != ECk_Intent_Phase::Active)
    { return INDEX_NONE; }

    return Row.Get_PhaseFrame();
}

auto
    UCk_Utils_IntentMatcher_UE::
    DoGet_ClaimedBy(
        const FCk_Handle_IntentMatcher& InMatcher,
        int32 InIntentIndex)
    -> FCk_Handle
{
    if (InIntentIndex == INDEX_NONE)
    { return {}; }

    const auto& Row = InMatcher.Get<ck::FFragment_IntentMatcher_Current>().Get_PhaseRows()[InIntentIndex];

    if (Row.Get_Phase() != ECk_Intent_Phase::Completed && Row.Get_Phase() != ECk_Intent_Phase::Active)
    { return {}; }

    return Row.Get_ClaimedBy();
}

auto
    UCk_Utils_IntentMatcher_UE::
    DoGet_LatestRecordFrame(
        const FCk_Handle_IntentMatcher& InMatcher)
    -> int32
{
    const auto Layer = UCk_Utils_InputLayer_UE::CastChecked(InMatcher);
    const auto Sampler = UCk_Utils_IntentSampler_UE::Cast(UCk_Utils_InputLayer_UE::Get_InputSource(Layer));

    if (ck::Is_NOT_Valid(Sampler))
    { return INDEX_NONE; }

    return UCk_Utils_IntentSampler_UE::Get_LatestFrame(Sampler).Get_FrameIndex();
}

auto
    UCk_Utils_IntentMatcher_UE::
    DoGet_IntentIndex_ByTag(
        const FCk_Handle_IntentMatcher& InMatcher,
        const FGameplayTag& InIntentTag)
    -> int32
{
    if (NOT InIntentTag.IsValid())
    { return INDEX_NONE; }

    const auto& Current = InMatcher.Get<ck::FFragment_IntentMatcher_Current>();
    const auto& Intents = Current.Get_ActiveSet().Get_Intents();

    const auto Index = Intents.IndexOfByPredicate(
    [&](const FCk_Intent_CompiledIntent& InIntent) -> bool
    {
        return InIntent.Get_IntentTag() == InIntentTag;
    });

    // The rows are sized to the set at swap time, so a set with rows shorter than its intents cannot occur — the
    // bound is a statement about that invariant rather than a recovery from it.
    if (Index == INDEX_NONE || Index >= Current.Get_PhaseRows().Num())
    { return INDEX_NONE; }

    return Index;
}

auto
    UCk_Utils_IntentMatcher_UE::
    DoGet_IntentIndex_ByName(
        const FCk_Handle_IntentMatcher& InMatcher,
        FName InIntentName)
    -> int32
{
    if (InIntentName == NAME_None)
    { return INDEX_NONE; }

    const auto& Current = InMatcher.Get<ck::FFragment_IntentMatcher_Current>();
    const auto& Intents = Current.Get_ActiveSet().Get_Intents();

    const auto Index = Intents.IndexOfByPredicate(
    [&](const FCk_Intent_CompiledIntent& InIntent) -> bool
    {
        return InIntent.Get_Name() == InIntentName;
    });

    if (Index == INDEX_NONE || Index >= Current.Get_PhaseRows().Num())
    { return INDEX_NONE; }

    return Index;
}

// --------------------------------------------------------------------------------------------------------------------
