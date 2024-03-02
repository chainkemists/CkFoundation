#pragma once

#include "CkEcs/Handle/CkHandle.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEntityBridge/CkEntityBridge_Fragment_Data.h"

#include <GameFramework/Info.h>

#include "CkActorProxy_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable, BlueprintType)
class CKACTORPROXY_API ACk_ActorProxy_UE : public AInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(ACk_ActorProxy_UE);

public:
    ACk_ActorProxy_UE();

public:
    auto
    OnConstruction(
        const FTransform& Transform) -> void override;

    auto
    BeginDestroy() -> void override;

    auto
    Destroyed() -> void override;

protected:
#if WITH_EDITOR
    auto
    PostEditChangeProperty(FPropertyChangedEvent& InPropertyChangedEvent) -> void override;
#endif

public:
    auto
    BeginPlay() -> void override;

private:
    auto
    DoSpawnActor() -> void;

private:
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly,
        meta = (AllowPrivateAccess = true, AllowAbstract = false))
    TSubclassOf<AActor> _ActorToSpawn;

    UPROPERTY()
    bool _ActorToSpawnIsReplicated = false;

    TWeakObjectPtr<AActor> _SpawnedActor;

private:
#if WITH_EDITORONLY_DATA
    UPROPERTY(EditDefaultsOnly, meta = (AllowPrivateAccess = true))
    TObjectPtr<UTexture2D> _Icon;
#endif
};

// --------------------------------------------------------------------------------------------------------------------
