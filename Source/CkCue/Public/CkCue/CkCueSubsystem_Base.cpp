#include "CkCueSubsystem_Base.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Math/Arithmetic/CkArithmetic_Utils.h"
#include "CkCore/Debug/CkDebug_Utils.h"
#include "CkCore/IO/CkIO_Utils.h"

#include "CkCue/CkCue_Fragment.h"
#include "CkCue/CkCue_Log.h"

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

// --------------------------------------------------------------------------------------------------------------------

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

// --------------------------------------------------------------------------------------------------------------------

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
        CK_ENSURE_IF_NOT(ck::IsValid(InOwnerEntity),
            TEXT("OwnerEntity is invalid when trying to execute Cue [{}]"), InCueName)
        { return {}; }

        CK_ENSURE_IF_NOT(ck::IsValid(InCueClass),
            TEXT("CueClass was INVALID when trying to execute Cue [{}]"), InCueName)
        { return {}; }

        if (auto CueDefaultObject = InCueClass->GetDefaultObject<UCk_CueBase_EntityScript>();
            ck::IsValid(CueDefaultObject) &&
            CueDefaultObject->Get_ConcurrencyPolicy() == ECk_Cue_ConcurrencyPolicy::RestartExisting)
        {
            if (auto ExistingCue = ck::ActiveCues_Utils::Get_ValidEntry_ByTag(InOwnerEntity, InCueName);
                ck::IsValid(ExistingCue))
            {
                // Get the cue entity script and call restart
                if (const auto CueScript = Cast<UCk_CueBase_EntityScript>(ExistingCue.Get<ck::FFragment_EntityScript_Current>().Get_Script().Get());
                    ck::IsValid(CueScript))
                {
                    UCk_Utils_EntityScript_UE::TryInjectEntityScriptSpawnParams(CueScript, InSpawnParams);
                    CueScript->Restart();
                    return FCk_Handle_PendingEntityScript{ExistingCue};
                }
            }
        }

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

// --------------------------------------------------------------------------------------------------------------------

ACk_CueExecutor_UE::
    ACk_CueExecutor_UE()
{
    bReplicates = true;
    bAlwaysRelevant = true;
    PrimaryActorTick.bCanEverTick = false;
    PrimaryActorTick.bTickEvenWhenPaused = false;

    _EntityBridge = CreateDefaultSubobject<UCk_EntityBridge_ActorComponent_UE>(TEXT("EntityBridge"));
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

    if (ck::Is_NOT_Valid(_Subsystem_CueExecutor))
    {
        _Subsystem_CueExecutor = Cast<UCk_CueExecutor_Subsystem_Base_UE>(GetWorld()->GetSubsystemBase(_Subsystem_CueExecutorClass));
        _Subsystem_CueExecutor->_CueExecutors.Emplace(this);
    }
}

auto
    ACk_CueExecutor_UE::
    Server_RequestExecuteCue_Implementation(
        FCk_Handle InOwnerEntity,
        FGameplayTag InCueName,
        FInstancedStruct InSpawnParams)
    -> void
{
    Request_ExecuteCue(InOwnerEntity, InCueName, InSpawnParams);
}

auto
    ACk_CueExecutor_UE::
    Request_ExecuteCue_Implementation(
        FCk_Handle InOwnerEntity,
        FGameplayTag InCueName,
        FInstancedStruct InSpawnParams)
    -> void
{
    if (GetWorld()->IsNetMode(NM_DedicatedServer))
    { return; }

    CK_ENSURE_IF_NOT(ck::IsValid(_Subsystem_CueExecutor),
        TEXT("CueExecutor subsystem is invalid"))
    { return; }

    const auto& CueSubsystemClass = _Subsystem_CueExecutor->Get_CueSubsystemClass();
    auto CueSubsystem = ck_cue_subsystem_base::Get_CueSubsystemFromClass(CueSubsystemClass);
    CK_ENSURE_IF_NOT(ck::IsValid(CueSubsystem),
        TEXT("CueSubsystem is invalid from executor"))
    { return; }

    const auto& CueClass = CueSubsystem->Get_CueEntityScript(InCueName);
    ck_cue_subsystem_base::ExecuteCueEntityScript(InOwnerEntity, InCueName, CueClass, InSpawnParams);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_CueExecutor_Subsystem_Base_UE::
    Initialize(
        FSubsystemCollectionBase& InCollection)
    -> void
{
    Super::Initialize(InCollection);

    if (GetWorld()->IsNetMode(NM_Client))
    { return; }

    _PostLoadMapWithWorldDelegateHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UCk_CueExecutor_Subsystem_Base_UE::OnPostLoadMapWithWorld);
    _PostLoginEventDelegateHandle = FGameModeEvents::GameModePostLoginEvent.AddUObject(this, &UCk_CueExecutor_Subsystem_Base_UE::OnPostLoginEvent);
}

auto
    UCk_CueExecutor_Subsystem_Base_UE::
    Deinitialize()
    -> void
{
    Super::Deinitialize();

    FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(_PostLoadMapWithWorldDelegateHandle);
    FGameModeEvents::GameModePostLoginEvent.Remove(_PostLoadMapWithWorldDelegateHandle);
}

auto
    UCk_CueExecutor_Subsystem_Base_UE::
    Request_ExecuteCue_Transient(
        FGameplayTag InCueName,
        FInstancedStruct InSpawnParams)
    -> FCk_Handle_PendingEntityScript
{
    CK_ENSURE_IF_NOT(_CueExecutors.Num() > 0,
        TEXT("No CueExecutor Actors available. Unable to Execute Cue"))
    { return {}; }

    auto CueExecutor = _CueExecutors[_NextAvailableExecutor];

    CK_ENSURE_IF_NOT(ck::IsValid(CueExecutor),
        TEXT("Next Available Cue Executor Actor at Index [{}] is INVALID"), _NextAvailableExecutor)
    { return {}; }

    auto CueExecutorEntity = UCk_Utils_OwningActor_UE::Get_ActorEntityHandle(CueExecutor);
    return Request_ExecuteCue(CueExecutorEntity, InCueName, InSpawnParams);
}

auto
    UCk_CueExecutor_Subsystem_Base_UE::
    Request_ExecuteCue_Transient_Local(
        FGameplayTag InCueName,
        FInstancedStruct InSpawnParams)
    -> FCk_Handle_PendingEntityScript
{
    CK_ENSURE_IF_NOT(_CueExecutors.Num() > 0,
        TEXT("No CueExecutor Actors available. Unable to Execute Cue"))
    { return {}; }

    auto CueExecutor = _CueExecutors[_NextAvailableExecutor];

    CK_ENSURE_IF_NOT(ck::IsValid(CueExecutor),
        TEXT("Next Available Cue Executor Actor at Index [{}] is INVALID"), _NextAvailableExecutor)
    { return {}; }

    auto CueExecutorEntity = UCk_Utils_OwningActor_UE::Get_ActorEntityHandle(CueExecutor);
    return Request_ExecuteCue_Local(CueExecutorEntity, InCueName, InSpawnParams);
}

auto
    UCk_CueExecutor_Subsystem_Base_UE::
    Request_ExecuteCue(
        const FCk_Handle& InOwnerEntity,
        FGameplayTag InCueName,
        FInstancedStruct InSpawnParams)
    -> FCk_Handle_PendingEntityScript
{
    CK_ENSURE_IF_NOT(ck::IsValid(InOwnerEntity),
        TEXT("OwnerEntity is invalid when trying to execute Cue [{}]"), InCueName)
    { return {}; }

    CK_ENSURE_IF_NOT(_CueExecutors.Num() > 0,
        TEXT("No CueExecutor Actors available. Unable to Execute Cue"))
    { return {}; }

    if (GetWorld()->IsNetMode(NM_Standalone))
    {
        // For standalone, execute directly without replication
        const auto& CueSubsystemClass = Get_CueSubsystemClass();
        auto CueSubsystem = ck_cue_subsystem_base::Get_CueSubsystemFromClass(CueSubsystemClass);
        CK_ENSURE_IF_NOT(ck::IsValid(CueSubsystem),
            TEXT("CueSubsystem is invalid in standalone mode"))
        { return {}; }

        const auto& CueClass = CueSubsystem->Get_CueEntityScript(InCueName);
        return ck_cue_subsystem_base::ExecuteCueEntityScript(InOwnerEntity, InCueName, CueClass, InSpawnParams);
    }

    _NextAvailableExecutor = UCk_Utils_Arithmetic_UE::Get_Increment_WithWrap(
        _NextAvailableExecutor, FCk_IntRange{0, _CueExecutors.Num()}, ECk_Inclusiveness::Exclusive);

    auto CueExecutor = _CueExecutors[_NextAvailableExecutor];
    CK_ENSURE_IF_NOT(ck::IsValid(CueExecutor),
        TEXT("Next Available Cue Executor Actor at Index [{}] is INVALID"), _NextAvailableExecutor)
    { return {}; }

    if (GetWorld()->IsNetMode(NM_DedicatedServer) || GetWorld()->IsNetMode(NM_ListenServer))
    {
        CueExecutor->Request_ExecuteCue(InOwnerEntity, InCueName, InSpawnParams);
    }
    else
    {
        CueExecutor->Server_RequestExecuteCue(InOwnerEntity, InCueName, InSpawnParams);
    }

    return {};
}

auto
    UCk_CueExecutor_Subsystem_Base_UE::
    Request_ExecuteCue_Local(
        const FCk_Handle& InOwnerEntity,
        FGameplayTag InCueName,
        FInstancedStruct InSpawnParams)
    -> FCk_Handle_PendingEntityScript
{
    CK_ENSURE_IF_NOT(ck::IsValid(InOwnerEntity),
        TEXT("OwnerEntity is invalid when trying to execute local Cue [{}]"), InCueName)
    { return {}; }

    const auto& CueSubsystemClass = Get_CueSubsystemClass();
    auto CueSubsystem = ck_cue_subsystem_base::Get_CueSubsystemFromClass(CueSubsystemClass);
    CK_ENSURE_IF_NOT(ck::IsValid(CueSubsystem),
        TEXT("CueSubsystem is invalid for local cue execution"))
    { return {}; }

    const auto& CueClass = CueSubsystem->Get_CueEntityScript(InCueName);
    return ck_cue_subsystem_base::ExecuteCueEntityScript(InOwnerEntity, InCueName, CueClass, InSpawnParams);
}

auto
    UCk_CueExecutor_Subsystem_Base_UE::
    DoSpawnCueExecutorActorsForPlayerController(
        APlayerController* InPlayerController)
    -> void
{
    auto AlreadyContainsPC = false;
    _ValidPlayerControllers.Add(InPlayerController, &AlreadyContainsPC);

    if (AlreadyContainsPC)
    { return; }

    // Spawn one executor per player controller for now
    // Derived classes can override this behavior if needed
    auto CueExecutor = Cast<ACk_CueExecutor_UE>
    (
        UCk_Utils_Actor_UE::Request_SpawnActor
        (
            FCk_Utils_Actor_SpawnActor_Params{GetWorld(), ACk_CueExecutor_UE::StaticClass()}
            .Set_SpawnPolicy(ECk_Utils_Actor_SpawnActorPolicy::CannotSpawnInPersistentLevel)
            .Set_NetworkingType(ECk_Actor_NetworkingType::Replicated),
            [&](AActor* InActor)
            {
                const auto& NewCueExecutor = Cast<ACk_CueExecutor_UE>(InActor);
                NewCueExecutor->InjectCueExecutorSubsystemClass(this->GetClass());
            }
        )
    );

    _CueExecutors.Emplace(CueExecutor);
}

auto
    UCk_CueExecutor_Subsystem_Base_UE::
    OnPostLoadMapWithWorld(
        UWorld* InWorld)
    -> void
{
    // NOTE: If Seamless Travel is enabled this (World) Subsystem will not be torn-down, but any spawned CueReplicator Actors will be destroyed.
    // Instead of adding the CueReplicator Actors to the list of actors that persist through the travel, we re-create them once the new world is loaded.
    // 'OnSwapPlayerControllers' from the GameMode is called before we enter this function, which means all available PC are the new ones created for
    // the world we just traveled to.

    if (ck::Is_NOT_Valid(InWorld))
    { return; }

    if (GetWorld()->IsNetMode(NM_Client))
    { return; }

    _NextAvailableExecutor = 0;

    for (const auto& ValidPlayerControllersList = _ValidPlayerControllers.Array();
         const auto& PC : ValidPlayerControllersList)
    {
        if (ck::IsValid(PC) && PC->GetWorld() == InWorld)
        { continue; }

        _ValidPlayerControllers.Remove(PC);
        _CueExecutors = ck::algo::Filter(_CueExecutors, [&](const ACk_CueExecutor_UE* InCueExecutor)
        {
            if (ck::Is_NOT_Valid(InCueExecutor))
            { return false; }

            if (ck::Is_NOT_Valid(PC))
            { return true; }

            return InCueExecutor->GetWorld() == PC->GetWorld();
        });
    }

    for (auto It = InWorld->GetPlayerControllerIterator(); It; ++It)
    {
       DoSpawnCueExecutorActorsForPlayerController(It->Get());
    }
}

auto
    UCk_CueExecutor_Subsystem_Base_UE::
    OnPostLoginEvent(
        AGameModeBase* GameMode,
        APlayerController* NewPlayer)
    -> void
{
    if (NOT _ValidPlayerControllers.Contains(NewPlayer))
    {
        DoSpawnCueExecutorActorsForPlayerController(NewPlayer);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_CueSubsystem_Base_UE::
    Initialize(
        FSubsystemCollectionBase& Collection)
    -> void
{
    Super::Initialize(Collection);

    if (GIsRunning)
    {
        // Engine init already completed
        DoOnEngineInitComplete();
    }
    else
    {
        FCoreDelegates::OnFEngineLoopInitComplete.AddUObject(this, &UCk_CueSubsystem_Base_UE::DoOnEngineInitComplete);
    }
}

auto
    UCk_CueSubsystem_Base_UE::
    Deinitialize()
    -> void
{
    if (_DiscoveryDeferralTickerHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(_DiscoveryDeferralTickerHandle);
        _DiscoveryDeferralTickerHandle.Reset();
    }

    Super::Deinitialize();
}

auto
    UCk_CueSubsystem_Base_UE::
    Request_PopulateAllCues()
    -> void
{
    Request_DeferredPopulateAllCues();
}

auto
    UCk_CueSubsystem_Base_UE::
    Request_PopulateBlueprintCues()
    -> void
{
    auto CueBaseClass = Get_CueBaseClass();
    if (ck::Is_NOT_Valid(CueBaseClass))
    { return; }

    const auto& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    const auto& AssetRegistry = AssetRegistryModule.Get();

    if (AssetRegistry.IsLoadingAssets())
    {
        ck::cue::Warning(TEXT("AssetRegistry is still loading assets during Blueprint discovery"));
    }

    TArray<FAssetData> AllAssets;
    AssetRegistry.GetAllAssets(AllAssets);

    int32 ProcessedAssets = 0;
    int32 ValidCueAssets = 0;

    for (const auto& InAssetData : AllAssets)
    {
#if WITH_EDITOR
        if (NOT InAssetData.IsInstanceOf<UBlueprint>())
        { continue; }

        auto IsCueAssetTag = InAssetData.GetTagValueRef<FString>("IsCueAsset");
        if (NOT IsCueAssetTag.Equals("true"))
        { continue; }
#else
        if (NOT InAssetData.IsInstanceOf<UBlueprintGeneratedClass>())
        { continue; }
#endif

        ProcessedAssets++;

        if (const auto& AssetName = InAssetData.AssetName.ToString();
            AssetName.Contains(TEXT("AssetFactory")) ||
            AssetName.Contains(TEXT("_Factory")) ||
            AssetName.Contains(TEXT("Default__")))
        { continue; }

#if WITH_EDITOR
        auto ResolvedObject = InAssetData.GetSoftObjectPath().TryLoad();
        if (ck::Is_NOT_Valid(ResolvedObject))
        { continue; }

        auto Blueprint = Cast<UBlueprint>(ResolvedObject);
        if (ck::Is_NOT_Valid(Blueprint))
        { continue; }

        if (ck::Is_NOT_Valid(Blueprint->GeneratedClass))
        { continue; }

        if (NOT Blueprint->GeneratedClass->IsChildOf(CueBaseClass))
        { continue; }

        auto DefaultObject = Blueprint->GeneratedClass->GetDefaultObject();
        if (ck::Is_NOT_Valid(DefaultObject))
        { continue; }

        auto CueObject = Cast<UCk_CueBase_EntityScript>(DefaultObject);
#else
        auto BlueprintGeneratedClass = Cast<UBlueprintGeneratedClass>(InAssetData.GetAsset());
        if (ck::Is_NOT_Valid(BlueprintGeneratedClass))
        { continue; }

        if (NOT BlueprintGeneratedClass->IsChildOf(CueBaseClass))
        { continue; }

        auto DefaultObject = BlueprintGeneratedClass->GetDefaultObject();
        if (ck::Is_NOT_Valid(DefaultObject))
        { continue; }

        auto CueObject = Cast<UCk_CueBase_EntityScript>(DefaultObject);
#endif

        if (ck::Is_NOT_Valid(CueObject))
        { continue; }

        auto CueName = CueObject->Get_CueName();

        if (CueName == TAG_Cue_DoNotExecute || NOT CueName.IsValid())
        { continue; }

        if (_DiscoveredCues.Contains(CueName))
        { continue; }

        _DiscoveredCues.Add(CueName, CueObject->GetClass());
        ValidCueAssets++;
    }

    ck::cue::Log(TEXT("Blueprint assets: Processed=[{}], Valid cues=[{}]"),
                 ProcessedAssets, ValidCueAssets);

    auto FallbackCueCount = 0;

    for (TObjectIterator<UBlueprintGeneratedClass> ClassIterator; ClassIterator; ++ClassIterator)
    {
        auto GeneratedClass = *ClassIterator;

        if (ck::Is_NOT_Valid(GeneratedClass))
        { continue; }

        if (GeneratedClass->HasAnyClassFlags(CLASS_Abstract))
        { continue; }

        if (UCk_Utils_IO_UE::Get_IsTemporaryAsset(GeneratedClass->GetName()))
        { continue; }

        if (NOT GeneratedClass->IsChildOf(CueBaseClass))
        { continue; }

        auto DefaultObject = Cast<UCk_CueBase_EntityScript>(GeneratedClass->GetDefaultObject());
        if (ck::Is_NOT_Valid(DefaultObject))
        { continue; }

        auto CueName = DefaultObject->Get_CueName();

        if (CueName == TAG_Cue_DoNotExecute || NOT CueName.IsValid())
        { continue; }

        if (_DiscoveredCues.Contains(CueName))
        { continue; }

        _DiscoveredCues.Add(CueName, GeneratedClass);
        FallbackCueCount++;
    }

    if (FallbackCueCount > 0)
    {
        ck::cue::Log(TEXT("Fallback class iteration found [{}] additional cues"), FallbackCueCount);
    }
}

auto
    UCk_CueSubsystem_Base_UE::
    DoOnEngineInitComplete()
    -> void
{
    Request_PopulateAllCues();

    const auto& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    AssetRegistryModule.Get().OnAssetAdded().AddUObject(this, &UCk_CueSubsystem_Base_UE::DoHandleAssetAddedDeleted);
    AssetRegistryModule.Get().OnAssetRemoved().AddUObject(this, &UCk_CueSubsystem_Base_UE::DoHandleAssetAddedDeleted);
    AssetRegistryModule.Get().OnAssetRenamed().AddUObject(this, &UCk_CueSubsystem_Base_UE::DoHandleRenamed);
    AssetRegistryModule.Get().OnAssetUpdated().AddUObject(this, &UCk_CueSubsystem_Base_UE::DoAssetUpdated);
    AssetRegistryModule.Get().OnAssetUpdatedOnDisk().AddUObject(this, &UCk_CueSubsystem_Base_UE::DoAssetUpdated);
}

auto
   UCk_CueSubsystem_Base_UE::
   DoHandleAssetAddedDeleted(
       const FAssetData& InAssetData)
    -> void
{
   Request_ProcessAssetUpdate(InAssetData);
}

auto
   UCk_CueSubsystem_Base_UE::
   Request_ProcessAssetUpdate(
       const FAssetData& InAssetData)
    -> void
{
   auto CueBaseClass = Get_CueBaseClass();
   if (ck::Is_NOT_Valid(CueBaseClass))
   { return; }

#if WITH_EDITOR
   // In editor, we can use tag filtering to avoid unnecessary loading
   if (const auto IsCueAssetTag = InAssetData.GetTagValueRef<FString>("IsCueAsset");
       NOT IsCueAssetTag.Equals("true"))
   { return; }

   if (InAssetData.IsInstanceOf<UBlueprint>())
   {
       auto Blueprint = Cast<UBlueprint>(InAssetData.GetAsset());
       if (ck::IsValid(Blueprint) &&
           ck::IsValid(Blueprint->GeneratedClass) &&
           Blueprint->GeneratedClass->IsChildOf(CueBaseClass))
       {
           Request_PopulateAllCues();
       }
   }
#else
   // In packaged builds, check all BlueprintGeneratedClass assets
   if (InAssetData.IsInstanceOf<UBlueprintGeneratedClass>())
   {
       auto GeneratedClass = Cast<UBlueprintGeneratedClass>(InAssetData.GetAsset());
       if (ck::IsValid(GeneratedClass) && GeneratedClass->IsChildOf(CueBaseClass))
       {
           Request_PopulateAllCues();
       }
   }
#endif
}

auto
    UCk_CueSubsystem_Base_UE::
    DoHandleRenamed(
        const FAssetData& InAssetData,
        const FString&)
    -> void
{
    DoHandleAssetAddedDeleted(InAssetData);
}

auto
    UCk_CueSubsystem_Base_UE::
    DoAssetUpdated(
        const FAssetData& InAssetData) -> void
{
    DoHandleAssetAddedDeleted(InAssetData);
}

auto
    UCk_CueSubsystem_Base_UE::
    Get_CueEntityScript(
        const FGameplayTag& InCueName)
    -> TSubclassOf<UCk_CueBase_EntityScript>
{
#if WITH_EDITOR
    if (_DiscoveredCues.IsEmpty())
    { Request_PopulateAllCues(); }
#endif

    const auto FoundCue = _DiscoveredCues.Find(InCueName);

    CK_ENSURE_IF_NOT(ck::IsValid(FoundCue, ck::IsValid_Policy_NullptrOnly{}),
        TEXT("Failed to find Cue with Name [{}]! Cue Subsystem [{}] has not discovered any Cue with that name, does it even exist?"),
        InCueName,
        this)
    { return {}; }

    return *FoundCue;
}

auto
    UCk_CueSubsystem_Base_UE::
    Get_DiscoveredCues() const
    -> const TMap<FGameplayTag, TSubclassOf<UCk_CueBase_EntityScript>>&
{
    return _DiscoveredCues;
}

auto
    UCk_CueSubsystem_Base_UE::
    Request_DeferredPopulateAllCues()
    -> void
{
    if (_DiscoveredCues.IsEmpty())
    {
        ck::cue::Log(TEXT("No cues discovered yet - executing discovery immediately"));
        DoExecutePopulateAllCues();
        return;
    }

    if (_DiscoveryDeferralTickerHandle.IsValid())
    {
        _DiscoveryDeferralFramesRemaining = DISCOVERY_DEFERRAL_FRAMES;
        ck::cue::Log(TEXT("Cue discovery deferred - resetting {} frame counter"), DISCOVERY_DEFERRAL_FRAMES);
    }
    else
    {
        _DiscoveryDeferralFramesRemaining = DISCOVERY_DEFERRAL_FRAMES;
        _DiscoveryDeferralTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
            FTickerDelegate::CreateUObject(this, &UCk_CueSubsystem_Base_UE::DoTickDeferredDiscovery)
        );
        ck::cue::Log(TEXT("Cue discovery deferred - starting {} frame counter"), DISCOVERY_DEFERRAL_FRAMES);
    }
}

auto
    UCk_CueSubsystem_Base_UE::
    DoTickDeferredDiscovery(
        float InDeltaTime)
    -> bool
{
    _DiscoveryDeferralFramesRemaining--;

    if (_DiscoveryDeferralFramesRemaining <= 0)
    {
        DoExecutePopulateAllCues();
        _DiscoveryDeferralTickerHandle.Reset();
        return false;
    }

    return true;
}

auto
    UCk_CueSubsystem_Base_UE::
    DoExecutePopulateAllCues()
    -> void
{
    const auto StartTime = FPlatformTime::Seconds();

    _DiscoveredCues.Empty();

    auto CueBaseClass = Get_CueBaseClass();
    CK_ENSURE_IF_NOT(ck::IsValid(CueBaseClass),
        TEXT("CueBaseClass is INVALID. Derived subsystem must implement Get_CueBaseClass"))
    { return; }

    ck::cue::Log(TEXT("=== CUE DISCOVERY START ==="));
    ck::cue::Log(TEXT("CueBaseClass: [{}]"), CueBaseClass->GetName());

    // Find all loaded classes that inherit from CueBaseClass (C++/Angelscript)
    auto LoadedClassCount = 0;
    auto CueChildClasses = 0;
    auto ValidCueClassCount = 0;

    const auto ClassScanStartTime = FPlatformTime::Seconds();

    for (TObjectIterator<UClass> ClassIterator; ClassIterator; ++ClassIterator)
    {
        auto Class = *ClassIterator;
        LoadedClassCount++;

        if (ck::Is_NOT_Valid(Class) || Class->HasAnyClassFlags(CLASS_Abstract))
        { continue; }

        if (UCk_Utils_IO_UE::Get_IsTemporaryAsset(Class->GetName()))
        { continue; }

        if (NOT Class->IsChildOf(CueBaseClass))
        { continue; }

        CueChildClasses++;

        auto DefaultObject = Cast<UCk_CueBase_EntityScript>(Class->GetDefaultObject());
        if (ck::Is_NOT_Valid(DefaultObject))
        { continue; }

        auto CueName = DefaultObject->Get_CueName();

        if (NOT ck::IsValid(CueName))
        { continue; }

        if (CueName == TAG_Cue_DoNotExecute)
        { continue; }

        if (_DiscoveredCues.Contains(CueName))
        {
            auto ExistingClassName = _DiscoveredCues[CueName]->GetName();
            auto NewClassName = Class->GetName();

            if (NewClassName.Contains(TEXT("REINST_")) || ExistingClassName.Contains(TEXT("REINST_")))
            {
                if (NOT NewClassName.Contains(TEXT("REINST_")))
                {
                    _DiscoveredCues[CueName] = Class;
                }
                continue;
            }

            CK_TRIGGER_ENSURE(TEXT("Duplicate CueName [{}] found! Existing: [{}], New: [{}]"),
                CueName, _DiscoveredCues[CueName], Class);
            continue;
        }

        _DiscoveredCues.Add(CueName, Class);
        ValidCueClassCount++;
    }

    const auto ClassScanDuration = (FPlatformTime::Seconds() - ClassScanStartTime) * 1000.0;
    ck::cue::Log(TEXT("C++ class scan: Total=[{}], CueChildren=[{}], Valid=[{}], Time=[{:.2f}ms]"),
                 LoadedClassCount, CueChildClasses, ValidCueClassCount, ClassScanDuration);

    const auto BlueprintScanStartTime = FPlatformTime::Seconds();
    Request_PopulateBlueprintCues();
    const auto BlueprintScanDuration = (FPlatformTime::Seconds() - BlueprintScanStartTime) * 1000.0;

    ck::cue::Log(TEXT("Blueprint scan: Time=[{:.2f}ms]"), BlueprintScanDuration);

    const auto TotalDuration = (FPlatformTime::Seconds() - StartTime) * 1000.0;

    ck::cue::Log(TEXT("=== FINAL CUE DISCOVERY RESULTS ==="));
    ck::cue::Log(TEXT("Total cues discovered: [{}], Total time: [{:.2f}ms]"),
                 _DiscoveredCues.Num(), TotalDuration);
}


// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_GenericCueExecutor_Subsystem_UE::
    Get_CueSubsystemClass() const
    -> TSubclassOf<UCk_CueSubsystem_Base_UE>
{
    return UCk_GenericCueSubsystem_UE::StaticClass();
}

auto
    UCk_GenericCueSubsystem_UE::
    Get_CueBaseClass() const
    -> TSubclassOf<UCk_CueBase_EntityScript>
{
    return UCk_GenericCue_EntityScript::StaticClass();
}

// --------------------------------------------------------------------------------------------------------------------
