#include "CkEntityBridge_Utils.h"

#include "CkEntityBridge_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkVariables/CkUnrealVariables_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EntityBridge_UE::
    Request_Spawn(
        const FCk_Handle& InHandle,
        const FCk_Request_EntityBridge_SpawnEntity& InRequest,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Delegate_EntityBridge_OnEntitySpawned& InDelegate)
    -> void
{
    auto RequestEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle);

    RequestEntity.AddOrGet<ck::FFragment_EntityBridge_Requests>()._Request = InRequest;
    UCk_Utils_Variables_InstancedStruct_UE::Set(RequestEntity, FGameplayTag::EmptyTag, InOptionalPayload);

    CK_SIGNAL_BIND_REQUEST_FULFILLED(ck::UUtils_Signal_OnEntitySpawned, RequestEntity, InDelegate);
}

// --------------------------------------------------------------------------------------------------------------------
