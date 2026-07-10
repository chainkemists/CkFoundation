#include "CkStateMachine_TestSupport.h"

#include "CkStateMachine/CkStateMachine_Log.h"
#include "CkStateMachine/Net/CkStateMachine_RepData.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

FCk_Handle_StateMachine
    UCk_Utils_StateMachine_Test_UE::
    Test_InjectFakeFingerprint(
        FCk_Handle_StateMachine& InSm,
        int32 InFakeHash,
        ECk_Sm_TestFakeFingerprintScope InScope)
{
#if WITH_DEV_AUTOMATION_TESTS
    CK_ENSURE_IF_NOT(ck::IsValid(InSm),
        TEXT("Test_InjectFakeFingerprint called with invalid SM handle.{}"), ck::Context(nullptr))
    { return InSm; }

    auto& Injection = InSm.AddOrGet<ck::FFragment_Sm_TestFakeFingerprintInjection>();
    Injection.Set_FakeFingerprint(InFakeHash);
    Injection.Set_Scope(InScope);
    Injection.Set_ConsumeOnUse(true);

    ck::sm::Verbose(
        TEXT("Test_InjectFakeFingerprint armed on SM [{}]: scope=[{}], fake=[{}]"),
        InSm, static_cast<uint8>(InScope), InFakeHash);
#endif

    return InSm;
}

// --------------------------------------------------------------------------------------------------------------------

int32
    UCk_Utils_StateMachine_Test_UE::
    Test_Get_LastPublishedFingerprint(
        FCk_Handle_StateMachine& InSm)
{
    if (NOT ck::IsValid(InSm))
    { return 0; }

    if (InSm.Has<FCk_RepData_StateMachine_WithHistory>())
    {
        const auto& RepData = InSm.Get<FCk_RepData_StateMachine_WithHistory>();
        const auto& History = RepData.Get_History();
        if (History.IsEmpty())
        { return RepData.Get_InitialStateFingerprint(); }
        return History.Last().Get_NewStateFingerprint();
    }

    if (InSm.Has<FCk_RepData_StateMachine_NoHistory>())
    { return InSm.Get<FCk_RepData_StateMachine_NoHistory>().Get_CurrentStateFingerprint(); }

    return 0;
}

// --------------------------------------------------------------------------------------------------------------------

bool
    UCk_Utils_StateMachine_Test_UE::
    Test_Get_HasDeterminismFault(
        FCk_Handle_StateMachine& InSm)
{
    if (NOT ck::IsValid(InSm))
    { return false; }

    return InSm.Has<ck::FTag_Sm_DeterminismFault>();
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Handle_StateMachine
    UCk_Utils_StateMachine_Test_UE::
    Test_ForceDeterminismFault(
        FCk_Handle_StateMachine& InSm)
{
#if WITH_DEV_AUTOMATION_TESTS
    CK_ENSURE_IF_NOT(ck::IsValid(InSm),
        TEXT("Test_ForceDeterminismFault called with invalid SM handle.{}"), ck::Context(nullptr))
    { return InSm; }

    InSm.AddOrGet<ck::FTag_Sm_DeterminismFault>();
#endif

    return InSm;
}

// --------------------------------------------------------------------------------------------------------------------
