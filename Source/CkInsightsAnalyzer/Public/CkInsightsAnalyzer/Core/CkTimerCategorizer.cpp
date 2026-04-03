#include "CkInsightsAnalyzer/Core/CkTimerCategorizer.h"

// --------------------------------------------------------------------------------------------------------------------

FCk_TimerCategorizer::FCk_TimerCategorizer()
{
    InitializeCategories();
    InitializeNameReplacements();
}

// --------------------------------------------------------------------------------------------------------------------
// Category Matching
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_TimerCategorizer::
    Categorize(const FString& TimerName) const
    -> FString
{
    const FString LowerName = TimerName.ToLower();

    for (const FCk_TimerCategory& Category : _Categories)
    {
        for (const FString& Keyword : Category.Keywords)
        {
            if (LowerName.Contains(Keyword.ToLower()))
            {
                return Category.Name;
            }
        }
    }

    return TEXT("Other");
}

auto
    FCk_TimerCategorizer::
    GetCategories() const
    -> const TArray<FCk_TimerCategory>&
{
    return _Categories;
}

auto
    FCk_TimerCategorizer::
    GetCategoryPriority(const FString& CategoryName) const
    -> int32
{
    for (const FCk_TimerCategory& Cat : _Categories)
    {
        if (Cat.Name == CategoryName)
        {
            return Cat.Priority;
        }
    }
    return MAX_int32;
}

// --------------------------------------------------------------------------------------------------------------------
// Name Simplification
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_TimerCategorizer::
    SimplifyName(const FString& TimerName)
    -> FString
{
    // Exact replacements (static map, initialized once)
    static const TMap<FString, FString> Replacements = []()
    {
        TMap<FString, FString> Map;
        Map.Add(TEXT("FActorComponentTickFunction::ExecuteTick"),            TEXT("Component Ticks"));
        Map.Add(TEXT("STAT_SlateFontCacheAddNewShapedEntry"),               TEXT("FontCache AddEntry"));
        Map.Add(TEXT("STAT_HandleCollisionEvents"),                         TEXT("HandleCollisionEvents"));
        Map.Add(TEXT("STAT_DispatchPhysicsCollisionHit"),                   TEXT("DispatchCollisionHit"));
        Map.Add(TEXT("STAT_FCurlMultiPollIOManager_Poll_MultiPoll"),        TEXT("Curl MultiPoll"));
        Map.Add(TEXT("FEndPhysicsTickFunction_ExecuteTick"),                TEXT("EndPhysicsTick"));
        Map.Add(TEXT("FStartPhysicsTickFunction_ExecuteTick"),              TEXT("StartPhysicsTick"));
        Map.Add(TEXT("CreateExternalAccelerationStructure"),                TEXT("CreateAccelStructure"));
        Map.Add(TEXT("UCharacterMovementComponent_TickComponent"),          TEXT("CharMovement Tick"));
        Map.Add(TEXT("ReplicationSystem_PreSendUpdate"),                    TEXT("Replication PreSend"));
        Map.Add(TEXT("FTaskBase::WaitImpl_StateChangeEvent_WaitFor"),       TEXT("Task WaitFor"));
        Map.Add(TEXT("USkeletalMeshComponent_CompleteParallelAnimationEvaluation"), TEXT("CompleteParallelAnim"));
        Map.Add(TEXT("STAT_RecastNavMeshGenerator_TileGeneratorRemoval"),   TEXT("Recast TileRemoval"));
        Map.Add(TEXT("STAT_Navigation_BuildHeightfieldLayers"),             TEXT("Nav BuildHeightfield"));
        Map.Add(TEXT("STAT_Navigation_RasterizeGeomRecastRasterizeTriangles"), TEXT("Nav Rasterize"));
        Map.Add(TEXT("UCk_Game_TickableWorldSubsystem_Base_UE"),            TEXT("CK TickableWorldSubsystem"));
        Map.Add(TEXT("FlushRenderingCommands"),                             TEXT("FlushRenderCommands"));
        Map.Add(TEXT("FSkeletalMeshComponentEndPhysicsTickFunction_ExecuteTick"), TEXT("SkelMesh EndPhysTick"));
        Map.Add(TEXT("TickableGameObjects Time"),                           TEXT("TickableGameObjects"));
        Map.Add(TEXT("MoveComponent(Primitive) Time"),                      TEXT("MoveComponent"));
        Map.Add(TEXT("Slate::AddShapedTextElement"),                        TEXT("ShapedTextElement"));
        return Map;
    }();

    // Check exact match
    if (const FString* Replacement = Replacements.Find(TimerName))
    {
        return *Replacement;
    }

    // Strip known prefixes
    static const TArray<FString> Prefixes = { TEXT("STAT_") };
    FString Result = TimerName;
    for (const FString& Prefix : Prefixes)
    {
        if (Result.StartsWith(Prefix))
        {
            Result.RightChopInline(Prefix.Len());
            break;
        }
    }

    return Result;
}

// --------------------------------------------------------------------------------------------------------------------
// Formatting Utilities
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_TimerCategorizer::
    FormatMs(double Ms)
    -> FString
{
    if (Ms >= 10.0)
    {
        return FString::Printf(TEXT("%.1fms"), Ms);
    }
    else if (Ms >= 1.0)
    {
        return FString::Printf(TEXT("%.2fms"), Ms);
    }
    else
    {
        return FString::Printf(TEXT("%.3fms"), Ms);
    }
}

auto
    FCk_TimerCategorizer::
    SeverityIcon(double Ms)
    -> FString
{
    // Geometric shapes from Unicode block U+25A0–U+25FF.
    // These are present in Roboto (Slate's default font), unlike emoji circles.
    if (Ms >= 5.0)
    {
        // Black square U+25A0 — critical
        return FString(TEXT("\u25A0"));
    }
    else if (Ms >= 2.0)
    {
        // Black diamond U+25C6 — warning
        return FString(TEXT("\u25C6"));
    }
    else if (Ms >= 1.0)
    {
        // Black circle U+25CF — moderate
        return FString(TEXT("\u25CF"));
    }
    else
    {
        // White circle U+25CB — normal
        return FString(TEXT("\u25CB"));
    }
}

auto
    FCk_TimerCategorizer::
    FormatCount(uint32 Count)
    -> FString
{
    if (Count >= 1000)
    {
        // Insert comma separators
        return FString::Printf(TEXT("%u,%03ux"), Count / 1000, Count % 1000);
    }
    return FString::Printf(TEXT("%ux"), Count);
}

// --------------------------------------------------------------------------------------------------------------------
// Category Initialization (ported from Python CATEGORY_KEYWORDS)
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_TimerCategorizer::
    InitializeCategories()
    -> void
{
    int32 Priority = 0;

    auto AddCategory = [this, &Priority](const FString& Name, TArray<FString> Keywords)
    {
        FCk_TimerCategory Cat;
        Cat.Name = Name;
        Cat.Keywords = MoveTemp(Keywords);
        Cat.Priority = Priority++;
        _Categories.Add(MoveTemp(Cat));
    };

    // Order matches Python CATEGORY_ORDER for display priority
    AddCategory(TEXT("Waiting/Blocking"), {
        TEXT("GameThreadWaitForTask"), TEXT("Game thread idle time"),
        TEXT("FAsyncTask::SyncCompletion"), TEXT("TaskWorkerIsLookingForWork"),
    });

    AddCategory(TEXT("Tick Overhead"), {
        TEXT("TickCompletionEvents"), TEXT("ReleaseTickGroup"),
        TEXT("WaitUntilTasksComplete"), TEXT("Tick Time"),
        TEXT("TG_PrePhysics"), TEXT("TG_DuringPhysics"), TEXT("TG_EndPhysics"),
        TEXT("TG_PostPhysics"), TEXT("TG_PostUpdateWork"), TEXT("TG_NewlySpawned"),
        TEXT("TG_LastDemotable"), TEXT("Start TG_"),
        TEXT("FActorComponentTickFunction::ExecuteTick"),
        TEXT("Component Tick"), TEXT("QuantizeIfDirty"),
        TEXT("Queue Ticks"), TEXT("Flip Results"),
    });

    AddCategory(TEXT("Slate/UI"), {
        TEXT("Slate"), TEXT("Widget"), TEXT("WBP_"), TEXT("SlateFontCache"), TEXT("Freetype"),
        TEXT("Load Font"), TEXT("Shape Bidirectional"), TEXT("SlatePrepass"), TEXT("SlateRender"),
        TEXT("_Paint"), TEXT("ScreenWidget"), TEXT("SWidget"),
    });

    AddCategory(TEXT("Scene Queries"), {
        TEXT("SceneQueryTotal"), TEXT("EnvQueryOverlap"),
    });

    AddCategory(TEXT("Physics (UE Overlaps)"), {
        TEXT("UpdateOverlaps"), TEXT("OverlapMulti"), TEXT("OverlapQuery"),
        TEXT("GeomOverlap"), TEXT("EndScopedMovement"),
    });

    AddCategory(TEXT("Physics (UE Collisions)"), {
        TEXT("DispatchPhysicsCollision"), TEXT("HandleCollisionEvents"),
        TEXT("Collisions::Generate"), TEXT("Collisions::Assign"),
        TEXT("FPhysScene_Chaos"), TEXT("SingleParticlePhysicsProxy"),
        TEXT("Phys SetBodyTransform"), TEXT("BufferPhysicsResults"),
        TEXT("PhysicsParallelFor"), TEXT("CreateExternalAcceleration"),
        TEXT("SwapAccelerationStructures"), TEXT("PreUpdatePass"),
        TEXT("Pull Constraints"), TEXT("Process Single Particle"),
        TEXT("JointConstraintPhysicsProxy"),
    });

    AddCategory(TEXT("Physics (Jolt)"), {
        TEXT("JoltPhysics"),
    });

    AddCategory(TEXT("Character Movement"), {
        TEXT("CharacterMovement"), TEXT("CharMoveComp"), TEXT("Char Tick"),
        TEXT("Char Movement"), TEXT("Char NonSimulated"), TEXT("Char Perform"),
        TEXT("MoveComponent"), TEXT("ComputeFloorDist"),
        TEXT("UpdateComponentToWorld"), TEXT("UpdateChildTransforms"),
    });

    AddCategory(TEXT("Rendering"), {
        TEXT("FRDGPass"), TEXT("BasePass"), TEXT("Nanite"), TEXT("MainPass"), TEXT("PostPass"),
        TEXT("Shadow"), TEXT("[Scene]"), TEXT("FDeferredShading"), TEXT("RHI_"),
        TEXT("GetPrimitiveUniformShader"), TEXT("FGatherShadow"),
        TEXT("BeginRenderingViewFamily"), TEXT("DeferredRenderUpdates"),
        TEXT("Transform or RenderData"), TEXT("StaticMeshComponent"),
        TEXT("D3D12"), TEXT("LockBuffer"), TEXT("UnlockBuffer"),
    });

    AddCategory(TEXT("AI/BehaviorTree"), {
        TEXT("BTDecorator"), TEXT("BTTask"), TEXT("BTService"), TEXT("BehaviorTree"),
        TEXT("BrainComponent"), TEXT("AIController"), TEXT("EQS_Query"),
    });

    AddCategory(TEXT("Navigation/NavMesh"), {
        TEXT("Recast"), TEXT("Nav Tick"), TEXT("Navigation_"), TEXT("NavMesh"), TEXT("ZoneGraph"),
    });

    AddCategory(TEXT("ECS (CK)"), {
        TEXT("ck::"), TEXT("Ck_"), TEXT("CkFoundation"),
        TEXT("AC_Fragment"), TEXT("ObjectReplicator"), TEXT("EcsWorld"),
    });

    AddCategory(TEXT("Networking"), {
        TEXT("NetDriver"), TEXT("NetBroadcast"), TEXT("Replication"), TEXT("GameNetDriver"),
        TEXT("Rep_"), TEXT("Net Broadcast"), TEXT("PollPushBased"), TEXT("PollAndCopy"),
        TEXT("ReplicationBridge"),
    });

    AddCategory(TEXT("Animation"), {
        TEXT("AnimGameThread"), TEXT("Skeleton"), TEXT("KinematicBones"), TEXT("Montage"),
        TEXT("UpdateBones"), TEXT("Post Anim"), TEXT("CompleteParallelAnim"),
        TEXT("BlueprintUpdateAnimation"), TEXT("SkeletalMeshComponent"),
        TEXT("RefreshBoneTransforms"),
    });

    AddCategory(TEXT("Blueprint"), {
        TEXT("Blueprint Time"), TEXT("ExecuteUbergraph"), TEXT("BndEvt__"),
    });

    AddCategory(TEXT("BusterBlock Game"), {
        TEXT("BB_"), TEXT("VHS_"), TEXT("ItemActor"), TEXT("Store"), TEXT("Customer"),
        TEXT("Employee"), TEXT("ThrowBox"), TEXT("Checkout"), TEXT("BusterBlock"),
        TEXT("NPC"), TEXT("_BB_BP"), TEXT("Bb_"),
    });
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_TimerCategorizer::
    InitializeNameReplacements()
    -> void
{
    _PrefixesToStrip = { TEXT("STAT_") };
}

// --------------------------------------------------------------------------------------------------------------------
