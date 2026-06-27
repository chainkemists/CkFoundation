#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <GameFramework/Info.h>

#include "CkNetVersionReport.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_NetVersion_WorldSubsystem_UE;

// --------------------------------------------------------------------------------------------------------------------

// Carries the build id (short git hash) across the network so a client/server version mismatch can be
// surfaced WITHOUT requiring the project to adopt any particular GameState/PlayerState base class.
//
// One instance is spawned by the server per player and SetOwner()'d to that player's controller, so it is
// only relevant to that connection and the owning client can route a Server RPC on it. It carries both
// directions of the comparison:
//   _ServerBuildId : server -> owning client. The client reads it to flag a wrong-version server.
//   _ClientBuildId : owning client -> server (RPC). The host reads every report to flag wrong-version clients.
//
// Lifecycle is owned by UCk_NetVersion_WorldSubsystem_UE (spawn on login, destroy on logout).
UCLASS()
class CKCORE_API ACk_NetVersionReport_UE : public AInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(ACk_NetVersionReport_UE);

public:
    ACk_NetVersionReport_UE();

public:
    auto BeginPlay() -> void override;
    auto EndPlay(
        const EEndPlayReason::Type InEndPlayReason) -> void override;
    // Fires on the owning client once Owner replicates — the robust, tick-free trigger for the initial
    // client -> server report (Owner is often unset at the client's BeginPlay).
    auto OnRep_Owner() -> void override;
    auto GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const -> void override;

#if NOT UE_BUILD_SHIPPING
    auto Tick(
        float InDeltaSeconds) -> void override;
#endif

public:
    // The server's build id as seen by THIS report's owning client. Empty until the server has stamped it.
    auto Get_ServerBuildId() const -> FString;

    // The owning client's reported build id (server-authoritative). Empty until the client has reported.
    auto Get_ClientBuildId() const -> FString;

private:
    // Owning client -> server. The server stores _ClientBuildId (replicated) and logs a mismatch — the
    // surface that works on a headless dedicated server (which renders no watermark).
    UFUNCTION(Server, Reliable, WithValidation)
    void Server_ReportClientBuildId(
        const FString& InClientBuildId);

private:
    // Sends this client's build id to the server from the locally-controlled report only. Idempotent and
    // change-gated, so it doubles as the live re-report path for the dev-only ck.Net.BuildIdOverride CVar.
    auto DoReportFromOwningClient() -> void;

private:
    UPROPERTY(Replicated)
    FString _ServerBuildId;

    // Server-only state: the owning client reports this via Server RPC (or the listen-host sets it directly),
    // and only the server reads it (host watermark rows + dedicated-server log). NOT replicated — under
    // bOnlyRelevantToOwner it would only reach the owning client, who never reads it.
    FString _ClientBuildId;

    FString _LastReportedClientBuildId;
};

// --------------------------------------------------------------------------------------------------------------------
