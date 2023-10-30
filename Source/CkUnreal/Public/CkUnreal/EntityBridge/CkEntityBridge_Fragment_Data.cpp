#include "CkEntityBridge_Fragment_Data.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Utils.h"
#include "CkEcs/EntityConstructionScript/CkEntity_ConstructionScript.h"

#include "CkUnreal/EntityBridge/CkEntityBridge_ConstructionScript.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_EntityBridge_Config_Base_PDA::
    Get_EntityConstructionScript() const
    -> UCk_EntityBridge_ConstructionScript_PDA*
{
    return DoGet_EntityConstructionScript();
}

auto
    UCk_EntityBridge_Config_Base_PDA::
    DoGet_EntityConstructionScript() const
    -> UCk_EntityBridge_ConstructionScript_PDA*
{
    CK_TRIGGER_ENSURE(TEXT("DoGet_EntityConstructionScript was not Overriden in [{}]"), this);
    return {};
}

auto
    UCk_EntityBridge_Config_PDA::
    DoGet_EntityConstructionScript() const
    -> UCk_EntityBridge_ConstructionScript_PDA*
{
    return _EntityConstructionScript;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_EntityBridge_Config_WithActor_PDA::
    DoGet_EntityConstructionScript() const
    -> UCk_EntityBridge_ConstructionScript_PDA*
{
    return _EntityConstructionScript;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_EntityBridge_Config_Base_PDA::
    Build(
        FCk_Handle InEntity) const
    -> void
{
    DoBuild(InEntity);
}

auto
    UCk_EntityBridge_Config_Base_PDA::
    DoBuild(
        FCk_Handle InHandle) const
    -> void
{
    const auto& EntityConstructionScript = Get_EntityConstructionScript();
    CK_ENSURE_IF_NOT(ck::IsValid(EntityConstructionScript), TEXT("INVALID ConstructionScript in EntityConfig [{}]"), GetPathName())
    { return; }

    EntityConstructionScript->Construct(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Request_EntityBridge_SpawnEntity::
    FCk_Request_EntityBridge_SpawnEntity(
        const UCk_EntityBridge_Config_Base_PDA* InEntityConfig)
    : ThisType{InEntityConfig, [](auto InHandle){}, [](auto InHandle){}}
{
}

FCk_Request_EntityBridge_SpawnEntity::
    FCk_Request_EntityBridge_SpawnEntity(
        const UCk_EntityBridge_Config_Base_PDA* InEntityConfig,
        PreBuildFunc InPreBuildFunc,
        PostSpawnFunc InPostSpawnFunc)
    : _EntityConfig(InEntityConfig)
    , _PreBuildFunc(InPreBuildFunc)
    , _PostSpawnFunc(InPostSpawnFunc)
{
}

// --------------------------------------------------------------------------------------------------------------------
