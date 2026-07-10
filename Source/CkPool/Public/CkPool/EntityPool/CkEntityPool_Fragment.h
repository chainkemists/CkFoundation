#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Request/CkRequest_Data.h"
#include "CkEcs/Signal/CkSignal_Fragment.h"
#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkEcs/Signal/CkSignal_Utils.h"

#include "CkPool/EntityPool/CkEntityPool_Fragment_Data.h"

#include "CkRecord/Record/CkRecord_Fragment.h"

#include <UObject/StrongObjectPtr.h>

#include <variant>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_EntityPool_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Pool bookkeeping is deliberately NOT snapshotable (all tags TRANSIENT, no CK_REGISTER_SNAPSHOTABLE):
    // free lists, counters, and parked acquires are runtime-only state. Pooled ENTITIES that round-trip a
    // snapshot come back as plain entities (FFragment_EntityPooled is not registered either) — destroy them
    // normally; never Release them into a pool that no longer tracks them.
    CK_DEFINE_ECS_TAG_TRANSIENT(FTag_EntityPool_NeedsSetup);
    CK_DEFINE_ECS_TAG_TRANSIENT(FTag_EntityPool_PrewarmInProgress);
    CK_DEFINE_ECS_TAG_TRANSIENT(FTag_EntityPool_Dormant);
    CK_DEFINE_ECS_TAG_TRANSIENT(FTag_EntityPool_InUse);

    // world-level pool registry: every EntityPool entity connects here at creation; the record lives on
    // the world's transient entity (transient record — same no-snapshot rationale as the tags above)
    CK_DEFINE_RECORD_OF_ENTITIES_TRANSIENT(FFragment_RecordOfEntityPools, FCk_Handle_EntityPool);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_EntityPool_Params = FCk_Fragment_EntityPool_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------
    // Internal C++-only requests. The BP/AS surface takes the trivial datum directly (see UCk_Utils_EntityPool_UE),
    // so none of these need to be USTRUCTs.

    struct CKPOOL_API FRequest_EntityPool_Acquire : public FRequest_Base
    {
        CK_GENERATED_BODY(FRequest_EntityPool_Acquire);
        CK_REQUEST_DEFINE_DEBUG_NAME(FRequest_EntityPool_Acquire);

    private:
        FCk_Handle _TicketEntity;
        FInstancedStruct _PerUseParams;

    public:
        CK_PROPERTY_GET(_TicketEntity);
        CK_PROPERTY(_PerUseParams);

    public:
        CK_DEFINE_CONSTRUCTORS(FRequest_EntityPool_Acquire, _TicketEntity);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPOOL_API FRequest_EntityPool_Release : public FRequest_Base
    {
        CK_GENERATED_BODY(FRequest_EntityPool_Release);
        CK_REQUEST_DEFINE_DEBUG_NAME(FRequest_EntityPool_Release);

    private:
        FCk_Handle _EntityToRelease;

    public:
        CK_PROPERTY_GET(_EntityToRelease);

    public:
        CK_DEFINE_CONSTRUCTORS(FRequest_EntityPool_Release, _EntityToRelease);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Enqueued by the EntityScript PostConstruction hook when a prewarm/grow instance finishes constructing
    struct CKPOOL_API FRequest_EntityPool_HandleConstructed : public FRequest_Base
    {
        CK_GENERATED_BODY(FRequest_EntityPool_HandleConstructed);
        CK_REQUEST_DEFINE_DEBUG_NAME(FRequest_EntityPool_HandleConstructed);

    private:
        FCk_Handle _ConstructedEntity;

    public:
        CK_PROPERTY_GET(_ConstructedEntity);

    public:
        CK_DEFINE_CONSTRUCTORS(FRequest_EntityPool_HandleConstructed, _ConstructedEntity);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Enqueued when a pooled entity is destroyed EXTERNALLY (i.e. not by the pool's own teardown) so the pool
    // can reconcile its bookkeeping. Dormancy/in-use are captured at enqueue time — the tags are gone by drain time
    struct CKPOOL_API FRequest_EntityPool_HandleDestroyed : public FRequest_Base
    {
        CK_GENERATED_BODY(FRequest_EntityPool_HandleDestroyed);
        CK_REQUEST_DEFINE_DEBUG_NAME(FRequest_EntityPool_HandleDestroyed);

    private:
        FCk_Handle _DestroyedEntity;
        bool _WasDormant = false;
        bool _WasInUse = false;

    public:
        CK_PROPERTY_GET(_DestroyedEntity);
        CK_PROPERTY_GET(_WasDormant);
        CK_PROPERTY_GET(_WasInUse);

    public:
        CK_DEFINE_CONSTRUCTORS(FRequest_EntityPool_HandleDestroyed, _DestroyedEntity, _WasDormant, _WasInUse);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPOOL_API FFragment_EntityPool_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_EntityPool_Requests);

    public:
        friend class FProcessor_EntityPool_HandleRequests;
        friend class UCk_Utils_EntityPool_UE;

    public:
        using RequestType = std::variant<FRequest_EntityPool_Acquire, FRequest_EntityPool_Release,
            FRequest_EntityPool_HandleConstructed, FRequest_EntityPool_HandleDestroyed>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // An acquire that could not be fulfilled from the dormant list — waiting on a grow-spawn or the next Release
    struct CKPOOL_API FEntityPool_PendingAcquireEntry
    {
    public:
        CK_GENERATED_BODY(FEntityPool_PendingAcquireEntry);

    private:
        FCk_Handle _TicketEntity;
        FInstancedStruct _PerUseParams;

    public:
        CK_PROPERTY_GET(_TicketEntity);
        CK_PROPERTY_GET(_PerUseParams);

    public:
        CK_DEFINE_CONSTRUCTORS(FEntityPool_PendingAcquireEntry, _TicketEntity, _PerUseParams);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPOOL_API FFragment_EntityPool_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_EntityPool_Current);

    public:
        friend class FProcessor_EntityPool_Setup;
        friend class FProcessor_EntityPool_Prewarm;
        friend class FProcessor_EntityPool_HandleRequests;
        friend class FProcessor_EntityPool_EndPlay;
        friend class UCk_Utils_EntityPool_UE;

    private:
        TArray<FCk_Handle> _DormantEntities;
        TArray<FEntityPool_PendingAcquireEntry> _PendingAcquires;
        // GC anchor for archetype pools — fragments are not GC-traced, so the construction template
        // must be pinned for the pool's lifetime (set synchronously at pool creation)
        TStrongObjectPtr<UCk_EntityScript_UE> _PinnedArchetype;
        int32 _NumInUse = 0;
        int32 _NumLiveInstances = 0;
        int32 _NumSpawnsInFlight = 0;
        int32 _NumPrewarmRemaining = 0;
        int32 _HighWaterMark = 0;
        int32 _NumHits = 0;
        int32 _NumMisses = 0;

    public:
        CK_PROPERTY_GET(_DormantEntities);
        CK_PROPERTY_GET(_PendingAcquires);
        CK_PROPERTY_GET(_NumInUse);
        CK_PROPERTY_GET(_NumLiveInstances);
        CK_PROPERTY_GET(_NumSpawnsInFlight);
        CK_PROPERTY_GET(_NumPrewarmRemaining);
        CK_PROPERTY_GET(_HighWaterMark);
        CK_PROPERTY_GET(_NumHits);
        CK_PROPERTY_GET(_NumMisses);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Lives on every pooled entity. _UseGeneration increments on every acquire — cache it alongside a pooled-entity
    // handle to detect that the entity was recycled to a new use (pooled entities keep the SAME EnTT id+version
    // across uses, so a stale handle from a previous use still passes ck::IsValid)
    struct CKPOOL_API FFragment_EntityPooled
    {
    public:
        CK_GENERATED_BODY(FFragment_EntityPooled);

    public:
        friend class FProcessor_EntityPool_HandleRequests;
        friend class UCk_Utils_EntityPool_UE;

    private:
        FCk_Handle_EntityPool _OwningPool;
        int32 _UseGeneration = 0;

    public:
        CK_PROPERTY_GET(_OwningPool);
        CK_PROPERTY_GET(_UseGeneration);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_EntityPooled, _OwningPool);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Fired on the acquire TICKET entity (see FCk_Handle_PendingEntityPoolAcquire)
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKPOOL_API, OnEntityPool_AcquireFulfilled, FCk_Delegate_EntityPool_Acquired, FCk_EntityPool_AcquireResult);

    // Fired on the POOLED entity when it is handed out — per-use (re)initialization hook
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKPOOL_API, OnEntityPool_EntityAcquired, FCk_Delegate_EntityPool_EntityAcquired, FCk_Handle, FInstancedStruct);

    // Fired on the POOLED entity when it is returned — quiescence hook (deactivate visuals/audio, cancel per-use
    // timers, unbind per-use signal bindings). The pool does NOT touch feature state; this is the implementer's job
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKPOOL_API, OnEntityPool_EntityReleased, FCk_Delegate_EntityPool_EntityReleased, FCk_Handle);

    // Fired on the POOL when an acquire misses and the pool cannot (or will not) grow
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKPOOL_API, OnEntityPool_Exhausted, FCk_Delegate_EntityPool_Exhausted, FCk_Handle_EntityPool);
}

// --------------------------------------------------------------------------------------------------------------------
