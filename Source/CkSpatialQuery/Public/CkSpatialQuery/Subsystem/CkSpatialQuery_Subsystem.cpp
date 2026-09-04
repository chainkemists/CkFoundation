#include "CkSpatialQuery_Subsystem.h"

#include "CkEcs/Registry/CkRegistry.h"

#include "CkSpatialQuery/CkSpatialQuery_Log.h"
#include "CkSpatialQuery/CkSpatialQuery_Stats.h"
#include "CkSpatialQuery/Probe/CkProbe_Fragment.h"
#include "CkSpatialQuery/Probe/CkProbeContactFilter.h"
#include "CkSpatialQuery/Probe/CkProbe_Utils.h"
#include "CkSpatialQuery/Settings/CkSpatialQuery_Settings.h"

#include <HAL/IConsoleManager.h>

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("JoltPhysics_ProcessQueuedContacts"), STAT_CkSpatialQuery_JoltProcessQueuedContacts, STATGROUP_CkSpatialQuery);
DECLARE_CYCLE_STAT(TEXT("JoltContacts_Added"), STAT_CkSpatialQuery_JoltContactsAdded, STATGROUP_CkSpatialQuery);
DECLARE_CYCLE_STAT(TEXT("JoltContacts_Persisted"), STAT_CkSpatialQuery_JoltContactsPersisted, STATGROUP_CkSpatialQuery);
DECLARE_CYCLE_STAT(TEXT("JoltContacts_Removed"), STAT_CkSpatialQuery_JoltContactsRemoved, STATGROUP_CkSpatialQuery);

DECLARE_DWORD_COUNTER_STAT(TEXT("SpatialQuery Contacts Added"), STAT_CkSpatialQuery_ContactsAdded, STATGROUP_CkSpatialQuery);
DECLARE_DWORD_COUNTER_STAT(TEXT("SpatialQuery Contacts Persisted"), STAT_CkSpatialQuery_ContactsPersisted, STATGROUP_CkSpatialQuery);
DECLARE_DWORD_COUNTER_STAT(TEXT("SpatialQuery Contacts Removed"), STAT_CkSpatialQuery_ContactsRemoved, STATGROUP_CkSpatialQuery);

// --------------------------------------------------------------------------------------------------------------------

static TAutoConsoleVariable<int32> CVarProbePairAttributionFrames(
    TEXT("ck.SpatialQuery.ProbePairAttributionFrames"),
    0,
    TEXT("Collect and dump exact Probe-body contact pairs for N drained-event frames, then disable.\n")
    TEXT("Default 0 is off. Diagnostic only; values above 0 add bounded game-thread aggregation cost."),
    ECVF_Cheat);

namespace probe_pair_attribution
{
    constexpr int32 PairCapacity = 128;
    constexpr int32 DumpRowCount = 20;
}

// --------------------------------------------------------------------------------------------------------------------

namespace contact_surface
{
    auto Get_ContactPhysicalMaterial(const FCk_Handle_Probe& InProbe) -> UPhysicalMaterial*
    {
        const auto& SurfaceInfo = UCk_Utils_Probe_UE::Get_SurfaceInfo(InProbe);

        switch (const auto& PhysicalMaterialSource = SurfaceInfo.Get_PhysicalMaterialSource())
        {
            case ECk_PhysicalMaterialSource::Direct:
            {
                return SurfaceInfo.Get_PhysicalMaterial().Get();
            }
            case ECk_PhysicalMaterialSource::Trace:
            {
                CK_TRIGGER_ENSURE(TEXT("Probe Physical Material Source [{}] is NOT supported yet"), PhysicalMaterialSource);
                return {};
            }
            default:
            {
                CK_INVALID_ENUM(PhysicalMaterialSource);
                return {};
            }
        }
    };
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SpatialQuery_Subsystem::
    Initialize(
        FSubsystemCollectionBase& InCollection)
        -> void
{
    Super::Initialize(InCollection);

    _EcsWorldSubsystem = InCollection.InitializeDependency<UCk_EcsWorld_Subsystem_UE>();
    _JoltSubsystem = InCollection.InitializeDependency<UCk_Jolt_Subsystem>();

    if (ck::Is_NOT_Valid(_EcsWorldSubsystem) || ck::Is_NOT_Valid(_JoltSubsystem))
    { return; }

    _EcsWorldSubsystem->Get_Registry().SetContext<ck::spatialquery::FCk_ProbeContactFilter_Context>();

    // Weak capture: the router registry outlives this subsystem.
    const auto WeakThis = TWeakObjectPtr<UCk_SpatialQuery_Subsystem>{this};
    _JoltSubsystem->RegisterContactRouter(TEXT("SpatialQuery.ProbeBridge"),
        [WeakThis](const TArray<FCk_Jolt_ContactEvent>& InEvents)
        {
            if (auto* Self = WeakThis.Get())
            { Self->ProcessQueuedContacts(InEvents); }
        });

    // Probes opt into Persisted contacts implicitly (BindTo_OnOverlapUpdated adds the tag, the last
    // unbind removes it), so this is re-asked every frame rather than refcounted.
    _JoltSubsystem->Register_PersistedContactInterestProvider(TEXT("SpatialQuery.ProbePersistContacts"),
        [WeakThis]() -> bool
        {
            auto* Self = WeakThis.Get();
            if (Self == nullptr || ck::Is_NOT_Valid(Self->_EcsWorldSubsystem))
            { return false; }

            return Self->_EcsWorldSubsystem->Get_Registry().Has_AnyLiveEntityWith<ck::FTag_Probe_PersistContacts>();
        });

    _JoltSubsystem->Set_DebugDrawGate([]()
    {
        return UCk_Utils_SpatialQuery_Settings::Get_DebugPreviewAllProbesUsingJolt();
    });
}

auto
    UCk_SpatialQuery_Subsystem::
    Deinitialize()
        -> void
{
    _ProbePairAttribution.Reset();
    _ProbePairAttributionFramesRemaining = 0;
    _ProbePairAttributionFramesObserved = 0;
    _ProbePairAttributionAdded = 0;
    _ProbePairAttributionPersisted = 0;
    _ProbePairAttributionRemoved = 0;
    _ProbePairAttributionDroppedEvents = 0;

    if (_JoltSubsystem.IsValid())
    {
        _JoltSubsystem->UnregisterContactRouter(TEXT("SpatialQuery.ProbeBridge"));
        _JoltSubsystem->Unregister_PersistedContactInterestProvider(TEXT("SpatialQuery.ProbePersistContacts"));
        _JoltSubsystem->Set_DebugDrawGate({});
    }

    Super::Deinitialize();
}

// Runs on the game thread inside UCk_Jolt_Subsystem::Tick's drained-events broadcast, so ECS
// mutations here are safe.
auto
    UCk_SpatialQuery_Subsystem::
    ProcessQueuedContacts(
        const TArray<FCk_Jolt_ContactEvent>& InEvents)
        -> void
{
    SCOPE_CYCLE_COUNTER(STAT_CkSpatialQuery_JoltProcessQueuedContacts);

    const auto RequestedAttributionFrames = FMath::Max(0, CVarProbePairAttributionFrames.GetValueOnGameThread());
    if (InEvents.IsEmpty() && RequestedAttributionFrames == 0 && _ProbePairAttributionFramesRemaining == 0)
    { return; }

    if (ck::Is_NOT_Valid(_EcsWorldSubsystem))
    { return; }

    if (RequestedAttributionFrames == 0 && _ProbePairAttributionFramesRemaining > 0)
    {
        _ProbePairAttribution.Reset();
        _ProbePairAttributionFramesRemaining = 0;
        _ProbePairAttributionFramesObserved = 0;
        _ProbePairAttributionAdded = 0;
        _ProbePairAttributionPersisted = 0;
        _ProbePairAttributionRemoved = 0;
        _ProbePairAttributionDroppedEvents = 0;
    }
    else if (RequestedAttributionFrames > 0 && _ProbePairAttributionFramesRemaining == 0)
    {
        _ProbePairAttribution.Reset();
        _ProbePairAttributionFramesRemaining = RequestedAttributionFrames;
        _ProbePairAttributionFramesObserved = 0;
        _ProbePairAttributionAdded = 0;
        _ProbePairAttributionPersisted = 0;
        _ProbePairAttributionRemoved = 0;
        _ProbePairAttributionDroppedEvents = 0;
    }

    const auto IsAttributingProbePairs = _ProbePairAttributionFramesRemaining > 0;

    const auto TransientEntity = _EcsWorldSubsystem->Get_TransientEntity();
    const auto RegView = TransientEntity.Get_RegistryView();

    // The RegView.IsValid pre-check is load-bearing: body UserData is a raw entity id baked in at
    // registration, a snapshot load can kill it, and Get_ValidHandle ENSURES on a stale id before
    // the ck::IsValid(Body) guards below could absorb it.
    const auto ResolveBodyEntity = [&](uint64 InUserData) -> FCk_Handle
    {
        const auto Entity = FCk_Entity{FCk_Entity::IdType{static_cast<FCk_Entity::IdType>(InUserData)}};
        if (NOT RegView.IsValid(Entity))
        { return {}; }

        auto Handle = TransientEntity.Get_ValidHandle(Entity.Get_ID());

        // Resolved by id, so no view exclusion applies, and this router ENQUEUES overlap requests onto the
        // probes it resolves. Answering invalid for an entity a load is still holding keeps a restored probe
        // out of an overlap set whose other end is mid-restore; the ck::IsValid guards below absorb it.
        if (Handle.Has<ck::FTag_Hydration_Quarantine>())
        { return {}; }

        return Handle;
    };

    // An entity can own multiple Jolt bodies which all carry the same entity id as UserData. Only the
    // body's index+sequence identifies whether this contact belongs to that entity's Probe body.
    const auto ResolveProbeBody = [](FCk_Handle InEntity, uint32 InBodyIndexAndSeq) -> FCk_Handle_Probe
    {
        auto Probe = UCk_Utils_Probe_UE::Cast(InEntity);
        if (ck::Is_NOT_Valid(Probe))
        { return {}; }

        if (Probe.Get<ck::FFragment_Probe_Current>().Get_BodyId().GetIndexAndSequenceNumber() != InBodyIndexAndSeq)
        { return {}; }

        return Probe;
    };

    const auto RecordProbePair = [&](const FCk_Handle_Probe& InBody1, const FCk_Handle_Probe& InBody2,
                                     FCk_Jolt_ContactEvent::EType InEventType) -> void
    {
        if (NOT IsAttributingProbePairs || ck::Is_NOT_Valid(InBody1) || ck::Is_NOT_Valid(InBody2))
        { return; }

        auto Name1 = UCk_Utils_Probe_UE::Get_Name(InBody1).GetTagName();
        auto Name2 = UCk_Utils_Probe_UE::Get_Name(InBody2).GetTagName();
        if (Name2.LexicalLess(Name1))
        { Swap(Name1, Name2); }

        const auto Key = ck_spatial_query::diagnostics::FProbePairKey{Name1, Name2};
        auto* Counts = _ProbePairAttribution.Find(Key);
        if (Counts == nullptr)
        {
            if (_ProbePairAttribution.Num() >= probe_pair_attribution::PairCapacity)
            {
                ++_ProbePairAttributionDroppedEvents;
                return;
            }

            Counts = &_ProbePairAttribution.Add(Key);
        }

        switch (InEventType)
        {
            case FCk_Jolt_ContactEvent::EType::Added:
                ++Counts->Added;
                ++_ProbePairAttributionAdded;
                break;
            case FCk_Jolt_ContactEvent::EType::Persisted:
                ++Counts->Persisted;
                ++_ProbePairAttributionPersisted;
                break;
            case FCk_Jolt_ContactEvent::EType::Removed:
                ++Counts->Removed;
                ++_ProbePairAttributionRemoved;
                break;
        }
    };

    int32 AddedCount = 0;
    int32 PersistedCount = 0;
    int32 RemovedCount = 0;

    for (const auto& Event : InEvents)
    {
        switch (Event.Type)
        {
            case FCk_Jolt_ContactEvent::EType::Added:
            {
                SCOPE_CYCLE_COUNTER(STAT_CkSpatialQuery_JoltContactsAdded);
                ++AddedCount;

                auto Body1Entity = ResolveBodyEntity(Event.Body1UserData);
                auto Body2Entity = ResolveBodyEntity(Event.Body2UserData);
                auto Body1 = ResolveProbeBody(Body1Entity, Event.Body1IndexAndSeq);
                auto Body2 = ResolveProbeBody(Body2Entity, Event.Body2IndexAndSeq);
                RecordProbePair(Body1, Body2, Event.Type);

                if (ck::Is_NOT_Valid(Body1) || ck::Is_NOT_Valid(Body2)
                    || UCk_Utils_Probe_UE::Get_IsEnabledDisabled(Body1) != ECk_EnableDisable::Enable
                    || UCk_Utils_Probe_UE::Get_IsEnabledDisabled(Body2) != ECk_EnableDisable::Enable)
                { break; }

                if (ck::IsValid(Body1) && UCk_Utils_Probe_UE::Get_CanOverlapWith(Body1, Body2))
                {
                    UCk_Utils_Probe_UE::Request_BeginOverlap(Body1,
                        FCk_Request_Probe_BeginOverlap{
                            Body2,
                            Event.ContactPointsOn1,
                            -Event.WorldSpaceNormal,
                            contact_surface::Get_ContactPhysicalMaterial(Body2)
                        }, {});
                }

                if (ck::IsValid(Body2) && UCk_Utils_Probe_UE::Get_CanOverlapWith(Body2, Body1))
                {
                    UCk_Utils_Probe_UE::Request_BeginOverlap(Body2,
                        FCk_Request_Probe_BeginOverlap{
                            Body1,
                            Event.ContactPointsOn2,
                            Event.WorldSpaceNormal,
                            contact_surface::Get_ContactPhysicalMaterial(Body1)
                        }, {});
                }
                break;
            }

            case FCk_Jolt_ContactEvent::EType::Persisted:
            {
                SCOPE_CYCLE_COUNTER(STAT_CkSpatialQuery_JoltContactsPersisted);
                ++PersistedCount;

                auto Body1Entity = ResolveBodyEntity(Event.Body1UserData);
                auto Body2Entity = ResolveBodyEntity(Event.Body2UserData);
                auto Body1 = ResolveProbeBody(Body1Entity, Event.Body1IndexAndSeq);
                auto Body2 = ResolveProbeBody(Body2Entity, Event.Body2IndexAndSeq);
                RecordProbePair(Body1, Body2, Event.Type);

                if (ck::Is_NOT_Valid(Body1) || ck::Is_NOT_Valid(Body2)
                    || UCk_Utils_Probe_UE::Get_IsEnabledDisabled(Body1) != ECk_EnableDisable::Enable
                    || UCk_Utils_Probe_UE::Get_IsEnabledDisabled(Body2) != ECk_EnableDisable::Enable)
                { break; }

                if (ck::IsValid(Body1) && Body1.Has<ck::FTag_Probe_PersistContacts>()
                    && UCk_Utils_Probe_UE::Get_CanOverlapWith(Body1, Body2))
                {
                    UCk_Utils_Probe_UE::Request_OverlapUpdated(Body1,
                        FCk_Request_Probe_OverlapUpdated{
                            Body2,
                            Event.ContactPointsOn1,
                            -Event.WorldSpaceNormal,
                            contact_surface::Get_ContactPhysicalMaterial(Body2)
                        }, {});
                }

                if (ck::IsValid(Body2) && Body2.Has<ck::FTag_Probe_PersistContacts>()
                    && UCk_Utils_Probe_UE::Get_CanOverlapWith(Body2, Body1))
                {
                    UCk_Utils_Probe_UE::Request_OverlapUpdated(Body2,
                        FCk_Request_Probe_OverlapUpdated{
                            Body1,
                            Event.ContactPointsOn2,
                            Event.WorldSpaceNormal,
                            contact_surface::Get_ContactPhysicalMaterial(Body1)
                        }, {});
                }
                break;
            }

            case FCk_Jolt_ContactEvent::EType::Removed:
            {
                SCOPE_CYCLE_COUNTER(STAT_CkSpatialQuery_JoltContactsRemoved);
                ++RemovedCount;

                auto Body1Entity = ResolveBodyEntity(Event.Body1UserData);
                auto Body2Entity = ResolveBodyEntity(Event.Body2UserData);

                auto Body1 = ResolveProbeBody(Body1Entity, Event.Body1IndexAndSeq);
                auto Body2 = ResolveProbeBody(Body2Entity, Event.Body2IndexAndSeq);
                RecordProbePair(Body1, Body2, Event.Type);

                // Probe bodies keep their identity through shape updates and enable/disable. Teardown explicitly
                // clears peers before destruction, so both exact IDs are required here as well; entity fallback
                // would let a sibling rigid body's Removed event end a still-live Probe pair.
                if (ck::Is_NOT_Valid(Body1) || ck::Is_NOT_Valid(Body2))
                { break; }

                if (UCk_Utils_Probe_UE::Get_IsOverlappingWith(Body1, Body2))
                {
                    UCk_Utils_Probe_UE::Request_EndOverlap(Body1, FCk_Request_Probe_EndOverlap{Body2}, {});
                }

                if (UCk_Utils_Probe_UE::Get_IsOverlappingWith(Body2, Body1))
                {
                    UCk_Utils_Probe_UE::Request_EndOverlap(Body2, FCk_Request_Probe_EndOverlap{Body1}, {});
                }
                break;
            }
        }
    }

    SET_DWORD_STAT(STAT_CkSpatialQuery_ContactsAdded, AddedCount);
    SET_DWORD_STAT(STAT_CkSpatialQuery_ContactsPersisted, PersistedCount);
    SET_DWORD_STAT(STAT_CkSpatialQuery_ContactsRemoved, RemovedCount);

    if (IsAttributingProbePairs)
    {
        ++_ProbePairAttributionFramesObserved;
        --_ProbePairAttributionFramesRemaining;

        if (_ProbePairAttributionFramesRemaining == 0)
        {
            using FRow = TPair<ck_spatial_query::diagnostics::FProbePairKey,
                ck_spatial_query::diagnostics::FProbePairCounts>;
            auto Rows = TArray<FRow>{};
            Rows.Reserve(_ProbePairAttribution.Num());
            for (const auto& Pair : _ProbePairAttribution)
            { Rows.Emplace(Pair.Key, Pair.Value); }

            Rows.Sort([](const FRow& InA, const FRow& InB)
            {
                const auto TotalA = InA.Value.Get_Total();
                const auto TotalB = InB.Value.Get_Total();
                if (TotalA != TotalB)
                { return TotalA > TotalB; }
                if (InA.Key.A != InB.Key.A)
                { return InA.Key.A.LexicalLess(InB.Key.A); }
                return InA.Key.B.LexicalLess(InB.Key.B);
            });

            ck::spatialquery::Display(
                TEXT("[ProbePairAttribution] frames={} distinctPairs={} added={} persisted={} removed={} droppedEvents={}"),
                _ProbePairAttributionFramesObserved,
                Rows.Num(),
                _ProbePairAttributionAdded,
                _ProbePairAttributionPersisted,
                _ProbePairAttributionRemoved,
                _ProbePairAttributionDroppedEvents);

            const auto RowsToDump = FMath::Min(probe_pair_attribution::DumpRowCount, Rows.Num());
            for (int32 Rank = 0; Rank < RowsToDump; ++Rank)
            {
                const auto& Row = Rows[Rank];
                ck::spatialquery::Display(
                    TEXT("[ProbePairAttribution] rank={} pair=[{}] <-> [{}] added={} persisted={} removed={} total={}"),
                    Rank + 1,
                    Row.Key.A,
                    Row.Key.B,
                    Row.Value.Added,
                    Row.Value.Persisted,
                    Row.Value.Removed,
                    Row.Value.Get_Total());
            }

            CVarProbePairAttributionFrames->Set(0, ECVF_SetByConsole);
            _ProbePairAttribution.Reset();
            _ProbePairAttributionFramesObserved = 0;
            _ProbePairAttributionAdded = 0;
            _ProbePairAttributionPersisted = 0;
            _ProbePairAttributionRemoved = 0;
            _ProbePairAttributionDroppedEvents = 0;
        }
    }

    ck::spatialquery::VeryVerbose(TEXT("ProcessQueuedContacts: [{}] events (Added: [{}], Persisted: [{}], Removed: [{}])"),
        InEvents.Num(), AddedCount, PersistedCount, RemovedCount);
}

// --------------------------------------------------------------------------------------------------------------------
