#include "CkAnimationToolbox_Detector.h"

#include "CkAnimationEditor_Log.h"

#include <AssetRegistry/AssetRegistryModule.h>
#include <AssetRegistry/IAssetRegistry.h>
#include <Animation/AnimSequence.h>
#include <Animation/AnimBlueprint.h>
#include <Animation/AnimData/IAnimationDataModel.h>
#include <Animation/AnimData/IAnimationDataController.h>
#include <Animation/AnimCurveTypes.h>
#include <Animation/AnimationPoseData.h>
#include <Animation/AttributesContainer.h>
#include <Animation/Skeleton.h>
#include <BoneContainer.h>
#include <BonePose.h>
#include <ISourceControlModule.h>
#include <SourceControlHelpers.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::animation_editor::detail
{
    static FTopLevelAssetPath AnimSequenceClassPath()
    {
        return FTopLevelAssetPath(TEXT("/Script/Engine"), TEXT("AnimSequence"));
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAnimScrunchDetector::
    DoCollectCandidates(const FCk_AnimationToolbox_ScanOptions& Options) const
    -> TArray<FAssetData>
{
    auto& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
    TArray<FAssetData> Candidates;

    using namespace ck::animation_editor::detail;

    switch (Options.Scope)
    {
        case ECk_AnimationToolbox_ScanScope::ProjectWide:
        {
            FARFilter Filter;
            Filter.ClassPaths.Add(AnimSequenceClassPath());
            Filter.bRecursiveClasses = true;
            Filter.PackagePaths.Add(TEXT("/Game"));
            Filter.bRecursivePaths = true;
            AssetRegistry.GetAssets(Filter, Candidates);
            break;
        }
        case ECk_AnimationToolbox_ScanScope::Path:
        {
            FARFilter Filter;
            Filter.ClassPaths.Add(AnimSequenceClassPath());
            Filter.bRecursiveClasses = true;
            Filter.PackagePaths.Add(FName(*Options.ContentPath));
            Filter.bRecursivePaths = true;
            AssetRegistry.GetAssets(Filter, Candidates);
            break;
        }
        case ECk_AnimationToolbox_ScanScope::AnimBlueprintDeps:
        {
            if (Options.AnimBlueprint.IsNull())
            { break; }

            const FName RootPackage = FName(*Options.AnimBlueprint.GetLongPackageName());
            TSet<FName> Visited;
            TArray<FName> Stack = { RootPackage };

            while (Stack.Num() > 0)
            {
                const FName Pkg = Stack.Pop();
                if (Visited.Contains(Pkg)) { continue; }
                Visited.Add(Pkg);

                TArray<FName> Deps;
                AssetRegistry.GetDependencies(Pkg, Deps, UE::AssetRegistry::EDependencyCategory::Package);

                for (const FName& Dep : Deps)
                {
                    if (Visited.Contains(Dep)) { continue; }

                    TArray<FAssetData> Assets;
                    AssetRegistry.GetAssetsByPackageName(Dep, Assets);
                    for (const FAssetData& AD : Assets)
                    {
                        const FName ClassName = AD.AssetClassPath.GetAssetName();
                        if (ClassName == TEXT("AnimSequence"))
                        {
                            Candidates.Add(AD);
                        }
                        else if (ClassName == TEXT("AnimBlueprint") ||
                                 ClassName == TEXT("BlendSpace")   ||
                                 ClassName == TEXT("BlendSpace1D") ||
                                 ClassName == TEXT("AimOffsetBlendSpace") ||
                                 ClassName == TEXT("AimOffsetBlendSpace1D") ||
                                 ClassName == TEXT("AnimMontage")  ||
                                 ClassName == TEXT("AnimComposite"))
                        {
                            Stack.Add(Dep);
                        }
                    }
                }
            }
            break;
        }
    }

    // Deduplicate by package name.
    TSet<FName> Seen;
    Candidates.RemoveAll([&Seen](const FAssetData& AD)
    {
        bool IsAlreadySeen = false;
        Seen.Add(AD.PackageName, &IsAlreadySeen);
        return IsAlreadySeen;
    });

    Candidates.Sort([](const FAssetData& A, const FAssetData& B) { return A.PackageName.LexicalLess(B.PackageName); });
    return Candidates;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAnimScrunchDetector::
    DoBuildComponentSpacePose(UAnimSequence* Anim, int32 FrameIndex, const TSet<FName>& /*TrackNameSet (unused)*/, TArray<FTransform>& OutTransforms) const
    -> void
{
    OutTransforms.Reset();
    if (!IsValid(Anim)) { return; }

    USkeleton* Skeleton = Anim->GetSkeleton();
    if (!IsValid(Skeleton)) { return; }

    const IAnimationDataModel* Model = Anim->GetDataModel();
    if (Model == nullptr) { return; }

    const FReferenceSkeleton& RefSkel = Skeleton->GetReferenceSkeleton();
    const int32 NumBones = RefSkel.GetNum();
    if (NumBones == 0) { return; }

    // FCompactPose / FBlendedCurve / FStackAttributeContainer use TMemStackAllocator — requires
    // an FMemMark on the stack. Without it, SetBoneContainer crashes on the first allocation.
    FMemMark MemMark(FMemStack::Get());

    // Required-bones list = every skeleton bone in order.
    TArray<FBoneIndexType> RequiredBones;
    RequiredBones.Reserve(NumBones);
    for (int32 i = 0; i < NumBones; ++i)
    { RequiredBones.Add(static_cast<FBoneIndexType>(i)); }

    FBoneContainer BoneContainer(RequiredBones, UE::Anim::FCurveFilterSettings(), *Skeleton);

    FCompactPose CompactPose;
    CompactPose.SetBoneContainer(&BoneContainer);
    CompactPose.ResetToRefPose();

    FBlendedCurve Curve;
    Curve.InitFrom(BoneContainer);

    UE::Anim::FStackAttributeContainer Attributes;
    FAnimationPoseData PoseData(CompactPose, Curve, Attributes);

    // Seconds for this frame from the asset's frame rate.
    const FFrameRate FrameRate = Model->GetFrameRate();
    const double TimeSeconds = FrameRate.AsSeconds(FFrameTime(FFrameNumber(FrameIndex)));

    FAnimExtractContext Ctx;
    Ctx.CurrentTime = TimeSeconds;
    Ctx.bLooping    = false;

    // bForceUseRawData = false — go through the compressed path (same as cook/runtime).
    // The scrunch is a compressed-data artifact; raw DataModel can't see it.
    Anim->GetBonePose(PoseData, Ctx, /*bForceUseRawData=*/false);

    // Read local transforms from the pose and accumulate component-space.
    OutTransforms.SetNum(NumBones);
    const TArray<FMeshBoneInfo>& BoneInfo = RefSkel.GetRefBoneInfo();
    for (int32 b = 0; b < NumBones; ++b)
    {
        const FTransform LocalT = CompactPose[FCompactPoseBoneIndex(b)];
        const int32 ParentIdx = BoneInfo[b].ParentIndex;
        OutTransforms[b] = (ParentIdx == INDEX_NONE)
            ? LocalT
            : LocalT * OutTransforms[ParentIdx];
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAnimScrunchDetector::
    DoComputePoseDelta(const TArray<FTransform>& A, const TArray<FTransform>& B) const
    -> float
{
    if (A.Num() == 0 || A.Num() != B.Num()) { return 0.0f; }

    float Total = 0.0f;
    for (int32 i = 0; i < A.Num(); ++i)
    {
        const float TransDelta = static_cast<float>((A[i].GetLocation() - B[i].GetLocation()).Size());
        const float AngleRad   = static_cast<float>(A[i].GetRotation().AngularDistance(B[i].GetRotation()));
        const float AngleDeg   = FMath::RadiansToDegrees(AngleRad);
        Total += TransDelta + AngleDeg;
    }
    return Total;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAnimScrunchDetector::
    DoGatherUnmappedBones(UAnimSequence* Anim, FCk_AnimationToolbox_AnimInfo& OutInfo) const
    -> void
{
    OutInfo.UnmappedBoneCount = 0;
    OutInfo.MappedBoneCount   = 0;
    OutInfo.UnmappedBoneSample.Reset();

    if (!IsValid(Anim)) { return; }

    const IAnimationDataModel* Model = Anim->GetDataModel();
    if (Model == nullptr) { return; }

    USkeleton* Skeleton = Anim->GetSkeleton();
    if (!IsValid(Skeleton)) { return; }

    const FReferenceSkeleton& RefSkel = Skeleton->GetReferenceSkeleton();
    TArray<FName> TrackNames;
    Model->GetBoneTrackNames(TrackNames);
    const TSet<FName> TrackNameSet(TrackNames);

    constexpr int32 SampleCap = 8;
    const TArray<FMeshBoneInfo>& BoneInfo = RefSkel.GetRefBoneInfo();
    for (int32 b = 0; b < BoneInfo.Num(); ++b)
    {
        if (TrackNameSet.Contains(BoneInfo[b].Name))
        {
            ++OutInfo.MappedBoneCount;
        }
        else
        {
            ++OutInfo.UnmappedBoneCount;
            if (OutInfo.UnmappedBoneSample.Num() < SampleCap)
            { OutInfo.UnmappedBoneSample.Add(BoneInfo[b].Name); }
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAnimScrunchDetector::
    DoEvaluateAsset(UAnimSequence* Anim, float OutlierMultiplier, bool bRequireLoopBoundaryMismatch, FCk_AnimationToolbox_AnimInfo& OutInfo) const
    -> void
{
    if (!IsValid(Anim))
    {
        OutInfo.Status = ECk_AnimationToolbox_Status::Failed;
        OutInfo.StatusMessage = TEXT("Asset failed to load");
        return;
    }

    OutInfo.AssetName   = Anim->GetName();
    OutInfo.PackagePath = Anim->GetPackage()->GetName();
    OutInfo.NumNotifies = Anim->Notifies.Num();

    const IAnimationDataModel* Model = Anim->GetDataModel();
    if (Model == nullptr)
    {
        OutInfo.Status = ECk_AnimationToolbox_Status::Failed;
        OutInfo.StatusMessage = TEXT("No DataModel");
        return;
    }

    TArray<FName> TrackNames;
    Model->GetBoneTrackNames(TrackNames);
    const TSet<FName> TrackNameSet(TrackNames);

    DoGatherUnmappedBones(Anim, OutInfo);

    const int32 NumKeys = Model->GetNumberOfKeys();
    OutInfo.NumFrames = NumKeys;

    // Need at least 4 frames so we have a baseline of >= 2 transition deltas after excluding the 0->1 one.
    if (NumKeys < 4)
    {
        OutInfo.Status = ECk_AnimationToolbox_Status::Skipped;
        OutInfo.StatusMessage = FString::Printf(TEXT("Too few keys (%d) — need at least 4"), NumKeys);
        return;
    }

    // Build all per-frame component-space poses, then diff adjacent pairs.
    // Component space is what the viewer renders; a local change on the root bone propagates
    // through every downstream bone here, which is the signature of the walk-cycle scrunch.
    TArray<TArray<FTransform>> Frames;
    Frames.SetNum(NumKeys);
    for (int32 f = 0; f < NumKeys; ++f)
    {
        DoBuildComponentSpacePose(Anim, f, TrackNameSet, Frames[f]);
    }

    OutInfo.Frame0To1Delta    = DoComputePoseDelta(Frames[0], Frames[1]);
    OutInfo.LoopBoundaryDelta = DoComputePoseDelta(Frames[NumKeys - 1], Frames[0]);

    TArray<float> OtherDeltas;
    OtherDeltas.Reserve(NumKeys - 2);
    for (int32 f = 1; f < NumKeys - 1; ++f)
    {
        OtherDeltas.Add(DoComputePoseDelta(Frames[f], Frames[f + 1]));
    }
    OtherDeltas.Sort();
    OutInfo.MedianDelta = OtherDeltas[OtherDeltas.Num() / 2];
    OutInfo.Ratio       = OutInfo.MedianDelta > KINDA_SMALL_NUMBER
        ? (OutInfo.Frame0To1Delta / OutInfo.MedianDelta)
        : 1.0f;
    OutInfo.LoopBoundaryRatio = OutInfo.MedianDelta > KINDA_SMALL_NUMBER
        ? (OutInfo.LoopBoundaryDelta / OutInfo.MedianDelta)
        : 1.0f;

    const bool bHeadOutlier  = OutInfo.Ratio > OutlierMultiplier;
    const bool bBoundaryOutlier = OutInfo.LoopBoundaryRatio > OutlierMultiplier;

    // If the loop-boundary requirement is on, both deltas must be outliers (skips non-looping
    // anims whose abrupt opening isn't a true scrunch). If off, frame 0→1 alone is enough.
    const bool bAffected = bRequireLoopBoundaryMismatch
        ? (bHeadOutlier && bBoundaryOutlier)
        : bHeadOutlier;

    FString Msg;
    if (bAffected)
    {
        OutInfo.Status = ECk_AnimationToolbox_Status::Affected;
        OutInfo.bSelectedForFix = true;
        Msg = FString::Printf(TEXT("0→1: %.1fx · last→0: %.1fx (both outliers)"),
            OutInfo.Ratio, OutInfo.LoopBoundaryRatio);
    }
    else if (bHeadOutlier && !bBoundaryOutlier && bRequireLoopBoundaryMismatch)
    {
        // Caught head-outlier but the loop boundary is clean → likely a legitimate non-loop anim.
        OutInfo.Status = ECk_AnimationToolbox_Status::Clean;
        Msg = FString::Printf(TEXT("non-loop? 0→1: %.1fx but last→0: %.1fx"),
            OutInfo.Ratio, OutInfo.LoopBoundaryRatio);
    }
    else
    {
        OutInfo.Status = ECk_AnimationToolbox_Status::Clean;
        Msg = FString::Printf(TEXT("OK (0→1: %.1fx · last→0: %.1fx)"),
            OutInfo.Ratio, OutInfo.LoopBoundaryRatio);
    }

    if (OutInfo.UnmappedBoneCount > 0)
    {
        Msg += FString::Printf(
            TEXT(" · %d of %d skeleton bones have no track (using ref pose)"),
            OutInfo.UnmappedBoneCount,
            OutInfo.UnmappedBoneCount + OutInfo.MappedBoneCount);
    }
    OutInfo.StatusMessage = Msg;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAnimScrunchDetector::
    Request_Scan(
        const FCk_AnimationToolbox_ScanOptions& Options,
        const FProgressCallback& OnProgress) const
    -> TArray<TSharedPtr<FCk_AnimationToolbox_AnimInfo>>
{
    TArray<FAssetData> Candidates = DoCollectCandidates(Options);
    TArray<TSharedPtr<FCk_AnimationToolbox_AnimInfo>> Results;
    Results.Reserve(Candidates.Num());

    for (int32 i = 0; i < Candidates.Num(); ++i)
    {
        const FAssetData& AD = Candidates[i];

        if (OnProgress && !OnProgress(i, Candidates.Num(), AD.AssetName.ToString()))
        { break; }

        auto Info = MakeShared<FCk_AnimationToolbox_AnimInfo>();
        Info->AssetName   = AD.AssetName.ToString();
        Info->PackagePath = AD.PackageName.ToString();
        Info->AnimPtr     = TSoftObjectPtr<UAnimSequence>(AD.GetSoftObjectPath());

        UAnimSequence* Anim = Cast<UAnimSequence>(AD.GetAsset());
        DoEvaluateAsset(Anim, Options.OutlierMultiplier, Options.bRequireLoopBoundaryMismatch, *Info);

        Results.Add(Info);
    }

    Results.Sort([](const TSharedPtr<FCk_AnimationToolbox_AnimInfo>& A,
                    const TSharedPtr<FCk_AnimationToolbox_AnimInfo>& B) -> bool
    {
        auto Rank = [](ECk_AnimationToolbox_Status S) -> int32
        {
            switch (S)
            {
                case ECk_AnimationToolbox_Status::Affected: return 0;
                case ECk_AnimationToolbox_Status::Clean:    return 1;
                case ECk_AnimationToolbox_Status::Skipped:  return 2;
                default:                                     return 3;
            }
        };
        const int32 RA = Rank(A->Status);
        const int32 RB = Rank(B->Status);
        if (RA != RB) { return RA < RB; }
        return A->AssetName < B->AssetName;
    });

    return Results;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAnimScrunchDetector::
    Request_ApplyFix(
        const TSharedPtr<FCk_AnimationToolbox_AnimInfo>& Info,
        ECk_AnimationToolbox_RewriteStrategy Strategy) const
    -> bool
{
    if (NOT Info.IsValid())
    { return false; }

    UAnimSequence* Anim = Info->AnimPtr.LoadSynchronous();
    if (!IsValid(Anim))
    {
        Info->Status = ECk_AnimationToolbox_Status::Failed;
        Info->StatusMessage = TEXT("Failed to load asset");
        return false;
    }

    IAnimationDataModel* Model = Anim->GetDataModel();
    if (Model == nullptr)
    {
        Info->Status = ECk_AnimationToolbox_Status::Failed;
        Info->StatusMessage = TEXT("No DataModel");
        return false;
    }

    const int32 NumKeys = Model->GetNumberOfKeys();
    if (NumKeys < 2)
    {
        Info->Status = ECk_AnimationToolbox_Status::Failed;
        Info->StatusMessage = TEXT("Not enough keys to rewrite");
        return false;
    }

    if (ISourceControlModule::Get().IsEnabled())
    {
        USourceControlHelpers::CheckOutFile(Anim->GetPackage()->GetName());
    }

    IAnimationDataController& Controller = Anim->GetController();
    Controller.OpenBracket(FText::FromString(TEXT("CkAnimationToolbox: rewrite leading frame")));

    TArray<FName> TrackNames;
    Model->GetBoneTrackNames(TrackNames);

    // Source-frame index for copy strategies; -1 means "trim".
    const int32 SourceIdx = [&]() -> int32
    {
        switch (Strategy)
        {
            case ECk_AnimationToolbox_RewriteStrategy::CopyFromFrameOne:  return 1;
            case ECk_AnimationToolbox_RewriteStrategy::CopyFromLastFrame: return NumKeys - 1;
            case ECk_AnimationToolbox_RewriteStrategy::TrimFirstFrame:    return INDEX_NONE;
        }
        return INDEX_NONE;
    }();

    // All frame numbers we need to sample for the full-track readback.
    TArray<FFrameNumber> AllFrames;
    AllFrames.Reserve(NumKeys);
    for (int32 f = 0; f < NumKeys; ++f)
    { AllFrames.Add(FFrameNumber(f)); }

    bool AnyTrackMutated = false;
    for (const FName& BoneName : TrackNames)
    {
        TArray<FTransform> Transforms;
        Model->GetBoneTrackTransforms(BoneName, AllFrames, Transforms);
        if (Transforms.Num() == 0)
        { continue; }

        if (Strategy == ECk_AnimationToolbox_RewriteStrategy::TrimFirstFrame)
        {
            // Drop the leading frame, shrinking the track by one.
            Transforms.RemoveAt(0);
        }
        else if (Transforms.IsValidIndex(SourceIdx))
        {
            // Overwrite frame 0 with the source frame (preserves length).
            Transforms[0] = Transforms[SourceIdx];
        }
        else
        {
            continue;
        }

        TArray<FVector> Positions;
        TArray<FQuat>   Rotations;
        TArray<FVector> Scales;
        Positions.Reserve(Transforms.Num());
        Rotations.Reserve(Transforms.Num());
        Scales.Reserve(Transforms.Num());
        for (const FTransform& T : Transforms)
        {
            Positions.Add(T.GetLocation());
            Rotations.Add(T.GetRotation());
            Scales.Add(T.GetScale3D());
        }

        Controller.SetBoneTrackKeys(BoneName, Positions, Rotations, Scales);
        AnyTrackMutated = true;
    }

    // For the trim strategy, shrink the overall frame count by one.
    if (Strategy == ECk_AnimationToolbox_RewriteStrategy::TrimFirstFrame && AnyTrackMutated)
    {
        Controller.SetNumberOfFrames(FFrameNumber(NumKeys - 1));
    }

    Controller.CloseBracket();

    // Force the compressed blob to regenerate from the just-mutated raw data,
    // synchronously on the current platform. Without this the .uasset will save
    // with new raw data + stale compressed data — cook then ships the stale blob
    // and the scrunch persists even though the editor (which previews from raw)
    // looks clean.
    Anim->CacheDerivedDataForCurrentPlatform();

    Anim->GetPackage()->MarkPackageDirty();

    Info->Status = ECk_AnimationToolbox_Status::Fixed;
    Info->StatusMessage = TEXT("Fixed — save asset to persist (Ctrl+S)");
    return AnyTrackMutated;
}

// --------------------------------------------------------------------------------------------------------------------
