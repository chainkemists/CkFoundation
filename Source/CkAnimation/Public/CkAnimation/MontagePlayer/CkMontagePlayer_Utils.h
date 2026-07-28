#pragma once

#include "CkAnimation/MontagePlayer/CkMontagePlayer_Fragment.h"
#include "CkEcsExt/CkEcsExt_Utils.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Request/CkRequest_Completion.h"

#include "CkMontagePlayer_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_MontagePlayer_HandleRequests;
}

struct FMontagePlayerRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_MontagePlayer"))
class CKANIMATION_API UCk_Utils_MontagePlayer_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_MontagePlayer_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_MontagePlayer);

public:
    friend class ck::FProcessor_MontagePlayer_HandleRequests;
    friend struct ::FMontagePlayerRepHandlerRegistrar;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|MontagePlayer",
              DisplayName="[Ck][MontagePlayer] Add Feature")
    static FCk_Handle_MontagePlayer
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_MontagePlayer_ParamsData& InParams,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|MontagePlayer",
              DisplayName="[Ck][MontagePlayer] Create")
    static FCk_Handle_MontagePlayer
    Create(
        UPARAM(ref) FCk_Handle& InOwner,
        const FCk_Fragment_MontagePlayer_ParamsData& InParams,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

    // Re-binds the runtime SkeletalMeshComponent target — the ONE param that cannot round-trip a save/load
    // (a live component can't be captured or rebuilt from a snapshot payload). A restored MontagePlayer
    // comes back composed but inert; call this with the re-created mesh (e.g. a rebuilt Iskm proxy's
    // BaseSKMC) to revive server-side playback.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|MontagePlayer",
              DisplayName="[Ck][MontagePlayer] Request Rebind SkeletalMeshComponent",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_MontagePlayer
    Request_RebindSkeletalMeshComponent(
        UPARAM(ref) FCk_Handle_MontagePlayer& InMontagePlayer,
        USkeletalMeshComponent* InSkeletalMeshComponent,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|MontagePlayer",
              DisplayName="[Ck][MontagePlayer] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|MontagePlayer",
        DisplayName="[Ck][MontagePlayer] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_MontagePlayer
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|MontagePlayer",
        DisplayName="[Ck][MontagePlayer] Handle -> MontagePlayer Handle",
        meta = (CompactNodeTitle = "<AsMontagePlayer>", BlueprintAutocast))
    static FCk_Handle_MontagePlayer
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid MontagePlayer Handle",
        Category = "Ck|Utils|MontagePlayer",
        meta = (CompactNodeTitle = "INVALID_MontagePlayerHandle", Keywords = "make"))
    static FCk_Handle_MontagePlayer
    Get_InvalidHandle() { return {}; }

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|MontagePlayer",
              DisplayName="[Ck][MontagePlayer] Get Current Montage")
    static UAnimMontage*
    Get_CurrentMontage(
        const FCk_Handle_MontagePlayer& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|MontagePlayer",
              DisplayName="[Ck][MontagePlayer] Get Current Section")
    static FName
    Get_CurrentSection(
        const FCk_Handle_MontagePlayer& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|MontagePlayer",
              DisplayName="[Ck][MontagePlayer] Get Is Playing")
    static bool
    Get_IsPlaying(
        const FCk_Handle_MontagePlayer& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|MontagePlayer",
              DisplayName="[Ck][MontagePlayer] Get Play Instance Id")
    static int32
    Get_PlayInstanceId(
        const FCk_Handle_MontagePlayer& InHandle);

public:
    /** AUTHORITY ONLY. Ensures and no-ops on clients. The replication callback uses
     *  the private FromReplication entry point instead. */
    UFUNCTION(BlueprintCallable,
              BlueprintAuthorityOnly,
              Category = "Ck|Utils|MontagePlayer",
              DisplayName="[Ck][MontagePlayer] Request Play",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_MontagePlayer
    Request_Play(
        UPARAM(ref) FCk_Handle_MontagePlayer& InHandle,
        const FCk_Request_MontagePlayer_Play& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              BlueprintAuthorityOnly,
              Category = "Ck|Utils|MontagePlayer",
              DisplayName="[Ck][MontagePlayer] Request Stop",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_MontagePlayer
    Request_Stop(
        UPARAM(ref) FCk_Handle_MontagePlayer& InHandle,
        const FCk_Request_MontagePlayer_Stop& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              BlueprintAuthorityOnly,
              Category = "Ck|Utils|MontagePlayer",
              DisplayName="[Ck][MontagePlayer] Request Pause",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_MontagePlayer
    Request_Pause(
        UPARAM(ref) FCk_Handle_MontagePlayer& InHandle,
        const FCk_Request_MontagePlayer_Pause& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              BlueprintAuthorityOnly,
              Category = "Ck|Utils|MontagePlayer",
              DisplayName="[Ck][MontagePlayer] Request Resume",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_MontagePlayer
    Request_Resume(
        UPARAM(ref) FCk_Handle_MontagePlayer& InHandle,
        const FCk_Request_MontagePlayer_Resume& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              BlueprintAuthorityOnly,
              Category = "Ck|Utils|MontagePlayer",
              DisplayName="[Ck][MontagePlayer] Request Jump To Section",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_MontagePlayer
    Request_JumpToSection(
        UPARAM(ref) FCk_Handle_MontagePlayer& InHandle,
        const FCk_Request_MontagePlayer_JumpToSection& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|MontagePlayer",
              DisplayName = "[Ck][MontagePlayer] Bind To OnStarted")
    static FCk_Handle_MontagePlayer
    BindTo_OnStarted(
        UPARAM(ref) FCk_Handle_MontagePlayer& InHandle,
        const FCk_Delegate_MontagePlayer_OnStarted& InDelegate,
        ECk_Signal_BindingPolicy InBehavior = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|MontagePlayer",
              DisplayName = "[Ck][MontagePlayer] Unbind From OnStarted")
    static FCk_Handle_MontagePlayer
    UnbindFrom_OnStarted(
        UPARAM(ref) FCk_Handle_MontagePlayer& InHandle,
        const FCk_Delegate_MontagePlayer_OnStarted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|MontagePlayer",
              DisplayName = "[Ck][MontagePlayer] Bind To OnFinished")
    static FCk_Handle_MontagePlayer
    BindTo_OnFinished(
        UPARAM(ref) FCk_Handle_MontagePlayer& InHandle,
        const FCk_Delegate_MontagePlayer_OnFinished& InDelegate,
        ECk_Signal_BindingPolicy InBehavior = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|MontagePlayer",
              DisplayName = "[Ck][MontagePlayer] Unbind From OnFinished")
    static FCk_Handle_MontagePlayer
    UnbindFrom_OnFinished(
        UPARAM(ref) FCk_Handle_MontagePlayer& InHandle,
        const FCk_Delegate_MontagePlayer_OnFinished& InDelegate);

public:
    static auto
    Request_TryReplicateMontagePlayer(
        FCk_Handle_MontagePlayer& InHandle) -> void;

private:
    static auto
    DoDispatchReplicatedState(
        FCk_Handle_MontagePlayer& InHandle,
        const FCk_MontagePlayer_State& InState,
        bool InIsOnAdd) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
