#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

#include "CkEcs/EntityScript/CkEntityScript_Fragment_Data.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Tag/CkTag.h"

#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "StructUtils/InstancedStruct.h"
#include "UObject/StrongObjectPtr.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_EntityScript_UE;
class UCk_Utils_EntityScript_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_EntityScript_ContinueConstruction);
    CK_DEFINE_ECS_TAG(FTag_EntityScript_FinishConstruction);
    CK_DEFINE_ECS_TAG(FTag_EntityScript_BeginPlay);
    CK_DEFINE_ECS_TAG(FTag_EntityScript_HasBegunPlay);
    CK_DEFINE_ECS_TAG(FTag_EntityScript_HasEndedPlay);
    CK_DEFINE_ECS_TAG(FTag_EntityScript_PendingReplicationRetry);

    // Sticky tag. Added by the spawn processor when at least one CkInject
    // site fails to resolve; removed by the finish-deferred-construct path
    // (callback fired all sites resolved) or by the deadline processor
    // (timeout cleanup). The deadline processor uses SkipPump so the tag
    // staying sticky doesn't multiply work.
    CK_DEFINE_ECS_TAG(FTag_EntityScript_AwaitingDependencies);

    // Per-entity bookkeeping for the deferred Construct path. Carries the
    // script + start timestamp + timeout + the original spawn params so the
    // finish-deferred-construct path can replay the standard Construct flow
    // (NetParams ensure, replication driver, Construct switch, post-construct
    // replication block).
    struct CKECS_API FFragment_EntityScript_AwaitingDependencies
    {
    public:
        CK_GENERATED_BODY(FFragment_EntityScript_AwaitingDependencies);

    public:
        FFragment_EntityScript_AwaitingDependencies() = default;

        FFragment_EntityScript_AwaitingDependencies(
            UCk_EntityScript_UE* InScript,
            FCk_Time             InStartedAt,
            FCk_Handle           InLifetimeOwner,
            FInstancedStruct     InOriginalSpawnParams,
            FCk_EntityScript_PostConstruction_Func InPostConstruction);

    private:
        // StrongObjectPtr — keep the script alive while we're deferring its
        // Construct. The script's Outer (the world) is the eventual lifetime
        // anchor, but during the awaiting-dependencies window the entity is
        // the only reference path.
        TStrongObjectPtr<UCk_EntityScript_UE>   _Script;
        FCk_Time                                _StartedAt;

        // Carried so DoFinishConstructionFlow can replay the post-injection
        // path with the same inputs the immediate-construct path saw.
        FCk_Handle                              _LifetimeOwner;
        FInstancedStruct                        _OriginalSpawnParams;
        FCk_EntityScript_PostConstruction_Func  _PostConstruction_Func;

    public:
        CK_PROPERTY_GET(_Script);
        CK_PROPERTY_GET(_StartedAt);
        CK_PROPERTY_GET(_LifetimeOwner);
        CK_PROPERTY_GET(_OriginalSpawnParams);
        CK_PROPERTY_GET(_PostConstruction_Func);
    };

    // Request fragment fired by the resolution-callback path when the last
    // pending dependency resolves and all sites on the script are now
    // satisfied. The finish processor consumes it and replays the standard
    // Construct pipeline via UCk_Utils_EntityScript_UE::DoFinishConstructionFlow.
    struct CKECS_API FRequest_EntityScript_FinishDeferredConstruct
    {
    public:
        CK_GENERATED_BODY(FRequest_EntityScript_FinishDeferredConstruct);

    public:
        FRequest_EntityScript_FinishDeferredConstruct() = default;

        FRequest_EntityScript_FinishDeferredConstruct(
            UCk_EntityScript_UE* InScript,
            FCk_Handle           InLifetimeOwner,
            FInstancedStruct     InOriginalSpawnParams,
            FCk_EntityScript_PostConstruction_Func InPostConstruction);

    private:
        TWeakObjectPtr<UCk_EntityScript_UE>     _Script;
        FCk_Handle                              _LifetimeOwner;
        FInstancedStruct                        _OriginalSpawnParams;
        FCk_EntityScript_PostConstruction_Func  _PostConstruction_Func;

    public:
        CK_PROPERTY_GET(_Script);
        CK_PROPERTY_GET(_LifetimeOwner);
        CK_PROPERTY_GET(_OriginalSpawnParams);
        CK_PROPERTY_GET(_PostConstruction_Func);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct FFragment_EntityScript_PendingReplicationRetryTimestamp
    {
    public:
        CK_GENERATED_BODY(FFragment_EntityScript_PendingReplicationRetryTimestamp);

    private:
        FCk_Time _TaggedAt;

    public:
        CK_PROPERTY_GET(_TaggedAt);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_EntityScript_PendingReplicationRetryTimestamp, _TaggedAt);
    };

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_EntityScript_RequestSpawnEntity = FCk_Request_EntityScript_SpawnEntity;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKECS_API FFragment_EntityScript_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_EntityScript_Current);

    public:
        friend class UCk_Utils_EntityScript_UE;

    public:
        FFragment_EntityScript_Current() = default;

        explicit
        FFragment_EntityScript_Current(
            UCk_EntityScript_UE* InScript);

    private:
        TStrongObjectPtr<UCk_EntityScript_UE> _Script;

    public:
        CK_PROPERTY_GET(_Script);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKECS_API FRequest_EntityScript_Replicate
    {
    public:
        CK_GENERATED_BODY(FRequest_EntityScript_Replicate);

    public:
        FRequest_EntityScript_Replicate() = default;
        FRequest_EntityScript_Replicate(
            const FCk_Handle& InOwner,
            const FInstancedStruct& InSpawnParams,
            UCk_EntityScript_UE* InScript);

    private:
        FCk_Handle _Owner;
        FInstancedStruct _SpawnParams;
        TWeakObjectPtr<UCk_EntityScript_UE> _Script;

    public:
        CK_PROPERTY_GET(_Owner);
        CK_PROPERTY_GET(_SpawnParams);
        CK_PROPERTY_GET(_Script);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKECS_API,
        OnConstructed,
        FCk_Delegate_EntityScript_Constructed,
        FCk_Handle_EntityScript);

    // --------------------------------------------------------------------------------------------------------------------

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_EntityScript_Current);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKECS_API FFragment_PendingReplication
    {
    public:
        CK_GENERATED_BODY(FFragment_PendingReplication);

    public:
        auto
        Add(
            UClass* InEntityScriptClass,
            FCk_Handle InPendingEntity,
            FInstancedStruct InSpawnParams) -> void;

        auto
        ConsumeFirst(
            UClass* InEntityScriptClass,
            const UCk_EntityScript_UE* InCDO,
            const FCk_Handle& InConstructedEntity) -> FCk_Handle;

        auto
        CleanupRemaining() -> void;

    private:
        struct FPendingEntry
        {
            FCk_Handle _Entity;
            FInstancedStruct _SpawnParams;
        };

        TMap<TObjectKey<UClass>, TArray<FPendingEntry>> _PendingByClass;
    };
}

// --------------------------------------------------------------------------------------------------------------------
