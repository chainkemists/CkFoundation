#include "CkAbilityCue_Subsystem.h"

#include "CkAbility/CkAbility_Log.h"
#include "CkAbility/Ability/CkAbility_Script.h"
#include "CkAbility/AbilityCue/CkAbilityCue_Fragment_Data.h"

#include "CkCore/Actor/CkActor_Utils.h"
#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/IO/CkIO_Utils.h"
#include "CkCore/Math/Arithmetic/CkArithmetic_Utils.h"

#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "CkEntityBridge/CkEntityBridge_Fragment_Data.h"

#include <GameplayCueManager.h>
#include <AssetRegistry/AssetRegistryModule.h>
#include <GameFramework/GameModeBase.h>

// --------------------------------------------------------------------------------------------------------------------

ACk_AbilityCueReplicator_UE::
    ACk_AbilityCueReplicator_UE()
{
    bReplicates = true;
    bAlwaysRelevant = true;
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
    CK_ENSURE_IF_NOT(ck::IsValid(InCueName),
        TEXT("Unable to ExecuteAbilityCue since the CueName is [{}]"), InCueName)
    { return; }

    const auto AbilityCueSubsystem = GEngine->GetEngineSubsystem<UCk_AbilityCue_Subsystem_UE>();
    const auto ConstructionScript = AbilityCueSubsystem->Get_AbilityCue_ConstructionScript(InCueName);

    if (ck::Is_NOT_Valid(ConstructionScript))
    { return; }

    const auto TransientEntity = GetWorld()->GetSubsystem<UCk_EcsWorld_Subsystem_UE>()->Get_TransientEntity();

    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(TransientEntity);
    NewEntity.Add<FCk_AbilityCue_Params>(InParams);

    ck::ability::Verbose(TEXT("Executing AbilityCue ConstructionScript [{}] with created Entity [{}]"), ConstructionScript, NewEntity);

    ConstructionScript->Construct(NewEntity, {});
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
}

auto
    UCk_AbilityCueReplicator_Subsystem_UE::
    Request_ExecuteAbilityCue(
        FGameplayTag InCueName,
        const FCk_AbilityCue_Params& InRequest)
    -> void
{
    CK_ENSURE_IF_NOT(_AbilityCueReplicators.Num() > 0,
        TEXT("No AbilityCueReplicator Actors available. Unable to Execute Ability Cue"))
    { return; }

    if (GetWorld()->IsNetMode(NM_Standalone))
    { return; }

    _NextAvailableReplicator = UCk_Utils_Arithmetic_UE::Get_Increment_WithWrap(
        _NextAvailableReplicator, FCk_IntRange{0, _AbilityCueReplicators.Num()});

    if (GetWorld()->IsNetMode(NM_DedicatedServer))
    {
        _AbilityCueReplicators[_NextAvailableReplicator]->Request_ExecuteAbilityCue(InCueName, InRequest);
    }
    else
    {
        _AbilityCueReplicators[_NextAvailableReplicator]->Server_RequestExecuteAbilityCue(InCueName, InRequest);
    }
}

auto
    UCk_AbilityCueReplicator_Subsystem_UE::
    OnLoginEvent(
        AGameModeBase* InGameModeBase,
        APlayerController* InPlayerController)
    -> void
{
    for (auto Index = 0; Index < NumberOfReplicators; ++Index)
    {
        _AbilityCueReplicators.Emplace
        (
            GetWorld()->SpawnActor<ACk_AbilityCueReplicator_UE>()
        );
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
    DoOnEngineInitComplete()
    -> void
{
    DoPopulateAllAggregators();
    const auto& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    AssetRegistryModule.Get().OnAssetAdded().AddUObject(this, &UCk_AbilityCue_Subsystem_UE::DoHandleAssetAddedDeleted);
    AssetRegistryModule.Get().OnAssetRemoved().AddUObject(this, &UCk_AbilityCue_Subsystem_UE::DoHandleAssetAddedDeleted);
    AssetRegistryModule.Get().OnAssetRenamed().AddUObject(this, &UCk_AbilityCue_Subsystem_UE::DoHandleRenamed);
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

        if (NOT ck::IsValid(Found, ck::IsValid_Policy_NullptrOnly{}))
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

        _AbilityCues.Add(Config->Get_CueName(), InAssetData.GetSoftObjectPath());

        Found->LoadSynchronous()->Request_Populate();
    }
}

auto
    UCk_AbilityCue_Subsystem_UE::
    DoPopulateAllAggregators()
    -> void
{
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
    if (_AbilityCues.IsEmpty())
    { DoPopulateAllAggregators(); }

    const auto FoundAbilityCue = _EntityConfigs.Find(InCueName);

    CK_ENSURE_IF_NOT(ck::IsValid(FoundAbilityCue, ck::IsValid_Policy_NullptrOnly{}), TEXT("Could not find AbilityCue with Name [{}]"), InCueName)
    { return {}; }

    auto EntityConfig = (*FoundAbilityCue)->Get_EntityConfig();

    CK_ENSURE_IF_NOT(ck::IsValid(EntityConfig), TEXT("EntityConfig INVALID for AbilityCue with Name [{}]"), InCueName)
    { return {}; }

    return EntityConfig->Get_EntityConstructionScript();
}

// --------------------------------------------------------------------------------------------------------------------
