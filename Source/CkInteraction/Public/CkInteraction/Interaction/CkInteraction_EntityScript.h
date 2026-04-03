#pragma once

#include "CkEcs/EntityScript/CkEntityScript.h"

#include "CkAttribute/FloatAttribute/CkFloatAttribute_Fragment_Data.h"
#include "CkInteraction/Interaction/CkInteraction_Fragment_Data.h"

#include "CkInteraction_EntityScript.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, BlueprintType)
class CKINTERACTION_API UCk_Interaction_EntityScript : public UCk_EntityScript_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Interaction_EntityScript);

public:
    UCk_Interaction_EntityScript(const FObjectInitializer& InInitializer);

protected:
    auto Construct(
        FCk_Handle& InHandle,
        const FInstancedStruct& InSpawnParams) -> ECk_EntityScript_ConstructionFlow override;
    auto BeginPlay() -> void override;

    auto ShowReplicationInEditor() const -> bool override;

private:
    UFUNCTION()
    void
    OnInteractionTimeMaxClamped(
        FCk_Handle InAttributeOwnerEntity,
        FCk_Payload_FloatAttribute_OnClamped InPayload);
};

// --------------------------------------------------------------------------------------------------------------------
