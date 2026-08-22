#include "CkJoltCook_EditorSubsystem.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Time/CkTime.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkJolt/CkJolt_Log.h"
#include "CkJolt/CkJolt_Utils.h"
#include "CkJolt/Settings/CkJolt_ProjectSettings.h"
#include "CkJolt/StaticWorld/CkJoltMeshShape_Utils.h"

#include "CkJoltEditor/Cook/CkJoltCook_MeshShapeCooker.h"
#include "CkJoltEditor/Cook/CkJoltCook_WorldCooker.h"
#include "CkJoltEditor/Settings/CkJoltCook_UserSettings.h"

#include <Editor.h>
#include <Engine/Level.h>
#include <Engine/StaticMesh.h>
#include <Engine/World.h>
#include <Framework/Notifications/NotificationManager.h>
#include <Misc/App.h>
#include <UObject/Package.h>
#include <UObject/UObjectHash.h>

#define LOCTEXT_NAMESPACE "CkJoltCookEditorSubsystem"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_jolt_cook_editor_subsystem
{
    // Lower = smoother editor + longer wall-clock; higher = the reverse. Matches the asset-registry
    // generator's slice budget.
    constexpr auto SliceBudget = FCk_Time{0.008};

    static auto Get_EditorWorld() -> UWorld*
    {
        if (ck::Is_NOT_Valid(GEditor))
        { return nullptr; }

        return GEditor->GetEditorWorldContext().World();
    }

    static auto Get_IsStreamingSublevelOf(const UWorld& InSavedWorld, const UWorld& InEditorWorld) -> bool
    {
        return ck::algo::AnyOf(InEditorWorld.GetLevels(), [&](const ULevel* InLevel)
        {
            return ck::IsValid(InLevel) && InLevel->GetTypedOuter<UWorld>() == &InSavedWorld;
        });
    }

    static auto Get_BelongsToEditorWorld(const UWorld& InSavedWorld, const UWorld& InEditorWorld) -> bool
    {
        return &InSavedWorld == &InEditorWorld || Get_IsStreamingSublevelOf(InSavedWorld, InEditorWorld);
    }

    static auto Collect_StaticMeshesInPackage(UPackage& InPackage) -> TArray<const UStaticMesh*>
    {
        auto Meshes = TArray<const UStaticMesh*>{};

        constexpr auto IncludeNestedObjects = false;
        ForEachObjectWithPackage(&InPackage, [&](UObject* InObject)
        {
            if (const auto* Mesh = Cast<UStaticMesh>(InObject))
            { Meshes.Emplace(Mesh); }

            return true;
        }, IncludeNestedObjects);

        return Meshes;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_JoltCook_EditorSubsystem_UE::
    Initialize(
        FSubsystemCollectionBase& InCollection)
    -> void
{
    Super::Initialize(InCollection);

    _PostSaveWorldHandle = FEditorDelegates::PostSaveWorldWithContext.AddUObject(
        this, &UCk_JoltCook_EditorSubsystem_UE::DoHandle_PostSaveWorld);

    _PackageSavedHandle = UPackage::PackageSavedWithContextEvent.AddUObject(
        this, &UCk_JoltCook_EditorSubsystem_UE::DoHandle_PackageSaved);
}

auto
    UCk_JoltCook_EditorSubsystem_UE::
    Deinitialize()
    -> void
{
    FEditorDelegates::PostSaveWorldWithContext.Remove(_PostSaveWorldHandle);
    UPackage::PackageSavedWithContextEvent.Remove(_PackageSavedHandle);

    if (ck::IsValid(GEditor))
    { GEditor->GetTimerManager()->ClearTimer(_AutoCookDebounceTimer); }

    Dismiss_DrainTicker();
    Dismiss_ProgressNotification();
    _WorldCookDriver.Reset();
    DoRelease_JoltGlobals();

    Super::Deinitialize();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_JoltCook_EditorSubsystem_UE::
    Get_IsAutoCookAllowed()
    -> bool
{
    if (ck::Is_NOT_Valid(GEditor))
    { return false; }

    // The AutoTest populator auto-saves its own level; without this every test lane would cook.
    if (IsRunningCommandlet() || FApp::IsUnattended() || GIsAutomationTesting)
    { return false; }

    if (GEditor->IsPlaySessionInProgress() || GEditor->PlayWorld != nullptr)
    { return false; }

    return true;
}

auto
    UCk_JoltCook_EditorSubsystem_UE::
    Get_IsMapExcludedFromCook(
        const FString& InMapPackageName)
    -> bool
{
    const auto ExcludedPrefixes = UCk_Utils_Jolt_ProjectSettings::Get_CookExcludedMapPathPrefixes();

    return ExcludedPrefixes.ContainsByPredicate([&](const FString& InPrefix)
    {
        return NOT InPrefix.IsEmpty() && InMapPackageName.StartsWith(InPrefix);
    });
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_JoltCook_EditorSubsystem_UE::
    DoHandle_PostSaveWorld(
        UWorld* InWorld,
        FObjectPostSaveContext InContext)
    -> void
{
    using namespace ck_jolt_cook_editor_subsystem;

    if (NOT Get_IsAutoCookAllowed() || InContext.IsProceduralSave())
    { return; }

    if (UCk_Utils_JoltCook_UserSettings::Get_AutoCookStaticWorldOnLevelSave() != ECk_EnableDisable::Enable)
    { return; }

    if (ck::Is_NOT_Valid(InWorld))
    { return; }

    auto* EditorWorld = Get_EditorWorld();
    if (ck::Is_NOT_Valid(EditorWorld) || NOT Get_BelongsToEditorWorld(*InWorld, *EditorWorld))
    { return; }

    const auto MapPackageName = EditorWorld->PersistentLevel->GetOutermost()->GetName();

    if (Get_IsMapExcludedFromCook(MapPackageName))
    {
        ck::jolt::Verbose(TEXT("JoltCook auto: map [{}] is excluded from cooking — skipping the on-save cook"),
            MapPackageName);
        return;
    }

    _PendingWorldCook = true;
    Request_ScheduleAutoCook();
}

auto
    UCk_JoltCook_EditorSubsystem_UE::
    DoHandle_PackageSaved(
        const FString& InPackageFileName,
        UPackage* InPackage,
        FObjectPostSaveContext InContext)
    -> void
{
    using namespace ck_jolt_cook_editor_subsystem;

    if (NOT Get_IsAutoCookAllowed() || InContext.IsProceduralSave())
    { return; }

    if (UCk_Utils_JoltCook_UserSettings::Get_AutoCookMeshShapeOnAssetSave() != ECk_EnableDisable::Enable)
    { return; }

    if (ck::Is_NOT_Valid(InPackage))
    { return; }

    const auto PackageName = InPackage->GetName();

    // The cook writes into this root; reacting to its own writes would loop.
    if (PackageName.StartsWith(UCk_Utils_Jolt_ProjectSettings::Get_CookedDataRootPath()))
    { return; }

    if (NOT ck::jolt::bake::mesh_shape_utils::Get_IsUnderBakedRoot(PackageName))
    { return; }

    for (const auto* Mesh : Collect_StaticMeshesInPackage(*InPackage))
    {
        _PendingMeshCooks.AddUnique(FSoftObjectPath{Mesh});
    }

    if (_PendingMeshCooks.IsEmpty())
    { return; }

    Request_ScheduleAutoCook();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_JoltCook_EditorSubsystem_UE::
    Request_ScheduleAutoCook()
    -> void
{
    const auto EditorIsValid = ck::IsValid(GEditor);
    CK_ENSURE_IF_NOT(EditorIsValid, TEXT("Cannot schedule a Jolt auto-cook — GEditor is not valid"))
    { return; }

    // Restarted on every save so a burst (Save All, a multi-sublevel save) coalesces into one cook.
    GEditor->GetTimerManager()->ClearTimer(_AutoCookDebounceTimer);

    constexpr auto MinimumDebounce = FCk_Time{0.1};
    const auto Debounce = FMath::Max(UCk_Utils_JoltCook_UserSettings::Get_AutoCookDebounce(), MinimumDebounce);

    constexpr auto Repeat = false;
    GEditor->GetTimerManager()->SetTimer(
        _AutoCookDebounceTimer,
        this,
        &UCk_JoltCook_EditorSubsystem_UE::Execute_ScheduledAutoCook,
        static_cast<float>(Debounce.Get_Seconds()),
        Repeat);
}

auto
    UCk_JoltCook_EditorSubsystem_UE::
    Execute_ScheduledAutoCook()
    -> void
{
    const auto HasPendingWork = _PendingWorldCook || NOT _PendingMeshCooks.IsEmpty();

    if (NOT HasPendingWork)
    { return; }

    // PIE can have started during the debounce window; hold the work rather than dropping the save.
    if (NOT Get_IsAutoCookAllowed())
    {
        Request_ScheduleAutoCook();
        return;
    }

    Start_Drain(LOCTEXT("JoltAutoCookProgress", "Cooking Jolt data"));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_JoltCook_EditorSubsystem_UE::
    Start_Drain(
        const FText& InProgressLabel)
    -> bool
{
    if (_DrainTickerHandle.IsValid())
    { return false; }

    _DrainWorldCookPending = _PendingWorldCook;
    _PendingWorldCook = false;

    _DrainTotalItems = _PendingMeshCooks.Num() + _SweepCandidates.Num() + (_DrainWorldCookPending ? 1 : 0);
    _DrainCompletedItems = 0;
    _DrainStats = FCk_Jolt_MeshShapeCooker::FCookStats{};

    if (_DrainTotalItems == 0)
    { return false; }

    DoAcquire_JoltGlobals();

    _ActiveProgressNotification = FSlateNotificationManager::Get().StartProgressNotification(
        InProgressLabel, _DrainTotalItems);

    _DrainTickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
        [this](float) -> bool
        {
            return Tick_Drain();
        }));

    return true;
}

auto
    UCk_JoltCook_EditorSubsystem_UE::
    Tick_Drain()
    -> bool
{
    using namespace ck_jolt_cook_editor_subsystem;
    using namespace ck::jolt::cook;

    Tick_MeshCooks(SliceBudget);

    const auto MeshCooksRemain = NOT _PendingMeshCooks.IsEmpty() || _SweepNextIndex < _SweepCandidates.Num();

    if (_ActiveProgressNotification.IsValid())
    {
        // A save landing mid-drain appends work, and the world cook only knows its own size once it
        // has read the index — so the total is re-reported rather than captured.
        const auto WorldCookUnitsRemaining = _WorldCookDriver.IsValid()
            ? _WorldCookDriver->Get_TotalUnits() - _WorldCookDriver->Get_CompletedUnits()
            : 0;

        _DrainTotalItems = FMath::Max(_DrainTotalItems,
            _DrainCompletedItems + _PendingMeshCooks.Num() + (_SweepCandidates.Num() - _SweepNextIndex)
                + WorldCookUnitsRemaining);

        FSlateNotificationManager::Get().UpdateProgressNotification(
            _ActiveProgressNotification, _DrainCompletedItems, _DrainTotalItems);
    }

    if (MeshCooksRemain)
    { return true; }

    if (_DrainWorldCookPending)
    { return Tick_WorldCook(SliceBudget); }

    Finish_Drain();
    return false;
}

auto
    UCk_JoltCook_EditorSubsystem_UE::
    Tick_MeshCooks(
        FCk_Time InBudget)
    -> void
{
    using namespace ck::jolt::cook;

    const auto SliceStart = FPlatformTime::Seconds();

    while (NOT _PendingMeshCooks.IsEmpty() || _SweepNextIndex < _SweepCandidates.Num())
    {
        auto Mesh = static_cast<const UStaticMesh*>(nullptr);

        if (NOT _PendingMeshCooks.IsEmpty())
        {
            const auto MeshPath = _PendingMeshCooks[0];
            _PendingMeshCooks.RemoveAt(0);
            Mesh = Cast<UStaticMesh>(MeshPath.ResolveObject());
        }
        else
        {
            Mesh = Cast<UStaticMesh>(_SweepCandidates[_SweepNextIndex].GetAsset());
            ++_SweepNextIndex;
        }

        ++_DrainCompletedItems;
        ++_DrainStats._NumMeshesConsidered;

        // Unloaded since the save: loading it again re-runs the save hook, so this is not a failure.
        if (ck::Is_NOT_Valid(Mesh))
        { continue; }

        auto CookedAssetPath = FString{};
        const auto Result = FCk_Jolt_MeshShapeCooker::Cook_SingleMeshShape(
            *Mesh, ECk_Jolt_CookMode::Cook, &CookedAssetPath);

        if (NOT CookedAssetPath.IsEmpty())
        { _SweepCookedPathsInUse.Add(CookedAssetPath); }

        FCk_Jolt_MeshShapeCooker::Accumulate_SingleResult(Result, _DrainStats);

        if (FCk_Time{FPlatformTime::Seconds() - SliceStart} >= InBudget)
        { return; }
    }
}

auto
    UCk_JoltCook_EditorSubsystem_UE::
    Tick_WorldCook(
        FCk_Time InBudget)
    -> bool
{
    using namespace ck_jolt_cook_editor_subsystem;
    using namespace ck::jolt::cook;

    // A drain spans many frames, so PIE can have started since it began.
    if (NOT Get_IsAutoCookAllowed())
    {
        _PendingWorldCook = true;
        _DrainWorldCookPending = false;
        _WorldCookDriver.Reset();
        Finish_Drain();
        return false;
    }

    if (_WorldCookDriver.IsValid() == false)
    {
        auto* EditorWorld = Get_EditorWorld();

        if (ck::Is_NOT_Valid(EditorWorld))
        {
            _DrainWorldCookPending = false;
            Finish_Drain();
            return false;
        }

        _WorldCookDriver = MakeUnique<FCk_Jolt_IncrementalCookDriver>(*EditorWorld, ECk_Jolt_CookMode::Cook);
        _WorldCookUnitsCounted = 0;
    }

    const auto Result = _WorldCookDriver->Step(InBudget);

    // The driver's unit count only firms up as it discovers the map; feed the delta through so the
    // bar advances instead of jumping at the end.
    const auto Completed = _WorldCookDriver->Get_CompletedUnits();
    _DrainCompletedItems += Completed - _WorldCookUnitsCounted;
    _WorldCookUnitsCounted = Completed;

    if (Result == ECk_Jolt_CookStepResult::InProgress)
    { return true; }

    if (Result == ECk_Jolt_CookStepResult::FullCookRequired)
    {
        auto* EditorWorld = Get_EditorWorld();
        _WorldCookDriver.Reset();

        // The full cook cannot be sliced (its World Partition walk owns its own loop), so this one
        // blocks. It only happens on a first cook or a contract change, never on a routine save.
        if (ck::IsValid(EditorWorld))
        { FCk_Jolt_WorldCooker::Cook_World(*EditorWorld, ECk_Jolt_CookMode::Cook); }
    }

    _WorldCookDriver.Reset();
    _DrainWorldCookPending = false;
    return true;
}

auto
    UCk_JoltCook_EditorSubsystem_UE::
    Finish_Drain()
    -> void
{
    // Only a full sweep enumerates every in-use path; a partial drain would flag the rest as orphans.
    if (_SweepReportsOrphans)
    {
        _DrainStats._NumOrphans = FCk_Jolt_MeshShapeCooker::Report_Orphans(_SweepCookedPathsInUse);
        _SweepReportsOrphans = false;
    }

    if (_DrainStats._NumMeshesConsidered > 0)
    {
        _DrainStats._Success = _DrainStats._NumFailed == 0;
        FCk_Jolt_MeshShapeCooker::Log_CookStats(_DrainStats, ck::jolt::cook::ECk_Jolt_CookMode::Cook);
    }

    _SweepCandidates.Reset();
    _SweepNextIndex = 0;
    _SweepCookedPathsInUse.Reset();
    _DrainTotalItems = 0;
    _DrainCompletedItems = 0;

    _WorldCookDriver.Reset();
    _DrainTickerHandle.Reset();
    Dismiss_ProgressNotification();
    DoRelease_JoltGlobals();

    // A save that landed mid-drain could not join it, so it gets its own pass.
    if (_PendingWorldCook || NOT _PendingMeshCooks.IsEmpty())
    { Request_ScheduleAutoCook(); }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_JoltCook_EditorSubsystem_UE::
    DoAcquire_JoltGlobals()
    -> void
{
    if (_HoldsJoltGlobals)
    { return; }

    ck::jolt::Request_GlobalJoltInit();
    _HoldsJoltGlobals = true;
}

auto
    UCk_JoltCook_EditorSubsystem_UE::
    DoRelease_JoltGlobals()
    -> void
{
    if (NOT _HoldsJoltGlobals)
    { return; }

    _HoldsJoltGlobals = false;
    ck::jolt::Request_GlobalJoltShutdown();
}

auto
    UCk_JoltCook_EditorSubsystem_UE::
    Dismiss_ProgressNotification()
    -> void
{
    if (NOT _ActiveProgressNotification.IsValid())
    { return; }

    FSlateNotificationManager::Get().CancelProgressNotification(_ActiveProgressNotification);
    _ActiveProgressNotification.Reset();
}

auto
    UCk_JoltCook_EditorSubsystem_UE::
    Dismiss_DrainTicker()
    -> void
{
    if (NOT _DrainTickerHandle.IsValid())
    { return; }

    FTSTicker::GetCoreTicker().RemoveTicker(_DrainTickerHandle);
    _DrainTickerHandle.Reset();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_JoltCook_EditorSubsystem_UE::
    Cook_CurrentWorld()
    -> bool
{
    auto* World = ck_jolt_cook_editor_subsystem::Get_EditorWorld();

    const auto WorldIsValid = ck::IsValid(World);
    CK_ENSURE_IF_NOT(WorldIsValid, TEXT("No editor world to cook"))
    { return false; }

    return FCk_Jolt_WorldCooker::Cook_World(*World, ck::jolt::cook::ECk_Jolt_CookMode::Cook)._Success;
}

auto
    UCk_JoltCook_EditorSubsystem_UE::
    Request_CookStaticWorld()
    -> bool
{
    if (_DrainTickerHandle.IsValid())
    {
        ck::jolt::Warning(TEXT("JoltCook: a cook is already running — ignoring the request"));
        return false;
    }

    _PendingWorldCook = true;
    return Start_Drain(LOCTEXT("JoltWorldCookProgress", "Cooking Jolt static world"));
}

auto
    UCk_JoltCook_EditorSubsystem_UE::
    Cook_CurrentWorld_Incremental()
    -> bool
{
    auto* World = ck_jolt_cook_editor_subsystem::Get_EditorWorld();

    const auto WorldIsValid = ck::IsValid(World);
    CK_ENSURE_IF_NOT(WorldIsValid, TEXT("No editor world to cook"))
    { return false; }

    return FCk_Jolt_WorldCooker::Cook_World_Incremental(*World, ck::jolt::cook::ECk_Jolt_CookMode::Cook)._Success;
}

auto
    UCk_JoltCook_EditorSubsystem_UE::
    Cook_CurrentWorld_DryRun()
    -> bool
{
    auto* World = ck_jolt_cook_editor_subsystem::Get_EditorWorld();

    const auto WorldIsValid = ck::IsValid(World);
    CK_ENSURE_IF_NOT(WorldIsValid, TEXT("No editor world to cook"))
    { return false; }

    return FCk_Jolt_WorldCooker::Cook_World(*World, ck::jolt::cook::ECk_Jolt_CookMode::DryRun)._Success;
}

auto
    UCk_JoltCook_EditorSubsystem_UE::
    Validate_CurrentWorld()
    -> bool
{
    auto* World = ck_jolt_cook_editor_subsystem::Get_EditorWorld();

    const auto WorldIsValid = ck::IsValid(World);
    CK_ENSURE_IF_NOT(WorldIsValid, TEXT("No editor world to validate"))
    { return false; }

    return FCk_Jolt_WorldCooker::Validate_World(*World)._Success;
}

auto
    UCk_JoltCook_EditorSubsystem_UE::
    Cook_MeshShapes()
    -> bool
{
    return FCk_Jolt_MeshShapeCooker::Cook_MeshShapes(ck::jolt::cook::ECk_Jolt_CookMode::Cook)._Success;
}

auto
    UCk_JoltCook_EditorSubsystem_UE::
    Request_CookMeshShapes()
    -> bool
{
    if (_DrainTickerHandle.IsValid())
    {
        ck::jolt::Warning(TEXT("JoltMeshCook: a cook is already running — ignoring the request"));
        return false;
    }

    _SweepCandidates = FCk_Jolt_MeshShapeCooker::Collect_Candidates();
    _SweepNextIndex = 0;
    _SweepCookedPathsInUse.Reset();
    _SweepReportsOrphans = true;

    if (_SweepCandidates.IsEmpty())
    {
        ck::jolt::Log(TEXT("JoltMeshCook: no meshes under the configured _BakedMeshShapeRoots — nothing to cook"));
        _SweepReportsOrphans = false;
        return false;
    }

    return Start_Drain(LOCTEXT("JoltMeshCookProgress", "Cooking Jolt mesh shapes"));
}

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE
