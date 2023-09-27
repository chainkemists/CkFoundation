#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkUnrealEntity_ActorProxy_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKUNREAL_API UCk_ActorProxy_Subsystem_UE : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_ActorProxy_Subsystem_UE);

    void PostInitialize() override;
    void Initialize(
        FSubsystemCollectionBase& Collection) override;

    /** Called when world is ready to start gameplay before the game mode transitions to the correct state and call BeginPlay on all actors */
    auto OnWorldBeginPlay(UWorld& InWorld) -> void override;

private:
    auto DoDestroyAllActorProxyChildActors() const -> void;
};

// --------------------------------------------------------------------------------------------------------------------
