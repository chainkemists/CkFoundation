#include "CkStateMachine/Task/CkSmTask_Utils.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Registry/CkRegistry.h"
#include "CkEcs/Registry/CkRegistry_SlotTable.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_SmTask_CanRunScopedWork,
    "CkFoundation.StateMachine.Task.CanRunScopedWork",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_SmTask_CanRunScopedWork::RunTest(const FString&)
{
    auto EnttRegistry = ck::registry_table::EnttRegistryType{};
    const auto RegistryHandle = ck::registry_table::Allocate(&EnttRegistry);
    auto Registry = FCk_Registry{RegistryHandle};
    ON_SCOPE_EXIT { ck::registry_table::Free(RegistryHandle); };

    const auto TransientEntityId = FCk_Entity{EnttRegistry.create()};
    Registry.SetContext<ck::FCtx_TransientEntity>(ck::FCtx_TransientEntity{TransientEntityId});

    const auto MakeTask = [&Registry]() -> TPair<FCk_Handle, FCk_Handle_SmTask>
    {
        auto Task = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(Registry);
        Task.Add<ck::FFragment_SmTask_Current>();
        Task.Add<ck::FFragment_SmTask_Params>();
        Task.Add<ck::FTag_SmTask_EnterExit>();
        Task.Add<ck::FTag_SmTask_Active>();
        return {Task, ck::StaticCast<FCk_Handle_SmTask>(Task)};
    };

    const auto MakeStateMachine = [&Registry](
        const ECk_Sm_AuthorityModel InAuthority,
        const ECk_Sm_NetContext InNetContext) -> TPair<FCk_Handle, FCk_Handle_StateMachine>
    {
        auto StateMachine = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(Registry);
        StateMachine.Add<ck::FFragment_Sm_Current>();
        StateMachine.Add<ck::FFragment_Sm_Params>();
        StateMachine.Add<ck::FFragment_Sm_NetIdentity>(InAuthority, InNetContext);
        return {StateMachine, ck::StaticCast<FCk_Handle_StateMachine>(StateMachine)};
    };

    const auto AttachAuthorityOwner = [&MakeStateMachine](FCk_Handle& InTask) -> FCk_Handle
    {
        auto [StateMachine, StateMachineTyped] = MakeStateMachine(
            ECk_Sm_AuthorityModel::ServerAuthoritative, ECk_Sm_NetContext::Standalone);
        ck::TUtils_Sm_OwningStateMachine::AddOrReplace(InTask, StateMachineTyped);
        return StateMachine;
    };

    const auto AssertStructuralRejectionLeavesMemoAbsent = [this](
        const TCHAR* InCase,
        const FCk_Handle_SmTask& InTask,
        const FCk_Handle& InCheckedStateMachine)
    {
        TestFalse(InCase, UCk_Utils_SmTask_UE::Get_CanRunScopedWork(InTask));
        TestFalse(FString::Printf(TEXT("%s does not add authority memo"), InCase),
            InCheckedStateMachine.Has<ck::FFragment_Sm_NetContextMemo>());
    };

    const auto InvalidTask = FCk_Handle_SmTask{};
    TestFalse(TEXT("invalid task is rejected"), UCk_Utils_SmTask_UE::Get_CanRunScopedWork(InvalidTask));

    auto [NonTaskRaw, NonTask] = MakeTask();
    NonTaskRaw.Try_Remove<ck::FFragment_SmTask_Params>();
    TestFalse(TEXT("non-task handle is rejected"), UCk_Utils_SmTask_UE::Get_CanRunScopedWork(NonTask));

    auto [InactiveRaw, InactiveTask] = MakeTask();
    const auto InactiveOwner = AttachAuthorityOwner(InactiveRaw);
    InactiveRaw.Try_Remove<ck::FTag_SmTask_Active>();
    TestFalse(TEXT("inactive task is rejected"), UCk_Utils_SmTask_UE::Get_CanRunScopedWork(InactiveTask));
    TestFalse(TEXT("inactive task does not resolve owner authority"),
        InactiveOwner.Has<ck::FFragment_Sm_NetContextMemo>());

    auto [PendingExitRaw, PendingExitTask] = MakeTask();
    const auto PendingExitOwner = AttachAuthorityOwner(PendingExitRaw);
    PendingExitRaw.Add<ck::FTag_SmTask_PendingExit>();
    TestFalse(TEXT("pending-exit task is rejected"), UCk_Utils_SmTask_UE::Get_CanRunScopedWork(PendingExitTask));
    TestFalse(TEXT("pending-exit task does not resolve owner authority"),
        PendingExitOwner.Has<ck::FFragment_Sm_NetContextMemo>());

    auto [DyingRaw, DyingTask] = MakeTask();
    const auto DyingOwner = AttachAuthorityOwner(DyingRaw);
    DyingRaw.Add<ck::FTag_DestroyEntity_Initiate>();
    TestFalse(TEXT("begin-destroy task is rejected"), UCk_Utils_SmTask_UE::Get_CanRunScopedWork(DyingTask));
    TestFalse(TEXT("dying task does not resolve owner authority"),
        DyingOwner.Has<ck::FFragment_Sm_NetContextMemo>());

    auto [MissingOwnerRaw, MissingOwnerTask] = MakeTask();
    TestFalse(TEXT("task without owning state machine is rejected"),
        UCk_Utils_SmTask_UE::Get_CanRunScopedWork(MissingOwnerTask));

    auto [WrongOwnerRaw, WrongOwnerTask] = MakeTask();
    auto [WrongOwnerEntity, WrongOwnerTypedAsTask] = MakeTask();
    auto WrongOwnerTypedAsSm = ck::StaticCast<FCk_Handle_StateMachine>(WrongOwnerTypedAsTask);
    ck::TUtils_Sm_OwningStateMachine::AddOrReplace(WrongOwnerRaw, WrongOwnerTypedAsSm);
    AssertStructuralRejectionLeavesMemoAbsent(TEXT("task used as owning SM is rejected"),
        WrongOwnerTask, WrongOwnerEntity);

    auto [MissingSmParamsRaw, MissingSmParamsTask] = MakeTask();
    auto MissingSmParams = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(Registry);
    MissingSmParams.Add<ck::FFragment_Sm_Current>();
    MissingSmParams.Add<ck::FFragment_Sm_NetIdentity>(
        ECk_Sm_AuthorityModel::ServerAuthoritative, ECk_Sm_NetContext::Standalone);
    auto MissingSmParamsTyped = ck::StaticCast<FCk_Handle_StateMachine>(MissingSmParams);
    ck::TUtils_Sm_OwningStateMachine::AddOrReplace(MissingSmParamsRaw, MissingSmParamsTyped);
    AssertStructuralRejectionLeavesMemoAbsent(TEXT("owner without SM params is rejected"),
        MissingSmParamsTask, MissingSmParams);

    auto [StaleOwnerRaw, StaleOwnerTask] = MakeTask();
    auto [StaleSmRaw, StaleSm] = MakeStateMachine(
        ECk_Sm_AuthorityModel::ServerAuthoritative, ECk_Sm_NetContext::Standalone);
    ck::TUtils_Sm_OwningStateMachine::AddOrReplace(StaleOwnerRaw, StaleSm);
    StaleSmRaw.Add<ck::FTag_DestroyEntity_Teardown>();
    AssertStructuralRejectionLeavesMemoAbsent(TEXT("stale owning SM is rejected"), StaleOwnerTask, StaleSmRaw);

    auto [CycleRaw, CycleTask] = MakeTask();
    auto [CycleSmRaw, CycleSm] = MakeStateMachine(
        ECk_Sm_AuthorityModel::ServerAuthoritative, ECk_Sm_NetContext::Standalone);
    ck::TUtils_Sm_OwningStateMachine::AddOrReplace(CycleRaw, CycleSm);
    ck::TUtils_Sm_OwningStateMachine::AddOrReplace(CycleSmRaw, CycleSm);
    AssertStructuralRejectionLeavesMemoAbsent(TEXT("cyclic owning SM chain is rejected"), CycleTask, CycleSmRaw);

    auto [TwoNodeCycleRaw, TwoNodeCycleTask] = MakeTask();
    auto [FirstCycleSmRaw, FirstCycleSm] = MakeStateMachine(
        ECk_Sm_AuthorityModel::ServerAuthoritative, ECk_Sm_NetContext::Standalone);
    auto [SecondCycleSmRaw, SecondCycleSm] = MakeStateMachine(
        ECk_Sm_AuthorityModel::ServerAuthoritative, ECk_Sm_NetContext::Standalone);
    ck::TUtils_Sm_OwningStateMachine::AddOrReplace(TwoNodeCycleRaw, FirstCycleSm);
    ck::TUtils_Sm_OwningStateMachine::AddOrReplace(FirstCycleSmRaw, SecondCycleSm);
    ck::TUtils_Sm_OwningStateMachine::AddOrReplace(SecondCycleSmRaw, FirstCycleSm);
    TestFalse(TEXT("two-node cyclic owning SM chain is rejected"),
        UCk_Utils_SmTask_UE::Get_CanRunScopedWork(TwoNodeCycleTask));
    TestFalse(TEXT("two-node cycle does not memoize first owner"),
        FirstCycleSmRaw.Has<ck::FFragment_Sm_NetContextMemo>());
    TestFalse(TEXT("two-node cycle does not memoize second owner"),
        SecondCycleSmRaw.Has<ck::FFragment_Sm_NetContextMemo>());

    auto [NonAuthorityRaw, NonAuthorityTask] = MakeTask();
    auto [NonAuthoritySmRaw, NonAuthoritySm] = MakeStateMachine(
        ECk_Sm_AuthorityModel::OwningClientAuthoritative, ECk_Sm_NetContext::NonOwningClient);
    ck::TUtils_Sm_OwningStateMachine::AddOrReplace(NonAuthorityRaw, NonAuthoritySm);
    TestFalse(TEXT("non-authority task is rejected"),
        UCk_Utils_SmTask_UE::Get_CanRunScopedWork(NonAuthorityTask));

    auto [AuthorityRaw, AuthorityTask] = MakeTask();
    auto [AuthoritySmRaw, AuthoritySm] = MakeStateMachine(
        ECk_Sm_AuthorityModel::ServerAuthoritative, ECk_Sm_NetContext::Standalone);
    ck::TUtils_Sm_OwningStateMachine::AddOrReplace(AuthorityRaw, AuthoritySm);
    TestTrue(TEXT("active EnterExitOnly authority task may run scoped work"),
        UCk_Utils_SmTask_UE::Get_CanRunScopedWork(AuthorityTask));

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING
