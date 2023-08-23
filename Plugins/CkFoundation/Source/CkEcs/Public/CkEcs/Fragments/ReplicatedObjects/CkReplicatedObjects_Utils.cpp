#include "CkReplicatedObjects_Utils.h"

#include "CkReplicatedObjects_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------
auto
    UCk_Utils_ReplicatedObjects_UE::
    Add(
        FCk_Handle InHandle,
        const FCk_ReplicatedObjects& InReplicatedObjects)
    -> void
{
    auto ReplicatedObjects = InReplicatedObjects;
    ReplicatedObjects.DoRequest_LinkAssociatedEntity(InHandle);
    InHandle.Add<ck::FCk_Fragment_ReplicatedObjects_Params>(ReplicatedObjects);
}

auto
    UCk_Utils_ReplicatedObjects_UE::
    Request_AddReplicatedObject(
        FCk_Handle InHandle,
        UCk_ReplicatedObject* InReplicatedObject)
    -> void
{
    InHandle.AddOrGet<ck::FCk_Fragment_ReplicatedObjects_Params>()
    .Update_ReplicatedObjects([&](FCk_ReplicatedObjects& InReplicatedObjects)
    {
        InReplicatedObjects.Update_ReplicatedObjects([&](TArray<UCk_ReplicatedObject*>& InArray)
        {
            InArray.Add(InReplicatedObject);
        });
    });
}

auto
    UCk_Utils_ReplicatedObjects_UE::
    Get_ReplicatedObjects(
        FCk_Handle InHandle)
    -> FCk_ReplicatedObjects
{
    return InHandle.Get<ck::FCk_Fragment_ReplicatedObjects_Params>().Get_ReplicatedObjects();
}
