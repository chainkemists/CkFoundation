#include "CkIskmAnimCollection_Fragment_Data.h"
#include "CkIskmAnimCollection_BakedPose.h"
#include "CkIskmRendererVF/CkIskm_BatchedRenderResources.h"

#include "CkAnimation/AnimBake/CkAnimBake.h"

#include "CkCore/Validation/CkIsValid.h"
#include "CkCore/Ensure/CkEnsure.h"

#include "CkIskmRenderer/Renderer/CkIskm_BatchedClusterComponent.h"

#include "Algo/Compare.h"
#include "RenderingThread.h"
#include "RenderUtils.h"
#include "RHIGlobals.h"
#include "Misc/App.h"
#include "ComponentRecreateRenderStateContext.h"
#include "UObject/UObjectIterator.h"

#include "Animation/Skeleton.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimationAsset.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkeletalMeshSocket.h"
#include "ReferenceSkeleton.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Rendering/SkeletalMeshLODRenderData.h"

#if WITH_EDITOR
    #include "Misc/DataValidation.h"
#endif

auto
    UCk_IskmAnimCollection_Data::
    Find_SequenceIndex_ByAsset(const UAnimSequenceBase* InAsset) const
    -> int32
{
    return _Sequences.IndexOfByPredicate([&](const FCk_IskmAnimCollection_SequenceDef& E)
    {
        return E.Get_Sequence() == InAsset;
    });
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_IskmAnimCollection_Data::
    Get_EffectiveSkeleton() const
    -> USkeleton*
{
    if (ck::IsValid(_Skeleton))
    { return _Skeleton; }
    if (ck::IsValid(_DefaultMesh))
    { return _DefaultMesh->GetSkeleton(); }
    return nullptr;
}

auto
    UCk_IskmAnimCollection_Data::
    Build_BakedPoseData()
    -> bool
{
    // NOT FApp::CanEverRender(): a -nullrhi run is not a dedicated server, still has mesh render data,
    // and still needs this CPU-only bake every tick.
    if (IsRunningDedicatedServer())
    { return false; }

    // A re-bake changes the frame layout, so previously materialized render data must not survive. The
    // scoped contexts recreate the proxies on function exit, re-entering EnsureRenderResources.
    TIndirectArray<FComponentRecreateRenderStateContext> RecreateContexts;
    if (ck::IsValid(_RenderData.Get(), ck::IsValid_Policy_NullptrOnly{}))
    {
        for (TObjectIterator<UCk_Iskm_BatchedClusterComponent> It; It; ++It)
        {
            if (It->Get_AnimCollection() == this && It->IsRegistered())
            { RecreateContexts.Add(new FComponentRecreateRenderStateContext(*It)); }
        }
        FlushRenderingCommands();
        ReleaseRenderResources();
        FlushRenderingCommands();
        _RenderData = nullptr;
    }

    USkeleton* const Skeleton = Get_EffectiveSkeleton();
    CK_ENSURE_IF_NOT(ck::IsValid(Skeleton) && ck::IsValid(_DefaultMesh),
        TEXT("[CkIskm] Batched bake needs a DefaultMesh and a Skeleton (own or the DefaultMesh's) on AnimCollection [{}] — "
             "every batched cluster using it will render NOTHING"),
        this)
    { return false; }

    FCk_AnimBake_SampleParams SampleParams;
    SampleParams.ExtractRootMotion = _ExtractRootMotion;
    SampleParams.DisableRetargeting = _DisableRetargeting;
    // Batched crowd must skin like the promoted SKMC path: invert the mesh bind pose, not the skeleton ref pose.
    SampleParams.UseMeshBindRefPose = true;

    const auto SkeletonData = ck::anim_bake::BuildSkeletonData(*Skeleton, *_DefaultMesh, SampleParams);
    CK_ENSURE_IF_NOT(SkeletonData.IsSet(),
        TEXT("[CkIskm] AnimCollection [{}] is not bakeable — DefaultMesh [{}] has no skeleton bones, no render data, or no skinned bones — "
             "batched clusters will render NOTHING"),
        this, _DefaultMesh)
    { return false; }

    TArray<UAnimSequenceBase*> SequenceAssets;
    SequenceAssets.Reserve(_Sequences.Num());
    for (const FCk_IskmAnimCollection_SequenceDef& Def : _Sequences)
    { SequenceAssets.Add(Def.Get_Sequence().Get()); }

    const auto Layout = ck::anim_bake::BuildFrameLayout(SequenceAssets, _SampleFrequency);

    auto Baked = MakePimpl<FCk_Iskm_BakedPose>();
    Baked->HighPrecision = true;
    Baked->RenderBoneCount = SkeletonData->RenderBoneCount;
    Baked->RenderRequiredBones = SkeletonData->RenderRequiredBones;
    Baked->SkeletonBoneToRenderBone = SkeletonData->SkeletonBoneToRenderBone;
    Baked->RefPoseInverse = SkeletonData->RefPoseInverse;

    Baked->Sequences.Reserve(Layout.Sequences.Num());
    for (const FCk_AnimBake_SequenceLayout& SeqLayout : Layout.Sequences)
    {
        FCk_Iskm_BakedSequence BakedSeq;
        BakedSeq.Sequence = SeqLayout.Sequence;
        BakedSeq.SampleFrequency = SeqLayout.SampleFrequency;
        BakedSeq.AnimationFrameCount = SeqLayout.FrameCount;
        BakedSeq.AnimationFrameIndex = SeqLayout.FrameIndex;
        Baked->Sequences.Add(BakedSeq);
    }
    Baked->FrameCountSequences = Layout.TotalFrameCount;
    Baked->TotalFrameCount = Layout.TotalFrameCount;

    const int32 RenderBoneCount = Baked->RenderBoneCount;
    Baked->Matrices.SetNumUninitialized(RenderBoneCount * Baked->TotalFrameCount);

    const FBox3f MeshBound = static_cast<FBox3f>(_DefaultMesh->GetBounds().GetBox());
    Baked->FrameBounds.Init(MeshBound, Baked->FrameCountSequences);

    struct FSocketResolve { int32 SocketSlot = INDEX_NONE; int32 BoneIndex = INDEX_NONE; FTransform LocalOffset; };
    TArray<FSocketResolve> ResolvedSockets;
    const FReferenceSkeleton& RefSkeleton = Skeleton->GetReferenceSkeleton();
    for (const FName SocketName : _BakedSockets)
    {
        FName BoneName = SocketName;
        FTransform LocalOffset = FTransform::Identity;
        if (const USkeletalMeshSocket* Socket = _DefaultMesh->FindSocket(SocketName))
        {
            BoneName = Socket->BoneName;
            LocalOffset = Socket->GetSocketLocalTransform();
        }
        const int32 BoneIndex = RefSkeleton.FindBoneIndex(BoneName);
        CK_ENSURE_IF_NOT(BoneIndex != INDEX_NONE,
            TEXT("[CkIskm] _BakedSockets entry [{}] on [{}] resolves to no socket or bone — its far cosmetics will be hidden"),
            SocketName, this)
        { continue; }

        FCk_Iskm_BakedSocket& Out = Baked->Sockets.Emplace_GetRef();
        Out.Name = SocketName;
        Out.BoneIndex = BoneIndex;
        Out.LocalOffset = static_cast<FTransform3f>(LocalOffset);
        Out.FrameTransforms.SetNum(Baked->TotalFrameCount);
        ResolvedSockets.Add(FSocketResolve{ Baked->Sockets.Num() - 1, BoneIndex, LocalOffset });
    }

    // ShaderMatrix[bone] = RefPoseInverse[bone] * ComponentSpaceBoneMatrix[bone], stored transposed 3x4
    // because the shader does mul(Matrix, Vector).
    const auto CalcRenderMatrices = [&Baked, RenderBoneCount](TArrayView<const FTransform> InPoseComponentSpace, int32 InFrameIndex)
    {
        FCk_Iskm_BoneMatrix3x4* const Out = Baked->Matrices.GetData() + (InFrameIndex * RenderBoneCount);
        for (int32 i = 0; i < RenderBoneCount; ++i)
        {
            const int32 CompactBoneIdx = Baked->RenderRequiredBones[i];
            const FMatrix44f BoneMatrix = static_cast<FTransform3f>(InPoseComponentSpace[CompactBoneIdx]).ToMatrixWithScale();
            const FMatrix44f ShaderMatrix = Baked->RefPoseInverse[CompactBoneIdx] * BoneMatrix;

            const float* RESTRICT Src = &ShaderMatrix.M[0][0];
            float* RESTRICT Dst = Out[i].M;
            Dst[0] = Src[0]; Dst[1] = Src[4]; Dst[2]  = Src[8];  Dst[3]  = Src[12];
            Dst[4] = Src[1]; Dst[5] = Src[5]; Dst[6]  = Src[9];  Dst[7]  = Src[13];
            Dst[8] = Src[2]; Dst[9] = Src[6]; Dst[10] = Src[10]; Dst[11] = Src[14];
        }
    };

    // The pose view is indexed by SKELETON bone (CkAnimBake contract), so socket bones need no remap.
    const auto PerFramePose = [&](TArrayView<const FTransform> InPoseComponentSpace, int32 InFrameIndex)
    {
        CalcRenderMatrices(InPoseComponentSpace, InFrameIndex);
        for (const FSocketResolve& S : ResolvedSockets)
        {
            Baked->Sockets[S.SocketSlot].FrameTransforms[InFrameIndex] =
                static_cast<FTransform3f>(S.LocalOffset * InPoseComponentSpace[S.BoneIndex]);
        }
    };
    const FBox BoneBoundsAllFrames =
        ck::anim_bake::SamplePoses(*Skeleton, *SkeletonData, Layout, SampleParams, PerFramePose);

    Baked->AnimatedBounds = ck::anim_bake::ComputeAnimatedBounds(*SkeletonData, BoneBoundsAllFrames, *_DefaultMesh);

    Baked->IsBaked = true;
    _BakedPose = MoveTemp(Baked);
    return true;
}

auto
    UCk_IskmAnimCollection_Data::
    Get_AnimatedMeshBounds() const
    -> FBox
{
    if (Get_IsBaked() && _BakedPose->AnimatedBounds.IsValid != 0)
    { return _BakedPose->AnimatedBounds; }
    if (ck::IsValid(_DefaultMesh))
    { return _DefaultMesh->GetBounds().GetBox(); }
    return FBox(FVector(-50.0), FVector(50.0));
}

auto
    UCk_IskmAnimCollection_Data::
    Get_BakedPose() const
    -> const FCk_Iskm_BakedPose*
{
    return _BakedPose.Get();
}

auto
    UCk_IskmAnimCollection_Data::
    Get_IsBaked() const
    -> bool
{
    return ck::IsValid(_BakedPose.Get(), ck::IsValid_Policy_NullptrOnly{}) && _BakedPose->IsBaked;
}

auto
    UCk_IskmAnimCollection_Data::
    Get_IsBakeStale() const
    -> bool
{
    if (NOT Get_IsBaked())
    { return false; }

    return NOT Algo::Compare(_BakedPose->Sequences, _Sequences,
        [this](const FCk_Iskm_BakedSequence& InBaked, const FCk_IskmAnimCollection_SequenceDef& InAuthored)
        {
            return InBaked.Sequence.Get() == InAuthored.Get_Sequence().Get() &&
                   InBaked.SampleFrequency == _SampleFrequency;
        });
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_IskmAnimCollection_Data::
    EnsureRenderResources()
    -> void
{
    if (FApp::CanEverRender() == false)
    { return; } // headless / -nullrhi: CPU bake only, no GPU resources

    if (NOT Get_IsBaked() || Get_IsBakeStale())
    {
        if (NOT Build_BakedPoseData())
        { return; }
    }

    if (ck::IsValid(_RenderData.Get(), ck::IsValid_Policy_NullptrOnly{}))
    { return; }

    const FCk_Iskm_BakedPose* Baked = Get_BakedPose();
    CK_ENSURE_IF_NOT(ck::IsValid(Baked, ck::IsValid_Policy_NullptrOnly{}) && Baked->Matrices.Num() > 0,
        TEXT("[CkIskm] AnimCollection [{}] reports baked but has no baked matrices — bake pipeline bug"),
        this)
    { return; }

    // Pre-flight BEFORE standing up render resources: the bake can be cached and _DefaultMesh is
    // script-mutable. Validation lives in this module because the engine-only VF module (no Ck deps)
    // cannot CK_ENSURE and silently no-ops on bad input.
    USkeleton* const EffectiveSkeleton = Get_EffectiveSkeleton();
    CK_ENSURE_IF_NOT(ck::IsValid(_DefaultMesh) && ck::IsValid(EffectiveSkeleton),
        TEXT("[CkIskm] AnimCollection [{}] lost its DefaultMesh/Skeleton after bake — batched clusters will render NOTHING"),
        this)
    { return; }

    const FSkeletalMeshRenderData* MeshRenderData = _DefaultMesh->GetResourceForRendering();
    CK_ENSURE_IF_NOT(ck::IsValid(MeshRenderData, ck::IsValid_Policy_NullptrOnly{}),
        TEXT("[CkIskm] DefaultMesh [{}] has no render data — batched clusters will render NOTHING"),
        _DefaultMesh)
    { return; }

    for (const FSkeletalMeshLODRenderData& LODData : MeshRenderData->LODRenderData)
    {
        for (const FSkelMeshRenderSection& Section : LODData.RenderSections)
        {
            CK_ENSURE_IF_NOT(Section.MaxBoneInfluences <= 8,
                TEXT("[CkIskm] Batched renderer supports <= 8 bone influences; a section has {} — strongest 8 kept, weights renormalized."),
                Section.MaxBoneInfluences)
            { }
        }

        // The bone-index/weight build reads skin weights on the CPU; cooked builds discard that copy
        // unless the mesh keeps CPU access, and those reads would hit freed memory.
        CK_ENSURE_IF_NOT(GIsEditor || LODData.GetSkinWeightVertexBuffer()->GetNeedsCPUAccess(),
            TEXT("[CkIskm] Batched renderer requires CPU-accessible skin weights in cooked builds — set bNeedsCPUAccess on mesh [{}]"),
            _DefaultMesh)
        { return; }
    }

    _RenderData = MakePimpl<FCk_Iskm_BatchedRenderData>();
    FCk_Iskm_BatchedRenderData* RD = _RenderData.Get();

    RD->AnimationBuffer.Matrices.SetNumUninitialized(Baked->Matrices.Num());
    FMemory::Memcpy(RD->AnimationBuffer.Matrices.GetData(), Baked->Matrices.GetData(),
        Baked->Matrices.Num() * sizeof(FCk_Iskm_BoneMatrix3x4));

    // Skeleton + remap table are passed explicitly so the engine-only VF module stays decoupled from this asset type.
    RD->DefaultMeshData.InitFromMesh(_DefaultMesh, EffectiveSkeleton, Baked->SkeletonBoneToRenderBone, GMaxRHIFeatureLevel);

    CK_ENSURE_IF_NOT(RD->DefaultMeshData.NumBoneRemapMisses == 0,
        TEXT("[CkIskm] [{}] vertex influences on mesh [{}] reference bones outside the bake's render-bone set — they rigid-bind to root (weight dropped)"),
        RD->DefaultMeshData.NumBoneRemapMisses, _DefaultMesh)
    { }

    const uint32 BoneCount = static_cast<uint32>(Baked->RenderBoneCount);

    ENQUEUE_RENDER_COMMAND(CkIskm_InitAnimCollectionResources)(
        [RD, BoneCount](FRHICommandListImmediate& RHICmdList)
        {
            RD->AnimationBuffer.InitResource(RHICmdList);

            FCk_Iskm_AnimCollectionUniformParams Params;
            Params.BoneCount = BoneCount;
            Params.AnimationBuffer = RD->AnimationBuffer.ShaderResourceViewRHI;
            RD->AnimCollectionUB = FCk_Iskm_AnimCollectionUniformParamsRef::CreateUniformBufferImmediate(Params, UniformBuffer_MultiFrame);

            RD->DefaultMeshData.InitResources(RHICmdList, RD->AnimCollectionUB.GetReference());
        });
}

auto
    UCk_IskmAnimCollection_Data::
    Get_DefaultMeshData() const
    -> const FCk_Iskm_BatchedMeshData*
{
    return ck::IsValid(_RenderData.Get(), ck::IsValid_Policy_NullptrOnly{}) ? &_RenderData->DefaultMeshData : nullptr;
}

auto
    UCk_IskmAnimCollection_Data::
    ReleaseRenderResources()
    -> void
{
    if (NOT ck::IsValid(_RenderData.Get(), ck::IsValid_Policy_NullptrOnly{}))
    { return; }

    // Game-thread ReleaseResource enqueues the render-thread ReleaseRHI; the _ReleaseResourcesFence (BeginDestroy)
    // gates UObject destruction until those complete, so the resource objects outlive their GPU release.
    _RenderData->DefaultMeshData.ReleaseResources();
    _RenderData->AnimationBuffer.ReleaseResource();
    _RenderData->AnimCollectionUB.SafeRelease();
}

auto
    UCk_IskmAnimCollection_Data::
    BeginDestroy()
    -> void
{
    Super::BeginDestroy();
    ReleaseRenderResources();
    _ReleaseResourcesFence.BeginFence();
}

auto
    UCk_IskmAnimCollection_Data::
    IsReadyForFinishDestroy()
    -> bool
{
    return Super::IsReadyForFinishDestroy() && _ReleaseResourcesFence.IsFenceComplete();
}

#if WITH_EDITOR
auto
    UCk_IskmAnimCollection_Data::
    PostEditChangeProperty(FPropertyChangedEvent& InPropertyChangedEvent)
    -> void
{
    Super::PostEditChangeProperty(InPropertyChangedEvent);
}

auto
    UCk_IskmAnimCollection_Data::
    IsDataValid(FDataValidationContext& InContext) const
    -> EDataValidationResult
{
    auto Result = Super::IsDataValid(InContext);

    // _Skeleton may legitimately be unset when a DefaultMesh carries the skeleton (script-authored
    // collections cannot assign _Skeleton) — validate against the effective skeleton.
    const auto* EffectiveSkeleton = Get_EffectiveSkeleton();
    if (ck::Is_NOT_Valid(EffectiveSkeleton))
    {
        InContext.AddError(FText::FromString(TEXT("AnimCollection has no Skeleton (own or via DefaultMesh).")));
        Result = EDataValidationResult::Invalid;
    }

    if (ck::IsValid(_DefaultMesh) && ck::IsValid(_Skeleton) && _DefaultMesh->GetSkeleton() != _Skeleton)
    {
        InContext.AddError(FText::FromString(TEXT("DefaultMesh skeleton does not match the AnimCollection's Skeleton.")));
        Result = EDataValidationResult::Invalid;
    }

    for (auto Index = 0; Index < _Sequences.Num(); ++Index)
    {
        const auto& Def = _Sequences[Index];
        const auto* Seq = Def.Get_Sequence().Get();
        if (ck::Is_NOT_Valid(Seq))
        {
            InContext.AddError(FText::FromString(FString::Printf(TEXT("Sequence [%d] (%s) is null."), Index, *Def.Get_Name().ToString())));
            Result = EDataValidationResult::Invalid;
            continue;
        }
        if (Seq->GetSkeleton() != EffectiveSkeleton)
        {
            InContext.AddError(FText::FromString(FString::Printf(TEXT("Sequence [%d] (%s) skeleton mismatch."), Index, *Def.Get_Name().ToString())));
            Result = EDataValidationResult::Invalid;
        }
    }

    return Result;
}
#endif
