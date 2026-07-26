#pragma once

#include "CoreMinimal.h"

#include "Kismet/BlueprintFunctionLibrary.h"

#include "CkStateMachine/StateMachine/CkStateMachine_Fragment_Data.h"

#include "CkStateMachine_TestSupport.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Test-only hooks for CkStateMachine replication AutoTests: fingerprint mismatches are synthesized by
// injecting a fake fingerprint on the publisher side rather than by authoring a divergent DefineState
// (whose determinism Warning escalates and fails the test). WITH_DEV_AUTOMATION_TESTS-gated throughout.

// --------------------------------------------------------------------------------------------------------------------

UENUM()
enum class ECk_Sm_TestFakeFingerprintScope : uint8
{
    // Next transition's _NewStateFingerprint is replaced on the publisher side (server for
    // ServerAuth, owning client for OwningClientAuth) before the rep payload writes.
    NextTransition,

    // The _InitialStateFingerprint stamped by DoBackfillFingerprintToRepData on first Setup.
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
    // UFUNCTION declarations cannot be #if-gated (UHT only allows WITH_EDITORONLY_DATA there), so the
    // .cpp implementations are WITH_DEV_AUTOMATION_TESTS-gated and stub out in non-test builds.

    // Arms the next published fingerprint (or initial-state seed) to be replaced with InFakeHash.
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|StateMachine|Test",
        DisplayName = "[Ck][SM][Test] Inject Fake Fingerprint")
    static FCk_Handle_StateMachine
    Test_InjectFakeFingerprint(
        UPARAM(ref) FCk_Handle_StateMachine& InSm,
        int32 InFakeHash,
        ECk_Sm_TestFakeFingerprintScope InScope = ECk_Sm_TestFakeFingerprintScope::NextTransition);

    // Reads the published fingerprint: history's last _NewStateFingerprint, else
    // _InitialStateFingerprint, else NoHistory's _CurrentStateFingerprint. 0 when no payload yet.
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|StateMachine|Test",
        DisplayName = "[Ck][SM][Test] Get Published Fingerprint")
    static int32
    Test_Get_LastPublishedFingerprint(
        UPARAM(ref) FCk_Handle_StateMachine& InSm);

    // True iff FTag_Sm_DeterminismFault is present on the SM.
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|StateMachine|Test",
        DisplayName = "[Ck][SM][Test] Has Determinism Fault")
    static bool
    Test_Get_HasDeterminismFault(
        UPARAM(ref) FCk_Handle_StateMachine& InSm);

    // Stamps FTag_Sm_DeterminismFault directly, simulating quarantine without the multi-client verify
    // path — pins that a state entering while the SM is faulted runs no EnterState side effects.
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|StateMachine|Test",
        DisplayName = "[Ck][SM][Test] Force Determinism Fault")
    static FCk_Handle_StateMachine
    Test_ForceDeterminismFault(
        UPARAM(ref) FCk_Handle_StateMachine& InSm);
};

// --------------------------------------------------------------------------------------------------------------------
