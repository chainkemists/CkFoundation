#include "CkCueSubsystem_Base.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Math/Arithmetic/CkArithmetic_Utils.h"
#include "CkCore/Debug/CkDebug_Utils.h"
#include "CkCore/IO/CkIO_Utils.h"

#include "CkCue/CkCue_Fragment.h"
#include "CkCue/CkCue_Log.h"

#include "CkEcs/EntityScript/CkEntityScript_Utils.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include <AssetRegistry/AssetRegistryModule.h>

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
}

auto
    ACk_CueExecutor_UE::
    BeginPlay()
    -> void
{
    Super::BeginPlay();

    _Subsystem_CueExecutor = GetWorld()->GetSubsystem<UCk_CueExecutor_Subsystem_Base_UE>();
    _Subsystem_EcsWorld = GetWorld()->GetSubsystem<UCk_EcsWorld_Subsystem_UE>();

    if (NOT IsNetMode(NM_Client))
    { return; }

    if (ck::Is_NOT_Valid(GetOwner()))
    { return; }

    _Subsystem_CueExecutor->_CueExecutors.Emplace(this);
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
    if (GetWorld()->IsNetMode(NM_DedicatedServer) || GetWorld()->IsNetMode(NM_ListenServer))
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
        APlayerController* InPlayerController) -> void
{
    auto AlreadyContainsPC = false;
    _ValidPlayerControllers.Add(InPlayerController, &AlreadyContainsPC);

    if (AlreadyContainsPC)
    { return; }

    // Spawn one executor per player controller for now
    // Derived classes can override this behavior if needed
    auto CueExecutor = GetWorld()->SpawnActor<ACk_CueExecutor_UE>();
    _CueExecutors.Emplace(CueExecutor);
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

    ck::cue::Log(TEXT("=== CUE DISCOVERY START ==="));
    ck::cue::Log(TEXT("CueBaseClass: [{}]"), CueBaseClass->GetName());

    // Find all loaded classes that inherit from CueBaseClass (C++/Angelscript)
    int32 LoadedClassCount = 0;
    int32 CueChildClasses = 0;
    int32 ValidCueClassCount = 0;

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
        ck::cue::Log(TEXT("Found CueBase child: [{}]"), Class->GetName());

        auto DefaultObject = Cast<UCk_CueBase_EntityScript>(Class->GetDefaultObject());
        if (ck::Is_NOT_Valid(DefaultObject))
        {
            ck::cue::Warning(TEXT("  Failed to cast to UCk_CueBase_EntityScript"));
            continue;
        }

        auto CueName = DefaultObject->Get_CueName();
        ck::cue::Log(TEXT("  CueName: [{}]"), CueName);

        if (NOT ck::IsValid(CueName))
        {
            ck::cue::Warning(TEXT("  Invalid CueName"));
            continue;
        }

        if (CueName == TAG_Cue_DoNotExecute)
        {
            ck::cue::Log(TEXT("  Skipping DoNotExecute"));
            continue;
        }

        if (_DiscoveredCues.Contains(CueName))
        {
            auto ExistingClassName = _DiscoveredCues[CueName]->GetName();
            auto NewClassName = Class->GetName();

            ck::cue::Warning(TEXT("  Duplicate CueName [{}] - Existing: [{}], New: [{}]"),
                           CueName, ExistingClassName, NewClassName);

            if (NewClassName.Contains(TEXT("REINST_")) || ExistingClassName.Contains(TEXT("REINST_")))
            {
                if (NOT NewClassName.Contains(TEXT("REINST_")))
                {
                    ck::cue::Log(TEXT("  Replacing REINST class"));
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
        ck::cue::Log(TEXT("  SUCCESS: Added [{}]"), CueName);
    }

    ck::cue::Log(TEXT("C++ class scan: Total=[{}], CueChildren=[{}], Valid=[{}]"),
                 LoadedClassCount, CueChildClasses, ValidCueClassCount);

    // Also check unloaded Blueprint assets
    Request_PopulateBlueprintCues();

    ck::cue::Log(TEXT("=== FINAL CUE DISCOVERY RESULTS ==="));
    ck::cue::Log(TEXT("Total cues discovered: [{}]"), _DiscoveredCues.Num());

    for (const auto& [CueName, CueClass] : _DiscoveredCues)
    {
        ck::cue::Log(TEXT("Final: [{}] -> [{}]"), CueName, CueClass->GetName());
    }
}

auto
    UCk_CueSubsystem_Base_UE::
    Request_PopulateBlueprintCues()
    -> void
{
    auto CueBaseClass = Get_CueBaseClass();
    if (ck::Is_NOT_Valid(CueBaseClass))
    {
        ck::cue::Warning(TEXT("CueBaseClass is invalid in Request_PopulateBlueprintCues"));
        return;
    }

    ck::cue::Log(TEXT("=== BLUEPRINT CUE DISCOVERY START ==="));
    ck::cue::Log(TEXT("CueBaseClass: [{}]"), CueBaseClass->GetName());

    const auto& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    const auto& AssetRegistry = AssetRegistryModule.Get();

    if (AssetRegistry.IsLoadingAssets())
    {
        ck::cue::Warning(TEXT("AssetRegistry is still loading assets!"));
    }

    // Try getting ALL assets first to see what's available
    TArray<FAssetData> AllAssets;
    AssetRegistry.GetAllAssets(AllAssets);
    ck::cue::Log(TEXT("Total assets in registry: [{}]"), AllAssets.Num());

    int32 BlueprintAssetCount = 0;
    int32 ProcessedAssets = 0;
    int32 ValidCueAssets = 0;

    for (const auto& InAssetData : AllAssets)
    {
#if WITH_EDITOR
        if (NOT InAssetData.IsInstanceOf<UBlueprint>())
        { continue; }
        BlueprintAssetCount++;

        ck::cue::Log(TEXT("Processing UBlueprint: [{}]"), InAssetData.AssetName.ToString());

        // Check for cue tag in editor
        auto IsCueAssetTag = InAssetData.GetTagValueRef<FString>("IsCueAsset");
        ck::cue::Log(TEXT("  IsCueAsset tag: [{}]"), IsCueAssetTag.IsEmpty() == false ? IsCueAssetTag : TEXT("NOT_SET"));

        if (NOT IsCueAssetTag.Equals("true"))
        {
            ck::cue::Log(TEXT("  Skipping - not tagged as cue asset"));
            continue;
        }
#else
        if (NOT InAssetData.IsInstanceOf<UBlueprintGeneratedClass>())
        { continue; }
        BlueprintAssetCount++;

        ck::cue::Log(TEXT("Processing UBlueprintGeneratedClass: [{}]"), InAssetData.AssetName.ToString());
#endif

        ProcessedAssets++;

        // Skip factory and default objects
        if (const auto& AssetName = InAssetData.AssetName.ToString();
            AssetName.Contains(TEXT("AssetFactory")) ||
            AssetName.Contains(TEXT("_Factory")) ||
            AssetName.Contains(TEXT("Default__")))
        {
            ck::cue::Log(TEXT("  Skipping factory/default: [{}]"), AssetName);
            continue;
        }

#if WITH_EDITOR
        ck::cue::Log(TEXT("  Loading blueprint: [{}]"), InAssetData.GetSoftObjectPath().ToString());
        auto ResolvedObject = InAssetData.GetSoftObjectPath().TryLoad();
        if (ck::Is_NOT_Valid(ResolvedObject))
        {
            ck::cue::Warning(TEXT("  Failed to load blueprint"));
            continue;
        }

        auto Blueprint = Cast<UBlueprint>(ResolvedObject);
        if (ck::Is_NOT_Valid(Blueprint))
        {
            ck::cue::Warning(TEXT("  Failed to cast to UBlueprint"));
            continue;
        }

        if (ck::Is_NOT_Valid(Blueprint->GeneratedClass))
        {
            ck::cue::Warning(TEXT("  Blueprint has no GeneratedClass"));
            continue;
        }

        ck::cue::Log(TEXT("  GeneratedClass: [{}]"), Blueprint->GeneratedClass->GetName());
        ck::cue::Log(TEXT("  Is child of CueBase: [{}]"), Blueprint->GeneratedClass->IsChildOf(CueBaseClass) ? TEXT("YES") : TEXT("NO"));

        if (NOT Blueprint->GeneratedClass->IsChildOf(CueBaseClass))
        {
            ck::cue::Log(TEXT("  Not a cue class, skipping"));
            continue;
        }

        auto DefaultObject = Blueprint->GeneratedClass->GetDefaultObject();
        if (ck::Is_NOT_Valid(DefaultObject))
        {
            ck::cue::Warning(TEXT("  Failed to get default object"));
            continue;
        }

        auto CueObject = Cast<UCk_CueBase_EntityScript>(DefaultObject);
#else
        ck::cue::Log(TEXT("  Getting asset: [{}]"), InAssetData.GetSoftObjectPath().ToString());
        auto BlueprintGeneratedClass = Cast<UBlueprintGeneratedClass>(InAssetData.GetAsset());
        if (ck::Is_NOT_Valid(BlueprintGeneratedClass))
        {
            ck::cue::Warning(TEXT("  Failed to get UBlueprintGeneratedClass"));
            continue;
        }

        ck::cue::Log(TEXT("  Class: [{}]"), BlueprintGeneratedClass->GetName());
        ck::cue::Log(TEXT("  Is child of CueBase: [{}]"), BlueprintGeneratedClass->IsChildOf(CueBaseClass) ? TEXT("YES") : TEXT("NO"));

        if (NOT BlueprintGeneratedClass->IsChildOf(CueBaseClass))
        {
            ck::cue::Log(TEXT("  Not a cue class, skipping"));
            continue;
        }

        auto DefaultObject = BlueprintGeneratedClass->GetDefaultObject();
        if (ck::Is_NOT_Valid(DefaultObject))
        {
            ck::cue::Warning(TEXT("  Failed to get default object"));
            continue;
        }

        auto CueObject = Cast<UCk_CueBase_EntityScript>(DefaultObject);
#endif

        if (ck::Is_NOT_Valid(CueObject))
        {
            ck::cue::Warning(TEXT("  Failed to cast to UCk_CueBase_EntityScript"));
            continue;
        }

        auto CueName = CueObject->Get_CueName();
        ck::cue::Log(TEXT("  CueName: [{}]"), CueName);

        if (CueName == TAG_Cue_DoNotExecute)
        {
            ck::cue::Log(TEXT("  Skipping DoNotExecute cue"));
            continue;
        }

        if (NOT CueName.IsValid())
        {
            ck::cue::Warning(TEXT("  Invalid CueName"));
            continue;
        }

        if (_DiscoveredCues.Contains(CueName))
        {
            ck::cue::Log(TEXT("  CueName already exists, skipping"));
            continue;
        }

        _DiscoveredCues.Add(CueName, CueObject->GetClass());
        ValidCueAssets++;
        ck::cue::Log(TEXT("  SUCCESS: Added cue [{}] -> [{}]"), CueName, CueObject->GetClass()->GetName());
    }

    ck::cue::Log(TEXT("=== BLUEPRINT CUE DISCOVERY END ==="));
    ck::cue::Log(TEXT("Blueprint assets: [{}], Processed: [{}], Valid cues: [{}]"),
                 BlueprintAssetCount, ProcessedAssets, ValidCueAssets);

    // FALLBACK: Try direct class iteration for BlueprintGeneratedClass
    ck::cue::Log(TEXT("=== FALLBACK: DIRECT CLASS ITERATION FOR BLUEPRINTS ==="));
    int32 FallbackCueCount = 0;

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

        ck::cue::Log(TEXT("Found Blueprint cue class via iterator: [{}]"), GeneratedClass->GetName());

        auto DefaultObject = Cast<UCk_CueBase_EntityScript>(GeneratedClass->GetDefaultObject());
        if (ck::Is_NOT_Valid(DefaultObject))
        {
            ck::cue::Warning(TEXT("  Failed to cast to cue script"));
            continue;
        }

        auto CueName = DefaultObject->Get_CueName();
        ck::cue::Log(TEXT("  CueName: [{}]"), CueName);

        if (CueName == TAG_Cue_DoNotExecute || NOT CueName.IsValid())
        { continue; }

        if (_DiscoveredCues.Contains(CueName))
        {
            ck::cue::Log(TEXT("  Already discovered"));
            continue;
        }

        _DiscoveredCues.Add(CueName, GeneratedClass);
        FallbackCueCount++;
        ck::cue::Log(TEXT("  FALLBACK SUCCESS: Added [{}] -> [{}]"), CueName, GeneratedClass->GetName());
    }

    ck::cue::Log(TEXT("=== FALLBACK END - Added [{}] additional cues ==="), FallbackCueCount);
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
