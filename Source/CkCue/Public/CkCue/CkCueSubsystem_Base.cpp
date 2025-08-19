#include "CkCueSubsystem_Base.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Object/CkObject_Utils.h"
#include "CkCore/Math/Arithmetic/CkArithmetic_Utils.h"
#include "CkCore/Debug/CkDebug_Utils.h"

#include "CkEcs/EntityScript/CkEntityScript_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include <AssetRegistry/AssetRegistryModule.h>

#if WITH_EDITOR
#include <Editor.h>
#include "Engine/Blueprint.h"
#endif

// --------------------------------------------------------------------------------------------------------------------

namespace ck_cue_subsystem_base
{
    auto ExecuteCueEntityScript(
        FCk_Handle InOwnerEntity,
        const FGameplayTag& InCueName,
        TSubclassOf<UCk_CueBase_EntityScript> InCueClass,
        const FInstancedStruct& InSpawnParams) -> FCk_Handle_PendingEntityScript
    {
        CK_ENSURE_IF_NOT(ck::IsValid(InOwnerEntity),
            TEXT("OwnerEntity is invalid when trying to execute Cue [{}]"), InCueName)
        { return {}; }

        CK_ENSURE_IF_NOT(ck::IsValid(InCueClass),
            TEXT("CueClass was INVALID when trying to execute Cue [{}]"), InCueName)
        { return {}; }

        auto PendingEntityScript = UCk_Utils_EntityScript_UE::Request_SpawnEntity(InOwnerEntity, InCueClass, InSpawnParams);

        return PendingEntityScript;
    }
}

// --------------------------------------------------------------------------------------------------------------------

ACk_CueReplicator_UE::
    ACk_CueReplicator_UE()
{
    bReplicates = true;
    bAlwaysRelevant = true;
    PrimaryActorTick.bCanEverTick = false;
    PrimaryActorTick.bTickEvenWhenPaused = false;
}

auto
    ACk_CueReplicator_UE::
    BeginPlay()
    -> void
{
    Super::BeginPlay();

    _Subsystem_Cue = GEngine->GetEngineSubsystem<UCk_CueSubsystem_Base_UE>();
    _Subsystem_EcsWorld = GetWorld()->GetSubsystem<UCk_EcsWorld_Subsystem_UE>();

    if (NOT IsNetMode(NM_Client))
    { return; }

    if (ck::Is_NOT_Valid(GetOwner()))
    { return; }

    const auto CueReplicator = GetWorld()->GetSubsystem<UCk_CueReplicator_Subsystem_Base_UE>();
    CueReplicator->_CueReplicators.Emplace(this);
}

auto
    ACk_CueReplicator_UE::
    Server_RequestExecuteCue_Implementation(
        FCk_Handle InOwnerEntity,
        FGameplayTag InCueName,
        FInstancedStruct InSpawnParams)
    -> void
{
    Request_ExecuteCue(InOwnerEntity, InCueName, InSpawnParams);
}

auto
    ACk_CueReplicator_UE::
    Request_ExecuteCue_Implementation(
        FCk_Handle InOwnerEntity,
        FGameplayTag InCueName,
        FInstancedStruct InSpawnParams)
    -> void
{
    if (GetWorld()->IsNetMode(NM_DedicatedServer) || GetWorld()->IsNetMode(NM_ListenServer))
    { return; }

    const auto CueClass = _Subsystem_Cue->Get_CueEntityScript(InCueName);
    ck_cue_subsystem_base::ExecuteCueEntityScript(InOwnerEntity, InCueName, CueClass, InSpawnParams);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_CueReplicator_Subsystem_Base_UE::
    Initialize(
        FSubsystemCollectionBase& InCollection)
    -> void
{
    Super::Initialize(InCollection);

    if (GetWorld()->IsNetMode(NM_Client))
    { return; }

    _PostLoadMapWithWorldDelegateHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UCk_CueReplicator_Subsystem_Base_UE::OnPostLoadMapWithWorld);
    _PostLoginEventDelegateHandle = FGameModeEvents::GameModePostLoginEvent.AddUObject(this, &UCk_CueReplicator_Subsystem_Base_UE::OnPostLoginEvent);
}

auto
    UCk_CueReplicator_Subsystem_Base_UE::
    Deinitialize()
    -> void
{
    Super::Deinitialize();

    FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(_PostLoadMapWithWorldDelegateHandle);
    FGameModeEvents::GameModePostLoginEvent.Remove(_PostLoadMapWithWorldDelegateHandle);
}

auto
    UCk_CueReplicator_Subsystem_Base_UE::
    Request_ExecuteCue(
        const FCk_Handle& InOwnerEntity,
        FGameplayTag InCueName,
        const FInstancedStruct& InSpawnParams)
    -> FCk_Handle_PendingEntityScript
{
    CK_ENSURE_IF_NOT(ck::IsValid(InOwnerEntity),
        TEXT("OwnerEntity is invalid when trying to execute Cue [{}]"), InCueName)
    { return {}; }

    CK_ENSURE_IF_NOT(_CueReplicators.Num() > 0,
        TEXT("No CueReplicator Actors available. Unable to Execute Cue"))
    { return {}; }

    if (GetWorld()->IsNetMode(NM_Standalone))
    {
        // For standalone, execute directly without replication
        const auto Subsystem_Cue = GEngine->GetEngineSubsystem<UCk_CueSubsystem_Base_UE>();
        const auto CueClass = Subsystem_Cue->Get_CueEntityScript(InCueName);
        return ck_cue_subsystem_base::ExecuteCueEntityScript(InOwnerEntity, InCueName, CueClass, InSpawnParams);
    }

    _NextAvailableReplicator = UCk_Utils_Arithmetic_UE::Get_Increment_WithWrap(
        _NextAvailableReplicator, FCk_IntRange{0, _CueReplicators.Num()}, ECk_Inclusiveness::Exclusive);

    const auto CueReplicator = _CueReplicators[_NextAvailableReplicator];
    CK_ENSURE_IF_NOT(ck::IsValid(CueReplicator),
        TEXT("Next Available Cue Replicator Actor at Index [{}] is INVALID"), _NextAvailableReplicator)
    { return {}; }

    if (GetWorld()->IsNetMode(NM_DedicatedServer) || GetWorld()->IsNetMode(NM_ListenServer))
    {
        CueReplicator->Request_ExecuteCue(InOwnerEntity, InCueName, InSpawnParams);
    }
    else
    {
        CueReplicator->Server_RequestExecuteCue(InOwnerEntity, InCueName, InSpawnParams);
    }

    return {};
}

auto
    UCk_CueReplicator_Subsystem_Base_UE::
    Request_ExecuteCue_Local(
        const FCk_Handle& InOwnerEntity,
        FGameplayTag InCueName,
        const FInstancedStruct& InSpawnParams)
    -> FCk_Handle_PendingEntityScript
{
    CK_ENSURE_IF_NOT(ck::IsValid(InOwnerEntity),
        TEXT("OwnerEntity is invalid when trying to execute local Cue [{}]"), InCueName)
    { return {}; }

    const auto Subsystem_Cue = GEngine->GetEngineSubsystem<UCk_CueSubsystem_Base_UE>();
    const auto CueClass = Subsystem_Cue->Get_CueEntityScript(InCueName);
    return ck_cue_subsystem_base::ExecuteCueEntityScript(InOwnerEntity, InCueName, CueClass, InSpawnParams);
}

auto
    UCk_CueReplicator_Subsystem_Base_UE::
    DoSpawnCueReplicatorActorsForPlayerController(
        APlayerController* InPlayerController) -> void
{
    auto AlreadyContainsPC = false;
    _ValidPlayerControllers.Add(InPlayerController, &AlreadyContainsPC);

    if (AlreadyContainsPC)
    { return; }

    // Spawn one replicator per player controller for now
    // Derived classes can override this behavior if needed
    auto* CueReplicator = GetWorld()->SpawnActor<ACk_CueReplicator_UE>();
    _CueReplicators.Emplace(CueReplicator);
}

auto
    UCk_CueReplicator_Subsystem_Base_UE::
    OnPostLoadMapWithWorld(
        UWorld* InWorld)
    -> void
{
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
        _CueReplicators = ck::algo::Filter(_CueReplicators, [&](const ACk_CueReplicator_UE* InCueReplicator)
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

auto
    UCk_CueReplicator_Subsystem_Base_UE::
    OnPostLoginEvent(
        AGameModeBase* GameMode,
        APlayerController* NewPlayer)
    -> void
{
    if (NOT _ValidPlayerControllers.Contains(NewPlayer))
    {
        DoSpawnCueReplicatorActorsForPlayerController(NewPlayer);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_CueSubsystem_Base_UE::
    Initialize(
        FSubsystemCollectionBase& Collection)
    -> void
{
    if (GIsRunning)
    {
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
    Super::Deinitialize();
}

auto
    UCk_CueSubsystem_Base_UE::
    Request_PopulateAllCues()
    -> void
{
    _DiscoveredCues.Empty();

    const auto CueBaseClass = Get_CueBaseClass();
    CK_ENSURE_IF_NOT(ck::IsValid(CueBaseClass),
        TEXT("CueBaseClass is INVALID. Derived subsystem must implement Get_CueBaseClass"))
    { return; }

    // Find all loaded classes that inherit from CueBaseClass (C++/Angelscript)
    for (TObjectIterator<UClass> ClassIterator; ClassIterator; ++ClassIterator)
    {
        const auto Class = *ClassIterator;
        if (ck::Is_NOT_Valid(Class) || Class->HasAnyClassFlags(CLASS_Abstract))
        { continue; }

        if (NOT Class->IsChildOf(CueBaseClass))
        { continue; }

        const auto DefaultObject = Cast<UCk_CueBase_EntityScript>(Class->GetDefaultObject());
        if (ck::Is_NOT_Valid(DefaultObject))
        { continue; }

        const auto& CueName = DefaultObject->Get_CueName();
        if (NOT ck::IsValid(CueName))
        { continue; }

        if (_DiscoveredCues.Contains(CueName))
        {
            CK_TRIGGER_ENSURE(TEXT("Duplicate CueName [{}] found! Existing: [{}], New: [{}]"),
                CueName, _DiscoveredCues[CueName], Class);
            continue;
        }

        _DiscoveredCues.Add(CueName, Class);
    }

    // Also check unloaded Blueprint assets
    Request_PopulateBlueprintCues();

    UE_LOG(LogTemp, Log, TEXT("Cue Discovery Complete: Found %d cues"), _DiscoveredCues.Num());
}

auto
    UCk_CueSubsystem_Base_UE::
    Request_PopulateBlueprintCues()
    -> void
{
    const auto CueBaseClass = Get_CueBaseClass();
    if (ck::Is_NOT_Valid(CueBaseClass))
    { return; }

    const auto& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

    FARFilter Filter;

#if WITH_EDITOR
    Filter.ClassPaths.Add(FTopLevelAssetPath{UBlueprint::StaticClass()});
#else
    Filter.ClassPaths.Add(FTopLevelAssetPath{UBlueprintGeneratedClass::StaticClass()});
#endif

    Filter.bRecursiveClasses = true;
    Filter.bRecursivePaths = true;
    Filter.bIncludeOnlyOnDiskAssets = false;

    FARCompiledFilter CompiledFilter;
    IAssetRegistry::Get()->CompileFilter(Filter, CompiledFilter);

    AssetRegistryModule.Get().EnumerateAssets(CompiledFilter, [&](const FAssetData& InAssetData)
    {
        constexpr auto ContinueIterating = true;

        const FString AssetName = InAssetData.AssetName.ToString();
        if (AssetName.Contains(TEXT("AssetFactory")) ||
            AssetName.Contains(TEXT("_Factory")) ||
            AssetName.Contains(TEXT("Default__")))
        {
            return ContinueIterating;
        }

        const auto ResolvedObject = InAssetData.GetSoftObjectPath().TryLoad();
        if (ck::Is_NOT_Valid(ResolvedObject))
        {
            return ContinueIterating;
        }

        const auto ResolvedObjectDefaultClassObject = [&]() -> UObject*
        {
#if WITH_EDITOR
            const auto& Blueprint = Cast<UBlueprint>(ResolvedObject);
            if (ck::Is_NOT_Valid(Blueprint) || ck::Is_NOT_Valid(Blueprint->GeneratedClass))
            { return nullptr; }

            const auto DefaultObject = Blueprint->GeneratedClass->GetDefaultObject();
            if (ck::Is_NOT_Valid(DefaultObject) || NOT DefaultObject->IsA(CueBaseClass))
            { return nullptr; }

            return DefaultObject;
#else
            const auto BlueprintGeneratedClass = Cast<UBlueprintGeneratedClass>(InAssetData.GetAsset());
            if (ck::Is_NOT_Valid(BlueprintGeneratedClass))
            { return nullptr; }

            const auto DefaultObject = BlueprintGeneratedClass->GetDefaultObject();
            if (ck::Is_NOT_Valid(DefaultObject) || NOT DefaultObject->IsA(CueBaseClass))
            { return nullptr; }

            return DefaultObject;
#endif
        }();

        if (ck::Is_NOT_Valid(ResolvedObjectDefaultClassObject))
        {
            return ContinueIterating;
        }

        const auto CueObject = Cast<UCk_CueBase_EntityScript>(ResolvedObjectDefaultClassObject);
        CK_ENSURE_IF_NOT(ck::IsValid(CueObject),
            TEXT("Cast to CueBaseClass failed for [{}]"), ResolvedObject->GetName())
        { return ContinueIterating; }

        const auto& CueName = CueObject->Get_CueName();
        CK_ENSURE_IF_NOT(ck::IsValid(CueName), TEXT("Cue [{}] has an invalid CueName"), CueObject)
        { return ContinueIterating; }

        if (_DiscoveredCues.Contains(CueName))
        {
            // Skip if already found by class iterator (C++/Angelscript takes precedence)
            return ContinueIterating;
        }

        _DiscoveredCues.Add(CueName, CueObject->GetClass());

        return ContinueIterating;
    });
}

auto
    UCk_CueSubsystem_Base_UE::
    DoOnEngineInitComplete()
    -> void
{
    Request_PopulateAllCues();

    // Process any pending assets now that we're initialized
    if (_PendingAssetUpdates.Num() > 0)
    {
        for (const auto& PendingAsset : _PendingAssetUpdates)
        {
            Request_ProcessAssetUpdate(PendingAsset);
        }
        _PendingAssetUpdates.Empty();
    }

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
   const auto CueBaseClass = Get_CueBaseClass();
   if (ck::Is_NOT_Valid(CueBaseClass))
   {
       _PendingAssetUpdates.AddUnique(InAssetData);
       return;
   }

   Request_ProcessAssetUpdate(InAssetData);

   if (_PendingAssetUpdates.Num() > 0)
   {
       for (const auto& PendingAsset : _PendingAssetUpdates)
       {
           Request_ProcessAssetUpdate(PendingAsset);
       }
       _PendingAssetUpdates.Empty();
   }
}

auto
   UCk_CueSubsystem_Base_UE::
   Request_ProcessAssetUpdate(
       const FAssetData& InAssetData)
    -> void
{
    CK_BREAK_IF_NAME(InAssetData.AssetName, TEXT("SimpleBackgroundMusicCue"));
   const auto CueBaseClass = Get_CueBaseClass();
   if (ck::Is_NOT_Valid(CueBaseClass))
   { return; }

#if WITH_EDITOR
   if (InAssetData.IsInstanceOf<UBlueprint>())
   {
       const auto Blueprint = Cast<UBlueprint>(InAssetData.GetAsset());
       if (ck::IsValid(Blueprint) &&
           ck::IsValid(Blueprint->GeneratedClass) &&
           Blueprint->GeneratedClass->IsChildOf(CueBaseClass))
       {
           Request_PopulateAllCues();
       }
   }
#else
   if (InAssetData.IsInstanceOf<UBlueprintGeneratedClass>())
   {
       const auto GeneratedClass = Cast<UBlueprintGeneratedClass>(InAssetData.GetAsset());
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

    return FoundCue ? *FoundCue : nullptr;
}

// --------------------------------------------------------------------------------------------------------------------
