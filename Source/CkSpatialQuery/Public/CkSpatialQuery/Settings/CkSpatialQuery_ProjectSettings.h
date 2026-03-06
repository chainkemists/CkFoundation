#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkSettings/Public/CkSettings/ProjectSettings/CkProjectSettings.h"

#include <CoreMinimal.h>

#include "CkSpatialQuery_ProjectSettings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "SpatialQuery"))
class CKSPATIALQUERY_API UCk_SpatialQuery_ProjectSettings_UE : public UCk_Plugin_ProjectSettings_UE
{
    GENERATED_BODY()

    CK_GENERATED_BODY(UCk_SpatialQuery_ProjectSettings_UE);

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

public:
    CK_PROPERTY_GET(_MaxBodies);
    CK_PROPERTY_GET(_MaxBodyPairs);
    CK_PROPERTY_GET(_MaxContactConstraints);
    CK_PROPERTY_GET(_TempAllocatorSizeMB);
    CK_PROPERTY_GET(_MaxPhysicsJobs);
    CK_PROPERTY_GET(_MaxPhysicsBarriers);
    CK_PROPERTY_GET(_CollisionSteps);
    CK_PROPERTY_GET(_EnableParallelPhysics);
    CK_PROPERTY_GET(_NumPhysicsThreads);
    CK_PROPERTY_GET(_EnableAsyncPhysicsUpdate);
};

// --------------------------------------------------------------------------------------------------------------------

class CKSPATIALQUERY_API UCk_Utils_SpatialQuery_ProjectSettings
{
public:
    static auto Get_MaxBodies() -> int32;
    static auto Get_MaxBodyPairs() -> int32;
    static auto Get_MaxContactConstraints() -> int32;
    static auto Get_TempAllocatorSizeMB() -> int32;
    static auto Get_MaxPhysicsJobs() -> int32;
    static auto Get_MaxPhysicsBarriers() -> int32;
    static auto Get_CollisionSteps() -> int32;
    static auto Get_EnableParallelPhysics() -> bool;
    static auto Get_NumPhysicsThreads() -> int32;
    static auto Get_EnableAsyncPhysicsUpdate() -> bool;
};

// --------------------------------------------------------------------------------------------------------------------
