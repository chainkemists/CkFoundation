#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <GameFramework/Info.h>

#include "CkNetVersionReport.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_NetVersion_WorldSubsystem_UE;

// --------------------------------------------------------------------------------------------------------------------

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
    // Trigger for the initial client -> server report: Owner is often unset at the client's BeginPlay.
    auto OnRep_Owner() -> void override;
    auto GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const -> void override;

#if NOT UE_BUILD_SHIPPING
    auto Tick(
        float InDeltaSeconds) -> void override;
#endif

public:
    // Empty until the server has stamped it.
    auto Get_ServerBuildId() const -> FString;

    // Server-authoritative; empty until the owning client has reported.
    auto Get_ClientBuildId() const -> FString;

private:
    // The mismatch surface that works on a headless dedicated server, which renders no watermark.
    UFUNCTION(Server, Reliable, WithValidation)
    void Server_ReportClientBuildId(
        const FString& InClientBuildId);

private:
    // Idempotent and change-gated, so it doubles as the live re-report path for ck.Net.BuildIdOverride.
    auto DoReportFromOwningClient() -> void;

private:
    UPROPERTY(Replicated)
    FString _ServerBuildId;

    // Server-only, deliberately NOT replicated: under bOnlyRelevantToOwner it would only reach the
    // owning client, who never reads it.
    FString _ClientBuildId;

    FString _LastReportedClientBuildId;
};

// --------------------------------------------------------------------------------------------------------------------
