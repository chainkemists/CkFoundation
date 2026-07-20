#include "CkPoi_EntityScript.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkPoi/CkPoi_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Poi_EntityScript::
    Construct(
        FCk_Handle& InHandle,
        const FInstancedStruct& InSpawnParams)
    -> ECk_EntityScript_ConstructionFlow
{
    const auto Ret = Super::Construct(InHandle, InSpawnParams);

    auto PoiParams = _PoiParams;
    auto SpawnTransform = _SpawnTransform;

    if (InSpawnParams.IsValid() && InSpawnParams.GetScriptStruct() == FCk_Poi_SpawnParams::StaticStruct())
    {
        const auto& RuntimeSpawnParams = InSpawnParams.Get<FCk_Poi_SpawnParams>();
        PoiParams = RuntimeSpawnParams.Get_PoiParams();
        SpawnTransform = RuntimeSpawnParams.Get_Transform();
    }

    UCk_Utils_Transform_UE::Add(_AssociatedEntity, SpawnTransform, _Replication);

    UCk_Utils_Poi_UE::Add(_AssociatedEntity, PoiParams);

    return Ret;
}

// --------------------------------------------------------------------------------------------------------------------
