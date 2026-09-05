#include "CkProbeContactFilter.h"

#include "CkJolt/CkJolt_Utils.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_ProbeContactFilter_RejectsCapacityExhaustion,
    "Ck.CkSpatialQuery.Probe.ContactFilterRejectsCapacityExhaustion",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_ProbeContactFilter_RejectsCapacityExhaustion::RunTest(const FString&)
{
    const auto JoltGlobals = ck::jolt::FCk_Jolt_ScopedGlobalInit{};
    auto Filter = JPH::Ref<ck::spatialquery::FCk_ProbeContactFilter>{
        new ck::spatialquery::FCk_ProbeContactFilter{1}};

    const auto NotifyParams = FCk_Fragment_Probe_ParamsData{TAG_Probe};
    auto SilentParams = NotifyParams;
    SilentParams.Set_ResponsePolicy(ECk_ProbeResponse_Policy::Silent);

    const auto FirstSignature = Filter->Get_OrRegisterSignature(NotifyParams);
    TestEqual(TEXT("first signature uses the reserved slot"), FirstSignature, uint32{0});

    // Editor builds report each deliberate rejection through both CkEnsure and CkEnsures.
    AddExpectedErrorPlain(
        TEXT("Probe contact-filter signature capacity [1] exhausted"),
        EAutomationExpectedErrorFlags::Contains,
        4);

    const auto RejectedSignature = Filter->Get_OrRegisterSignature(SilentParams);
    TestEqual(TEXT("capacity exhaustion is explicitly representable"),
        RejectedSignature, JPH::CollisionGroup::cInvalidSubGroup);

    const auto RejectedRetry = Filter->Get_OrRegisterSignature(SilentParams);
    TestEqual(TEXT("rejected signature does not mutate or publish the table"),
        RejectedRetry, JPH::CollisionGroup::cInvalidSubGroup);

    TestEqual(TEXT("an already published signature remains stable after rejection"),
        Filter->Get_OrRegisterSignature(NotifyParams), FirstSignature);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_ProbeContactFilter_ContactParticipationAdmission,
    "Ck.CkSpatialQuery.Probe.ContactFilterContactParticipationAdmission",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_ProbeContactFilter_ContactParticipationAdmission::RunTest(const FString&)
{
    const auto JoltGlobals = ck::jolt::FCk_Jolt_ScopedGlobalInit{};
    auto Filter = JPH::Ref<ck::spatialquery::FCk_ProbeContactFilter>{
        new ck::spatialquery::FCk_ProbeContactFilter{3}};

    const auto NotifyParams = FCk_Fragment_Probe_ParamsData{TAG_Probe};
    auto SilentParams = NotifyParams;
    SilentParams.Set_ResponsePolicy(ECk_ProbeResponse_Policy::Silent);
    auto QueryOnlyParams = NotifyParams;
    QueryOnlyParams.Set_ContactParticipation(ECk_Probe_ContactParticipation::QueryOnly);

    const auto NotifyGroup = JPH::CollisionGroup{
        Filter.GetPtr(), 0, Filter->Get_OrRegisterSignature(NotifyParams)};
    const auto SilentGroup = JPH::CollisionGroup{
        Filter.GetPtr(), 0, Filter->Get_OrRegisterSignature(SilentParams)};
    const auto QueryOnlyGroup = JPH::CollisionGroup{
        Filter.GetPtr(), 0, Filter->Get_OrRegisterSignature(QueryOnlyParams)};

    // Silent cannot receive, but Notify can: preserve the directional targetable contract.
    TestTrue(TEXT("Notify receiver admits a Silent target"), Filter->CanCollide(NotifyGroup, SilentGroup));
    TestTrue(TEXT("Silent target is admitted regardless of Jolt pair ordering"), Filter->CanCollide(SilentGroup, NotifyGroup));

    // QueryOnly is a bilateral physical-contact exclusion, independent of either receiver policy.
    TestFalse(TEXT("QueryOnly rejects Notify contacts"), Filter->CanCollide(QueryOnlyGroup, NotifyGroup));
    TestFalse(TEXT("Notify rejects QueryOnly contacts regardless of pair ordering"), Filter->CanCollide(NotifyGroup, QueryOnlyGroup));
    TestFalse(TEXT("QueryOnly rejects Silent contacts"), Filter->CanCollide(QueryOnlyGroup, SilentGroup));
    TestFalse(TEXT("Silent rejects QueryOnly contacts regardless of pair ordering"), Filter->CanCollide(SilentGroup, QueryOnlyGroup));

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
