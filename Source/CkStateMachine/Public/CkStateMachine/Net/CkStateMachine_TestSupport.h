#pragma once

#include "CoreMinimal.h"

#include "Kismet/BlueprintFunctionLibrary.h"

#include "CkStateMachine/StateMachine/CkStateMachine_Fragment_Data.h"

#include "CkStateMachine_TestSupport.generated.h"

// --------------------------------------------------------------------------------------------------------------------
//
// Test-only support hooks for CkStateMachine replication AutoTests. The §13 fingerprint-mismatch
// tests cannot synthesize the mismatch by authoring a divergent DefineState — the determinism
// Warning fires before tests can ack it, which escalates and fails the test even when
// FinishSuccess was called (project memory feedback_autotest_warning_escalation). Instead, tests
// inject a fake fingerprint on the publisher side: the wire value arriving at the non-authority
// client is corrupted, the receive-side verify path runs unchanged, and the test asserts the
// genuine quarantine response while _ExpectedLogErrors suppresses the Warning escalation.
//
// Everything in this file is WITH_DEV_AUTOMATION_TESTS-gated. Shipping builds have no fragment,
// no utility, no production-side check.
//
// --------------------------------------------------------------------------------------------------------------------

UENUM()
enum class ECk_Sm_TestFakeFingerprintScope : uint8
{
    // Next transition's _NewStateFingerprint is replaced on the publisher side before the rep
    // payload writes. Server-auth: server's CommitPendingTransition substitutes. OwningClient-
    // auth: owning client's CommitPendingTransition substitutes (the buffered batch the relay
    // RPC will carry).
    NextTransition,

    // The _InitialStateFingerprint stamped by DoBackfillFingerprintToRepData on first Setup is
    // replaced. Used for test 9 (initial-state mismatch).
    InitialState
};

#if WITH_DEV_AUTOMATION_TESTS

namespace ck
{
    struct CKSTATEMACHINE_API FFragment_Sm_TestFakeFingerprintInjection
    {
    public:
        CK_GENERATED_BODY(FFragment_Sm_TestFakeFingerprintInjection);

    private:
        int32 _FakeFingerprint = 0;
        ECk_Sm_TestFakeFingerprintScope _Scope = ECk_Sm_TestFakeFingerprintScope::NextTransition;
        bool _ConsumeOnUse = true;

    public:
        CK_PROPERTY(_FakeFingerprint);
        CK_PROPERTY(_Scope);
        CK_PROPERTY(_ConsumeOnUse);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Sm_TestFakeFingerprintInjection,
            _FakeFingerprint, _Scope);
    };
}

#endif // WITH_DEV_AUTOMATION_TESTS

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKSTATEMACHINE_API UCk_Utils_StateMachine_Test_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // UFUNCTION declarations cannot be #if-gated (UHT only allows WITH_EDITORONLY_DATA there).
    // Implementations are WITH_DEV_AUTOMATION_TESTS-gated in the .cpp; in non-test builds these
    // become no-op stubs returning the input handle / 0 / false.

    // Arms the next-published fingerprint (or initial-state seed) to be replaced with InFakeHash.
    // Adds ck::FFragment_Sm_TestFakeFingerprintInjection to the SM entity. Publisher path consumes
    // the fragment by default. No-op in non-test builds.
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|StateMachine|Test",
        DisplayName = "[Ck][SM][Test] Inject Fake Fingerprint")
    static FCk_Handle_StateMachine
    Test_InjectFakeFingerprint(
        UPARAM(ref) FCk_Handle_StateMachine& InSm,
        int32 InFakeHash,
        ECk_Sm_TestFakeFingerprintScope InScope = ECk_Sm_TestFakeFingerprintScope::NextTransition);

    // Reads the published fingerprint value off the SM's rep payload — history's last
    // _NewStateFingerprint, or _InitialStateFingerprint when history is empty, or NoHistory's
    // _CurrentStateFingerprint. Returns 0 if the SM doesn't have a rep payload yet.
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|StateMachine|Test",
        DisplayName = "[Ck][SM][Test] Get Published Fingerprint")
    static int32
    Test_Get_LastPublishedFingerprint(
        UPARAM(ref) FCk_Handle_StateMachine& InSm);

    // Returns true iff the SM has been faulted (FTag_Sm_DeterminismFault present). The
    // fingerprint mismatch path stamps this tag via DoVerifyFingerprintAgainstExpected.
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|StateMachine|Test",
        DisplayName = "[Ck][SM][Test] Has Determinism Fault")
    static bool
    Test_Get_HasDeterminismFault(
        UPARAM(ref) FCk_Handle_StateMachine& InSm);

    // Directly stamps FTag_Sm_DeterminismFault on the SM, simulating a quarantine without needing
    // the (multi-client, replication-dependent) verify path to produce it. Used to pin the
    // Enter-despite-fault contract: a state entering while the owning SM is faulted must not run
    // its EnterState side effects. No-op in non-test builds.
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|StateMachine|Test",
        DisplayName = "[Ck][SM][Test] Force Determinism Fault")
    static FCk_Handle_StateMachine
    Test_ForceDeterminismFault(
        UPARAM(ref) FCk_Handle_StateMachine& InSm);
};

// --------------------------------------------------------------------------------------------------------------------
