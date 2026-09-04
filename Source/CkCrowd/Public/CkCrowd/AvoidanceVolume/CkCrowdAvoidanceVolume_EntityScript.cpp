#include "CkCrowd/AvoidanceVolume/CkCrowdAvoidanceVolume_EntityScript.h"

#include "CkCrowd/AvoidanceVolume/CkCrowdAvoidanceVolume_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"

UCk_CrowdAvoidanceVolume_EntityScript::UCk_CrowdAvoidanceVolume_EntityScript(
    const FObjectInitializer& InObjectInitializer)
    : Super(InObjectInitializer)
{
    _Replication = ECk_Replication::DoesNotReplicate;
    _ShowInPlaceActors = true;
}

auto UCk_CrowdAvoidanceVolume_EntityScript::Construct(
    FCk_Handle& InHandle,
    const FInstancedStruct& InSpawnParams)
    -> ECk_EntityScript_ConstructionFlow
{
    const auto Ret = Super::Construct(InHandle, InSpawnParams);
    auto Transform = UCk_Utils_Transform_UE::Add(InHandle, _SpawnTransform, ECk_Replication::DoesNotReplicate);
    const auto Volume = UCk_Utils_CrowdAvoidanceVolume_UE::Add(Transform, _Params);
    const auto IsVolumeValid = ck::IsValid(Volume);
    CK_ENSURE_IF_NOT(IsVolumeValid,
        TEXT("CrowdAvoidanceVolume EntityScript failed to compose its volume feature."))
    { UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle); }
    return Ret;
}
