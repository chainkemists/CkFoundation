#pragma once

#include "CkCore/Actor/CkActor.h"

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"
#include "CkEcs/Handle/CkHandle.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkCore/Component/CkActorComponent.h"

#include <InstancedStruct.h>

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

UCLASS(NotBlueprintType, NotBlueprintable)
class CKECS_API UCk_EntityOwningActor_ActorComponent_UE
    : public UCk_ActorComponent_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_EntityOwningActor_ActorComponent_UE);

public:
    UCk_EntityOwningActor_ActorComponent_UE();

public:
    friend class UCk_Utils_OwningActor_UE;
    friend class UCk_EntityBridge_ActorComponent_UE;
    friend class UCk_Fragment_EntityReplicationDriver_Rep;

private:
    FCk_Handle _EntityHandle;

public:
    CK_PROPERTY_GET(_EntityHandle);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintType, NotBlueprintable)
class CKECS_API UCk_EntityBridge_ActorComponent_Base_UE : public UCk_ActorComponent_UE
{
    GENERATED_BODY()

    friend class UCk_EntityBridge_ActorComponent_UE;
    friend class UCk_Fragment_EntityReplicationDriver_Rep;

private:
    enum class EInvoke_Caller
    {
        ReplicationDriver,
        EntityBridge,
    };

private:
    virtual auto
    TryInvoke_OnPreConstruct(
        const FCk_Handle& Entity,
        EInvoke_Caller InCaller) const -> void {}

    virtual auto
    TryInvoke_OnReplicationComplete(
        EInvoke_Caller) -> void {}
};

// --------------------------------------------------------------------------------------------------------------------
// IsValid and Formatters

CK_DEFINE_CUSTOM_IS_VALID(FCk_EntityOwningActor_BasicDetails, ck::IsValid_Policy_Default,
[=](const FCk_EntityOwningActor_BasicDetails& InBasicDetails)
{
    return ck::IsValid(InBasicDetails.Get_Actor()) && ck::IsValid(InBasicDetails.Get_Handle());
});

CK_DEFINE_CUSTOM_FORMATTER(FCk_EntityOwningActor_BasicDetails, [&]()
{
    return ck::Format
    (
        TEXT("Actor: [{}], Handle: [{}]"),
        InObj.Get_Actor(),
        InObj.Get_Handle()
    );
});

// --------------------------------------------------------------------------------------------------------------------
