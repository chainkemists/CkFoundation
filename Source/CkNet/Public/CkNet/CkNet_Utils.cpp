#include "CkNet_Utils.h"

#include "CkCore/Game/CkGame_Utils.h"
#include "CkCore/Time/CkTime_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Net/CkNet_Fragment.h"

#include <Engine/World.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Net_UE::
    Add(
        FCk_Handle& InEntity,
        const FCk_Net_ConnectionSettings& InConnectionSettings)
    -> void
{
    InEntity.AddOrGet<ck::FFragment_Net_Params>(InConnectionSettings);

    if (InConnectionSettings.Get_NetRole() == ECk_Net_EntityNetRole::Authority)
    {
        InEntity.AddOrGet<ck::FTag_HasAuthority>();
    }
    if (InConnectionSettings.Get_NetMode() == ECk_Net_NetModeType::Host)
    {
        InEntity.AddOrGet<ck::FTag_NetMode_IsHost>();
    }
}

auto
    UCk_Utils_Net_UE::
    Copy(
        const FCk_Handle& InFrom,
        FCk_Handle& InTo)
    -> void
{
    if (NOT Has(InFrom))
    { return; }

    Add(InTo, InFrom.Get<ck::FFragment_Net_Params>().Get_ConnectionSettings());
}

auto
    UCk_Utils_Net_UE::
    Has(
        const FCk_Handle& InHandle)
    -> bool
{
    return InHandle.Has<ck::FFragment_Net_Params>();
}

auto
    UCk_Utils_Net_UE::
    Ensure(
        const FCk_Handle& InHandle)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InHandle), TEXT("Handle [{}] does NOT have Network Info"), InHandle)
    { return false; }

    return true;
}

auto
    UCk_Utils_Net_UE::
    Get_IsActorLocallyControlled(
        AActor* InActor)
    -> ECk_Utils_Net_IsLocallyControlled_Result
{
    CK_ENSURE_IF_NOT(ck::IsValid(InActor), TEXT("Invalid Actor supplied to Get_IsActorLocallyControlled"))
    { return ECk_Utils_Net_IsLocallyControlled_Result::IsNotValidPawn; }

    const auto& ActorAsPawn = Cast<APawn>(InActor);
    if (ck::Is_NOT_Valid(ActorAsPawn))
    { return ECk_Utils_Net_IsLocallyControlled_Result::IsNotValidPawn; }

    return ActorAsPawn->IsLocallyControlled()
            ? ECk_Utils_Net_IsLocallyControlled_Result::IsLocallyControlled
            : ECk_Utils_Net_IsLocallyControlled_Result::IsNotLocallyControlled;
}

auto
    UCk_Utils_Net_UE::
    Get_IsActorPlayerControlled(
        AActor* InActor)
    -> ECk_Utils_Net_IsPlayerControlled_Result
{
    CK_ENSURE_IF_NOT(ck::IsValid(InActor), TEXT("Invalid Actor supplied to Get_IsActorPlayerControlled"))
    { return ECk_Utils_Net_IsPlayerControlled_Result::IsNotValidPawn; }

    const auto& ActorAsPawn = Cast<APawn>(InActor);
    if (ck::Is_NOT_Valid(ActorAsPawn))
    { return ECk_Utils_Net_IsPlayerControlled_Result::IsNotValidPawn; }

    return ActorAsPawn->IsPlayerControlled()
            ? ECk_Utils_Net_IsPlayerControlled_Result::IsPlayerControlled
            : ECk_Utils_Net_IsPlayerControlled_Result::IsNotPayerControlled;
}

auto
    UCk_Utils_Net_UE::
    Get_IsActorBotControlled(
        AActor* InActor)
    -> ECk_Utils_Net_IsBotControlled_Result
{
    CK_ENSURE_IF_NOT(ck::IsValid(InActor), TEXT("Invalid Actor supplied to Get_IsActorBotControlled"))
    { return ECk_Utils_Net_IsBotControlled_Result::IsNotValidPawn; }

    const auto& ActorAsPawn = Cast<APawn>(InActor);
    if (ck::Is_NOT_Valid(ActorAsPawn))
    { return ECk_Utils_Net_IsBotControlled_Result::IsNotValidPawn; }

    return ActorAsPawn->IsBotControlled()
            ? ECk_Utils_Net_IsBotControlled_Result::IsBotControlled
            : ECk_Utils_Net_IsBotControlled_Result::IsNotBotControlled;
}

auto
    UCk_Utils_Net_UE::
    Get_IsActorLocallyControlled_ByPlayer(
        AActor* InActor)
    -> ECk_Utils_Net_IsLocallyControlled_Result
{
    CK_ENSURE_IF_NOT(ck::IsValid(InActor), TEXT("Invalid Actor supplied to Get_IsActorLocallyControlled_ByPlayer"))
    { return ECk_Utils_Net_IsLocallyControlled_Result::IsNotValidPawn; }

    const auto& ActorAsPawn = Cast<APawn>(InActor);
    if (ck::Is_NOT_Valid(ActorAsPawn))
    { return ECk_Utils_Net_IsLocallyControlled_Result::IsNotValidPawn; }

    return ActorAsPawn->IsLocallyControlled() && ActorAsPawn->IsPlayerControlled()
            ? ECk_Utils_Net_IsLocallyControlled_Result::IsLocallyControlled
            : ECk_Utils_Net_IsLocallyControlled_Result::IsNotLocallyControlled;
}

auto
    UCk_Utils_Net_UE::
    Get_IsEntityPlayerControlled(
        const FCk_Handle& InEntity)
    -> ECk_Utils_Net_IsPlayerControlled_Result
{
    CK_ENSURE_IF_NOT(ck::IsValid(InEntity), TEXT("Invalid Entity supplied to Get_IsEntityPlayerControlled"))
    { return ECk_Utils_Net_IsPlayerControlled_Result::IsNotValidPawn; }

    if (NOT UCk_Utils_OwningActor_UE::Has(InEntity))
    { return ECk_Utils_Net_IsPlayerControlled_Result::IsNotValidPawn; }

    const auto& EntityOwningActor = UCk_Utils_OwningActor_UE::Get_EntityOwningActor(InEntity);

    if (ck::Is_NOT_Valid(EntityOwningActor))
    { return ECk_Utils_Net_IsPlayerControlled_Result::IsNotValidPawn; }

    return Get_IsActorPlayerControlled(EntityOwningActor);
}

auto
    UCk_Utils_Net_UE::
    Get_IsEntityBotControlled(
        const FCk_Handle& InEntity)
    -> ECk_Utils_Net_IsBotControlled_Result
{
    CK_ENSURE_IF_NOT(ck::IsValid(InEntity), TEXT("Invalid Entity supplied to Get_IsEntityBotControlled"))
    { return ECk_Utils_Net_IsBotControlled_Result::IsNotValidPawn; }

    if (NOT UCk_Utils_OwningActor_UE::Has(InEntity))
    { return ECk_Utils_Net_IsBotControlled_Result::IsNotValidPawn; }

    const auto& EntityOwningActor = UCk_Utils_OwningActor_UE::Get_EntityOwningActor(InEntity);

    if (ck::Is_NOT_Valid(EntityOwningActor))
    { return ECk_Utils_Net_IsBotControlled_Result::IsNotValidPawn; }

    return Get_IsActorBotControlled(EntityOwningActor);
}

auto
    UCk_Utils_Net_UE::
    Get_IsEntityLocallyControlled(
        const FCk_Handle& InEntity)
    -> ECk_Utils_Net_IsLocallyControlled_Result
{
    CK_ENSURE_IF_NOT(ck::IsValid(InEntity), TEXT("Invalid Entity supplied to Get_IsEntityLocallyControlled"))
    { return ECk_Utils_Net_IsLocallyControlled_Result::IsNotValidPawn; }

    if (NOT UCk_Utils_OwningActor_UE::Has(InEntity))
    { return ECk_Utils_Net_IsLocallyControlled_Result::IsNotValidPawn; }

    const auto& EntityOwningActor = UCk_Utils_OwningActor_UE::Get_EntityOwningActor(InEntity);

    if (ck::Is_NOT_Valid(EntityOwningActor))
    { return ECk_Utils_Net_IsLocallyControlled_Result::IsNotValidPawn; }

    return Get_IsActorLocallyControlled(EntityOwningActor);
}

auto
    UCk_Utils_Net_UE::
    Get_IsEntityLocallyControlled_ByPlayer(
        const FCk_Handle& InEntity)
    -> ECk_Utils_Net_IsLocallyControlled_Result
{
    CK_ENSURE_IF_NOT(ck::IsValid(InEntity), TEXT("Invalid Entity supplied to Get_IsEntityLocallyControlled_ByPlayer"))
    { return ECk_Utils_Net_IsLocallyControlled_Result::IsNotValidPawn; }

    if (NOT UCk_Utils_OwningActor_UE::Has(InEntity))
    { return ECk_Utils_Net_IsLocallyControlled_Result::IsNotValidPawn; }

    const auto& EntityOwningActor = UCk_Utils_OwningActor_UE::Get_EntityOwningActor(InEntity);

    if (ck::Is_NOT_Valid(EntityOwningActor))
    { return ECk_Utils_Net_IsLocallyControlled_Result::IsNotValidPawn; }

    return Get_IsActorLocallyControlled_ByPlayer(EntityOwningActor);
}

auto
    UCk_Utils_Net_UE::
    Get_EntityNetRole(
        const FCk_Handle& InEntity)
    -> ECk_Net_EntityNetRole
{
    // this is now a pass-through because Entity Lifetime utils needs the Net Fragment to be defined in CkEcs
    return UCk_Utils_EntityLifetime_UE::Get_EntityNetRole(InEntity);
}

auto
    UCk_Utils_Net_UE::
    Get_EntityNetMode(
        const FCk_Handle& InEntity)
    -> ECk_Net_NetModeType
{
    // this is now a pass-through because Entity Lifetime utils needs the Net Fragment to be defined in CkEcs
    return UCk_Utils_EntityLifetime_UE::Get_EntityNetMode(InEntity);
}

auto
    UCk_Utils_Net_UE::
    Get_IsEntityRoleMatching(
        const FCk_Handle& InEntity,
        ECk_Net_ReplicationType InReplicationType)
    -> bool
{
    switch (InReplicationType)
    {
        case ECk_Net_ReplicationType::LocalOnly:
        {
            if (const auto& MaybeActor = UCk_Utils_OwningActor_UE::TryGet_EntityOwningActor_Recursive(InEntity);
                ck::IsValid(MaybeActor))
            {
                if (MaybeActor->GetLocalRole() == ROLE_AutonomousProxy)
                { return true; }

                if (MaybeActor->GetRemoteRole() == ROLE_AutonomousProxy)
                { return false; }
            }

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
        case ECk_Net_ReplicationType::LocalClientOnly:
        {
            return Get_HasAuthority(InEntity) && Get_IsEntityNetMode_Client(InEntity);
        }
        case ECk_Net_ReplicationType::RemoteClientsOnly:
        {
            return NOT Get_HasAuthority(InEntity) && Get_IsEntityNetMode_Client(InEntity);
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
        const FCk_Handle& InEntity)
    -> bool
{
    if (ck::Is_NOT_Valid(InEntity))
    { return false; }

    if (Has(InEntity))
    { return InEntity.Has<ck::FTag_HasAuthority>(); }

    return Get_HasAuthority(UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InEntity));
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

    const auto IsLocallyOwned = Get_IsActorLocallyOwned(InActor);
    const auto NetRole        = Get_NetRole(InActor);

    switch(InReplicationType)
    {
        case ECk_Net_ReplicationType::LocalOnly:
        {
            if (IsLocallyOwned)
            { return true; }

            break;
        }
        case ECk_Net_ReplicationType::LocalAndHost:
        {
            if (IsLocallyOwned || NetRole == ECk_Net_NetModeType::Host)
            { return true; }

            break;
        }
        case ECk_Net_ReplicationType::HostOnly:
        {
            if (NetRole == ECk_Net_NetModeType::Host)
            { return true; }

            break;
        }
        case ECk_Net_ReplicationType::ClientsOnly:
        {
            if (NetRole == ECk_Net_NetModeType::Client)
            { return true; }

            break;
        }
        case ECk_Net_ReplicationType::LocalClientOnly:
        {
            if (IsLocallyOwned && NetRole == ECk_Net_NetModeType::Client)
            { return true; }

            break;
        }
        case ECk_Net_ReplicationType::RemoteClientsOnly:
        {
            if (NOT IsLocallyOwned && NetRole == ECk_Net_NetModeType::Client)
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
        return ECk_Net_NetModeType::Host;
    }

    const auto GetIsServer = [InContext]() -> bool
    {
        if (ck::Is_NOT_Valid(InContext))
        { return true; }

        const auto* World = InContext->GetWorld();

        if (ck::Is_NOT_Valid(World))
        { return true; }

        return World->IsNetMode(NM_DedicatedServer) || World->IsNetMode(NM_ListenServer);
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
    case NM_ListenServer:
    case NM_Standalone:
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
        const FCk_Handle& InHandle)
    -> bool
{
    const auto FoundHandle = UCk_Utils_EntityLifetime_UE::Get_EntityInOwnershipChain_If(InHandle,
    [](const FCk_Handle& Handle)
    {
        return Handle.Has<ck::FTag_NetMode_IsHost>();
    });

    return ck::IsValid(FoundHandle);
}

auto
    UCk_Utils_Net_UE::
    Get_IsEntityNetMode_Client(
        const FCk_Handle& InHandle)
    -> bool
{
    return NOT Get_IsEntityNetMode_Host(InHandle);
}

auto
    UCk_Utils_Net_UE::
    Get_EntityReplication(
        const FCk_Handle& InHandle)
    -> ECk_Replication
{
    if (NOT Ensure(InHandle))
    { return ECk_Replication::DoesNotReplicate; }

    return InHandle.Get<ck::FFragment_Net_Params>().Get_ConnectionSettings().Get_Replication();
}

auto
    UCk_Utils_Net_UE::
    Get_MinPing(
        const UObject* InContext)
    -> FCk_Time
{
    const auto& PrimaryPlayerState = Cast<ACk_PlayerState_UE>(UCk_Utils_Game_UE::Get_PrimaryPlayerState_AsClient(InContext));

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
    const auto& PrimaryPlayerState = Cast<ACk_PlayerState_UE>(UCk_Utils_Game_UE::Get_PrimaryPlayerState_AsClient(InContext));

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
    CK_ENSURE_IF_NOT(ck::IsValid(InContext),
        TEXT("Could not get Average Ping.{}"), ck::Context(InContext))
    { return {}; }

    if (InContext->GetWorld()->IsNetMode(NM_Standalone))
    { return {}; }

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
    const auto& PrimaryPlayerState = Cast<ACk_PlayerState_UE>(UCk_Utils_Game_UE::Get_PrimaryPlayerState_AsClient(nullptr));

    if (ck::Is_NOT_Valid(PrimaryPlayerState))
    { return {}; }

    return PrimaryPlayerState->Get_PingRangeHistoryEntries();
}
#endif
// --------------------------------------------------------------------------------------------------------------------

