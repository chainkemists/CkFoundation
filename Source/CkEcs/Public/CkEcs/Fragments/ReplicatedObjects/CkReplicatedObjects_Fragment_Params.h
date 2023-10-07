#pragma once
#include "CkCore/ObjectReplication/CkObjectReplicatorComponent.h"
#include "CkCore/ObjectReplication/CkReplicatedObject.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkReplicatedObjects_Fragment_Params.generated.h"

// --------------------------------------------------------------------------------------------------------------------

#define CK_REP_OBJ_EXECUTE_IF_VALID(_Func_)              \
    if (NOT Get_AssociatedEntity().IsValid())            \
    { return; }                                          \
                                                         \
    CK_ENSURE_VALID_UNREAL_WORLD_IF_NOT(this)            \
    { return; }                                          \
                                                         \
    _Func_()

#define CK_REP_OBJ_EXECUTE_IF_VALID_IGNORE_SERVER(_Func_)\
    if (NOT Get_AssociatedEntity().IsValid())            \
    { return; }                                          \
                                                         \
    CK_ENSURE_VALID_UNREAL_WORLD_IF_NOT(this)            \
    { return; }                                          \
                                                         \
    if (GetWorld()->IsNetMode(NM_DedicatedServer))       \
    { return; }                                          \
    _Func_()

#define CK_GENERATED_BODY_FRAGMENT_REP(_ClassType_)          \
    CK_GENERATED_BODY(_ClassType_);                          \
    protected:                                               \
    auto OnLink() -> void override                           \
    {                                                        \
        _AssociatedEntity.Add<TObjectPtr<ThisType>>() = this;\
    }

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintType, NotBlueprintable)
class CKECS_API UCk_Ecs_ReplicatedObject_UE
    : public UCk_ReplicatedObject_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Ecs_ReplicatedObject_UE);

public:
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
    virtual auto OnLink() -> void PURE_VIRTUAL(UCk_Ecs_ReplicateObject_UE::OnLink,);

public:
    UFUNCTION(NetMulticast, Reliable)
    void Request_TriggerDestroyAssociatedEntity();

protected:
    UPROPERTY(Transient)
    FCk_Handle _AssociatedEntity;

    UPROPERTY(Replicated)
    AActor* _ReplicatedActor = nullptr;

public:
    CK_PROPERTY_GET(_AssociatedEntity);
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
