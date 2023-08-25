#include "CkUnrealEntity_Fragment_Params.h"

#include "CkUnreal/Entity/CkUnrealEntity_ConstructionScript.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto UCk_UnrealEntity_Base_PDA::
Get_EntityConstructionScript() const -> UCk_UnrealEntity_ConstructionScript_PDA*
{
    return DoGet_EntityConstructionScript();
}

auto UCk_UnrealEntity_Base_PDA::
DoGet_EntityConstructionScript() const -> UCk_UnrealEntity_ConstructionScript_PDA*
{
    CK_ENSURE_FALSE(TEXT("DoGet_EntityConstructionScript was not Overriden in [{}]"), this);
    return {};
}

auto UCk_UnrealEntity_PDA::
DoGet_EntityConstructionScript() const -> UCk_UnrealEntity_ConstructionScript_PDA*
{
    return _EntityConstructionScript;
}

// --------------------------------------------------------------------------------------------------------------------

UCk_UnrealEntity_ConstructionScript_PDA*
    UCk_UnrealEntity_WithActor_PDA::DoGet_EntityConstructionScript() const
{
    return _EntityConstructionScript;
}

// --------------------------------------------------------------------------------------------------------------------

auto UCk_UnrealEntity_Base_PDA::
Build(FCk_Handle InEntity) const -> void
{
    DoBuild(InEntity);
}

auto UCk_UnrealEntity_Base_PDA::
DoBuild(FCk_Handle InHandle) const -> void
{
    const auto& EntityConstructionScript = Get_EntityConstructionScript();
    CK_ENSURE_IF_NOT(ck::IsValid(EntityConstructionScript), TEXT("INVALID ConstructionScript in UnrealEntity [{}]"), GetPathName())
    { return; }

    EntityConstructionScript->Construct(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Request_UnrealEntity_Spawn::
FCk_Request_UnrealEntity_Spawn(const UCk_UnrealEntity_Base_PDA* InUnrealEntity)
    : ThisType{InUnrealEntity, [](auto InHandle){}, [](auto InHandle){}}
{
}

FCk_Request_UnrealEntity_Spawn::
FCk_Request_UnrealEntity_Spawn(
    const UCk_UnrealEntity_Base_PDA* InUnrealEntity,
    PreBuildFunc InPreBuildFunc,
    PostSpawnFunc InPostSpawnFunc)
    : _UnrealEntity(InUnrealEntity)
    , _PreBuildFunc(InPreBuildFunc)
    , _PostSpawnFunc(InPostSpawnFunc)
{
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Payload_UnrealEntity_EntityCreated::
FCk_Payload_UnrealEntity_EntityCreated(FCk_Handle InHandle, FCk_Handle InCreatedEntity)
    : _Handle(InHandle)
    , _CreatedEntity(InCreatedEntity)
{
}

// --------------------------------------------------------------------------------------------------------------------
