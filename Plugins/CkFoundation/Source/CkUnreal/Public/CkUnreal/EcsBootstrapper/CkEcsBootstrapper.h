#pragma once

#include "CkActor/ActorInfo/CkActorInfo_Fragment_Params.h"

#include "CkCore/ObjectReplication/CkReplicatedObject.h"

#include "CkUnreal/Entity/CkUnrealEntity_ConstructionScript.h"

#include "CkEcsBootstrapper.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct FCk_Bootstrapper_Construction_Params
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Bootstrapper_Construction_Params);

public:
    FCk_Bootstrapper_Construction_Params() = default;
    FCk_Bootstrapper_Construction_Params(
        TObjectPtr<AActor> InOuter,
        TObjectPtr<UCk_UnrealEntity_Base_PDA> InUnrealEntity);

public:
    UPROPERTY(meta=(AllowPrivateAccess))
    TObjectPtr<AActor> _Outer;

    UPROPERTY(meta=(AllowPrivateAccess))
    TObjectPtr<UCk_UnrealEntity_Base_PDA> _UnrealEntity;

public:
    CK_PROPERTY_GET(_Outer);
    CK_PROPERTY_GET(_UnrealEntity);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKUNREAL_API UCk_EcsBootstrapper_UE : public UCk_EcsBootstrapper_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_EcsBootstrapper_UE);

public:
    static auto Create(
        FCk_Bootstrapper_Construction_Params InParams) -> UCk_EcsBootstrapper_UE*;

private:
    auto DoInvoke_ConstructionScript(
        FCk_Bootstrapper_Construction_Params InParams) -> void;

public:
    virtual auto GetLifetimeReplicatedProps(TArray<FLifetimeProperty>&) const -> void override;

private:
    UFUNCTION()
    void OnRep_ConstructionParams(const FCk_Bootstrapper_Construction_Params& InConstructionScript);

    virtual void OnRep_ReplicatedActor(AActor* InActor) override;

private:
    UPROPERTY(ReplicatedUsing = OnRep_ConstructionParams)
    FCk_Bootstrapper_Construction_Params _ConstructionParams;

    bool _IsConstructed = false;
};

// --------------------------------------------------------------------------------------------------------------------
