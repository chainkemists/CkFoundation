#include "CkEntityScript_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_CUSTOM_IS_VALID(FCk_Handle_PendingEntityScript, IsValid_Policy_Default, [=](const FCk_Handle_PendingEntityScript& InHandle)
{
    return ck::IsValid(InHandle.Get_EntityUnderConstruction());
});

FCk_Request_EntityScript_SpawnEntity::
FCk_Request_EntityScript_SpawnEntity(
    const FCk_Handle& InNewEntity,
    const FCk_Handle& InOwner,
    TSubclassOf<UCk_EntityScript_UE> InEntityScriptClass)
    : FCk_Request_Base()
    , _NewEntity(InNewEntity)
    , _Owner(InOwner)
    , _EntityScriptClassArchetype(InEntityScriptClass.GetDefaultObject())
{
}
