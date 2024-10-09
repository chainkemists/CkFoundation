#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <GameplayTagContainer.h>
#include <PhysicalMaterials/PhysicalMaterial.h>

#include "CkPhysicalMaterialWithTags.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class UCk_PhysicalMaterialWithTags : public UPhysicalMaterial
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(UCk_PhysicalMaterialWithTags);

private:
	// A container of gameplay tags that game code can use to reason about this physical material
	UPROPERTY(EditAnywhere, BlueprintReadOnly,
		Category = "PhysicalProperties",
		meta = (AllowPrivateAccess = true))
	FGameplayTagContainer _Tags;

public:
	CK_PROPERTY(_Tags);
};

// --------------------------------------------------------------------------------------------------------------------
