#include "CkSpatialQuery_Subsystem.h"

#include "CkSpatialQuery/CkSpatialQuery_Log.h"
#include "CkSpatialQuery/CkSpatialQuery_Stats.h"
#include "CkSpatialQuery/Probe/CkProbe_Fragment.h"
#include "CkSpatialQuery/Probe/CkProbe_Utils.h"
#include "CkSpatialQuery/Settings/CkSpatialQuery_Settings.h"

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("JoltPhysics_ProcessQueuedContacts"), STAT_CkSpatialQuery_JoltProcessQueuedContacts, STATGROUP_CkSpatialQuery);
DECLARE_CYCLE_STAT(TEXT("JoltContacts_Added"), STAT_CkSpatialQuery_JoltContactsAdded, STATGROUP_CkSpatialQuery);
DECLARE_CYCLE_STAT(TEXT("JoltContacts_Persisted"), STAT_CkSpatialQuery_JoltContactsPersisted, STATGROUP_CkSpatialQuery);
DECLARE_CYCLE_STAT(TEXT("JoltContacts_Removed"), STAT_CkSpatialQuery_JoltContactsRemoved, STATGROUP_CkSpatialQuery);

DECLARE_DWORD_COUNTER_STAT(TEXT("SpatialQuery Contacts Added"), STAT_CkSpatialQuery_ContactsAdded, STATGROUP_CkSpatialQuery);
DECLARE_DWORD_COUNTER_STAT(TEXT("SpatialQuery Contacts Persisted"), STAT_CkSpatialQuery_ContactsPersisted, STATGROUP_CkSpatialQuery);
DECLARE_DWORD_COUNTER_STAT(TEXT("SpatialQuery Contacts Removed"), STAT_CkSpatialQuery_ContactsRemoved, STATGROUP_CkSpatialQuery);

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

    if (ck::Is_NOT_Valid(_JoltSubsystem))
    { return; }

    // Weak capture: the router registry outlives this subsystem.
    const auto WeakThis = TWeakObjectPtr<UCk_SpatialQuery_Subsystem>{this};
    _JoltSubsystem->RegisterContactRouter(TEXT("SpatialQuery.ProbeBridge"),
        [WeakThis](const TArray<FCk_Jolt_ContactEvent>& InEvents)
        {
            if (auto* Self = WeakThis.Get())
            { Self->ProcessQueuedContacts(InEvents); }
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
    if (_JoltSubsystem.IsValid())
    {
        _JoltSubsystem->UnregisterContactRouter(TEXT("SpatialQuery.ProbeBridge"));
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

    if (InEvents.IsEmpty())
    { return; }

    if (ck::Is_NOT_Valid(_EcsWorldSubsystem))
    { return; }

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
        return TransientEntity.Get_ValidHandle(Entity.Get_ID());
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

                auto Body1 = UCk_Utils_Probe_UE::Cast(Body1Entity);
                auto Body2 = UCk_Utils_Probe_UE::Cast(Body2Entity);

                if (ck::IsValid(Body1) && UCk_Utils_Probe_UE::Get_CanOverlapWith(Body1, Body2))
                {
                    UCk_Utils_Probe_UE::Request_BeginOverlap(Body1,
                        FCk_Request_Probe_BeginOverlap{
                            Body2,
                            Event.ContactPointsOn1,
                            -Event.WorldSpaceNormal,
                            contact_surface::Get_ContactPhysicalMaterial(Body2)
                        });
                }

                if (ck::IsValid(Body2) && UCk_Utils_Probe_UE::Get_CanOverlapWith(Body2, Body1))
                {
                    UCk_Utils_Probe_UE::Request_BeginOverlap(Body2,
                        FCk_Request_Probe_BeginOverlap{
                            Body1,
                            Event.ContactPointsOn2,
                            Event.WorldSpaceNormal,
                            contact_surface::Get_ContactPhysicalMaterial(Body1)
                        });
                }
                break;
            }

            case FCk_Jolt_ContactEvent::EType::Persisted:
            {
                SCOPE_CYCLE_COUNTER(STAT_CkSpatialQuery_JoltContactsPersisted);
                ++PersistedCount;

                auto Body1Entity = ResolveBodyEntity(Event.Body1UserData);
                auto Body2Entity = ResolveBodyEntity(Event.Body2UserData);

                auto Body1 = UCk_Utils_Probe_UE::Cast(Body1Entity);
                auto Body2 = UCk_Utils_Probe_UE::Cast(Body2Entity);

                if (ck::IsValid(Body1) && Body1.Has<ck::FTag_Probe_PersistContacts>()
                    && UCk_Utils_Probe_UE::Get_CanOverlapWith(Body1, Body2))
                {
                    UCk_Utils_Probe_UE::Request_OverlapUpdated(Body1,
                        FCk_Request_Probe_OverlapUpdated{
                            Body2,
                            Event.ContactPointsOn1,
                            -Event.WorldSpaceNormal,
                            contact_surface::Get_ContactPhysicalMaterial(Body2)
                        });
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
                        });
                }
                break;
            }

            case FCk_Jolt_ContactEvent::EType::Removed:
            {
                SCOPE_CYCLE_COUNTER(STAT_CkSpatialQuery_JoltContactsRemoved);
                ++RemovedCount;

                auto Body1Entity = ResolveBodyEntity(Event.Body1UserData);
                auto Body2Entity = ResolveBodyEntity(Event.Body2UserData);

                auto Body1 = UCk_Utils_Probe_UE::Cast(Body1Entity);
                auto Body2 = UCk_Utils_Probe_UE::Cast(Body2Entity);

                if (ck::IsValid(Body1) && UCk_Utils_Probe_UE::Get_CanOverlapWith(Body1, Body2))
                {
                    UCk_Utils_Probe_UE::Request_EndOverlap(Body1, FCk_Request_Probe_EndOverlap{Body2});
                }

                if (ck::IsValid(Body2) && UCk_Utils_Probe_UE::Get_CanOverlapWith(Body2, Body1))
                {
                    UCk_Utils_Probe_UE::Request_EndOverlap(Body2, FCk_Request_Probe_EndOverlap{Body1});
                }
                break;
            }
        }
    }

    SET_DWORD_STAT(STAT_CkSpatialQuery_ContactsAdded, AddedCount);
    SET_DWORD_STAT(STAT_CkSpatialQuery_ContactsPersisted, PersistedCount);
    SET_DWORD_STAT(STAT_CkSpatialQuery_ContactsRemoved, RemovedCount);

    ck::spatialquery::VeryVerbose(TEXT("ProcessQueuedContacts: [{}] events (Added: [{}], Persisted: [{}], Removed: [{}])"),
        InEvents.Num(), AddedCount, PersistedCount, RemovedCount);
}

// --------------------------------------------------------------------------------------------------------------------
