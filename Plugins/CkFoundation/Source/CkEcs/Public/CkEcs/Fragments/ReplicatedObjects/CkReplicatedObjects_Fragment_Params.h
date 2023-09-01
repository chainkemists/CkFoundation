#pragma once
#include "CkCore/ObjectReplication/CkObjectReplicatorComponent.h"
#include "CkCore/ObjectReplication/CkReplicatedObject.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkMacros/CkMacros.h"

#include "CkReplicatedObjects_Fragment_Params.generated.h"

// --------------------------------------------------------------------------------------------------------------------

#define CK_REP_OBJ_EXECUTE_IF_VALID(_Func_)\
    if (NOT Get_AssociatedEntity().IsValid())\
    { return; }\
\
    CK_ENSURE_VALID_UNREAL_WORLD_IF_NOT(this)\
    { return; }\
\
    _Func_()

#define CK_REP_OBJ_EXECUTE_IF_VALID_IGNORE_SERVER(_Func_)\
    if (NOT Get_AssociatedEntity().IsValid())\
    { return; }\
\
    CK_ENSURE_VALID_UNREAL_WORLD_IF_NOT(this)\
    { return; }\
\
    if (GetWorld()->IsNetMode(NM_DedicatedServer))\
    { return; }\
    _Func_()

// TODO: Move this to its own file
UCLASS(NotBlueprintType, NotBlueprintable)
class CKECS_API UCk_Ecs_ReplicatedObject_UE
    : public UCk_ReplicatedObject_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Ecs_ReplicatedObject_UE);

public:
    // TODO: Remove as ActorInfo is no longer required to know about ReplicatedObject
    friend class UCk_Utils_OwningActor_UE;
    friend class ACk_World_Actor_Replicated_UE;
    friend struct FCk_ReplicatedObjects;

public:
    static auto Create(
        TSubclassOf<UCk_Ecs_ReplicatedObject_UE> InReplicatedObject,
        AActor* InTopmostOwningActor,
        FName InName,
        FCk_Handle InAssociatedEntity) -> UCk_Ecs_ReplicatedObject_UE*;

    static auto Destroy(
        UCk_Ecs_ReplicatedObject_UE* InRo) -> void;

public:
    virtual auto GetLifetimeReplicatedProps(TArray<FLifetimeProperty>&) const -> void override;

    virtual auto BeginDestroy() -> void override;
    virtual auto PreDestroyFromReplication() -> void override;

protected:
    virtual auto OnLink() -> void;

public:
    UFUNCTION(NetMulticast, Reliable)
    void Request_TriggerDestroyAssociatedEntity();

protected:
    UPROPERTY(Transient)
    FCk_Handle _AssociatedEntity;

    UPROPERTY(Replicated)
    AActor* _ReplicatedActor = nullptr;

public:
    CK_PROPERTY(_AssociatedEntity);
    CK_PROPERTY_GET(_ReplicatedActor);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct CKECS_API FCk_ReplicatedObjects
{
    GENERATED_BODY()

    friend class UCk_Utils_ReplicatedObjects_UE;

public:
    CK_GENERATED_BODY(FCk_ReplicatedObjects);

    // TODO: clean up the struct

private:
    UPROPERTY()
    TArray<UCk_ReplicatedObject_UE*> _ReplicatedObjects;

protected:
    auto DoRequest_LinkAssociatedEntity(FCk_Handle InEntity) -> void;

public:
    CK_PROPERTY(_ReplicatedObjects);
};

// --------------------------------------------------------------------------------------------------------------------
