#include "CkUnrealEntity_Fragment_Params.h"

#include "CkUnreal/Entity/CkUnrealEntity_ConstructionScript.h"

#include "CkLifetime/Public/CkLifetime/EntityLifetime/CkEntityLifetime_Fragment_Utils.h"

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

auto UCk_UnrealEntity_Base_PDA::
Build(FCk_Registry& InRegistry) const -> FCk_Handle
{
    const auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InRegistry);
    DoBuild(NewEntity);

    return NewEntity;
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
    : ThisType{InUnrealEntity, [](auto InHandle){}}
{
}

FCk_Request_UnrealEntity_Spawn::
FCk_Request_UnrealEntity_Spawn(const UCk_UnrealEntity_Base_PDA* InUnrealEntity,
    PostSpawnFunc InPostSpawnFunc)
    : _UnrealEntity(InUnrealEntity)
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
