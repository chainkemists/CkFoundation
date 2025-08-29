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

    _Subsystem_CueReplicator = GetWorld()->GetSubsystem<UCk_CueExecutor_Subsystem_Base_UE>();
    _Subsystem_EcsWorld = GetWorld()->GetSubsystem<UCk_EcsWorld_Subsystem_UE>();

    if (NOT IsNetMode(NM_Client))
    { return; }

    if (ck::Is_NOT_Valid(GetOwner()))
    { return; }

    const auto CueReplicator = GetWorld()->GetSubsystem<UCk_CueExecutor_Subsystem_Base_UE>();
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

    CK_ENSURE_IF_NOT(ck::IsValid(_Subsystem_CueReplicator),
        TEXT("CueReplicator subsystem is invalid"))
    { return; }

    auto CueSubsystem = _Subsystem_CueReplicator->Get_CueSubsystem();
    CK_ENSURE_IF_NOT(ck::IsValid(CueSubsystem),
        TEXT("CueSubsystem is invalid from replicator"))
    { return; }

    auto CueClass = CueSubsystem->Get_CueEntityScript(InCueName);
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
    Request_ExecuteCue(
        const FCk_Handle& InOwnerEntity,
        FGameplayTag InCueName,
        FInstancedStruct InSpawnParams)
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
        auto CueSubsystem = Get_CueSubsystem();
        CK_ENSURE_IF_NOT(ck::IsValid(CueSubsystem),
            TEXT("CueSubsystem is invalid in standalone mode"))
        { return {}; }

        auto CueClass = CueSubsystem->Get_CueEntityScript(InCueName);
        return ck_cue_subsystem_base::ExecuteCueEntityScript(InOwnerEntity, InCueName, CueClass, InSpawnParams);
    }

    _NextAvailableReplicator = UCk_Utils_Arithmetic_UE::Get_Increment_WithWrap(
        _NextAvailableReplicator, FCk_IntRange{0, _CueReplicators.Num()}, ECk_Inclusiveness::Exclusive);

    auto CueReplicator = _CueReplicators[_NextAvailableReplicator];
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

    auto CueSubsystem = Get_CueSubsystem();
    CK_ENSURE_IF_NOT(ck::IsValid(CueSubsystem),
        TEXT("CueSubsystem is invalid for local cue execution"))
    { return {}; }

    auto CueClass = CueSubsystem->Get_CueEntityScript(InCueName);
    return ck_cue_subsystem_base::ExecuteCueEntityScript(InOwnerEntity, InCueName, CueClass, InSpawnParams);
}

auto
    UCk_CueExecutor_Subsystem_Base_UE::
    DoSpawnCueReplicatorActorsForPlayerController(
        APlayerController* InPlayerController) -> void
{
    auto AlreadyContainsPC = false;
    _ValidPlayerControllers.Add(InPlayerController, &AlreadyContainsPC);

    if (AlreadyContainsPC)
    { return; }

    // Spawn one replicator per player controller for now
    // Derived classes can override this behavior if needed
    auto CueReplicator = GetWorld()->SpawnActor<ACk_CueReplicator_UE>();
    _CueReplicators.Emplace(CueReplicator);
}

auto
    UCk_CueExecutor_Subsystem_Base_UE::
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
    UCk_CueExecutor_Subsystem_Base_UE::
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
    Super::Deinitialize();
}

auto
    UCk_CueSubsystem_Base_UE::
    Request_PopulateAllCues()
    -> void
{
    _DiscoveredCues.Empty();

    auto CueBaseClass = Get_CueBaseClass();
    CK_ENSURE_IF_NOT(ck::IsValid(CueBaseClass),
        TEXT("CueBaseClass is INVALID. Derived subsystem must implement Get_CueBaseClass"))
    { return; }

    // Find all loaded classes that inherit from CueBaseClass (C++/Angelscript)
    for (TObjectIterator<UClass> ClassIterator; ClassIterator; ++ClassIterator)
    {
        auto Class = *ClassIterator;
        if (ck::Is_NOT_Valid(Class) || Class->HasAnyClassFlags(CLASS_Abstract))
        { continue; }

        if (NOT Class->IsChildOf(CueBaseClass))
        { continue; }

        auto DefaultObject = Cast<UCk_CueBase_EntityScript>(Class->GetDefaultObject());
        if (ck::Is_NOT_Valid(DefaultObject))
        { continue; }

        auto CueName = DefaultObject->Get_CueName();
        if (NOT ck::IsValid(CueName))
        { continue; }

        if (_DiscoveredCues.Contains(CueName))
        {
            // Skip REINST classes from hot reload - they're temporary
            const auto ExistingClassName = _DiscoveredCues[CueName]->GetName();

            if (const auto NewClassName = Class->GetName();
                NewClassName.Contains(TEXT("REINST_")) || ExistingClassName.Contains(TEXT("REINST_")))
            {
                // Prefer non-REINST class
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
    auto CueBaseClass = Get_CueBaseClass();
    if (ck::Is_NOT_Valid(CueBaseClass))
    { return; }

    auto& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

    FARFilter Filter;

#if WITH_EDITOR
    Filter.ClassPaths.Add(FTopLevelAssetPath{UBlueprint::StaticClass()});
#else
    Filter.ClassPaths.Add(FTopLevelAssetPath{UBlueprintGeneratedClass::StaticClass()});
#endif

    // CRITICAL: Filter by our cue asset tag to avoid loading non-cue blueprints
    // This eliminates the asset registry ensure by preventing AnimBP/ControlRig loading
    Filter.TagsAndValues.Add("IsCueAsset", TOptional<FString>("true"));

    Filter.bRecursiveClasses = true;
    Filter.bRecursivePaths = true;
    Filter.bIncludeOnlyOnDiskAssets = false;

    FARCompiledFilter CompiledFilter;
    IAssetRegistry::Get()->CompileFilter(Filter, CompiledFilter);

    AssetRegistryModule.Get().EnumerateAssets(CompiledFilter, [&](const FAssetData& InAssetData)
    {
        constexpr auto ContinueIterating = true;

        // Skip factory and default objects (shouldn't happen with tag filtering but safety first)
        auto AssetName = InAssetData.AssetName.ToString();
        if (AssetName.Contains(TEXT("AssetFactory")) ||
            AssetName.Contains(TEXT("_Factory")) ||
            AssetName.Contains(TEXT("Default__")))
        {
            return ContinueIterating;
        }

        // Now it's safe to load since we know this is tagged as a cue asset
        auto ResolvedObject = InAssetData.GetSoftObjectPath().TryLoad();
        if (ck::Is_NOT_Valid(ResolvedObject))
        {
            return ContinueIterating;
        }

        UObject* ResolvedObjectDefaultClassObject = nullptr;

#if WITH_EDITOR
        auto Blueprint = Cast<UBlueprint>(ResolvedObject);
        if (ck::Is_NOT_Valid(Blueprint) || ck::Is_NOT_Valid(Blueprint->GeneratedClass))
        {
            return ContinueIterating;
        }

        auto DefaultObject = Blueprint->GeneratedClass->GetDefaultObject();
        if (ck::Is_NOT_Valid(DefaultObject) || NOT DefaultObject->IsA(CueBaseClass))
        {
            return ContinueIterating;
        }

        ResolvedObjectDefaultClassObject = DefaultObject;
#else
        auto BlueprintGeneratedClass = Cast<UBlueprintGeneratedClass>(InAssetData.GetAsset());
        if (ck::Is_NOT_Valid(BlueprintGeneratedClass))
        {
            return ContinueIterating;
        }

        auto DefaultObject = BlueprintGeneratedClass->GetDefaultObject();
        if (ck::Is_NOT_Valid(DefaultObject) || NOT DefaultObject->IsA(CueBaseClass))
        {
            return ContinueIterating;
        }

        ResolvedObjectDefaultClassObject = DefaultObject;
#endif

        if (ck::Is_NOT_Valid(ResolvedObjectDefaultClassObject))
        {
            return ContinueIterating;
        }

        auto CueObject = Cast<UCk_CueBase_EntityScript>(ResolvedObjectDefaultClassObject);
        CK_ENSURE_IF_NOT(ck::IsValid(CueObject),
            TEXT("Asset tagged as Cue but cast to CueBaseClass failed for [{}]"), ResolvedObject->GetName())
        { return ContinueIterating; }

        auto CueName = CueObject->Get_CueName();
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

    auto& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
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

   // Early out for assets that aren't tagged as cues - prevents unnecessary loading
   auto IsCueAssetTag = InAssetData.GetTagValueRef<FString>("IsCueAsset");
   if (NOT IsCueAssetTag.Equals("true"))
   {
       return;
   }

#if WITH_EDITOR
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

    auto FoundCue = _DiscoveredCues.Find(InCueName);

    return FoundCue ? *FoundCue : nullptr;
}

auto
    UCk_CueSubsystem_Base_UE::
    Get_DiscoveredCues() const
    -> const TMap<FGameplayTag, TSubclassOf<UCk_CueBase_EntityScript>>&
{
    return _DiscoveredCues;
}

// --------------------------------------------------------------------------------------------------------------------
