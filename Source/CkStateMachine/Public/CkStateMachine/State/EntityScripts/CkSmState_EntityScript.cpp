#include "CkSmState_EntityScript.h"

#include "CkStateMachine/CkStateMachine_Log.h"
#include "CkStateMachine/Net/CkStateMachine_NetContextUtils.h"
#include "CkStateMachine/Net/CkStateMachine_RepData.h"
#include "CkStateMachine/Net/CkStateMachine_TestSupport.h"
#include "CkStateMachine/Debug/CkStateMachine_Debug_GraphWalk_Fragment.h"
#include "CkStateMachine/State/CkSmState_Fingerprint.h"
#include "CkStateMachine/State/CkSmState_Fragment.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Fragment.h"

#include "CkStateMachine/Task/CkSmTask_Utils.h"
#include "CkStateMachine/Task/EntityScripts/CkSmTask_EntityScript.h"
#include "CkStateMachine/Transition/CkSmTransition_Utils.h"
#include "CkStateMachine/Condition/CkSmCondition_Utils.h"
#include "CkStateMachine/Condition/EntityScripts/CkSmCondition_EntityScript.h"
#include "CkStateMachine/State/CkSmState_Utils.h"
#include "CkStateMachine/StateMachine/CkStateMachine_Utils.h"
#include "CkEcs/ContextOwner/CkContextOwner_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript_Utils.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkCore/GameplayTag/CkGameplayTag_Utils.h"
#include "CkCore/Object/CkObject_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmState_EntityScript::
    Construct(
        FCk_Handle& InHandle,
        const FInstancedStruct& InSpawnParams)
    -> ECk_EntityScript_ConstructionFlow
{
    const auto ParentFlow = Super::Construct(InHandle, InSpawnParams);

    if (ck::TUtils_Sm_OwningStateMachine::Has(InHandle))
    {
        _OwnerStateMachine = ck::TUtils_Sm_OwningStateMachine::Get_StoredEntity(InHandle);
    }

    auto StateHandle = ck::StaticCast<FCk_Handle_SmState_UnderConstruction>(InHandle);

    // Transient scratch used by ComposeFromState to record composed-from classes during
    // DefineState. Removed below once the fingerprint is built. Pre-seeded here so that
    // nested ComposeFromState calls (which call back into DefineState on a different CDO
    // but with this same handle) append into the host state's list rather than try to
    // create the fragment from a possibly-iterating context.
    StateHandle.AddOrGet<ck::FFragment_SmState_ComposedFromInProgress>();

    DefineState(StateHandle);

    DoComputeFingerprint(StateHandle);

    DoVerifyFingerprintAgainstExpected(StateHandle);

    DoBackfillFingerprintToRepData(StateHandle);

    return ParentFlow;
}

auto
    UCk_SmState_EntityScript::
    BeginPlay()
    -> void
{
    Super::BeginPlay();

    // Graph-walk temp entities exist only to let DefineState populate the SM's static
    // structure for the debugger. They must not activate — EnterState stamps
    // FTag_SmState_Active which would admit them to the runtime evaluation loop and
    // cause their tasks' DoEnterTask to run real gameplay side effects on an SM that
    // is not supposed to be advancing.
    if (_AssociatedEntity.Has<ck::FTag_Sm_Debug_GraphWalkEntity>())
    { return; }

    const auto Self = UCk_Utils_SmState_UE::CastChecked(_AssociatedEntity);
    const auto NetContext = ck::statemachine::ComputeNetContext(_OwnerStateMachine);
    EnterState(Self, NetContext);
}

auto
    UCk_SmState_EntityScript::
    EndPlay()
    -> void
{
    // Fallback for cascade-destroyed states whose FProcessor_SmState_Exit never runs
    // (e.g. sub-SM states destroyed via parent task cascade). Dedup'd via FTag_SmState_Active
    // inside ExitState — if the processor already handled this state, the tag is gone and
    // ExitState is a no-op.
    auto Self = UCk_Utils_SmState_UE::CastChecked(_AssociatedEntity);
    if (ck::IsValid(Self))
    {
        const auto NetContext = ck::statemachine::ComputeNetContext(_OwnerStateMachine);
        ExitState(Self, NetContext);
    }
    Super::EndPlay();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmState_EntityScript::
    Get_EffectiveReplication() const
    -> ECk_Replication
{
    // State/condition/task entities are NEVER independent net objects, regardless of whether the
    // owning SM replicates. The SM's transition-history container fragment is the sole server→client
    // transport; non-authority machines rebuild the entire state sub-graph locally via the replay
    // path (FProcessor_Sm_ApplyReplicatedHistory → DoEnterState → Create). Letting these children
    // inherit the CkEntityScript default of Replicates made the server push each state out as an Iris
    // net object too, so non-owning clients reconstructed a second, malformed copy via SpawnProcessor:
    // no FFragment_RecordOfSmTransitions (Create never ran) and an owner ref that resolved to a
    // tombstone — the orphaned initial-state husks. Forcing DoesNotReplicate removes that second path.
    return ECk_Replication::DoesNotReplicate;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmState_EntityScript::
    EnterState(
        FCk_Handle_SmState InHandle,
        ECk_Sm_NetContext InNetContext)
    -> void
{
    ck::sm::VeryVerbose(TEXT("[SM Lifecycle] EnterState [{}] on entity [{}]"), GetClass(), InHandle);

    InHandle.AddOrGet<ck::FTag_SmState_Active>();
    DoEnterState(InHandle, InNetContext);
}

auto
    UCk_SmState_EntityScript::
    ExitState(
        FCk_Handle_SmState InHandle,
        ECk_Sm_NetContext InNetContext)
    -> void
{
    if (NOT InHandle.Has<ck::FTag_SmState_Active>())
    { return; }

    ck::sm::VeryVerbose(TEXT("[SM Lifecycle] ExitState [{}] on entity [{}]"), GetClass(), InHandle);

    InHandle.Try_Remove<ck::FTag_SmState_Active>();
    DoExitState(InHandle, InNetContext);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmState_EntityScript::
    DefineState(
        FCk_Handle_SmState_UnderConstruction& InHandle)
    -> void
{
    DoDefineState(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmState_EntityScript::
    Get_StatesToOverride() const
    -> TArray<FGameplayTag>
{
    return DoGet_StatesToOverride();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmState_EntityScript::
    AddTask(
        FCk_Handle_SmState_UnderConstruction& InStateHandle,
        TSubclassOf<UCk_SmTask_EntityScript> InTaskClass) const
    -> FCk_Handle_SmTask
{
    return UCk_Utils_SmTask_UE::Create(InStateHandle, InTaskClass);
}

auto
    UCk_SmState_EntityScript::
    AddTransition(
        FCk_Handle_SmState_UnderConstruction& InStateHandle,
        TSubclassOf<UCk_SmState_EntityScript> InTargetStateClass) const
    -> FCk_Handle_SmTransition
{
    return UCk_Utils_SmTransition_UE::Create(InStateHandle, InTargetStateClass);
}

auto
    UCk_SmState_EntityScript::
    AddCondition(
        FCk_Handle_SmTransition& InTransition,
        TSubclassOf<UCk_SmCondition_EntityScript> InConditionClass) const
    -> FCk_Handle_SmCondition
{
    return UCk_Utils_SmCondition_UE::Create(InTransition, InConditionClass);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmState_EntityScript::
    ComposeFromState(
        FCk_Handle_SmState_UnderConstruction& InStateHandle,
        TSubclassOf<UCk_SmState_EntityScript> InOtherStateClass) const
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InOtherStateClass),
        TEXT("Invalid OtherStateClass in ComposeFromState on [{}]"), InStateHandle)
    { return; }

    thread_local TSet<UClass*> VisitedClasses;

    CK_ENSURE_IF_NOT(NOT VisitedClasses.Contains(InOtherStateClass.Get()),
        TEXT("Infinite recursion detected in ComposeFromState: [{}] already visited"),
        InOtherStateClass)
    { return; }

    VisitedClasses.Add(InOtherStateClass.Get());
    ON_SCOPE_EXIT { VisitedClasses.Remove(InOtherStateClass.Get()); };

    // Record this compose call against the host state's in-progress fingerprint inputs.
    // The scratch fragment was added by Construct just before invoking DefineState; nested
    // composes append in call order.
    if (InStateHandle.Has<ck::FFragment_SmState_ComposedFromInProgress>())
    {
        auto& InProgress = InStateHandle.Get<ck::FFragment_SmState_ComposedFromInProgress>();
        InProgress._ComposedFromClasses.Add(InOtherStateClass);
    }

    auto* CDO = InOtherStateClass->GetDefaultObject<UCk_SmState_EntityScript>();
    CDO->DefineState(InStateHandle);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmState_EntityScript::
    DoComputeFingerprint(
        FCk_Handle_SmState_UnderConstruction& InStateHandle)
    -> void
{
    auto Inputs = ck::statemachine::FFingerprintInputs{};

    UCk_Utils_StateMachine_UE::RecordOfSmTasks_Utils::ForEach_ValidEntry(InStateHandle,
    [&](FCk_Handle_SmTask InTask) -> ECk_Record_ForEachIterationResult
    {
        Inputs._TaskClasses.Add(UCk_Utils_SmTask_UE::Get_ScriptClass(InTask));
        return ECk_Record_ForEachIterationResult::Continue;
    });

    UCk_Utils_StateMachine_UE::RecordOfSmTransitions_Utils::ForEach_ValidEntry(InStateHandle,
    [&](FCk_Handle_SmTransition InTransition) -> ECk_Record_ForEachIterationResult
    {
        Inputs._TransitionTargetClasses.Add(UCk_Utils_SmTransition_UE::Get_TargetStateClass(InTransition));

        auto& ConditionList = Inputs._TransitionConditionLists.AddDefaulted_GetRef();
        UCk_Utils_StateMachine_UE::RecordOfSmConditions_Utils::ForEach_ValidEntry(InTransition,
        [&](FCk_Handle_SmCondition InCondition) -> ECk_Record_ForEachIterationResult
        {
            ConditionList.Add(UCk_Utils_SmCondition_UE::Get_ScriptClass(InCondition));
            return ECk_Record_ForEachIterationResult::Continue;
        });

        return ECk_Record_ForEachIterationResult::Continue;
    });

    if (InStateHandle.Has<ck::FFragment_SmState_ComposedFromInProgress>())
    {
        const auto& InProgress = InStateHandle.Get<ck::FFragment_SmState_ComposedFromInProgress>();
        Inputs._ComposedFromClasses = InProgress.Get_ComposedFromClasses();
        InStateHandle.Try_Remove<ck::FFragment_SmState_ComposedFromInProgress>();
    }

    auto& FingerprintFrag = InStateHandle.AddOrGet<ck::FFragment_SmState_Fingerprint>();
    FingerprintFrag._Hash = ck::statemachine::ComputeFingerprint(Inputs);

    // Populate the per-class cache so future replications can publish the fingerprint
    // synchronously without waiting for an instance to exist (see Get_CachedFingerprint).
    Set_CachedFingerprint(GetClass(), FingerprintFrag._Hash);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmState_EntityScript::
    DoVerifyFingerprintAgainstExpected(
        FCk_Handle_SmState_UnderConstruction& InStateHandle)
    -> void
{
    if (NOT InStateHandle.Has<ck::FFragment_SmState_ExpectedFingerprint>())
    { return; }

    if (NOT InStateHandle.Has<ck::FFragment_SmState_Fingerprint>())
    { return; }

    const auto Expected = InStateHandle.Get<ck::FFragment_SmState_ExpectedFingerprint>().Get_Hash();
    const auto Local    = InStateHandle.Get<ck::FFragment_SmState_Fingerprint>().Get_Hash();

    // Carrier consumed regardless of match — Construct only runs once per state instance, so
    // there's no second chance to verify and no reason to leave the fragment behind.
    InStateHandle.Try_Remove<ck::FFragment_SmState_ExpectedFingerprint>();

    if (Expected == Local)
    { return; }

    ck::sm::Warning(
        TEXT("Determinism fingerprint mismatch on state [{}] of SM [{}]: expected [{}] from authority, local computed [{}]. ")
        TEXT("Faulting the SM — no further transitions will land. This usually means DefineState diverges between authority and this machine."),
        GetClass(), _OwnerStateMachine, Expected, Local);

    CK_ENSURE_IF_NOT(false,
        TEXT("SM determinism fingerprint mismatch on state [{}] of SM [{}]: expected [{}], local [{}]"),
        GetClass(), _OwnerStateMachine, Expected, Local)
    {
        // Body always runs because cond is false. Mark the SM faulted and exit the state.
        if (ck::IsValid(_OwnerStateMachine))
        {
            _OwnerStateMachine.AddOrGet<ck::FTag_Sm_DeterminismFault>();
        }

        auto SelfState = UCk_Utils_SmState_UE::CastChecked(InStateHandle);
        if (ck::IsValid(SelfState))
        {
            UCk_Utils_SmState_UE::Request_Exit(SelfState);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmState_EntityScript::
    DoBackfillFingerprintToRepData(
        FCk_Handle_SmState_UnderConstruction& InStateHandle)
    -> void
{
    // The replicated payload is published by FProcessor_Sm_CommitPendingTransition (or by the
    // initial-state path at Setup) with _NewStateFingerprint = 0, because at publication time
    // the new state entity hasn't yet run DefineState — fingerprint isn't available.
    // DoComputeFingerprint just ran above; the structural hash is now valid on this state.
    // Patch it into the rep payload so subsequent rep deltas to non-authority clients carry
    // a non-zero fingerprint (useful for inspection / debugger / future synchronous publication).
    //
    // Authority-only: clients receive the payload, they don't write it. Local-only SMs (no
    // Replicates intent) skip the whole branch via the gate below.
    //
    // Race note: clients that already applied a transition with fingerprint=0 won't replay
    // when the backfilled rep delta arrives (Seq dedup). The full verify path requires
    // synchronous fingerprint computation at publication time — that's a follow-up task
    // requiring per-class fingerprint cache via the graph-walk pattern.

    if (NOT ck::IsValid(_OwnerStateMachine))
    { return; }

    if (NOT _OwnerStateMachine.Has<ck::FFragment_Sm_Params>())
    { return; }

    const auto& Params = _OwnerStateMachine.Get<ck::FFragment_Sm_Params>();
    if (Params.Get_Replication() != ECk_Replication::Replicates)
    { return; }

    const auto NetContext = ck::statemachine::ComputeNetContext(_OwnerStateMachine);
    const auto AuthModel  = UCk_Utils_StateMachine_UE::Get_EffectiveAuthorityModel(_OwnerStateMachine);
    const auto IsAuthority =
        (NetContext == ECk_Sm_NetContext::Server
            && AuthModel == ECk_Sm_AuthorityModel::ServerAuthoritative)
        || (NetContext == ECk_Sm_NetContext::OwningClient
            && AuthModel == ECk_Sm_AuthorityModel::OwningClientAuthoritative);

    if (NOT IsAuthority)
    { return; }

    if (NOT InStateHandle.Has<ck::FFragment_SmState_Fingerprint>())
    { return; }

    auto LocalFingerprint = InStateHandle.Get<ck::FFragment_SmState_Fingerprint>().Get_Hash();
    const auto SelfClass        = GetClass();

#if WITH_DEV_AUTOMATION_TESTS
    // Test-only fake-fingerprint injection for spec §13 test 9 (initial-state mismatch).
    // The InitialState scope replaces the LocalFingerprint reaching the rep payload so the
    // FlushPendingReplication_InitialCheck pass on the receive side observes a divergence.
    if (_OwnerStateMachine.Has<ck::FFragment_Sm_TestFakeFingerprintInjection>())
    {
        auto& Injection = _OwnerStateMachine.Get<ck::FFragment_Sm_TestFakeFingerprintInjection>();
        if (Injection.Get_Scope() == ECk_Sm_TestFakeFingerprintScope::InitialState)
        {
            LocalFingerprint = Injection.Get_FakeFingerprint();
            if (Injection.Get_ConsumeOnUse())
            { _OwnerStateMachine.Try_Remove<ck::FFragment_Sm_TestFakeFingerprintInjection>(); }
        }
    }
#endif

    switch (Params.Get_ReplicationModel())
    {
        case ECk_Sm_ReplicationModel::WithHistory:
        {
            UCk_Utils_Net_UE::TryUpdateContainerFragment<FCk_RepData_StateMachine_WithHistory>(
                _OwnerStateMachine,
                [&](FCk_RepData_StateMachine_WithHistory& RepData) -> void
                {
                    auto& History = RepData.Get_History();
                    if (History.IsEmpty())
                    {
                        // Initial state — no transition events yet. Stamp the seed slot.
                        RepData.Set_InitialStateFingerprint(LocalFingerprint);
                        return;
                    }

                    // Patch the most recent event only if its NewStateClass actually matches our
                    // class. Defensive guard against the race where a follow-up transition
                    // commits and publishes its own event before this Construct callback fires —
                    // in that case the back-patch would write to the wrong event. The cost of
                    // skipping under that race is a single zero fingerprint left in the ring,
                    // which is benign for the (currently informational) verify path.
                    auto& Latest = History.Last();
                    if (Latest.Get_NewStateClass() == SelfClass)
                    {
                        Latest.Set_NewStateFingerprint(LocalFingerprint);
                    }
                });
            break;
        }
        case ECk_Sm_ReplicationModel::WithoutHistory:
        {
            UCk_Utils_Net_UE::TryUpdateContainerFragment<FCk_RepData_StateMachine_NoHistory>(
                _OwnerStateMachine,
                [&](FCk_RepData_StateMachine_NoHistory& RepData) -> void
                {
                    // Only patch when the rep payload's CurrentStateClass matches our class.
                    // If the SM has already transitioned past this state (faster authority),
                    // the payload reflects a different state and we shouldn't overwrite.
                    if (RepData.Get_CurrentStateClass() == SelfClass)
                    {
                        RepData.Set_CurrentStateFingerprint(LocalFingerprint);

                        // Initial-state seed too — covers the WithoutHistory analogue of the
                        // WithHistory.IsEmpty() branch above. If the initial-state Setup never
                        // populated the field, Seq is still 0 and we're patching alongside.
                    }
                });
            break;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmState_EntityScript::
    RemoveTask(
        FCk_Handle_SmState_UnderConstruction& InStateHandle,
        TSubclassOf<UCk_SmTask_EntityScript> InTaskClass) const
    -> bool
{
    auto MaybeSmTask = UCk_Utils_StateMachine_UE::RecordOfSmTasks_Utils::Get_ValidEntry_If(
    InStateHandle,
    [&](const FCk_Handle_SmTask& Entry)
    {
        return InTaskClass == UCk_Utils_SmTask_UE::Get_ScriptClass(Entry);
    });

    CK_ENSURE_IF_NOT(ck::IsValid(MaybeSmTask),
        TEXT("RemoveTask: no task of class [{}] found in state [{}]"),
        InTaskClass, InStateHandle)
    { return false; }

    UCk_Utils_StateMachine_UE::RecordOfSmTasks_Utils::Request_Disconnect(InStateHandle, MaybeSmTask);

    // Cancel any pending script attach so the commit processor cannot race this destroy.
    // CK_IGNORE_PENDING_KILL does not exclude FTag_DestroyEntity_Initiate, so without
    // stripping these the commit would still fire Construct/BeginPlay on a doomed entity.
    MaybeSmTask.Try_Remove<ck::FTag_SmScript_PendingAttach>();
    MaybeSmTask.Try_Remove<ck::FFragment_SmScript_PendingAttach>();

    UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(MaybeSmTask);

    return true;
}

auto
    UCk_SmState_EntityScript::
    ReplaceTask(
        FCk_Handle_SmState_UnderConstruction& InStateHandle,
        TSubclassOf<UCk_SmTask_EntityScript> InOldTaskClass,
        TSubclassOf<UCk_SmTask_EntityScript> InNewTaskClass) const
    -> FCk_Handle_SmTask
{
    CK_ENSURE_IF_NOT(RemoveTask(InStateHandle, InOldTaskClass),
        TEXT("ReplaceTask: remove failed for [{}] in state [{}]"),
        InOldTaskClass, InStateHandle)
    { return {}; }

    return AddTask(InStateHandle, InNewTaskClass);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmState_EntityScript::
    Get_OwnerStateMachine() const
    -> FCk_Handle_StateMachine
{
    return _OwnerStateMachine;
}

auto
    UCk_SmState_EntityScript::
    Get_StateMachineContext() const
    -> FCk_Handle
{
    return UCk_Utils_ContextOwner_UE::Get_ContextOwner(DoGet_ScriptEntity());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmState_EntityScript::
    Get_StateTag() const
    -> FGameplayTag
{
    return UCk_Utils_Object_UE::Get_TagFromClassName(
        GetClass(),
        ck::Format_UE(TEXT("Auto-generated state tag for {}"), GetClass()));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmState_EntityScript::
    Get_StateTagForClass(
        TSubclassOf<UCk_SmState_EntityScript> InClass)
    -> FGameplayTag
{
    CK_ENSURE_IF_NOT(ck::IsValid(InClass),
        TEXT("Invalid state class in Get_StateTagForClass"))
    { return {}; }

    return UCk_Utils_Object_UE::Get_TagFromClassName(
        InClass,
        ck::Format_UE(TEXT("Auto-generated state tag for {}"), InClass));
}

// --------------------------------------------------------------------------------------------------------------------

// Static cache shared across the module. FName-keyed (not UClass*-keyed) so hot reload + PIE
// restart cycles that recreate UClass objects don't leave dangling keys — a stale FName just
// re-maps to the new class on next compute. Single-threaded: all CkFoundation processor work
// runs on the main thread.
static TMap<FName, int32> GSmStateFingerprintCache;

auto
    UCk_SmState_EntityScript::
    Get_CachedFingerprint(
        TSubclassOf<UCk_SmState_EntityScript> InClass)
    -> int32
{
    if (ck::Is_NOT_Valid(InClass))
    { return 0; }

    const auto* Found = GSmStateFingerprintCache.Find(InClass->GetFName());
    return Found != nullptr ? *Found : 0;
}

auto
    UCk_SmState_EntityScript::
    Set_CachedFingerprint(
        TSubclassOf<UCk_SmState_EntityScript> InClass,
        int32 InFingerprint)
    -> void
{
    if (ck::Is_NOT_Valid(InClass))
    { return; }

    GSmStateFingerprintCache.Add(InClass->GetFName(), InFingerprint);
}

// --------------------------------------------------------------------------------------------------------------------
