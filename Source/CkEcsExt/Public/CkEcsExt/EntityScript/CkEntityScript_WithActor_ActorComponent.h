#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/OwningActor/CkOwningActor_Fragment_Data.h"

#include "CkEntityScript_WithActor_ActorComponent.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_EntityScript_WithActor_UE;

// --------------------------------------------------------------------------------------------------------------------

// Add this component to any Actor to make it ECS-ready without writing spawn code: pick an
// EntityScript class and the component requests the WithActor spawn on BeginPlay (authority-gated;
// clients receive the Entity through the EntityScript replication pipeline and link up in
// Construct). With no class set it is just the passive link holder the runtime auto-adds at link
// time. Non-replicated — the Entity itself carries all replication. Do NOT pair this with a manual
// Request_SpawnEntityScript_OnActor call on the same Actor; that would link two Entities to one Actor.
UCLASS(BlueprintType, ClassGroup = "Ck",
       meta = (BlueprintSpawnableComponent, DisplayName = "Ck Entity Script (With Actor)"))
class CKECSEXT_API UCk_EntityScript_WithActor_ActorComponent_UE
    : public UCk_EntityOwningActor_ActorComponent_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_EntityScript_WithActor_ActorComponent_UE);

protected:
    auto
    BeginPlay() -> void override;

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              Category = "Ck",
              meta = (AllowPrivateAccess = true))
    TSubclassOf<UCk_EntityScript_WithActor_UE> _EntityScript;

public:
    CK_PROPERTY(_EntityScript);
};

// --------------------------------------------------------------------------------------------------------------------
