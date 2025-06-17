#include "CkEcsEditor_Subsystem.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_EcsEditor_Subsystem::
    Initialize(
        FSubsystemCollectionBase& Collection)
    -> void
{
    Super::Initialize(Collection);
}

auto
    UCk_EcsEditor_Subsystem::
    Request_AddOrGet_EntityForObject(
        UObject* InObject)
    -> FCk_Handle
{
    auto EditorWorld = Get_EditorWorld();

    const auto Found = _ObjectToHandle.Find(InObject);
    if (ck::IsValid(Found, ck::IsValid_Policy_NullptrOnly{}))
    {
        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(*Found);
    }

    return _ObjectToHandle.Add(InObject, [&]()
    {
        auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(_TransientEntity);
        NewEntity.Add<ck::FTag_EditorOnlyEntity>();
        return NewEntity;
    }());
}

auto
    UCk_EcsEditor_Subsystem::
    Get_EditorWorld()
    -> ck::FEcsWorld
{
    static ck::FEcsWorld EditorWorld;

    if (ck::IsValid(_TransientEntity))
    { return EditorWorld; }

    _TransientEntity = UCk_Utils_EntityLifetime_UE::Get_TransientEntity(EditorWorld.Get_Registry());

    UCk_Utils_Net_UE::Add(_TransientEntity, FCk_Net_ConnectionSettings{
        ECk_Replication::DoesNotReplicate,
        ECk_Net_NetModeType::Host,
        ECk_Net_EntityNetRole::Authority
    });

    return EditorWorld;
}

// --------------------------------------------------------------------------------------------------------------------
