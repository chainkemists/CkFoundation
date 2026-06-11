#pragma once

#include "CkCore/Actor/CkActor.h"
#include "CkCore/Component/CkActorComponent.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"

#include <StructUtils/InstancedStruct.h>

#include "CkOwningActor_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKECS_API FCk_EntityOwningActor_BasicDetails
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_EntityOwningActor_BasicDetails);

public:
    FCk_EntityOwningActor_BasicDetails() = default;
    FCk_EntityOwningActor_BasicDetails(AActor* InActor, FCk_Handle InHandle);

public:
    auto operator==(const ThisType& InOther) const -> bool;
    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(ThisType);

private:
    UPROPERTY(BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    TWeakObjectPtr<AActor> _Actor;

    UPROPERTY(BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _Handle;

public:
    CK_PROPERTY_GET(_Actor)
    CK_PROPERTY_GET(_Handle)
};

auto CKECS_API GetTypeHash(const FCk_EntityOwningActor_BasicDetails& InBasicDetails) -> uint32;

// --------------------------------------------------------------------------------------------------------------------

// Fired once the Actor becomes ECS ready, i.e. its OwningActor component has been linked to a valid Entity.
DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_OwningActor_OnEcsReady,
    AActor*, InActor,
    FCk_Handle, InEntity);

// --------------------------------------------------------------------------------------------------------------------

// Controls when Promise_OnActorEcsReady fires.
UENUM(BlueprintType)
enum class ECk_ActorEcsReady_Policy : uint8
{
    // Fire once the Entity is linked AND its replicated values have been applied
    // (OnReplicationComplete). On authority and for non-replicated Entities this collapses to
    // link-time. The multiplayer-safe default — replicated values (attributes, team, state
    // machine state, etc.) are readable inside the callback on every world.
    ValuesReplicated,

    // Fire as soon as the Actor↔Entity link is established. On clients this is OnConstructed-time:
    // the Entity is composed but replicated values may NOT be applied yet.
    LinkEstablished,
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintType, NotBlueprintable)
class CKECS_API UCk_EntityOwningActor_ActorComponent_UE
    : public UCk_ActorComponent_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_EntityOwningActor_ActorComponent_UE);

public:
    UCk_EntityOwningActor_ActorComponent_UE();

protected:
    auto
    EndPlay(
        const EEndPlayReason::Type InEndPlayReason) -> void override;

public:
    friend class UCk_Utils_OwningActor_UE;
    friend class UCk_Fragment_EntityReplicationDriver_Rep;

private:
    UFUNCTION()
    void
    DoHandle_LinkedEntityReplicationComplete(
        FCk_Handle InEntity);

private:
    FCk_Handle _EntityHandle;

    // Promises queued by Promise_OnActorEcsReady while the Actor was not yet ECS ready (and, for the
    // ValuesReplicated policy, while the linked Entity's replication was still pending). These live
    // with the component, so they are discarded automatically if the Actor is destroyed before ever
    // becoming ready. LinkEstablished queues flush the moment the OwningActor link is established;
    // ValuesReplicated queues flush once the linked Entity's OnReplicationComplete fires (immediately
    // at link-time when the Entity does not replicate).
    TArray<FCk_Delegate_OwningActor_OnEcsReady> _PendingEcsReadyDelegates_LinkEstablished;
    TArray<TFunction<void(AActor*, FCk_Handle)>> _PendingEcsReadyCallbacks_LinkEstablished;
    TArray<FCk_Delegate_OwningActor_OnEcsReady> _PendingEcsReadyDelegates_ValuesReplicated;
    TArray<TFunction<void(AActor*, FCk_Handle)>> _PendingEcsReadyCallbacks_ValuesReplicated;
    bool _OnReplicationCompleteTrampolineBound = false;

public:
    CK_PROPERTY_GET(_EntityHandle);
};

// --------------------------------------------------------------------------------------------------------------------
// IsValid and Formatters

CK_DEFINE_CUSTOM_IS_VALID_INLINE(FCk_EntityOwningActor_BasicDetails, IsValid_Policy_Default,
[=](const FCk_EntityOwningActor_BasicDetails& InBasicDetails)
{
    return ck::IsValid(InBasicDetails.Get_Actor()) && ck::IsValid(InBasicDetails.Get_Handle());
});

CK_DEFINE_CUSTOM_FORMATTER_INLINE(FCk_EntityOwningActor_BasicDetails, [](const FCk_EntityOwningActor_BasicDetails& InObj)
{
    return ck::Format
    (
        TEXT("Actor: [{}], Handle: [{}]"),
        InObj.Get_Actor(),
        InObj.Get_Handle()
    );
});

// --------------------------------------------------------------------------------------------------------------------
