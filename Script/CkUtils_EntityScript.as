namespace utils_entity_script
{
    FCk_Handle_PendingEntityScript
    Request_SpawnEntity(FCk_Handle InLifetimeOwner, TSubclassOf<UCk_EntityScript_UE> InEntityScriptClass, FAngelscriptAnyStructParameter InSpawnParams)
    {
        auto LifetimeOwner = InLifetimeOwner;
        return UCk_Utils_EntityScript_UE::Request_SpawnEntity(LifetimeOwner, InEntityScriptClass, InSpawnParams.InstancedStruct);
    }
}