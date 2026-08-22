#include "CkJolt_ProjectSettings.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Object/CkObject_Utils.h"

#include <GameFramework/Pawn.h>

// --------------------------------------------------------------------------------------------------------------------

UCk_Jolt_ProjectSettings_UE::UCk_Jolt_ProjectSettings_UE(
    const FObjectInitializer& InObjectInitializer)
    : Super(InObjectInitializer)
{
    _CookedDataRootPath.Path = TEXT("/Game/CkJoltData");

    // Pawn collision is dynamic-object territory — baking a level-placed pawn's capsule into the
    // static world would be wrong vs Chaos, where it is a movable queryable object.
    _BakeExcludedActorClasses.Emplace(APawn::StaticClass());

    const auto NoBakeTag = FName{TEXT("Ck.Jolt.NoBake")};
    _BakeExcludedActorTags.Emplace(NoBakeTag);
    _BakeExcludedComponentTags.Emplace(NoBakeTag);
}

// --------------------------------------------------------------------------------------------------------------------

// Fallback values used when the CDO is unavailable (e.g. during early engine init).
// These are intentionally conservative — e.g. parallel physics falls back to disabled.
namespace ck_jolt_constants
{
    constexpr int32 Fallback_MaxBodies = 65536;
    constexpr int32 Fallback_MaxBodyPairs = 65536;
    constexpr int32 Fallback_MaxContactConstraints = 10240;
    constexpr int32 Fallback_TempAllocatorSizeMB = 10;
    constexpr int32 Fallback_MaxPhysicsJobs = 2048;
    constexpr int32 Fallback_MaxPhysicsBarriers = 8;
    constexpr int32 Fallback_CollisionSteps = 1;
    constexpr int32 Fallback_FixedTimestepHz = 60;
    constexpr int32 Fallback_MaxPhysicsStepsPerFrame = 4;
    constexpr auto  Fallback_RestitutionCombineMode = ECk_Jolt_RestitutionCombineMode::Average;
    constexpr bool  Fallback_EnableParallelPhysics = false;
    constexpr int32 Fallback_NumPhysicsThreads = 0;
    constexpr bool  Fallback_EnableAsyncPhysicsUpdate = false;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Jolt_ProjectSettings::
    Get_MaxBodies()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Jolt_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return ck_jolt_constants::Fallback_MaxBodies; }

    return Settings->Get_MaxBodies();
}

auto
    UCk_Utils_Jolt_ProjectSettings::
    Get_MaxBodyPairs()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Jolt_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return ck_jolt_constants::Fallback_MaxBodyPairs; }

    return Settings->Get_MaxBodyPairs();
}

auto
    UCk_Utils_Jolt_ProjectSettings::
    Get_MaxContactConstraints()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Jolt_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return ck_jolt_constants::Fallback_MaxContactConstraints; }

    return Settings->Get_MaxContactConstraints();
}

auto
    UCk_Utils_Jolt_ProjectSettings::
    Get_TempAllocatorSizeMB()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Jolt_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return ck_jolt_constants::Fallback_TempAllocatorSizeMB; }

    return Settings->Get_TempAllocatorSizeMB();
}

auto
    UCk_Utils_Jolt_ProjectSettings::
    Get_MaxPhysicsJobs()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Jolt_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return ck_jolt_constants::Fallback_MaxPhysicsJobs; }

    return Settings->Get_MaxPhysicsJobs();
}

auto
    UCk_Utils_Jolt_ProjectSettings::
    Get_MaxPhysicsBarriers()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Jolt_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return ck_jolt_constants::Fallback_MaxPhysicsBarriers; }

    return Settings->Get_MaxPhysicsBarriers();
}

auto
    UCk_Utils_Jolt_ProjectSettings::
    Get_CollisionSteps()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Jolt_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return ck_jolt_constants::Fallback_CollisionSteps; }

    return Settings->Get_CollisionSteps();
}

auto
    UCk_Utils_Jolt_ProjectSettings::
    Get_FixedTimestepHz()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Jolt_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return ck_jolt_constants::Fallback_FixedTimestepHz; }

    return Settings->Get_FixedTimestepHz();
}

auto
    UCk_Utils_Jolt_ProjectSettings::
    Get_MaxPhysicsStepsPerFrame()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Jolt_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return ck_jolt_constants::Fallback_MaxPhysicsStepsPerFrame; }

    return Settings->Get_MaxPhysicsStepsPerFrame();
}

auto
    UCk_Utils_Jolt_ProjectSettings::
    Get_EnableParallelPhysics()
    -> bool
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Jolt_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return ck_jolt_constants::Fallback_EnableParallelPhysics; }

    return Settings->Get_EnableParallelPhysics();
}

auto
    UCk_Utils_Jolt_ProjectSettings::
    Get_NumPhysicsThreads()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Jolt_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return ck_jolt_constants::Fallback_NumPhysicsThreads; }

    return Settings->Get_NumPhysicsThreads();
}

auto
    UCk_Utils_Jolt_ProjectSettings::
    Get_EnableAsyncPhysicsUpdate()
    -> bool
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Jolt_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return ck_jolt_constants::Fallback_EnableAsyncPhysicsUpdate; }

    return Settings->Get_EnableAsyncPhysicsUpdate();
}

auto
    UCk_Utils_Jolt_ProjectSettings::
    Get_PIEStaticWorldMode()
    -> ECk_Jolt_PIEStaticWorldMode
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Jolt_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return ECk_Jolt_PIEStaticWorldMode::LiveExtract; }

    return Settings->Get_PIEStaticWorldMode();
}

auto
    UCk_Utils_Jolt_ProjectSettings::
    Get_CookedDataRootPath()
    -> FString
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Jolt_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return TEXT("/Game/CkJoltData"); }

    return Settings->Get_CookedDataRootPath().Path;
}

auto
    UCk_Utils_Jolt_ProjectSettings::
    Get_BakeGridCellSize()
    -> float
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Jolt_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return 25600.0f; }

    return Settings->Get_BakeGridCellSize();
}

auto
    UCk_Utils_Jolt_ProjectSettings::
    Get_CompoundShapeInstanceThreshold()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Jolt_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return 32; }

    return Settings->Get_CompoundShapeInstanceThreshold();
}

auto
    UCk_Utils_Jolt_ProjectSettings::
    Get_BroadphaseOptimizeThreshold()
    -> int32
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Jolt_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return 512; }

    return Settings->Get_BroadphaseOptimizeThreshold();
}

auto
    UCk_Utils_Jolt_ProjectSettings::
    Get_CookExcludedMapPathPrefixes()
    -> TArray<FString>
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Jolt_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return {}; }

    return ck::algo::Transform<TArray<FString>>(Settings->Get_CookExcludedMapPathPrefixes(),
        [](const FDirectoryPath& InPath) { return InPath.Path; });
}

auto
    UCk_Utils_Jolt_ProjectSettings::
    Get_BakeMobilityPolicy()
    -> ECk_Jolt_BakeMobilityPolicy
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Jolt_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return ECk_Jolt_BakeMobilityPolicy::All; }

    return Settings->Get_BakeMobilityPolicy();
}

auto
    UCk_Utils_Jolt_ProjectSettings::
    Get_BakeExcludedActorClasses()
    -> TArray<TSoftClassPtr<AActor>>
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Jolt_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return {}; }

    return Settings->Get_BakeExcludedActorClasses();
}

auto
    UCk_Utils_Jolt_ProjectSettings::
    Get_BakeExcludedActorTags()
    -> TArray<FName>
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Jolt_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return {}; }

    return Settings->Get_BakeExcludedActorTags();
}

auto
    UCk_Utils_Jolt_ProjectSettings::
    Get_BakeExcludedComponentTags()
    -> TArray<FName>
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Jolt_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return {}; }

    return Settings->Get_BakeExcludedComponentTags();
}

auto
    UCk_Utils_Jolt_ProjectSettings::
    Get_BakeExcludedObjectChannels()
    -> TArray<TEnumAsByte<ECollisionChannel>>
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Jolt_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return {}; }

    return Settings->Get_BakeExcludedObjectChannels();
}

auto
    UCk_Utils_Jolt_ProjectSettings::
    Get_BakeExcludedCollisionProfiles()
    -> TArray<FName>
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Jolt_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return {}; }

    return Settings->Get_BakeExcludedCollisionProfiles();
}

auto
    UCk_Utils_Jolt_ProjectSettings::
    Get_BakedMeshShapeRoots()
    -> TArray<FString>
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Jolt_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return {}; }

    return ck::algo::Transform<TArray<FString>>(Settings->Get_BakedMeshShapeRoots(),
        [](const FDirectoryPath& InPath) { return InPath.Path; });
}

auto
    UCk_Utils_Jolt_ProjectSettings::
    Get_BakeExcludeOverlapOnlyComponents()
    -> ECk_EnableDisable
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Jolt_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return ECk_EnableDisable::Disable; }

    return Settings->Get_BakeExcludeOverlapOnlyComponents();
}

auto
    UCk_Utils_Jolt_ProjectSettings::
    Get_RestitutionCombineMode()
    -> ECk_Jolt_RestitutionCombineMode
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Jolt_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return ck_jolt_constants::Fallback_RestitutionCombineMode; }

    return Settings->Get_RestitutionCombineMode();
}

// --------------------------------------------------------------------------------------------------------------------
