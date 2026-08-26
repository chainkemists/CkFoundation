#include "CkInsightsAnalyzer/Core/CkTimerCategorizer.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

// --------------------------------------------------------------------------------------------------------------------

FCk_TimerCategorizer::FCk_TimerCategorizer()
{
    InitializeCategories();
    InitializeNameReplacements();
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck_timer_categorizer
{
    // Marks a keyword as whole-name rather than substring. See Categorize.
    const FString ExactMatchPrefix = TEXT("=");
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_TimerCategorizer::
    Categorize(const FString& TimerName) const
    -> FString
{
    const auto LowerName = TimerName.ToLower();

    const auto Matches = [&LowerName](const FString& InKeyword) -> bool
    {
        // '=' prefix means whole-name; the default is substring. The Physics (Jolt) category
        // documents why the distinction exists.
        if (InKeyword.StartsWith(ck_timer_categorizer::ExactMatchPrefix))
        { return LowerName == InKeyword.RightChop(ck_timer_categorizer::ExactMatchPrefix.Len()).ToLower(); }

        return LowerName.Contains(InKeyword.ToLower());
    };

    // Not ck::algo::FindIf: its TArray overload returns TOptional<ElementType> — a copy of the
    // category and its keyword array per call. AnyOf on the inner list has no such cost.
    for (const auto& Category : _Categories)
    {
        if (ck::algo::AnyOf(Category.Keywords, Matches))
        { return Category.Name; }
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

auto
    FCk_TimerCategorizer::
    SimplifyName(const FString& TimerName)
    -> FString
{
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

    if (const FString* Replacement = Replacements.Find(TimerName))
    {
        return *Replacement;
    }

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
    // Geometric shapes (U+25A0..U+25FF) exist in Roboto, Slate's default font; emoji circles do not.
    constexpr auto BlackSquare_Critical = TEXT("\u25A0");
    constexpr auto BlackDiamond_Warning = TEXT("\u25C6");
    constexpr auto BlackCircle_Moderate = TEXT("\u25CF");
    constexpr auto WhiteCircle_Normal   = TEXT("\u25CB");

    if (Ms >= 5.0)
    {
        return FString(BlackSquare_Critical);
    }
    else if (Ms >= 2.0)
    {
        return FString(BlackDiamond_Warning);
    }
    else if (Ms >= 1.0)
    {
        return FString(BlackCircle_Moderate);
    }
    else
    {
        return FString(WhiteCircle_Normal);
    }
}

auto
    FCk_TimerCategorizer::
    FormatCount(uint32 Count)
    -> FString
{
    if (Count >= 1000)
    {
        return FString::Printf(TEXT("%u,%03ux"), Count / 1000, Count % 1000);
    }
    return FString::Printf(TEXT("%ux"), Count);
}

auto
    FCk_TimerCategorizer::
    IsWaitTimer(const FString& TimerName)
    -> bool
{
    // "WaitForTask" (singular) also covers WaitForTasks and GameThreadWaitForTask.
    static const TArray<FString> WaitKeywords = {
        TEXT("WaitForTask"),
        TEXT("FTaskBase::Wait"),
        TEXT("Game thread idle time"),
        TEXT("FAsyncTask::SyncCompletion"),
        TEXT("TaskWorkerIsLookingForWork"),
        TEXT("WaitUntilTasksComplete"),
        TEXT("ParallelFor.Wait"),
    };

    for (const FString& Keyword : WaitKeywords)
    {
        if (TimerName.Contains(Keyword, ESearchCase::IgnoreCase))
        {
            return true;
        }
    }
    return false;
}

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

    // The second row is evidence-driven: each entry was observed landing in "Other" in a real
    // capture because the scope name carries no other Slate token. Keep additions here NARROW —
    // this category is matched third, so a broad keyword silently steals rows from ECS and the
    // game category below.
    AddCategory(TEXT("Slate/UI"), {
        TEXT("Slate"), TEXT("Widget"), TEXT("WBP_"), TEXT("SlateFontCache"), TEXT("Freetype"),
        TEXT("Load Font"), TEXT("Shape Bidirectional"), TEXT("SlatePrepass"), TEXT("SlateRender"),
        TEXT("_Paint"), TEXT("ScreenWidget"), TEXT("SWidget"),

        TEXT("Paint: "), TEXT("VisibilityAttributes"), TEXT("Text Layout"),
        TEXT("GatherWindowElements"), TEXT("Tooltip"), TEXT("DrawStringInternal"),
        TEXT("QueryCursor"), TEXT("ProcessMouse"), TEXT("ProcessKey"), TEXT("HitTestGrid"),
        TEXT("SConstraintCanvas"),
    });

    // "SceneQuery" (covering SceneQueryTotal and the engine's UnknownSceneQuery) is safe to widen:
    // no Ck module declares a scope containing it, so nothing is pulled out of ECS below.
    AddCategory(TEXT("Scene Queries"), {
        TEXT("SceneQuery"), TEXT("EnvQueryOverlap"),
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

    // "=JoltWorld_Step" is whole-name on purpose. This category outranks ECS below, and every
    // substring form of that stat name is ALSO a substring of ck::FProcessor_JoltWorld_Step — so a
    // plain keyword would pull that processor's row out of ECS (CK) and silently change attribution
    // existing captures were measured against.
    AddCategory(TEXT("Physics (Jolt)"), {
        TEXT("JoltPhysics"), TEXT("=JoltWorld_Step"), TEXT("JoltContacts"),
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
        TEXT("AddPrimitive"), TEXT("GetStreamingRenderAssetInfo"),
    });

    AddCategory(TEXT("AI/BehaviorTree"), {
        TEXT("BTDecorator"), TEXT("BTTask"), TEXT("BTService"), TEXT("BehaviorTree"),
        TEXT("BrainComponent"), TEXT("AIController"), TEXT("EQS_Query"),
    });

    AddCategory(TEXT("Navigation/NavMesh"), {
        TEXT("Recast"), TEXT("Nav Tick"), TEXT("Navigation_"), TEXT("NavMesh"), TEXT("ZoneGraph"),
    });

    // The second row covers Ck scopes declared via DECLARE_CYCLE_STAT with a bare subsystem
    // prefix rather than a ck:: / Ck_ token — they carry no other category keyword and were
    // landing in "Other". Known residual: "Scheduler::MainPass" still matches Rendering's
    // "MainPass" keyword (matched at a higher priority). It is ~0.04 ms exclusive, and narrowing
    // Rendering would mis-bin Nanite's own bare "MainPass" RDG scope, so it is left alone.
    AddCategory(TEXT("ECS (CK)"), {
        TEXT("ck::"), TEXT("Ck_"), TEXT("CkFoundation"),
        TEXT("AC_Fragment"), TEXT("ObjectReplicator"), TEXT("EcsWorld"),
        TEXT("script::"),   // scheduler-emitted script-processor scopes (see CkProcessorScheduler.cpp)

        // The AngelScript-side dispatch event every script processor runs through — the aggregate
        // sibling of the per-processor script:: rows above. Exact-match: it is a bare UFUNCTION
        // name, and a substring form could claim unrelated scopes that merely contain it.
        TEXT("=ForEachBatch"),

        TEXT("Scheduler::"), TEXT("SmTask::"), TEXT("Sm::"), TEXT("EntityTag::"),
        TEXT("Record::ForEach"), TEXT("Ism::"), TEXT("DestroyEntities"),
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

        TEXT("BehaviorLeaf"), TEXT("Locomotion"), TEXT("Minimap"), TEXT("HeadCutaway"),
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
