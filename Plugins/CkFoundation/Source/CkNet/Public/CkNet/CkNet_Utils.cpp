#include "CkNet_Utils.h"

#include "CkNet_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Net_UE::
    Get_IsActorLocallyOwned(
        AActor* InActor)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InActor), TEXT("Invalid Actor supplied to Get_IsActorLocallyOwned"))
    { return {}; }

    if (NOT InActor->GetIsReplicated())
    { return true; }

    const auto& world = InActor->GetWorld();

    CK_ENSURE_IF_NOT(ck::IsValid(world), TEXT("Invalid World for Actor [{}]"), InActor)
    { return {}; }

    CK_ENSURE
    (
        world->IsNetMode(NM_DedicatedServer) || InActor->bExchangedRoles,
        TEXT("Get_IsActorLocallyOwned called on Replicated Actor [{}] as a CLIENT before it has exchanged roles! This may return the wrong result"),
        InActor
    );

    const auto& netConnection = InActor->GetNetConnection();
    const auto& owningActor = ck::IsValid(netConnection) ? netConnection->OwningActor : InActor->GetOwner();
    const auto& owningController = Cast<APlayerController>(owningActor);

    if (InActor->HasAuthority())
    {
        return NOT ck::IsValid(owningController) || owningController->IsLocalController();
    }

    return ck::IsValid(owningController) && owningController->IsLocalController();
}

auto
    UCk_Utils_Net_UE::
    Get_IsEntityNetMode_DedicatedServer(
        FCk_Handle InHandle)
    -> bool
{
    return InHandle.Has_Any<ck::FCk_Tag_NetMode_DedicatedServer>();
}

auto
    UCk_Utils_Net_UE::
    Get_IsEntityNetMode_Client(
        FCk_Handle InHandle)
    -> bool
{
    return NOT InHandle.Has_Any<ck::FCk_Tag_NetMode_DedicatedServer>();
}

auto
    UCk_Utils_Net_UE::
    Request_MarkEntityAs_DedicatedServer(
        FCk_Handle InHandle)
    -> void
{
    InHandle.Add<ck::FCk_Tag_NetMode_DedicatedServer>();
}

// --------------------------------------------------------------------------------------------------------------------

