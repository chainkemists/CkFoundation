#include "CkCueSubsystem_Base.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Math/Arithmetic/CkArithmetic_Utils.h"
#include "CkCore/Debug/CkDebug_Utils.h"
#include "CkCore/IO/CkIO_Utils.h"

#include "CkCue/CkCue_Fragment.h"
#include "CkCue/CkCue_Log.h"
#include "CkCue/Settings/CkCue_Settings.h"
#include "CkEcs/EntityConstructionScript/CkEntity_ConstructionScript.h"

#include "CkEcs/EntityScript/CkEntityScript_Utils.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"
#include "CkEntityBridge/Public/CkEntityBridge/CkEntityBridge_ConstructionScript.h"

#include <AssetRegistry/AssetRegistryModule.h>
#include <Net/UnrealNetwork.h>
#include <Net/Core/PushModel/PushModel.h>

#if WITH_EDITOR
#include <Editor.h>
#include "Engine/Blueprint.h"
#endif

/*─────────────────────────────────────────────────────────────────────────────┐
│                           CONSOLE COMMANDS                                   │
└─────────────────────────────────────────────────────────────────────────────*/

static FAutoConsoleCommand ConsoleCommand_PrintCues(
    TEXT("ck.cue.print"),
    TEXT("Print all discovered cues from all cue subsystems"),
    FConsoleCommandDelegate::CreateLambda([]()
    {
        int32 SubsystemCount = 0;
        int32 TotalCues = 0;

        for (TObjectIterator<UCk_CueSubsystem_Base_UE> It; It; ++It)
        {
            auto CueSubsystem = *It;
            if (ck::Is_NOT_Valid(CueSubsystem))
            { continue; }

            SubsystemCount++;
            const auto& DiscoveredCues = CueSubsystem->Get_DiscoveredCues();

            ck::cue::Log(TEXT("=== SUBSYSTEM: [{}] ==="), CueSubsystem->GetClass()->GetName());
            ck::cue::Log(TEXT("Discovered cues: [{}]"), DiscoveredCues.Num());

            for (const auto& [CueName, CueClass] : DiscoveredCues)
            {
                ck::cue::Log(TEXT("  [{}] -> [{}]"), CueName, CueClass->GetName());
                TotalCues++;
            }
        }

        ck::cue::Log(TEXT("=== SUMMARY ==="));
        ck::cue::Log(TEXT("Total subsystems: [{}], Total cues: [{}]"), SubsystemCount, TotalCues);

        if (SubsystemCount == 0)
        {
            ck::cue::Warning(TEXT("No CueSubsystems found!"));
        }
    })
);

static FAutoConsoleCommand ConsoleCommand_RediscoverCues(
    TEXT("ck.cue.rediscover"),
    TEXT("Force re-discovery of all cues in all cue subsystems"),
    FConsoleCommandDelegate::CreateLambda([]()
    {
        int32 SubsystemCount = 0;

        for (TObjectIterator<UCk_CueSubsystem_Base_UE> It; It; ++It)
        {
            auto CueSubsystem = *It;
            if (ck::Is_NOT_Valid(CueSubsystem))
            { continue; }

            SubsystemCount++;
            ck::cue::Log(TEXT("Re-discovering cues for subsystem: [{}]"), CueSubsystem->GetClass()->GetName());
            CueSubsystem->Request_PopulateAllCues();
            ck::cue::Log(TEXT("  Found [{}] cues"), CueSubsystem->Get_DiscoveredCues().Num());
        }

        if (SubsystemCount == 0)
        {
            ck::cue::Warning(TEXT("No CueSubsystems found!"));
        }
        else
        {
            ck::cue::Log(TEXT("Re-discovery complete for [{}] subsystems"), SubsystemCount);
        }
    })
);

/*─────────────────────────────────────────────────────────────────────────────┐
│                             INTERNAL HELPERS                                  │
└─────────────────────────────────────────────────────────────────────────────*/

namespace ck_cue_subsystem_base
{
    auto
        ExecuteCueEntityScript(
            FCk_Handle InOwnerEntity,
            const FGameplayTag& InCueName,
            TSubclassOf<UCk_CueBase_EntityScript> InCueClass,
            const FInstancedStruct& InSpawnParams)
        -> FCk_Handle_PendingEntityScript
    {
        CK_ENSURE_IF_NOT(ck::IsValid(InCueClass),
            TEXT("CueClass was INVALID when trying to execute Cue [{}]"), InCueName)
        { return {}; }

        auto CueDefaultObject = InCueClass->GetDefaultObject<UCk_CueBase_EntityScript>();

        if (ck::Is_NOT_Valid(InOwnerEntity))
        {
            if (ck::IsValid(CueDefaultObject) &&
                CueDefaultObject->Get_OwnerValidationPolicy() == ECk_Cue_OwnerValidationPolicy::RequireValid)
            {
                CK_ENSURE_IF_NOT(false,
                    TEXT("OwnerEntity is invalid when trying to execute Cue [{}] (OwnerValidationPolicy = RequireValid)"), InCueName)
                { return {}; }
            }

            ck::cue::Verbose(TEXT("Skipping cue [{}] - OwnerEntity is invalid (OwnerValidationPolicy = SkipIfInvalid)"), InCueName);
            return {};
        }

        if (ck::IsValid(CueDefaultObject) &&
            CueDefaultObject->Get_ConcurrencyPolicy() == ECk_Cue_ConcurrencyPolicy::RestartExisting)
        {
            if (auto ExistingCue = ck::ActiveCues_Utils::Get_ValidEntry_ByTag(InOwnerEntity, InCueName);
                ck::IsValid(ExistingCue))
            {
                if (const auto CueScript = Cast<UCk_CueBase_EntityScript>(ExistingCue.Get<ck::FFragment_EntityScript_Current>().Get_Script().Get());
                    ck::IsValid(CueScript))
                {
                    ck::cue::Verbose(TEXT("Restarting existing cue [{}] on entity [{}]"), InCueName, InOwnerEntity);
                    UCk_Utils_EntityScript_UE::TryInjectEntityScriptSpawnParams(CueScript, InSpawnParams);
                    CueScript->Restart();
                    return FCk_Handle_PendingEntityScript{ExistingCue};
                }
            }
        }

        ck::cue::Verbose(TEXT("Executing cue [{}] on entity [{}]"), InCueName, InOwnerEntity);
        auto PendingEntityScript = UCk_Utils_EntityScript_UE::Request_SpawnEntity(InOwnerEntity, InCueClass, InSpawnParams);

        return PendingEntityScript;
    }

    auto
        Get_CueSubsystemFromClass(
            TSubclassOf<UCk_CueSubsystem_Base_UE> InCueSubsystemClass)
        -> UCk_CueSubsystem_Base_UE*
    {
        CK_ENSURE_IF_NOT(ck::IsValid(GEngine),
            TEXT("GEngine is invalid when trying to get CueSubsystem"))
        { return {}; }

        CK_ENSURE_IF_NOT(ck::IsValid(InCueSubsystemClass),
            TEXT("CueSubsystemClass is invalid"))
        { return {}; }

        return Cast<UCk_CueSubsystem_Base_UE>(GEngine->GetEngineSubsystemBase(InCueSubsystemClass));
    }
}

/*─────────────────────────────────────────────────────────────────────────────┐
│                              CUE EXECUTOR ACTOR                              │
└─────────────────────────────────────────────────────────────────────────────*/

ACk_CueExecutor_UE::
    ACk_CueExecutor_UE()
{
    bReplicates = true;
    bAlwaysRelevant = true;
    PrimaryActorTick.bCanEverTick = false;
    PrimaryActorTick.bTickEvenWhenPaused = false;

    _EntityBridge = CreateDefaultSubobject<UCk_EntityBridge_ActorComponent_UE>(TEXT("EntityBridge"));
    _EntityBridge->_ConstructionScript = UCk_Entity_ConstructionScript_WithTransform_PDA::StaticClass();
}

auto
    ACk_CueExecutor_UE::
    BeginPlay()
    -> void
{
    Super::BeginPlay();

    _Subsystem_EcsWorld = GetWorld()->GetSubsystem<UCk_EcsWorld_Subsystem_UE>();
    OnRep_CueExecutorSubsystemClass();
}

auto
    ACk_CueExecutor_UE::
    GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const
    -> void
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    constexpr auto Params = FDoRepLifetimeParams{COND_None, REPNOTIFY_Always, true};

    DOREPLIFETIME_WITH_PARAMS_FAST(ThisType, _Subsystem_CueExecutorClass, Params);
}

auto
    ACk_CueExecutor_UE::
    InjectCueExecutorSubsystemClass(
        TSubclassOf<class UCk_CueExecutor_Subsystem_Base_UE> InCueExecutorSubsystemClass)
    -> void
{
    _Subsystem_CueExecutorClass = InCueExecutorSubsystemClass;
    MARK_PROPERTY_DIRTY_FROM_NAME(ThisType, _Subsystem_CueExecutorClass, this);
}

auto
    ACk_CueExecutor_UE::
    OnRep_CueExecutorSubsystemClass()
    -> void
{
    if (ck::Is_NOT_Valid(_Subsystem_CueExecutorClass))
    { return; }

    if (ck::IsValid(_Subsystem_CueExecutor))
    { return; }

    _Subsystem_CueExecutor = Cast<UCk_CueExecutor_Subsystem_Base_UE>(GetWorld()->GetSubsystemBase(_Subsystem_CueExecutorClass));
    _Subsystem_CueExecutor->_CueExecutors.Emplace(this);

    if (DoTryRegisterPlayerState())
    { return; }

    ck::cue::Verbose(TEXT("PlayerState not yet replicated for CueExecutor. Will retry every 100ms"));

    // PlayerState may not be available immediately on client join.
    // Retry via timer until it's valid. An event-driven approach (binding to
    // OnRep_PlayerState) would be preferable but requires engine modification.
    auto WeakThis = TWeakObjectPtr(this);
    GetWorld()->GetTimerManager().SetTimer( _PlayerStateRetryTimerHandle, [WeakThis]()
    {
        if (ck::Is_NOT_Valid(WeakThis))
        { return; }

        if (WeakThis->DoTryRegisterPlayerState())
        {
            ck::cue::Verbose(TEXT("Successfully registered PlayerState for CueExecutor"));
            WeakThis->GetWorld()->GetTimerManager().ClearTimer(WeakThis->_PlayerStateRetryTimerHandle);
        }
    }, 0.1f, true);
}

auto
    ACk_CueExecutor_UE::
    DoTryRegisterPlayerState()
    -> bool
{
    if (auto* OwnerPlayerState = Cast<APlayerState>(GetOwner()))
    {
        _Subsystem_CueExecutor->_ExecutorsByPlayerState.Add(OwnerPlayerState, this);
        return true;
    }
    return false;
}

auto
    ACk_CueExecutor_UE::
    Server_RequestExecuteCue_Implementation(
        FCk_Handle InOwnerEntity,
        FGameplayTag InCueName,
        const FInstancedStruct& InSpawnParams)
    -> void
{
    Request_ExecuteCue(InOwnerEntity, InCueName, InSpawnParams);
}

auto
    ACk_CueExecutor_UE::
    Server_RequestExecuteCue_Reliable_Implementation(
        FCk_Handle InOwnerEntity,
        FGameplayTag InCueName,
        const FInstancedStruct& InSpawnParams)
    -> void
{
    Request_ExecuteCue_Reliable(InOwnerEntity, InCueName, InSpawnParams);
}

auto
    ACk_CueExecutor_UE::
    Server_RequestExecuteCue_ServerOnly_Implementation(
        FCk_Handle InOwnerEntity,
        FGameplayTag InCueName,
        const FInstancedStruct& InSpawnParams)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(_Subsystem_CueExecutor),
        TEXT("CueExecutor subsystem is invalid when checking dedicated server policy for cue [{}]"), InCueName)
    { return; }

    if (GetWorld()->IsNetMode(NM_DedicatedServer) &&
        _Subsystem_CueExecutor->Get_DedicatedServerPolicy() == ECk_Cue_DedicatedServerPolicy::CosmeticOnly)
    { return; }

    CK_ENSURE_IF_NOT(ck::IsValid(_Subsystem_CueExecutor),
        TEXT("CueExecutor subsystem is invalid when executing server-only cue [{}]"), InCueName)
    { return; }

    const auto& CueSubsystemClass = _Subsystem_CueExecutor->Get_CueSubsystemClass();
    auto CueSubsystem = ck_cue_subsystem_base::Get_CueSubsystemFromClass(CueSubsystemClass);
    CK_ENSURE_IF_NOT(ck::IsValid(CueSubsystem),
        TEXT("CueSubsystem is invalid from executor when executing server-only cue [{}]"), InCueName)
    { return; }

    ck::cue::Verbose(TEXT("Executing server-only cue [{}] on entity [{}]"), InCueName, InOwnerEntity);
    const auto& CueClass = CueSubsystem->Get_CueEntityScript(InCueName);
    ck_cue_subsystem_base::ExecuteCueEntityScript(InOwnerEntity, InCueName, CueClass, InSpawnParams);
}

auto
    ACk_CueExecutor_UE::
    Server_RequestExecuteCue_ServerOnly_Reliable_Implementation(
        FCk_Handle InOwnerEntity,
        FGameplayTag InCueName,
        const FInstancedStruct& InSpawnParams)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(_Subsystem_CueExecutor),
        TEXT("CueExecutor subsystem is invalid when checking dedicated server policy for reliable cue [{}]"), InCueName)
    { return; }

    if (GetWorld()->IsNetMode(NM_DedicatedServer) &&
        _Subsystem_CueExecutor->Get_DedicatedServerPolicy() == ECk_Cue_DedicatedServerPolicy::CosmeticOnly)
    { return; }

    CK_ENSURE_IF_NOT(ck::IsValid(_Subsystem_CueExecutor),
        TEXT("CueExecutor subsystem is invalid when executing reliable server-only cue [{}]"), InCueName)
    { return; }

    const auto& CueSubsystemClass = _Subsystem_CueExecutor->Get_CueSubsystemClass();
    auto CueSubsystem = ck_cue_subsystem_base::Get_CueSubsystemFromClass(CueSubsystemClass);
    CK_ENSURE_IF_NOT(ck::IsValid(CueSubsystem),
        TEXT("CueSubsystem is invalid from executor when executing reliable server-only cue [{}]"), InCueName)
    { return; }

    ck::cue::Verbose(TEXT("Executing reliable server-only cue [{}] on entity [{}]"), InCueName, InOwnerEntity);
    const auto& CueClass = CueSubsystem->Get_CueEntityScript(InCueName);
    ck_cue_subsystem_base::ExecuteCueEntityScript(InOwnerEntity, InCueName, CueClass, InSpawnParams);
}

auto
    ACk_CueExecutor_UE::
    Server_RequestExecuteCue_ExcludingSender_Implementation(
        FCk_Handle InOwnerEntity,
        FGameplayTag InCueName,
        const FInstancedStruct& InSpawnParams)
    -> void
{
    auto OriginatingPlayerState = Cast<APlayerState>(GetOwner());
    Request_ExecuteCue_ExcludingSender(InOwnerEntity, InCueName, InSpawnParams, OriginatingPlayerState);
}

auto
    ACk_CueExecutor_UE::
    Server_RequestExecuteCue_ExcludingSender_Reliable_Implementation(
        FCk_Handle InOwnerEntity,
        FGameplayTag InCueName,
        const FInstancedStruct& InSpawnParams)
    -> void
{
    auto OriginatingPlayerState = Cast<APlayerState>(GetOwner());
    Request_ExecuteCue_ExcludingSender_Reliable(InOwnerEntity, InCueName, InSpawnParams, OriginatingPlayerState);
}

auto
    ACk_CueExecutor_UE::
    Request_ExecuteCue_Implementation(
        FCk_Handle InOwnerEntity,
        FGameplayTag InCueName,
        const FInstancedStruct& InSpawnParams)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(_Subsystem_CueExecutor),
        TEXT("CueExecutor subsystem is invalid when checking dedicated server policy for multicast cue [{}]"), InCueName)
    { return; }

    if (GetWorld()->IsNetMode(NM_DedicatedServer) &&
        _Subsystem_CueExecutor->Get_DedicatedServerPolicy() == ECk_Cue_DedicatedServerPolicy::CosmeticOnly)
    { return; }

    CK_ENSURE_IF_NOT(ck::IsValid(_Subsystem_CueExecutor),
        TEXT("CueExecutor subsystem is invalid when executing cue [{}]"), InCueName)
    { return; }

    const auto& CueSubsystemClass = _Subsystem_CueExecutor->Get_CueSubsystemClass();
    auto CueSubsystem = ck_cue_subsystem_base::Get_CueSubsystemFromClass(CueSubsystemClass);
    CK_ENSURE_IF_NOT(ck::IsValid(CueSubsystem),
        TEXT("CueSubsystem is invalid from executor when executing cue [{}]"), InCueName)
    { return; }

    const auto& CueClass = CueSubsystem->Get_CueEntityScript(InCueName);
    ck_cue_subsystem_base::ExecuteCueEntityScript(InOwnerEntity, InCueName, CueClass, InSpawnParams);
}

auto
    ACk_CueExecutor_UE::
    Request_ExecuteCue_Reliable_Implementation(
        FCk_Handle InOwnerEntity,
        FGameplayTag InCueName,
        const FInstancedStruct& InSpawnParams)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(_Subsystem_CueExecutor),
        TEXT("CueExecutor subsystem is invalid when checking dedicated server policy for reliable multicast cue [{}]"), InCueName)
    { return; }

    if (GetWorld()->IsNetMode(NM_DedicatedServer) &&
        _Subsystem_CueExecutor->Get_DedicatedServerPolicy() == ECk_Cue_DedicatedServerPolicy::CosmeticOnly)
    { return; }

    CK_ENSURE_IF_NOT(ck::IsValid(_Subsystem_CueExecutor),
        TEXT("CueExecutor subsystem is invalid when executing reliable cue [{}]"), InCueName)
    { return; }

    const auto& CueSubsystemClass = _Subsystem_CueExecutor->Get_CueSubsystemClass();
    auto CueSubsystem = ck_cue_subsystem_base::Get_CueSubsystemFromClass(CueSubsystemClass);
    CK_ENSURE_IF_NOT(ck::IsValid(CueSubsystem),
        TEXT("CueSubsystem is invalid from executor when executing reliable cue [{}]"), InCueName)
    { return; }

    const auto& CueClass = CueSubsystem->Get_CueEntityScript(InCueName);
    ck_cue_subsystem_base::ExecuteCueEntityScript(InOwnerEntity, InCueName, CueClass, InSpawnParams);
}

auto
    ACk_CueExecutor_UE::
    Request_ExecuteCue_ExcludingSender_Implementation(
        FCk_Handle InOwnerEntity,
        FGameplayTag InCueName,
        const FInstancedStruct& InSpawnParams,
        APlayerState* InExcludedPlayerState)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(_Subsystem_CueExecutor),
        TEXT("CueExecutor subsystem is invalid when checking dedicated server policy for multicast cue [{}]"), InCueName)
    { return; }

    if (GetWorld()->IsNetMode(NM_DedicatedServer) &&
        _Subsystem_CueExecutor->Get_DedicatedServerPolicy() == ECk_Cue_DedicatedServerPolicy::CosmeticOnly)
    { return; }

    if (GetWorld()->IsNetMode(NM_Client))
    {
        auto LocalPC = GetWorld()->GetFirstPlayerController();
        if (ck::IsValid(LocalPC) && ck::IsValid(LocalPC->PlayerState) && LocalPC->PlayerState == InExcludedPlayerState)
        {
            ck::cue::Verbose(TEXT("Skipping cue [{}] on excluded client"), InCueName);
            return;
        }
    }

    CK_ENSURE_IF_NOT(ck::IsValid(_Subsystem_CueExecutor),
        TEXT("CueExecutor subsystem is invalid when executing cue [{}]"), InCueName)
    { return; }

    const auto& CueSubsystemClass = _Subsystem_CueExecutor->Get_CueSubsystemClass();
    auto CueSubsystem = ck_cue_subsystem_base::Get_CueSubsystemFromClass(CueSubsystemClass);
    CK_ENSURE_IF_NOT(ck::IsValid(CueSubsystem),
        TEXT("CueSubsystem is invalid from executor when executing cue [{}]"), InCueName)
    { return; }

    const auto& CueClass = CueSubsystem->Get_CueEntityScript(InCueName);
    ck_cue_subsystem_base::ExecuteCueEntityScript(InOwnerEntity, InCueName, CueClass, InSpawnParams);
}

auto
    ACk_CueExecutor_UE::
    Request_ExecuteCue_ExcludingSender_Reliable_Implementation(
        FCk_Handle InOwnerEntity,
        FGameplayTag InCueName,
        const FInstancedStruct& InSpawnParams,
        APlayerState* InExcludedPlayerState)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(_Subsystem_CueExecutor),
        TEXT("CueExecutor subsystem is invalid when checking dedicated server policy for reliable multicast cue [{}]"), InCueName)
    { return; }

    if (GetWorld()->IsNetMode(NM_DedicatedServer) &&
        _Subsystem_CueExecutor->Get_DedicatedServerPolicy() == ECk_Cue_DedicatedServerPolicy::CosmeticOnly)
    { return; }

    if (GetWorld()->IsNetMode(NM_Client))
    {
        auto LocalPC = GetWorld()->GetFirstPlayerController();
        if (ck::IsValid(LocalPC) && ck::IsValid(LocalPC->PlayerState) && LocalPC->PlayerState == InExcludedPlayerState)
        {
            ck::cue::Verbose(TEXT("Skipping reliable cue [{}] on excluded client"), InCueName);
            return;
        }
    }

    CK_ENSURE_IF_NOT(ck::IsValid(_Subsystem_CueExecutor),
        TEXT("CueExecutor subsystem is invalid when executing reliable cue [{}]"), InCueName)
    { return; }

    const auto& CueSubsystemClass = _Subsystem_CueExecutor->Get_CueSubsystemClass();
    auto CueSubsystem = ck_cue_subsystem_base::Get_CueSubsystemFromClass(CueSubsystemClass);
    CK_ENSURE_IF_NOT(ck::IsValid(CueSubsystem),
        TEXT("CueSubsystem is invalid from executor when executing reliable cue [{}]"), InCueName)
    { return; }

    const auto& CueClass = CueSubsystem->Get_CueEntityScript(InCueName);
    ck_cue_subsystem_base::ExecuteCueEntityScript(InOwnerEntity, InCueName, CueClass, InSpawnParams);
}

/*─────────────────────────────────────────────────────────────────────────────┐
│                         GENERIC CUE IMPLEMENTATIONS                          │
└─────────────────────────────────────────────────────────────────────────────*/

auto
    UCk_GenericCueExecutor_Subsystem_UE::
    Get_CueSubsystemClass() const
    -> TSubclassOf<UCk_CueSubsystem_Base_UE>
{
    return UCk_GenericCueSubsystem_UE::StaticClass();
}

auto
    UCk_GenericCueExecutor_Subsystem_UE::
    Get_DedicatedServerPolicy() const
    -> ECk_Cue_DedicatedServerPolicy
{
    return ECk_Cue_DedicatedServerPolicy::GameplayRelevant;
}

auto
    UCk_GenericCueSubsystem_UE::
    Get_CueBaseClass() const
    -> TSubclassOf<UCk_CueBase_EntityScript>
{
    return UCk_GenericCue_EntityScript::StaticClass();
}
