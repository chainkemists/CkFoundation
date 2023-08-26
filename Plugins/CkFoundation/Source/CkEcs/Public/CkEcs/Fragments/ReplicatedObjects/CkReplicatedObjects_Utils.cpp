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
    InHandle.Add<ck::FCk_Tag_Replicated>();
}

auto
    UCk_Utils_ReplicatedObjects_UE::
    Has(
        FCk_Handle InHandle)
    -> bool
{
    return InHandle.Has<ck::FCk_Fragment_ReplicatedObjects_Params>();
}

auto
    UCk_Utils_ReplicatedObjects_UE::
    Ensure(
        FCk_Handle InHandle)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InHandle), TEXT("Entity [{}] does NOT have ReplicatedObjects Fragment!"), InHandle)
    { return false; }

    return true;
}

auto
    UCk_Utils_ReplicatedObjects_UE::
    Request_AddReplicatedObject(
        FCk_Handle InHandle,
        UCk_ReplicatedObject_UE* InReplicatedObject)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InReplicatedObject), TEXT("Invalid Replicated Object request to add to Entity [{}]"), InHandle)
    { return; }

    // TODO: Cleanup this
    if (NOT InHandle.Has<ck::FCk_Tag_Replicated>())
    {
        InHandle.Add<ck::FCk_Tag_Replicated>();
    }

    InHandle.AddOrGet<ck::FCk_Fragment_ReplicatedObjects_Params>()
    .Update_ReplicatedObjects([&](FCk_ReplicatedObjects& InReplicatedObjects)
    {
        InReplicatedObjects.Update_ReplicatedObjects([&](TArray<UCk_ReplicatedObject_UE*>& InArray)
        {
            InArray.Add(InReplicatedObject);
        });
    });
}

auto
    UCk_Utils_ReplicatedObjects_UE::
    Get_NetRole(
        FCk_Handle InHandle)
    -> ENetRole
{
    if (NOT Has(InHandle))
    { return ROLE_Authority; }

    auto RoleToReturn = ENetRole::ROLE_None;

    OnFirstValidReplicatedObject(InHandle, [&](UCk_ReplicatedObject_UE* InRO)
    {
        const auto& ReplicatedObjectAsActor = Cast<AActor>(InRO->GetOuter());

        CK_ENSURE_IF_NOT(ck::IsValid(ReplicatedObjectAsActor, ck::IsValid_Policy_NullptrOnly{}),
            TEXT("Outer of Replicated Object [{}] for Entity [{}] is NOT an Actor when expected it to be"), InRO, InHandle)
        { return; }

        RoleToReturn = ReplicatedObjectAsActor->GetLocalRole();
    });

    return RoleToReturn;
}

auto
    UCk_Utils_ReplicatedObjects_UE::
    OnFirstValidReplicatedObject(
        FCk_Handle InHandle,
        const std::function<void(UCk_ReplicatedObject_UE* InRO)>& InFunc)
    -> void
{
    if (NOT Ensure(InHandle))
    { return; }

    const auto ReplicatedObjects = Get_ReplicatedObjects(InHandle).Get_ReplicatedObjects();

    const auto& ReplicatedObjectToUse = ReplicatedObjects.FindByPredicate([&](UCk_ReplicatedObject_UE* InRO)
    {
        return ck::IsValid(InRO, ck::IsValid_Policy_NullptrOnly{});
    });

    CK_ENSURE_IF_NOT(ck::IsValid(ReplicatedObjectToUse, ck::IsValid_Policy_NullptrOnly{}),
        TEXT("No valid Replicated Object found for Entity [{}]"), InHandle)
    { return; }

    InFunc(*ReplicatedObjectToUse);
}

auto
    UCk_Utils_ReplicatedObjects_UE::
    Get_ReplicatedObjects(
        FCk_Handle InHandle)
    -> FCk_ReplicatedObjects
{
    return InHandle.Get<ck::FCk_Fragment_ReplicatedObjects_Params>().Get_ReplicatedObjects();
}

// --------------------------------------------------------------------------------------------------------------------
