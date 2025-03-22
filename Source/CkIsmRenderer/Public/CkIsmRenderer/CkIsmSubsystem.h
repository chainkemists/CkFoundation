#pragma once

#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"

#include "CkIsmSubsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, NotBlueprintType)
class CKISMRENDERER_API ACk_IsmRenderer_Actor_UE final : public AInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(ACk_IsmRenderer_Actor_UE);

public:
    ACk_IsmRenderer_Actor_UE();

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UInstancedStaticMeshComponent* _InstancedStaticMeshComponent;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(DisplayName = "CkSubsystem_IsmRenderer")
class CKISMRENDERER_API UCk_IsmRenderer_Subsystem_UE : public UCk_Game_WorldSubsystem_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_IsmRenderer_Subsystem_UE);
};
