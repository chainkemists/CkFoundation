#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "CkIskmRenderer/AnimCollection/CkIskmAnimCollection_Fragment_Data.h"

#include "CkIskmAnimCollection_Utils.generated.h"

UCLASS(NotBlueprintable)
class CKISKMRENDERER_API UCk_Utils_IskmAnimCollection_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_IskmAnimCollection_UE);

public:
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmAnimCollection",
        DisplayName="[Ck][IskmAnimCollection] Find Sequence Index By Asset")
    static int32
    Find_SequenceIndex_ByAsset(
        const UCk_IskmAnimCollection_Data* InAsset,
        const UAnimSequenceBase* InSequence);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmAnimCollection",
        DisplayName="[Ck][IskmAnimCollection] Get Skeleton")
    static USkeleton*
    Get_Skeleton(const UCk_IskmAnimCollection_Data* InAsset);

    // ---- Plan-2 CPU bake (the headless-verifiable surface) ----

    // Builds (or rebuilds) the CPU bone-matrix bake. Returns true on success. CPU-only / headless-safe.
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|IskmAnimCollection",
        DisplayName="[Ck][IskmAnimCollection] Build Baked Pose Data")
    static bool
    Build_BakedPoseData(UCk_IskmAnimCollection_Data* InAsset);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmAnimCollection",
        DisplayName="[Ck][IskmAnimCollection] Get Is Baked")
    static bool
    Get_IsBaked(const UCk_IskmAnimCollection_Data* InAsset);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmAnimCollection",
        DisplayName="[Ck][IskmAnimCollection] Get Render Bone Count")
    static int32
    Get_RenderBoneCount(const UCk_IskmAnimCollection_Data* InAsset);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmAnimCollection",
        DisplayName="[Ck][IskmAnimCollection] Get Total Frame Count")
    static int32
    Get_TotalFrameCount(const UCk_IskmAnimCollection_Data* InAsset);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmAnimCollection",
        DisplayName="[Ck][IskmAnimCollection] Get Frame Count Sequences")
    static int32
    Get_FrameCountSequences(const UCk_IskmAnimCollection_Data* InAsset);

    // Number of baked 3x4 matrices = RenderBoneCount * TotalFrameCount.
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmAnimCollection",
        DisplayName="[Ck][IskmAnimCollection] Get Baked Matrix Count")
    static int32
    Get_BakedMatrixCount(const UCk_IskmAnimCollection_Data* InAsset);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmAnimCollection",
        DisplayName="[Ck][IskmAnimCollection] Get Baked Sequence Count")
    static int32
    Get_BakedSequenceCount(const UCk_IskmAnimCollection_Data* InAsset);

    // Base frame offset into the flat matrix buffer for the given sequence (-1 if invalid).
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmAnimCollection",
        DisplayName="[Ck][IskmAnimCollection] Get Sequence Frame Index")
    static int32
    Get_SequenceFrameIndex(const UCk_IskmAnimCollection_Data* InAsset, int32 InSequenceIndex);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmAnimCollection",
        DisplayName="[Ck][IskmAnimCollection] Get Sequence Frame Count")
    static int32
    Get_SequenceFrameCount(const UCk_IskmAnimCollection_Data* InAsset, int32 InSequenceIndex);

    // GlobalFrame(seq, local) = SequenceFrameIndex(seq) + clamp(local, 0, FrameCount-1).
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmAnimCollection",
        DisplayName="[Ck][IskmAnimCollection] Get Global Frame")
    static int32
    Get_GlobalFrame(const UCk_IskmAnimCollection_Data* InAsset, int32 InSequenceIndex, int32 InLocalFrame);

    // True if all of frame 0's baked matrices are within InTolerance of identity (the reference pose).
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|IskmAnimCollection",
        DisplayName="[Ck][IskmAnimCollection] Get Is Ref Pose Frame Identity")
    static bool
    Get_IsRefPoseFrameIdentity(const UCk_IskmAnimCollection_Data* InAsset, float InTolerance = 0.01f);
};
