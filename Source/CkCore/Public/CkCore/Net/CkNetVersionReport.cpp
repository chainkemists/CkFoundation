#include "CkNetVersionReport.h"

#include "CkCore/BuildId/CkBuildId.h"
#include "CkCore/CkCoreLog.h"
#include "CkCore/Net/CkNetVersionSubsystem.h"
#include "CkCore/Validation/CkIsValid.h"

#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

// --------------------------------------------------------------------------------------------------------------------

ACk_NetVersionReport_UE::
    ACk_NetVersionReport_UE()
{
    bReplicates = true;
    bAlwaysRelevant = false;
    // Only the owning client needs its own report; the host (server) sees all of them regardless.
    bOnlyRelevantToOwner = true;

#if NOT UE_BUILD_SHIPPING
    // Tick only exists to live-apply the dev-only ck.Net.BuildIdOverride CVar at runtime.
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
#else
    PrimaryActorTick.bCanEverTick = false;
#endif
}

auto
    ACk_NetVersionReport_UE::
    BeginPlay()
    -> void
{
    Super::BeginPlay();

    if (HasAuthority())
    {
        _ServerBuildId = ck::Get_ReportedBuildId();
        MARK_PROPERTY_DIRTY_FROM_NAME(ACk_NetVersionReport_UE, _ServerBuildId, this);
    }

    DoReportFromOwningClient();

    if (const auto World = GetWorld())
    {
        if (auto* Subsystem = World->GetSubsystem<UCk_NetVersion_WorldSubsystem_UE>())
        { Subsystem->Request_RegisterReport(this); }
    }
}

auto
    ACk_NetVersionReport_UE::
    EndPlay(
        const EEndPlayReason::Type InEndPlayReason)
    -> void
{
    if (const auto World = GetWorld())
    {
        if (auto* Subsystem = World->GetSubsystem<UCk_NetVersion_WorldSubsystem_UE>())
        { Subsystem->Request_UnregisterReport(this); }
    }

    Super::EndPlay(InEndPlayReason);
}

auto
    ACk_NetVersionReport_UE::
    OnRep_Owner()
    -> void
{
    Super::OnRep_Owner();

    DoReportFromOwningClient();
}

auto
    ACk_NetVersionReport_UE::
    GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const
    -> void
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    constexpr auto Params = FDoRepLifetimeParams{COND_None, REPNOTIFY_Always, true};

    DOREPLIFETIME_WITH_PARAMS_FAST(ThisType, _ServerBuildId, Params);
}

#if NOT UE_BUILD_SHIPPING
auto
    ACk_NetVersionReport_UE::
    Tick(
        float InDeltaSeconds)
    -> void
{
    Super::Tick(InDeltaSeconds);

    // Both branches are cheap no-ops when the override is unset (values already equal).
    if (HasAuthority())
    {
        if (const auto Reported = ck::Get_ReportedBuildId();
            _ServerBuildId != Reported)
        {
            _ServerBuildId = Reported;
            MARK_PROPERTY_DIRTY_FROM_NAME(ACk_NetVersionReport_UE, _ServerBuildId, this);
        }
    }

    DoReportFromOwningClient();
}
#endif

auto
    ACk_NetVersionReport_UE::
    DoReportFromOwningClient()
    -> void
{
    const auto* OwnerController = Cast<APlayerController>(GetOwner());
    if (ck::Is_NOT_Valid(OwnerController) || NOT OwnerController->IsLocalController())
    { return; }

    const auto LocalBuildId = ck::Get_ReportedBuildId();
    if (LocalBuildId == _LastReportedClientBuildId)
    { return; }
    _LastReportedClientBuildId = LocalBuildId;

    if (HasAuthority())
    {
        _ClientBuildId = LocalBuildId;
    }
    else
    {
        Server_ReportClientBuildId(LocalBuildId);
    }
}

bool
    ACk_NetVersionReport_UE::
    Server_ReportClientBuildId_Validate(
        const FString& InClientBuildId)
{
    return true;
}

void
    ACk_NetVersionReport_UE::
    Server_ReportClientBuildId_Implementation(
        const FString& InClientBuildId)
{
    _ClientBuildId = InClientBuildId;

    // Compares the REAL id, not Get_ReportedBuildId — the dev override deliberately fakes a mismatch.
    const auto ServerBuildId = ck::Get_BuildId();
    if (InClientBuildId == ServerBuildId)
    { return; }

    const auto* OwnerController = Cast<APlayerController>(GetOwner());
    const auto PlayerName = (ck::IsValid(OwnerController) && ck::IsValid(OwnerController->PlayerState))
        ? OwnerController->PlayerState->GetPlayerName()
        : FString{TEXT("<unknown>")};

    ck::core::Warning(TEXT("[Version] Client [{}] connected with build [{}] != server build [{}] - VERSION MISMATCH"),
        PlayerName, InClientBuildId, ServerBuildId);
}

auto
    ACk_NetVersionReport_UE::
    Get_ServerBuildId() const
    -> FString
{
    return _ServerBuildId;
}

auto
    ACk_NetVersionReport_UE::
    Get_ClientBuildId() const
    -> FString
{
    return _ClientBuildId;
}

// --------------------------------------------------------------------------------------------------------------------
