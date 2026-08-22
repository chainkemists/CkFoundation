#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkSettings/Public/CkSettings/ProjectSettings/CkProjectSettings.h"

#include <CoreMinimal.h>
#include <Engine/EngineTypes.h>

#include "CkJolt_ProjectSettings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class AActor;

// --------------------------------------------------------------------------------------------------------------------

/// Where the Jolt static world comes from in PIE. Packaged builds ALWAYS use cooked data.
UENUM(BlueprintType)
enum class ECk_Jolt_PIEStaticWorldMode : uint8
{
    // Extract collision live from level actors on load — designers never need to re-cook to
    // iterate, and stale cooked data can never silently affect PIE. (Default)
    LiveExtract,
    // Load cooked Jolt data like a packaged build (for validating cooks in PIE).
    Cooked,
    // No static world in PIE.
    Disabled
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Jolt_PIEStaticWorldMode);

// --------------------------------------------------------------------------------------------------------------------

/// Which component mobilities the static-world LEVEL SWEEP bakes. ExplicitActor bakes (Request_BakeActor)
/// always bake every mobility — the caller declared the actor static-in-intent.
UENUM(BlueprintType)
enum class ECk_Jolt_BakeMobilityPolicy : uint8
{
    // Bake every collision-bearing component regardless of mobility. A baked Movable is a SNAPSHOT at
    // sweep time — if gameplay moves it later, the static body does not follow. (Default)
    All,
    // Bake Static and Stationary components; skip Movable.
    StaticAndStationary,
    // Bake Static-mobility components only.
    StaticOnly
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Jolt_BakeMobilityPolicy);

// --------------------------------------------------------------------------------------------------------------------

/// How a contact pair's two restitutions resolve into the one the solver uses. Mirrors UE's
/// EFrictionCombineMode (Chaos reads the same four modes off UPhysicsSettingsCore), because a
/// physical material authored against Chaos carries its numbers here unchanged and has to land on
/// the same value.
UENUM(BlueprintType)
enum class ECk_Jolt_RestitutionCombineMode : uint8
{
    // (rA + rB) / 2 — what Chaos does when no material overrides the mode, which is the default in
    // every UE project. (Default)
    Average,
    // min(rA, rB) — the least bouncy surface wins.
    Min,
    // rA * rB.
    Multiply,
    // max(rA, rB) — Jolt's own stock behaviour, and NOT what a Chaos-authored material expects: it
    // lets one bouncy surface impose full elasticity on every dead surface it touches.
    Max
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Jolt_RestitutionCombineMode);

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "Jolt"))
class CKJOLT_API UCk_Jolt_ProjectSettings_UE : public UCk_Plugin_ProjectSettings_UE
{
    GENERATED_BODY()

    CK_GENERATED_BODY(UCk_Jolt_ProjectSettings_UE);

private:
    // Maximum number of physics bodies that can exist simultaneously
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Jolt Physics",
              meta = (AllowPrivateAccess = true, ClampMin = 1024, UIMin = 1024))
    int32 _MaxBodies = 65536;

    // Maximum number of body pairs that can be detected simultaneously
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Jolt Physics",
              meta = (AllowPrivateAccess = true, ClampMin = 1024, UIMin = 1024))
    int32 _MaxBodyPairs = 65536;

    // Maximum number of contact constraints that can be active simultaneously
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Jolt Physics",
              meta = (AllowPrivateAccess = true, ClampMin = 1024, UIMin = 1024))
    int32 _MaxContactConstraints = 10240;

    // Size of the temporary allocator in megabytes, used for per-frame physics allocations
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Jolt Physics",
              meta = (AllowPrivateAccess = true, ClampMin = 1, UIMin = 1, UIMax = 256))
    int32 _TempAllocatorSizeMB = 10;

    // Maximum number of physics jobs that can be scheduled per update
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Jolt Physics",
              meta = (AllowPrivateAccess = true, ClampMin = 256, UIMin = 256))
    int32 _MaxPhysicsJobs = 2048;

    // Maximum number of job synchronization barriers
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Jolt Physics",
              meta = (AllowPrivateAccess = true, ClampMin = 1, UIMin = 1))
    int32 _MaxPhysicsBarriers = 8;

    // Number of collision steps per physics update. Higher values improve accuracy at the cost of performance
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Jolt Physics",
              meta = (AllowPrivateAccess = true, ClampMin = 1, ClampMax = 4, UIMin = 1, UIMax = 4))
    int32 _CollisionSteps = 1;

    // Fixed-timestep rate (Hz) the ECS step pump advances the simulation at. The pump accumulates real
    // delta and runs whole fixed sub-steps, keeping physics deterministic regardless of frame rate.
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Jolt Physics|Simulation",
              meta = (AllowPrivateAccess = true, ClampMin = 15, ClampMax = 240, UIMin = 15, UIMax = 240))
    int32 _FixedTimestepHz = 60;

    // Maximum fixed sub-steps run in a single frame. Caps accumulated time so a hitch cannot trigger a
    // spiral of death; excess accumulated time beyond this budget is dropped.
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Jolt Physics|Simulation",
              meta = (AllowPrivateAccess = true, ClampMin = 1, ClampMax = 16, UIMin = 1, UIMax = 16))
    int32 _MaxPhysicsStepsPerFrame = 4;

    // How a contact pair's two restitutions combine. Jolt's own default is Max, which diverges from
    // Chaos on any pair whose surfaces disagree — a restitution-1.0 bouncy material keeps 100% of its
    // normal velocity against a default-0.3 floor and never stops bouncing. Average is Chaos's
    // behaviour and the default here.
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Jolt Physics|Simulation",
              meta = (AllowPrivateAccess = true))
    ECk_Jolt_RestitutionCombineMode _RestitutionCombineMode = ECk_Jolt_RestitutionCombineMode::Average;

    // Enable multi-threaded physics simulation using Jolt's JobSystemThreadPool.
    // When disabled, all physics runs on a single thread (JobSystemSingleThreaded).
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Jolt Physics|Threading",
              meta = (AllowPrivateAccess = true))
    bool _EnableParallelPhysics = true;

    // Number of threads for parallel physics. 0 = automatic (hardware_concurrency - 1).
    // Only used when EnableParallelPhysics is true.
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Jolt Physics|Threading",
              meta = (AllowPrivateAccess = true, ClampMin = 0, UIMin = 0,
                      EditCondition = "_EnableParallelPhysics"))
    int32 _NumPhysicsThreads = 0;

    // Run PhysicsSystem::Update() on a background thread (one-frame latent results).
    // Frees the game thread from blocking during physics. Orthogonal to EnableParallelPhysics.
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Jolt Physics|Threading",
              meta = (AllowPrivateAccess = true))
    bool _EnableAsyncPhysicsUpdate = false;

    // Where the Jolt static world comes from in PIE (packaged builds always use cooked data).
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Jolt Physics|Static World",
              meta = (AllowPrivateAccess = true))
    ECk_Jolt_PIEStaticWorldMode _PIEStaticWorldMode = ECk_Jolt_PIEStaticWorldMode::LiveExtract;

    // Content root for cooked Jolt data assets. Must be listed in DirectoriesToAlwaysCook
    // (the cook commandlet ensures this loudly).
    UPROPERTY(Config, EditDefaultsOnly, Category = "Jolt Physics|Static World",
              meta = (AllowPrivateAccess = true, LongPackageName))
    FDirectoryPath _CookedDataRootPath;

    // Bake-grid cell size (uu) used to partition cooked bodies for streaming.
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Jolt Physics|Static World",
              meta = (AllowPrivateAccess = true, ClampMin = 1600, UIMin = 1600))
    float _BakeGridCellSize = 25600.0f;

    // Instanced-mesh components with at least this many instances bake as ONE StaticCompoundShape
    // body instead of per-instance bodies (dense kitbashed clusters would otherwise flood the
    // broadphase with thousands of entries).
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Jolt Physics|Static World",
              meta = (AllowPrivateAccess = true, ClampMin = 2, UIMin = 2))
    int32 _CompoundShapeInstanceThreshold = 32;

    // Body add/removes since the last broadphase optimize that trigger another optimize pass.
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Jolt Physics|Static World",
              meta = (AllowPrivateAccess = true, ClampMin = 1, UIMin = 1))
    int32 _BroadphaseOptimizeThreshold = 512;

    // Map path prefixes excluded from cooking (e.g. /Game/Developers) — honored by the commandlet's
    // Cook_AllMaps sweep AND by the editor's auto-cook-on-level-save.
    UPROPERTY(Config, EditDefaultsOnly, Category = "Jolt Physics|Static World",
              meta = (AllowPrivateAccess = true, LongPackageName))
    TArray<FDirectoryPath> _CookExcludedMapPathPrefixes;

    // Which component mobilities the level sweep bakes. A baked Movable is a snapshot at sweep time —
    // the static body does not follow later movement.
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Jolt Physics|Static World|Bake Filter",
              meta = (AllowPrivateAccess = true))
    ECk_Jolt_BakeMobilityPolicy _BakeMobilityPolicy = ECk_Jolt_BakeMobilityPolicy::All;

    // Actors of these classes (including subclasses) are excluded from the level sweep.
    // Pawns are excluded by default: their collision is dynamic-object territory, not static world.
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Jolt Physics|Static World|Bake Filter",
              meta = (AllowPrivateAccess = true))
    TArray<TSoftClassPtr<AActor>> _BakeExcludedActorClasses;

    // Actors carrying any of these tags are excluded from the level sweep.
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Jolt Physics|Static World|Bake Filter",
              meta = (AllowPrivateAccess = true))
    TArray<FName> _BakeExcludedActorTags;

    // Components carrying any of these tags are excluded from the level sweep.
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Jolt Physics|Static World|Bake Filter",
              meta = (AllowPrivateAccess = true))
    TArray<FName> _BakeExcludedComponentTags;

    // Components whose collision OBJECT TYPE is one of these channels are excluded from the level sweep.
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Jolt Physics|Static World|Bake Filter",
              meta = (AllowPrivateAccess = true))
    TArray<TEnumAsByte<ECollisionChannel>> _BakeExcludedObjectChannels;

    // Components using any of these named collision profiles are excluded from the level sweep.
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Jolt Physics|Static World|Bake Filter",
              meta = (AllowPrivateAccess = true))
    TArray<FName> _BakeExcludedCollisionProfiles;

    // Content roots whose static meshes get PRE-BAKED per-asset Jolt shapes (the CkJoltEditor mesh
    // cook sweeps these; runtime uses the cooked shape instead of building hulls/tri-meshes).
    // In cooked-data contexts, a hull/tri-mesh mesh under one of these roots with NO cooked shape
    // is a loud ensure. Empty = the per-mesh pre-bake is off.
    UPROPERTY(Config, EditDefaultsOnly, Category = "Jolt Physics|Static World",
              meta = (AllowPrivateAccess = true, LongPackageName))
    TArray<FDirectoryPath> _BakedMeshShapeRoots;

    // Exclude components that Block NOTHING (pure overlap volumes / triggers) from the level sweep.
    // A component that blocks ANY channel — including project custom channels, which often default to
    // Block even under the stock OverlapAll profile — is NOT overlap-only.
    // Disabled by default: an overlap-only body still answers overlap-semantics queries with UE parity.
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Jolt Physics|Static World|Bake Filter",
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _BakeExcludeOverlapOnlyComponents = ECk_EnableDisable::Disable;

public:
    explicit UCk_Jolt_ProjectSettings_UE(
        const FObjectInitializer& InObjectInitializer);

public:
    CK_PROPERTY_GET(_MaxBodies);
    CK_PROPERTY_GET(_MaxBodyPairs);
    CK_PROPERTY_GET(_MaxContactConstraints);
    CK_PROPERTY_GET(_TempAllocatorSizeMB);
    CK_PROPERTY_GET(_MaxPhysicsJobs);
    CK_PROPERTY_GET(_MaxPhysicsBarriers);
    CK_PROPERTY_GET(_CollisionSteps);
    CK_PROPERTY_GET(_FixedTimestepHz);
    CK_PROPERTY_GET(_MaxPhysicsStepsPerFrame);
    CK_PROPERTY_GET(_RestitutionCombineMode);
    CK_PROPERTY_GET(_EnableParallelPhysics);
    CK_PROPERTY_GET(_NumPhysicsThreads);
    CK_PROPERTY_GET(_EnableAsyncPhysicsUpdate);
    CK_PROPERTY_GET(_PIEStaticWorldMode);
    CK_PROPERTY_GET(_CookedDataRootPath);
    CK_PROPERTY_GET(_BakeGridCellSize);
    CK_PROPERTY_GET(_CompoundShapeInstanceThreshold);
    CK_PROPERTY_GET(_BroadphaseOptimizeThreshold);
    CK_PROPERTY_GET(_CookExcludedMapPathPrefixes);
    CK_PROPERTY_GET(_BakeMobilityPolicy);
    CK_PROPERTY_GET(_BakeExcludedActorClasses);
    CK_PROPERTY_GET(_BakeExcludedActorTags);
    CK_PROPERTY_GET(_BakeExcludedComponentTags);
    CK_PROPERTY_GET(_BakeExcludedObjectChannels);
    CK_PROPERTY_GET(_BakeExcludedCollisionProfiles);
    CK_PROPERTY_GET(_BakeExcludeOverlapOnlyComponents);
    CK_PROPERTY_GET(_BakedMeshShapeRoots);
};

// --------------------------------------------------------------------------------------------------------------------

class CKJOLT_API UCk_Utils_Jolt_ProjectSettings
{
public:
    static auto Get_MaxBodies() -> int32;
    static auto Get_MaxBodyPairs() -> int32;
    static auto Get_MaxContactConstraints() -> int32;
    static auto Get_TempAllocatorSizeMB() -> int32;
    static auto Get_MaxPhysicsJobs() -> int32;
    static auto Get_MaxPhysicsBarriers() -> int32;
    static auto Get_CollisionSteps() -> int32;
    static auto Get_FixedTimestepHz() -> int32;
    static auto Get_MaxPhysicsStepsPerFrame() -> int32;
    static auto Get_RestitutionCombineMode() -> ECk_Jolt_RestitutionCombineMode;
    static auto Get_EnableParallelPhysics() -> bool;
    static auto Get_NumPhysicsThreads() -> int32;
    static auto Get_EnableAsyncPhysicsUpdate() -> bool;
    static auto Get_PIEStaticWorldMode() -> ECk_Jolt_PIEStaticWorldMode;
    static auto Get_CookedDataRootPath() -> FString;
    static auto Get_BakeGridCellSize() -> float;
    static auto Get_CompoundShapeInstanceThreshold() -> int32;
    static auto Get_BroadphaseOptimizeThreshold() -> int32;
    static auto Get_CookExcludedMapPathPrefixes() -> TArray<FString>;
    static auto Get_BakeMobilityPolicy() -> ECk_Jolt_BakeMobilityPolicy;
    static auto Get_BakeExcludedActorClasses() -> TArray<TSoftClassPtr<AActor>>;
    static auto Get_BakeExcludedActorTags() -> TArray<FName>;
    static auto Get_BakeExcludedComponentTags() -> TArray<FName>;
    static auto Get_BakeExcludedObjectChannels() -> TArray<TEnumAsByte<ECollisionChannel>>;
    static auto Get_BakeExcludedCollisionProfiles() -> TArray<FName>;
    static auto Get_BakeExcludeOverlapOnlyComponents() -> ECk_EnableDisable;
    static auto Get_BakedMeshShapeRoots() -> TArray<FString>;
};

// --------------------------------------------------------------------------------------------------------------------
