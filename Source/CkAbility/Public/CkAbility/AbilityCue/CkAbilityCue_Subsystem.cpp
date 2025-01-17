#include "CkAbilityCue_Subsystem.h"

#include "CkAbility/CkAbility_Log.h"
#include "CkAbility/Ability/CkAbility_Script.h"
#include "CkAbility/Ability/CkAbility_Utils.h"
#include "CkAbility/AbilityCue/CkAbilityCue_Fragment_Data.h"
#include "CkAbility/AbilityOwner/CkAbilityOwner_Utils.h"
#include "CkAbility/Settings/CkAbility_Settings.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/IO/CkIO_Utils.h"
#include "CkCore/Math/Arithmetic/CkArithmetic_Utils.h"
#include "CkCore/Object/CkObject_Utils.h"

#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "CkEntityBridge/CkEntityBridge_Fragment_Data.h"

#include <AssetRegistry/AssetRegistryModule.h>

namespace ck_ability_cue_subsystem
{
    auto
    TrySetDebugName(
        FCk_Handle& InCueOwningEntity) -> void
    {
        auto CueOwningEntityAsAbilityOwner = UCk_Utils_AbilityOwner_UE::Cast(InCueOwningEntity);

        if (ck::Is_NOT_Valid(CueOwningEntityAsAbilityOwner))
        { return; }

        const auto& DefaultAbilities = UCk_Utils_AbilityOwner_UE::Get_DefaultAbilities(CueOwningEntityAsAbilityOwner);
        const auto& DefaultAbilityNames = ck::algo::Transform<TArray<FString>>(DefaultAbilities, [](const TSubclassOf<class UCk_Ability_Script_PDA>& InDefaultAbility)
        {
            return UCk_Utils_Debug_UE::Get_DebugName(InDefaultAbility).ToString();
        });

        UCk_Utils_Handle_UE::Set_DebugName(InCueOwningEntity,*ck::Format_UE(TEXT("Cue Owning Entity [{}]"), FString::Join(DefaultAbilityNames, TEXT(" - "))));
    }

    auto
    SpawnCue(
        FGameplayTag InCueName,
        UCk_AbilityCue_Subsystem_UE* InSubsystem_AbilityCue,
        const UCk_EcsWorld_Subsystem_UE* InSubsystem_EcsWorldSubsystem,
        const FCk_AbilityCue_Params& InParams)
    {
        CK_LOG_ERROR_IF_NOT(ck::ability, ck::IsValid(InSubsystem_AbilityCue),
            TEXT("AbilityCue Subsystem was INVALID when trying to spawn Cue [{}]."), InCueName)
        { return; }

        CK_LOG_ERROR_IF_NOT(ck::ability, ck::IsValid(InSubsystem_EcsWorldSubsystem),
            TEXT("EcsWorld Subsystem was INVALID when trying to spawn Cue [{}]."), InCueName)
        { return; }

        CK_ENSURE_IF_NOT(ck::IsValid(InCueName),
            TEXT("Unable to ExecuteAbilityCue since the CueName is [{}]"), InCueName)
        { return; }

        const auto ConstructionScript = InSubsystem_AbilityCue->Get_AbilityCue_ConstructionScript(InCueName);

        if (ck::Is_NOT_Valid(ConstructionScript))
        { return; }

        const auto TransientEntity = InSubsystem_EcsWorldSubsystem->Get_TransientEntity();

        auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(TransientEntity);
        NewEntity.Add<FCk_AbilityCue_Params>(InParams);

        ck::ability::Verbose(TEXT("Executing AbilityCue [{}] ConstructionScript [{}] with created Entity [{}]"), InCueName, ConstructionScript, NewEntity);

        ConstructionScript->Construct(NewEntity, {});
        TrySetDebugName(NewEntity);
    }
}

// --------------------------------------------------------------------------------------------------------------------

ACk_AbilityCueReplicator_UE::
    ACk_AbilityCueReplicator_UE()
{
    bReplicates = true;
    bAlwaysRelevant = true;
    this->PrimaryActorTick.bCanEverTick = false;
    this->PrimaryActorTick.bTickEvenWhenPaused = false;
}

auto
    ACk_AbilityCueReplicator_UE::
    BeginPlay()
    -> void
{
    Super::BeginPlay();

    _Subsystem_AbilityCue = GEngine->GetEngineSubsystem<UCk_AbilityCue_Subsystem_UE>();
    _Subsystem_EcsWorldSubsystem = GetWorld()->GetSubsystem<UCk_EcsWorld_Subsystem_UE>();

    if (NOT IsNetMode(NM_Client))
    { return; }

    if (ck::Is_NOT_Valid(GetOwner()))
    { return; }

    const auto AbilityCueReplicator = GetWorld()->GetSubsystem<UCk_AbilityCueReplicator_Subsystem_UE>();
    AbilityCueReplicator->_AbilityCueReplicators.Emplace(this);
}

auto
    ACk_AbilityCueReplicator_UE::
    Server_RequestExecuteAbilityCue_Implementation(
        FGameplayTag InCueName,
        FCk_AbilityCue_Params InParams)
    -> void
{
    Request_ExecuteAbilityCue(InCueName, InParams);
}

auto
    ACk_AbilityCueReplicator_UE::
    Request_ExecuteAbilityCue_Implementation(
        FGameplayTag InCueName,
        FCk_AbilityCue_Params InParams)
    -> void
{
    if (GetWorld()->IsNetMode(NM_DedicatedServer))
    { return; }

    ck_ability_cue_subsystem::SpawnCue(InCueName, _Subsystem_AbilityCue.Get(), _Subsystem_EcsWorldSubsystem.Get(), InParams);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_AbilityCueReplicator_Subsystem_UE::
    Initialize(
        FSubsystemCollectionBase& InCollection)
    -> void
{
    Super::Initialize(InCollection);

    if (GetWorld()->IsNetMode(NM_Client))
    { return; }

    // We cannot create the replicated Actors right here. We need to wait until BeginPlay
    _PostLoginDelegateHandle = FGameModeEvents::GameModePostLoginEvent.AddUObject(this, &UCk_AbilityCueReplicator_Subsystem_UE::OnLoginEvent);

    _PostLoadMapWithWorldDelegateHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UCk_AbilityCueReplicator_Subsystem_UE::OnPostLoadMapWithWorld);

    // TODO: GameInstance Subsystems are not ready yet. Investigate what the lifetime of Subsystems is
    //const auto& GameSessionSubsystem = UCk_Utils_Game_UE::Get_GameInstance(this)->GetSubsystem<UCk_GameSession_Subsystem_UE>();

    //CK_ENSURE_IF_NOT(ck::IsValid(GameSessionSubsystem), TEXT("Failed to retrieve the GameSession Subsystem!"))
    //{ return; }

    //_PostFireUnbind_Connection = ck::UUtils_Signal_OnLoginEvent_PostFireUnbind::Bind<&ThisType::OnPlayerControllerReady>
    //(
    //    this,
    //    GameSessionSubsystem->Get_SignalHandle(),
    //    ECk_Signal_BindingPolicy::FireIfPayloadInFlight,
    //    ECk_Signal_PostFireBehavior::Unbind
    //);
}

auto
    UCk_AbilityCueReplicator_Subsystem_UE::
    Deinitialize()
    -> void
{
    Super::Deinitialize();

    FGameModeEvents::GameModePostLoginEvent.Remove(_PostLoginDelegateHandle);
    FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(_PostLoadMapWithWorldDelegateHandle);
}

auto
    UCk_AbilityCueReplicator_Subsystem_UE::
    Request_ExecuteAbilityCue(
        FGameplayTag InCueName,
        const FCk_AbilityCue_Params& InRequest)
    -> void
{
    CK_LOG_ERROR_IF_NOT(ck::ability, _AbilityCueReplicators.Num() > 0,
        TEXT("No AbilityCueReplicator Actors available. Unable to Execute Ability Cue"))
    { return; }

    if (GetWorld()->IsNetMode(NM_Standalone))
    { return; }

    _NextAvailableReplicator = UCk_Utils_Arithmetic_UE::Get_Increment_WithWrap(
        _NextAvailableReplicator, FCk_IntRange{0, _AbilityCueReplicators.Num()});

    const auto CueReplicator = _AbilityCueReplicators[_NextAvailableReplicator];
    CK_LOG_ERROR_IF_NOT(ck::ability, ck::IsValid(CueReplicator),
        TEXT("Next Available Ability Cue Replicator Actor at Index [{}] is INVALID"), _NextAvailableReplicator)
    { return; }

    if (GetWorld()->IsNetMode(NM_DedicatedServer))
    {
        CueReplicator->Request_ExecuteAbilityCue(InCueName, InRequest);
    }
    else
    {
        CueReplicator->Server_RequestExecuteAbilityCue(InCueName, InRequest);
    }
}

auto
    UCk_AbilityCueReplicator_Subsystem_UE::
    Request_ExecuteAbilityCue_Local(
        FGameplayTag InCueName,
        const FCk_AbilityCue_Params& InRequest)
    -> void
{
    const auto Subsystem_AbilityCue = GEngine->GetEngineSubsystem<UCk_AbilityCue_Subsystem_UE>();
    const auto Subsystem_EcsWorldSubsystem = GetWorld()->GetSubsystem<UCk_EcsWorld_Subsystem_UE>();

    ck_ability_cue_subsystem::SpawnCue(InCueName, Subsystem_AbilityCue, Subsystem_EcsWorldSubsystem, InRequest);
}

auto
    UCk_AbilityCueReplicator_Subsystem_UE::
    DoSpawnCueReplicatorActorsForPlayerController(
        APlayerController* InPlayerController)
    -> void
{
    auto AlreadyContainsPC = false;
    _ValidPlayerControllers.Add(InPlayerController, &AlreadyContainsPC);

    if (AlreadyContainsPC)
    { return; }

    for (auto Index = 0; Index < UCk_Utils_Ability_Settings_UE::Get_NumberOfCueReplicators(); ++Index)
    {
        auto* AbilityCueReplicator = GetWorld()->SpawnActor<ACk_AbilityCueReplicator_UE>();
        _AbilityCueReplicators.Emplace(AbilityCueReplicator);
    }
}

auto
    UCk_AbilityCueReplicator_Subsystem_UE::
    OnLoginEvent(
        AGameModeBase* InGameModeBase,
        APlayerController* InPlayerController)
    -> void
{
    DoSpawnCueReplicatorActorsForPlayerController(InPlayerController);
}

auto
    UCk_AbilityCueReplicator_Subsystem_UE::
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

    _NextAvailableReplicator = 0;

    for (const auto& ValidPlayerControllersList = _ValidPlayerControllers.Array();
         const auto& PC : ValidPlayerControllersList)
    {
        if (ck::IsValid(PC) && PC->GetWorld() == InWorld)
        { continue; }

        _ValidPlayerControllers.Remove(PC);
        _AbilityCueReplicators = ck::algo::Filter(_AbilityCueReplicators, [&](const ACk_AbilityCueReplicator_UE* InCueReplicator)
        {
            if (ck::Is_NOT_Valid(InCueReplicator))
            { return false; }

            if (ck::Is_NOT_Valid(PC))
            { return true; }

            return InCueReplicator->GetWorld() == PC->GetWorld();
        });
    }

    for (auto It = InWorld->GetPlayerControllerIterator(); It; ++It)
    {
       DoSpawnCueReplicatorActorsForPlayerController(It->Get());
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_AbilityCue_Subsystem_UE::
    Initialize(
        FSubsystemCollectionBase& Collection)
    -> void
{
    if (GIsRunning)
    {
        // Engine init already completed
        DoOnEngineInitComplete();
    }
    else
    {
        FCoreDelegates::OnFEngineLoopInitComplete.AddUObject(this, &UCk_AbilityCue_Subsystem_UE::DoOnEngineInitComplete);
    }
}

auto
    UCk_AbilityCue_Subsystem_UE::
    Deinitialize()
    -> void
{
    Super::Deinitialize();
}

auto
    UCk_AbilityCue_Subsystem_UE::
    Request_PopulateAllAggregators()
    -> void
{
    _EntityConfigs.Empty();

    const auto& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    const auto& AssetRegistry       = AssetRegistryModule.Get();

    FARFilter Filter;
    Filter.ClassPaths.Add(FTopLevelAssetPath{UCk_AbilityCue_Aggregator_PDA::StaticClass()});
    Filter.bRecursiveClasses = true;

    FARCompiledFilter CompiledFilter;
    AssetRegistry.CompileFilter(Filter, CompiledFilter);

    TArray<TSoftObjectPtr<UCk_AbilityCue_Aggregator_PDA>> Assets;
    AssetRegistry.EnumerateAssets(CompiledFilter, [&](const FAssetData& InEnumeratedAssetData)
    {
        DoAddAggregator(InEnumeratedAssetData);
        Assets.Emplace(TSoftObjectPtr<UCk_AbilityCue_Aggregator_PDA>{InEnumeratedAssetData.GetSoftObjectPath()});
        return true;
    });

    ck::algo::ForEach(Assets, [&](const TSoftObjectPtr<class UCk_AbilityCue_Aggregator_PDA>& Asset)
    {
        const auto Aggregator = Asset.LoadSynchronous();
        Aggregator->Request_Populate();

        _EntityConfigs.Append(Aggregator->Get_AbilityCueConfigs());
    });
}

auto
    UCk_AbilityCue_Subsystem_UE::
    DoOnEngineInitComplete()
    -> void
{
    Request_PopulateAllAggregators();
    const auto& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    AssetRegistryModule.Get().OnAssetAdded().AddUObject(this, &UCk_AbilityCue_Subsystem_UE::DoHandleAssetAddedDeleted);
    AssetRegistryModule.Get().OnAssetRemoved().AddUObject(this, &UCk_AbilityCue_Subsystem_UE::DoHandleAssetAddedDeleted);
    AssetRegistryModule.Get().OnAssetRenamed().AddUObject(this, &UCk_AbilityCue_Subsystem_UE::DoHandleRenamed);
    AssetRegistryModule.Get().OnAssetUpdated().AddUObject(this, &UCk_AbilityCue_Subsystem_UE::DoAssetUpdated);
    AssetRegistryModule.Get().OnAssetUpdatedOnDisk().AddUObject(this, &UCk_AbilityCue_Subsystem_UE::DoAssetUpdated);
}

auto
    UCk_AbilityCue_Subsystem_UE::
    DoHandleAssetAddedDeleted(
        const FAssetData& InAssetData)
    -> void
{
    DoHandleAssetAddedDeletedOrRenamed(InAssetData);
}

auto
    UCk_AbilityCue_Subsystem_UE::
    DoHandleRenamed(
        const FAssetData& InAssetData,
        const FString&)
    -> void
{
    DoHandleAssetAddedDeletedOrRenamed(InAssetData);
}

auto
    UCk_AbilityCue_Subsystem_UE::
    DoAssetUpdated(
        const FAssetData& InAssetData)
    -> void
{
    if (InAssetData.IsInstanceOf<UCk_AbilityCue_Aggregator_PDA>() || InAssetData.IsInstanceOf<UCk_Ability_EntityConfig_PDA>())
    {
        Request_PopulateAllAggregators();
        return;
    }

    const auto& CueTypes = UCk_Utils_Ability_Settings_UE::Get_CueTypes();

    // hot-reload/Live++ can result in the Cue's getting GC'ed and replaced by new Cues
    ck::algo::ForEachIsValid(CueTypes, [&](const auto& CueType)
    {
        if (const auto CueTypeBlueprint = UCk_Utils_Object_UE::Get_ClassGeneratedByBlueprint(CueType);
            InAssetData.IsInstanceOf(CueTypeBlueprint->GetClass(), EResolveClass::Yes))
        {
            Request_PopulateAllAggregators();
        }
    });
}

auto
    UCk_AbilityCue_Subsystem_UE::
    DoHandleAssetAddedDeletedOrRenamed(
        const FAssetData& InAssetData)
    -> void
{
    if (InAssetData.IsInstanceOf<UCk_AbilityCue_Aggregator_PDA>())
    {
        DoAddAggregator(InAssetData);
    }
    else if (InAssetData.IsInstanceOf<ConfigType>())
    {
        const auto& PathOnly = UCk_Utils_IO_UE::Get_ExtractPath(InAssetData.GetObjectPathString());

        const auto Found = _Aggregators.Find(FName{PathOnly});

        const auto Object = InAssetData.GetSoftObjectPath().ResolveObject();

        if (ck::Is_NOT_Valid(Found, ck::IsValid_Policy_NullptrOnly{}))
        {
            UCk_Utils_EditorOnly_UE::Request_PushNewEditorMessage
            (
                FCk_Utils_EditorOnly_PushNewEditorMessage_Params
                {
                    TEXT("Ability"),
                    FCk_MessageSegments
                    {
                        {
                            FCk_TokenizedMessage{TEXT("Ability Cue added to a folder WITHOUT an Aggregator")}.Set_TargetObject(Object)
                        }
                    }
                }
                .Set_MessageSeverity(ECk_EditorMessage_Severity::Error)
                .Set_ToastNotificationDisplayPolicy(ECk_EditorMessage_ToastNotification_DisplayPolicy::Display)
                .Set_MessageLogDisplayPolicy(ECk_EditorMessage_MessageLog_DisplayPolicy::Focus)
            );

            return;
        }

        const auto Config = Cast<ConfigType>(Object);

        CK_ENSURE_IF_NOT(ck::IsValid(Config),
            TEXT("Object [{}] is NOT a [{}]. Something went wrong with the aggregation logic"),
            Object,
            ck::Get_RuntimeTypeToString<ConfigType>())
        { return; }

        _EntityConfigs.Emplace(Config->Get_CueName(), Config);

        Found->LoadSynchronous()->Request_Populate();
    }
}

auto
    UCk_AbilityCue_Subsystem_UE::
    DoAddAggregator(
        const FAssetData& InAggregatorData)
    -> void
{
    const auto Aggregator = TSoftObjectPtr<UCk_AbilityCue_Aggregator_PDA>{InAggregatorData.GetSoftObjectPath()};

    CK_ENSURE_IF_NOT(ck::IsValid(Aggregator.LoadSynchronous()),
        TEXT("Could not load Aggregator using AssetData [{}]. Either the AssetData is incorrect OR there is a redirector involved."),
        InAggregatorData)
    { return; }

    _Aggregators.Add(InAggregatorData.PackagePath, Aggregator);
}

auto
    UCk_AbilityCue_Subsystem_UE::
    Get_AbilityCue_ConstructionScript(
        const FGameplayTag& InCueName)
    -> class UCk_Entity_ConstructionScript_PDA*
{
#if WITH_EDITOR
    if (_EntityConfigs.IsEmpty())
    { Request_PopulateAllAggregators(); }
#endif

    const auto FoundAbilityCue = _EntityConfigs.Find(InCueName);

    CK_ENSURE_IF_NOT(ck::IsValid(FoundAbilityCue, ck::IsValid_Policy_NullptrOnly{}), TEXT("Could not find AbilityCue with Name [{}]"), InCueName)
    { return {}; }

    auto EntityConfig = (*FoundAbilityCue)->Get_EntityConfig();

    CK_ENSURE_IF_NOT(ck::IsValid(EntityConfig), TEXT("EntityConfig INVALID for AbilityCue with Name [{}]"), InCueName)
    { return {}; }

    return EntityConfig->Get_EntityConstructionScript();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AbilityCue_Subsystem_UE::
    Request_PopulateAggregators()
    -> void
{
    GEngine->GetEngineSubsystem<SubsystemType>()->Request_PopulateAllAggregators();
}

// --------------------------------------------------------------------------------------------------------------------
