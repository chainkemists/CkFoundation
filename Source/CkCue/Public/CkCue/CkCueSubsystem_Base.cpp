#include "CkCueSubsystem_Base.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Object/CkObject_Utils.h"
#include "CkCore/Math/Arithmetic/CkArithmetic_Utils.h"

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
        const FGameplayTag& InCueName,
        TSubclassOf<UCk_CueBase_EntityScript> InCueClass,
        const UCk_EcsWorld_Subsystem_UE* InEcsWorldSubsystem,
        const FInstancedStruct& InSpawnParams) -> void
    {
        CK_ENSURE_IF_NOT(ck::IsValid(InCueClass),
            TEXT("CueClass was INVALID when trying to execute Cue [{}]"), InCueName)
        { return; }

        CK_ENSURE_IF_NOT(ck::IsValid(InEcsWorldSubsystem),
            TEXT("EcsWorld Subsystem was INVALID when trying to execute Cue [{}]"), InCueName)
        { return; }

        auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_TransientOwner(InEcsWorldSubsystem);

        UCk_Utils_EntityScript_UE::Add(NewEntity, InCueClass, InSpawnParams);

#if NOT CK_DISABLE_ECS_HANDLE_DEBUGGING
        UCk_Utils_Handle_UE::Set_DebugName(NewEntity, *ck::Format_UE(TEXT("Cue [{}]"), InCueName));
#endif
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
    BeginPlay() -> void
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
        FGameplayTag InCueName,
        FInstancedStruct InSpawnParams) -> void
{
    Request_ExecuteCue(InCueName, InSpawnParams);
}

auto
    ACk_CueReplicator_UE::
    Request_ExecuteCue_Implementation(
        FGameplayTag InCueName,
        FInstancedStruct InSpawnParams) -> void
{
    if (GetWorld()->IsNetMode(NM_DedicatedServer) || GetWorld()->IsNetMode(NM_ListenServer))
    { return; }

    const auto CueClass = _Subsystem_Cue->Get_CueEntityScript(InCueName);
    ck_cue_subsystem_base::ExecuteCueEntityScript(InCueName, CueClass, _Subsystem_EcsWorld.Get(), InSpawnParams);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_CueReplicator_Subsystem_Base_UE::
    Initialize(FSubsystemCollectionBase& InCollection) -> void
{
    Super::Initialize(InCollection);

    if (GetWorld()->IsNetMode(NM_Client))
    { return; }

    _PostLoadMapWithWorldDelegateHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UCk_CueReplicator_Subsystem_Base_UE::OnPostLoadMapWithWorld);
    _PostLoginEventDelegateHandle = FGameModeEvents::GameModePostLoginEvent.AddUObject(this, &UCk_CueReplicator_Subsystem_Base_UE::OnPostLoginEvent);
}

auto
    UCk_CueReplicator_Subsystem_Base_UE::
    Deinitialize() -> void
{
    Super::Deinitialize();

    FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(_PostLoadMapWithWorldDelegateHandle);
    FGameModeEvents::GameModePostLoginEvent.Remove(_PostLoadMapWithWorldDelegateHandle);
}

auto
    UCk_CueReplicator_Subsystem_Base_UE::
    Request_ExecuteCue(
        FGameplayTag InCueName,
        const FInstancedStruct& InSpawnParams) -> void
{
    CK_ENSURE_IF_NOT(_CueReplicators.Num() > 0,
        TEXT("No CueReplicator Actors available. Unable to Execute Cue"))
    { return; }

    if (GetWorld()->IsNetMode(NM_Standalone))
    { return; }

    _NextAvailableReplicator = UCk_Utils_Arithmetic_UE::Get_Increment_WithWrap(
        _NextAvailableReplicator, FCk_IntRange{0, _CueReplicators.Num()}, ECk_Inclusiveness::Exclusive);

    const auto CueReplicator = _CueReplicators[_NextAvailableReplicator];
    CK_ENSURE_IF_NOT(ck::IsValid(CueReplicator),
        TEXT("Next Available Cue Replicator Actor at Index [{}] is INVALID"), _NextAvailableReplicator)
    { return; }

    if (GetWorld()->IsNetMode(NM_DedicatedServer) || GetWorld()->IsNetMode(NM_ListenServer))
    {
        CueReplicator->Request_ExecuteCue(InCueName, InSpawnParams);
    }
    else
    {
        CueReplicator->Server_RequestExecuteCue(InCueName, InSpawnParams);
    }
}

auto
    UCk_CueReplicator_Subsystem_Base_UE::
    Request_ExecuteCue_Local(
        FGameplayTag InCueName,
        const FInstancedStruct& InSpawnParams) -> void
{
    const auto Subsystem_Cue = GEngine->GetEngineSubsystem<UCk_CueSubsystem_Base_UE>();
    const auto Subsystem_EcsWorld = GetWorld()->GetSubsystem<UCk_EcsWorld_Subsystem_UE>();

    const auto CueClass = Subsystem_Cue->Get_CueEntityScript(InCueName);
    ck_cue_subsystem_base::ExecuteCueEntityScript(InCueName, CueClass, Subsystem_EcsWorld, InSpawnParams);
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
    OnPostLoadMapWithWorld(UWorld* InWorld) -> void
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
        APlayerController* NewPlayer) -> void
{
    if (NOT _ValidPlayerControllers.Contains(NewPlayer))
    {
        DoSpawnCueReplicatorActorsForPlayerController(NewPlayer);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_CueSubsystem_Base_UE::
    Initialize(FSubsystemCollectionBase& Collection) -> void
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
    Deinitialize() -> void
{
    Super::Deinitialize();
}

auto
    UCk_CueSubsystem_Base_UE::
    Request_PopulateAllCues() -> void
{
    _DiscoveredCues.Empty();

    const auto CueBaseClass = Get_CueBaseClass();

    CK_ENSURE_IF_NOT(ck::IsValid(CueBaseClass),
        TEXT("CueBaseClass is INVALID. Derived subsystem must implement Get_CueBaseClass"))
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

        // Early filtering: Skip assets that clearly won't be our cue classes
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
            // Don't ensure here - just skip invalid objects silently
            return ContinueIterating;
        }

        const auto ResolvedObjectDefaultClassObject = [&]() -> UObject*
        {
#if WITH_EDITOR
            const auto& Blueprint = Cast<UBlueprint>(ResolvedObject);
            if (ck::Is_NOT_Valid(Blueprint))
            { return nullptr; }

            // Additional safety check for Blueprint validity
            if (ck::Is_NOT_Valid(Blueprint->GeneratedClass))
            { return nullptr; }

            const auto DefaultObject = Blueprint->GeneratedClass->GetDefaultObject();

            // Verify the default object is actually our cue type before proceeding
            if (ck::Is_NOT_Valid(DefaultObject) || NOT DefaultObject->IsA(CueBaseClass))
            { return nullptr; }

            return DefaultObject;
#else
            const auto BlueprintGeneratedClass = Cast<UBlueprintGeneratedClass>(InAssetData.GetAsset());
            if (ck::Is_NOT_Valid(BlueprintGeneratedClass))
            { return nullptr; }

            const auto DefaultObject = BlueprintGeneratedClass->GetDefaultObject();

            // Verify the default object is actually our cue type before proceeding
            if (ck::Is_NOT_Valid(DefaultObject) || NOT DefaultObject->IsA(CueBaseClass))
            { return nullptr; }

            return DefaultObject;
#endif
        }();

        if (ck::Is_NOT_Valid(ResolvedObjectDefaultClassObject))
        {
            // Don't ensure here - just skip objects that don't have valid defaults or aren't our type
            return ContinueIterating;
        }

        // We've already verified this is our cue type above, so this cast should be safe
        const auto CueObject = Cast<UCk_CueBase_EntityScript>(ResolvedObjectDefaultClassObject);

        CK_ENSURE_IF_NOT(ck::IsValid(CueObject),
            TEXT("Cast to CueBaseClass failed for [{}] - this should not happen after type verification"),
            ResolvedObject->GetName())
        { return ContinueIterating; }

        const auto& CueName = CueObject->Get_CueName();

        CK_ENSURE_IF_NOT(ck::IsValid(CueName), TEXT("Cue [{}] has an invalid CueName"), CueObject)
        { return ContinueIterating; }

        // Check for duplicate cue names
        if (_DiscoveredCues.Contains(CueName))
        {
            CK_TRIGGER_ENSURE(TEXT("Duplicate CueName [{}] found! Existing: [{}], New: [{}]"),
                CueName, _DiscoveredCues[CueName], CueObject);
            return ContinueIterating;
        }

        _DiscoveredCues.Add(CueName, CueObject->GetClass());

        return ContinueIterating;
    });

    UE_LOG(LogTemp, Log, TEXT("AudioCue Discovery Complete: Found %d cues"), _DiscoveredCues.Num());
}

auto
    UCk_CueSubsystem_Base_UE::
    DoOnEngineInitComplete() -> void
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
        const FAssetData& InAssetData) -> void
{
    const auto CueBaseClass = Get_CueBaseClass();

    if (ck::Is_NOT_Valid(CueBaseClass))
    { return; }

    const auto CueBaseBlueprint = UCk_Utils_Object_UE::Get_ClassGeneratedByBlueprint(CueBaseClass);

    if (const auto CueBlueprint = UCk_Utils_Object_UE::Get_ClassGeneratedByBlueprint(CueBaseClass);
        InAssetData.IsInstanceOf(CueBlueprint->GetClass(), EResolveClass::Yes))
    {
        Request_PopulateAllCues();
    }
}

auto
    UCk_CueSubsystem_Base_UE::
    DoHandleRenamed(
        const FAssetData& InAssetData,
        const FString&) -> void
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
        const FGameplayTag& InCueName) -> TSubclassOf<UCk_CueBase_EntityScript>
{
#if WITH_EDITOR
    if (_DiscoveredCues.IsEmpty())
    { Request_PopulateAllCues(); }
#endif

    const auto FoundCue = _DiscoveredCues.Find(InCueName);

    return FoundCue ? *FoundCue : nullptr;
}

// --------------------------------------------------------------------------------------------------------------------