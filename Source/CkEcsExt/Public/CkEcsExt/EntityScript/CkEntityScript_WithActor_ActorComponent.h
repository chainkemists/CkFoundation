#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/OwningActor/CkOwningActor_Fragment_Data.h"

#include "CkEntityScript_WithActor_ActorComponent.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_EntityScript_WithActor_UE;

// --------------------------------------------------------------------------------------------------------------------

// Makes an Actor ECS-ready without spawn code: the component requests the WithActor spawn of
// _EntityScript on BeginPlay (authority-gated). With no class set it is just the passive link holder
// the runtime auto-adds. Do NOT also call Request_SpawnEntityScript_OnActor on the same Actor.
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
