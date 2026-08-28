#pragma once

#include "CkEcs/EntityScript/CkGenericEntityScript.h"
#include "CkCrowd/AvoidanceVolume/CkCrowdAvoidanceVolume_Fragment_Data.h"

#include "CkCrowdAvoidanceVolume_EntityScript.generated.h"

USTRUCT(BlueprintType)
struct CKCROWD_API FCk_CrowdAvoidanceVolume_SpawnParams
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_CrowdAvoidanceVolume_SpawnParams);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Fragment_CrowdAvoidanceVolume_ParamsData _Params;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FTransform _SpawnTransform = FTransform::Identity;

public:
    CK_PROPERTY_GET(_Params);
    CK_PROPERTY_GET(_SpawnTransform);
    CK_DEFINE_CONSTRUCTORS(FCk_CrowdAvoidanceVolume_SpawnParams, _Params, _SpawnTransform);
};

UCLASS(Blueprintable, BlueprintType)
class CKCROWD_API UCk_CrowdAvoidanceVolume_EntityScript : public UCk_GenericEntityScript_UE
{
    GENERATED_BODY()
    CK_GENERATED_BODY(UCk_CrowdAvoidanceVolume_EntityScript);

protected:
    UCk_CrowdAvoidanceVolume_EntityScript(const FObjectInitializer& InObjectInitializer);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Avoidance Volume",
        meta = (AllowPrivateAccess = true, ExposeOnSpawn = true))
    FCk_Fragment_CrowdAvoidanceVolume_ParamsData _Params;

    // EntitySpawner injects its actor transform here for level placement. Runtime callers pass
    // FCk_CrowdAvoidanceVolume_SpawnParams instead.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crowd Avoidance Volume",
        meta = (AllowPrivateAccess = true, ExposeOnSpawn = true))
    FTransform _SpawnTransform = FTransform::Identity;

protected:
    auto Construct(FCk_Handle& InHandle, const FInstancedStruct& InSpawnParams) -> ECk_EntityScript_ConstructionFlow override;
};

// --------------------------------------------------------------------------------------------------------------------
