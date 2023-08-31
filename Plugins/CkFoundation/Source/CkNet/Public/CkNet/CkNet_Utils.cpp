#include "CkNet_Utils.h"

#include "CkNet_Fragment.h"
#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment.h"

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

    const auto& World = InActor->GetWorld();

    CK_ENSURE_IF_NOT(ck::IsValid(World), TEXT("Invalid World for Actor [{}]"), InActor)
    { return {}; }

    CK_ENSURE
    (
        World->IsNetMode(NM_DedicatedServer) || InActor->bExchangedRoles,
        TEXT("Get_IsActorLocallyOwned called on Replicated Actor [{}] as a CLIENT before it has exchanged roles! This may return the wrong result"),
        InActor
    );

    switch (InActor->GetLocalRole())
    {
        case ROLE_SimulatedProxy:
        {
            return false;
        }
        case ROLE_None:
        case ROLE_Authority:
        case ROLE_AutonomousProxy:
        {
            return true;
        }
        case ROLE_MAX:
        default:
        {
            CK_ENSURE_FALSE(TEXT("Unsupported Local Net Role for Actor [{}]"), InActor);
            return true;
        }
    }
}

auto
    UCk_Utils_Net_UE::
    Get_IsRoleMatching(
        AActor*                 InActor,
        ECk_Net_ReplicationType InReplicationType)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InActor), TEXT("Invalid Actor!"))
    { return {}; }

    const auto isLocallyOwned = Get_IsActorLocallyOwned(InActor);
    const auto netRole        = Get_NetRole(InActor);

    switch(InReplicationType)
    {
        case ECk_Net_ReplicationType::LocalOnly:
        {
            if (isLocallyOwned)
            { return true; }

            break;
        }
        case ECk_Net_ReplicationType::LocalAndHost:
        {
            if (isLocallyOwned || netRole == ECk_Net_NetRoleType::Server || netRole == ECk_Net_NetRoleType::Host)
            { return true; }

            break;
        }
        case ECk_Net_ReplicationType::HostOnly:
        {
            if (netRole == ECk_Net_NetRoleType::Host || netRole == ECk_Net_NetRoleType::Server)
            { return true; }

            break;
        }
        case ECk_Net_ReplicationType::ClientsOnly:
        {
            if (netRole != ECk_Net_NetRoleType::Server)
            { return true; }

            break;
        }
        case ECk_Net_ReplicationType::ServerOnly:
        {
            if (netRole == ECk_Net_NetRoleType::Server)
            { return true; }

            break;
        }
        case ECk_Net_ReplicationType::All:
        {
            return true;
        }
        default:
        {
            CK_INVALID_ENUM(InReplicationType);
            break;
        }
    }

    return false;
}

auto
    UCk_Utils_Net_UE::
    Get_NetRole(
        const UObject* InContext)
    -> ECk_Net_NetRoleType
{
    if (IsRunningDedicatedServer())
    {
        return ECk_Net_NetRoleType::Server;
    }

    const auto& isServer = [InContext]() -> bool
    {
        if (ck::Is_NOT_Valid(InContext))
        { return true; }

        const auto* world = InContext->GetWorld();

        if (ck::Is_NOT_Valid(world))
        { return true; }

        return world->IsServer();
    }();

    if (isServer)
    {
        return ECk_Net_NetRoleType::Host;
    }

    return ECk_Net_NetRoleType::Client;
}

auto
    UCk_Utils_Net_UE::
    Get_IsEntityNetMode_DedicatedServer(
        FCk_Handle InHandle)
    -> bool
{
    return InHandle.Has<ck::FCk_Tag_NetMode_DedicatedServer>();
}

auto
    UCk_Utils_Net_UE::
    Get_IsEntityNetMode_Client(
        FCk_Handle InHandle)
    -> bool
{
    return NOT InHandle.Has<ck::FCk_Tag_NetMode_DedicatedServer>();
}

auto
    UCk_Utils_Net_UE::
    Get_IsEntityReplicated(
        FCk_Handle InHandle)
    -> bool
{
    return InHandle.Has<ck::FCk_Tag_Replicated>();
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

