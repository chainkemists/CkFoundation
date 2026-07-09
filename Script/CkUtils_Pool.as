// AnyStruct-parameter conveniences on top of the auto-generated utils_entity_pool /
// utils_object_pool namespaces (Script/Generated/) — lets callers pass any struct as
// per-use params without building an FInstancedStruct first (same pattern as
// CkUtils_EntityScript.as for spawn params).
namespace utils_entity_pool
{
    FCk_Handle_PendingEntityPoolAcquire
    Request_Acquire(TSubclassOf<UCk_EntityScript_UE> InEntityScriptClass, FAngelscriptAnyStructParameter InPerUseParams)
    {
        return UCk_Utils_EntityPool_UE::Request_Acquire(InEntityScriptClass, InPerUseParams.InstancedStruct);
    }

    FCk_Handle_PendingEntityPoolAcquire
    Request_Acquire_OnPool(FCk_Handle_EntityPool InPool, FAngelscriptAnyStructParameter InPerUseParams)
    {
        auto Pool = InPool;
        return Pool.Request_Acquire_OnPool(InPerUseParams.InstancedStruct);
    }
}

namespace utils_object_pool
{
    UObject
    Acquire(TSubclassOf<UObject> InObjectClass, FAngelscriptAnyStructParameter InPerUseParams)
    {
        return UCk_Utils_ObjectPool_UE::Acquire(InObjectClass, InPerUseParams.InstancedStruct);
    }

    AActor
    Acquire_Actor(TSubclassOf<AActor> InActorClass, FTransform InTransform, FAngelscriptAnyStructParameter InPerUseParams)
    {
        return UCk_Utils_ObjectPool_UE::Acquire_Actor(InActorClass, InTransform, InPerUseParams.InstancedStruct);
    }
}
