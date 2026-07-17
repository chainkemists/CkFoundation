#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkSettings/Public/CkSettings/ProjectSettings/CkProjectSettings.h"

#include <CoreMinimal.h>

#include "CkJolt_ProjectSettings.generated.h"

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
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Jolt Physics|Static World",
              meta = (AllowPrivateAccess = true))
    FString _CookedDataRootPath = TEXT("/Game/CkJoltData");

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

    // Map path prefixes excluded from Cook_AllMaps (e.g. /Game/Developers).
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Jolt Physics|Static World",
              meta = (AllowPrivateAccess = true))
    TArray<FString> _CookExcludedMapPathPrefixes;

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
    CK_PROPERTY_GET(_EnableParallelPhysics);
    CK_PROPERTY_GET(_NumPhysicsThreads);
    CK_PROPERTY_GET(_EnableAsyncPhysicsUpdate);
    CK_PROPERTY_GET(_PIEStaticWorldMode);
    CK_PROPERTY_GET(_CookedDataRootPath);
    CK_PROPERTY_GET(_BakeGridCellSize);
    CK_PROPERTY_GET(_CompoundShapeInstanceThreshold);
    CK_PROPERTY_GET(_BroadphaseOptimizeThreshold);
    CK_PROPERTY_GET(_CookExcludedMapPathPrefixes);
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
    static auto Get_EnableParallelPhysics() -> bool;
    static auto Get_NumPhysicsThreads() -> int32;
    static auto Get_EnableAsyncPhysicsUpdate() -> bool;
    static auto Get_PIEStaticWorldMode() -> ECk_Jolt_PIEStaticWorldMode;
    static auto Get_CookedDataRootPath() -> FString;
    static auto Get_BakeGridCellSize() -> float;
    static auto Get_CompoundShapeInstanceThreshold() -> int32;
    static auto Get_BroadphaseOptimizeThreshold() -> int32;
    static auto Get_CookExcludedMapPathPrefixes() -> TArray<FString>;
};

// --------------------------------------------------------------------------------------------------------------------
