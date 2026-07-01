#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkIskm_BatchedUtils.generated.h"

class UCk_IskmAnimCollection_Data;
class UCk_Iskm_BatchedClusterComponent;

// ====================================================================================================================
//  CkIskmRenderer Plan-2 — debug/gym driver for the batched skeletal renderer.
//  The batched path's production entry point is the same UCk_Utils_IskmProxy_UE::Add + PoseSource routing as Plan-1
//  (wired in a later phase). This utility is a direct spawn helper for gyms/tests to stand up a cluster now.
// ====================================================================================================================
UCLASS(NotBlueprintable)
class CKISKMRENDERER_API UCk_Utils_IskmBatched_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_IskmBatched_UE);

public:
    // Spawns an actor hosting a batched cluster component with an InGridSize x InGridSize grid of instances, each
    // playing InSequenceIndex with a per-instance phase offset (independent looping animation). InRate 0 = hold the
    // sequence's first frame. Returns the component (or null on failure). CPU-safe under -nullrhi (component spawns;
    // the GPU proxy + animation only materialize when the app can render).
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmBatched",
        meta = (WorldContext = "InWorldContextObject"),
        DisplayName = "[Ck][IskmBatched] Debug Spawn Cluster")
    static UCk_Iskm_BatchedClusterComponent*
    Debug_SpawnCluster(
        UObject* InWorldContextObject,
        UCk_IskmAnimCollection_Data* InCollection,
        const FTransform& InBaseTransform,
        int32 InGridSize = 1,
        float InSpacing = 150.0f,
        int32 InSequenceIndex = 0,
        float InRate = 1.0f);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmBatched",
        DisplayName = "[Ck][IskmBatched] Get Cluster Instance Count")
    static int32
    Get_InstanceCount(const UCk_Iskm_BatchedClusterComponent* InCluster);
};
