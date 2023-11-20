#include "CkNet_Utils.h"

#include "CkNet_Fragment.h"
#include "CkCore/Time/CkTime_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Utils.h"
#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment.h"
#include "Engine/World.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Net_UE::
    Add(
        FCk_Handle InEntity,
        FCk_Net_ConnectionSettings InConnectionSettings)
    -> void
{
    auto& Params = InEntity.Add<ck::FFragment_Net_Params>(InConnectionSettings);

    if (InConnectionSettings.Get_NetRole() == ECk_Net_EntityNetRole::Authority)
    {
        InEntity.Add<ck::FTag_HasAuthority>();
    }
    if (InConnectionSettings.Get_NetMode() == ECk_Net_NetModeType::Host)
    {
        InEntity.Add<ck::FTag_NetMode_IsHost>();
    }
}

auto
    UCk_Utils_Net_UE::
    Copy(
        FCk_Handle InFrom,
        FCk_Handle InTo)
    -> void
{
    if (NOT Has(InFrom))
    { return; }

    Add(InTo, InFrom.Get<ck::FFragment_Net_Params>().Get_ConnectionSettings());
}

auto
    UCk_Utils_Net_UE::
    Has(
        FCk_Handle InHandle)
    -> bool
{
    return InHandle.Has<ck::FFragment_Net_Params>();
}

auto
    UCk_Utils_Net_UE::
    Ensure(
        FCk_Handle InHandle)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InHandle), TEXT("Handle [{}] does NOT have Network Info"), InHandle)
    { return false; }

    return true;
}

auto
    UCk_Utils_Net_UE::
    Get_EntityNetRole(
        FCk_Handle InEntity)
    -> ECk_Net_EntityNetRole
{
    if (NOT Has(InEntity))
    { return {}; }

    return InEntity.Get<ck::FFragment_Net_Params>().Get_ConnectionSettings().Get_NetRole();
}

auto
    UCk_Utils_Net_UE::
    Get_EntityNetMode(
        FCk_Handle InEntity)
    -> ECk_Net_NetModeType
{
    if (NOT Has(InEntity))
    { return {}; }

    return InEntity.Get<ck::FFragment_Net_Params>().Get_ConnectionSettings().Get_NetMode();
}

auto
    UCk_Utils_Net_UE::
    Get_IsEntityRoleMatching(
        FCk_Handle              InEntity,
        ECk_Net_ReplicationType InReplicationType)
    -> bool
{
    switch (InReplicationType)
    {
        case ECk_Net_ReplicationType::LocalOnly:
        {
            return Get_HasAuthority(InEntity);
        }
        case ECk_Net_ReplicationType::LocalAndHost:
        {
            return Get_HasAuthority(InEntity) || Get_IsEntityNetMode_Host(InEntity);
        }
        case ECk_Net_ReplicationType::HostOnly:
        {
            return Get_IsEntityNetMode_Host(InEntity);
        }
        case ECk_Net_ReplicationType::ClientsOnly:
        {
            return Get_EntityNetMode(InEntity) == ECk_Net_NetModeType::Client;
        }
        case ECk_Net_ReplicationType::ServerOnly:
        {
            return Get_EntityNetMode(InEntity) == ECk_Net_NetModeType::Server;
        }
        case ECk_Net_ReplicationType::All:
        {
            return true;
        }
        default:
        {
            CK_INVALID_ENUM(InReplicationType);
            return false;
        }
    }
}

auto
    UCk_Utils_Net_UE::
    Get_HasAuthority(
        FCk_Handle InEntity)
    -> bool
{
    return InEntity.Has<ck::FTag_HasAuthority>();
}

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

    if (World->IsNetMode(NM_Standalone))
    { return true; }

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
            CK_TRIGGER_ENSURE(TEXT("Unsupported Local Net Role for Actor [{}]"), InActor);
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
            if (isLocallyOwned || netRole == ECk_Net_NetModeType::Server || netRole == ECk_Net_NetModeType::Host)
            { return true; }

            break;
        }
        case ECk_Net_ReplicationType::HostOnly:
        {
            if (netRole == ECk_Net_NetModeType::Host || netRole == ECk_Net_NetModeType::Server)
            { return true; }

            break;
        }
        case ECk_Net_ReplicationType::ClientsOnly:
        {
            if (netRole != ECk_Net_NetModeType::Server)
            { return true; }

            break;
        }
        case ECk_Net_ReplicationType::ServerOnly:
        {
            if (netRole == ECk_Net_NetModeType::Server)
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
    -> ECk_Net_NetModeType
{
    if (IsRunningDedicatedServer())
    {
        return ECk_Net_NetModeType::Server;
    }

    const auto GetIsServer = [InContext]() -> bool
    {
        if (ck::Is_NOT_Valid(InContext))
        { return true; }

        const auto* world = InContext->GetWorld();

        if (ck::Is_NOT_Valid(world))
        { return true; }

        return world->IsNetMode(NM_DedicatedServer) || world->IsNetMode(NM_ListenServer);
    };

    if (GetIsServer())
    { return ECk_Net_NetModeType::Host; }

    return ECk_Net_NetModeType::Client;
}

auto
    UCk_Utils_Net_UE::
    Get_NetMode(
        const UObject* InContext)
    -> ECk_Net_NetModeType
{
    CK_ENSURE_IF_NOT(ck::IsValid(InContext), TEXT("Invalid Object!"))
    { return {}; }

    switch(InContext->GetWorld()->GetNetMode())
    {
    case NM_DedicatedServer:
        return ECk_Net_NetModeType::Server;
    case NM_Standalone:
    case NM_ListenServer:
        return ECk_Net_NetModeType::Host;
    case NM_Client:
        return ECk_Net_NetModeType::Client;
    case NM_MAX:
    default:
        CK_TRIGGER_ENSURE(TEXT("Invalid NetMode for [{}]."), InContext);
        return ECk_Net_NetModeType::None;
    }
}

auto
    UCk_Utils_Net_UE::
    Get_IsEntityNetMode_Host(
        FCk_Handle InHandle)
    -> bool
{
    const auto FoundHandle = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwnerIf(InHandle, [](FCk_Handle Handle)
    {
        return Handle.Has<ck::FTag_NetMode_IsHost>();
    });

    return ck::IsValid(FoundHandle);
}

auto
    UCk_Utils_Net_UE::
    Get_IsEntityNetMode_Client(
        FCk_Handle InHandle)
    -> bool
{
    return NOT Get_IsEntityNetMode_Host(InHandle);
}

auto
    UCk_Utils_Net_UE::
    Get_IsEntityReplicated(
        FCk_Handle InHandle)
    -> bool
{
    return InHandle.Has<ck::FTag_Replicated>();
}

auto
    UCk_Utils_Net_UE::
    Get_MinPing(
        const UObject* InContext)
    -> FCk_Time
{
    const auto& PrimaryPlayerState = UCk_Utils_Game_UE::Get_PrimaryPlayerState_AsClient(InContext);

    if (ck::Is_NOT_Valid(PrimaryPlayerState))
    { return {}; }

    return PrimaryPlayerState->Get_MinPing();
}

auto
    UCk_Utils_Net_UE::
    Get_MaxPing(
        const UObject* InContext)
    -> FCk_Time
{
    const auto& PrimaryPlayerState = UCk_Utils_Game_UE::Get_PrimaryPlayerState_AsClient(InContext);

    if (ck::Is_NOT_Valid(PrimaryPlayerState))
    { return {}; }

    return PrimaryPlayerState->Get_MaxPing();
}

auto
    UCk_Utils_Net_UE::
    Get_AveragePing(
        const UObject* InContext)
    -> FCk_Time
{
    const auto& PrimaryPlayerState = UCk_Utils_Game_UE::Get_PrimaryPlayerState_AsClient(InContext);

    if (ck::Is_NOT_Valid(PrimaryPlayerState))
    { return {}; }

    const auto& AveragePingMs = PrimaryPlayerState->ExactPing;

    return UCk_Utils_Time_UE::Make_FromMilliseconds(AveragePingMs);
}

auto
    UCk_Utils_Net_UE::
    Get_AverageLatency(
        const UObject* InContext)
    -> FCk_Time
{
    const auto& PrimaryPlayerState = UCk_Utils_Game_UE::Get_PrimaryPlayerState_AsClient(InContext);

    if (ck::Is_NOT_Valid(PrimaryPlayerState))
    { return {}; }

    const auto& AverageLatencyMs = PrimaryPlayerState->ExactPing / 2.0f;

    return UCk_Utils_Time_UE::Make_FromMilliseconds(AverageLatencyMs);
}

#if CK_BUILD_TEST
auto
    UCk_Utils_Net_UE::
    Get_PingRangeHistoryEntries()
    -> TArray<FCk_PlayerState_PingRange_History_Entry>
{
    const auto& PrimaryPlayerState = UCk_Utils_Game_UE::Get_PrimaryPlayerState_AsClient(nullptr);

    if (ck::Is_NOT_Valid(PrimaryPlayerState))
    { return {}; }

    return PrimaryPlayerState->Get_PingRangeHistoryEntries();
}
#endif
// --------------------------------------------------------------------------------------------------------------------

