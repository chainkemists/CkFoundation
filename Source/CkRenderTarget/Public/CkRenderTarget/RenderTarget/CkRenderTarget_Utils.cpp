#include "CkRenderTarget_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkDebugCallstack_Macros.h"
#include "CkEcs/Net/CkNet_Utils.h"

#include "CkLabel/CkLabel_Utils.h"

#include "CkRenderTarget/CkRenderTarget_Log.h"
#include "CkRenderTarget/Pixels/CkRenderTarget_PixelMath.h"
#include "CkRenderTarget/RenderTarget/CkRenderTarget_Processor.h"
#include "CkRenderTarget/Settings/CkRenderTarget_Settings.h"

#include <Engine/TextureRenderTarget2D.h>
#include <Misc/App.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_RenderTarget_UE::
    Add(
        FCk_Handle& InOwnerEntity,
        const FCk_Fragment_RenderTarget_ParamsData& InParams)
    -> FCk_Handle_RenderTarget
{
    CK_ENSURE_IF_NOT(ck::IsValid(InParams.Get_SyncName()),
        TEXT("Cannot add a RenderTarget to [{}] — _SyncName is invalid. Sync names are the network "
             "identity of the target and are required (category: RenderTarget)."),
        InOwnerEntity)
    { return {}; }

    CK_ENSURE_IF_NOT(ck::Is_NOT_Valid(TryGet_RenderTarget(InOwnerEntity, InParams.Get_SyncName())),
        TEXT("Cannot add a RenderTarget with SyncName [{}] to [{}] — one already exists with that name."),
        InParams.Get_SyncName(), InOwnerEntity)
    { return {}; }

    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwnerEntity, [&](FCk_Handle InNewEntity)
    {
        UCk_Utils_GameplayLabel_UE::Add(InNewEntity, InParams.Get_SyncName());

        InNewEntity.Add<ck::FFragment_RenderTarget_Params>(InParams);
        InNewEntity.Add<ck::FFragment_RenderTarget_Current>();
        InNewEntity.Add<ck::FFragment_RenderTarget_PixelSync>();
        InNewEntity.Add<ck::FFragment_RenderTarget_ClientStaging>();
        InNewEntity.Add<ck::FTag_RenderTarget_NeedsSetup>();
    });

    auto NewRenderTargetEntity = ck::StaticCast<FCk_Handle_RenderTarget>(NewEntity);

    RecordOfRenderTargets_Utils::AddIfMissing(InOwnerEntity, ECk_Record_EntryHandlingPolicy::Default);
    RecordOfRenderTargets_Utils::Request_Connect(InOwnerEntity, NewRenderTargetEntity, ECk_Record_LabelRequirementPolicy::Optional);

    return NewRenderTargetEntity;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_RenderTarget_UE::
    Has_Any(
        const FCk_Handle& InHandle)
    -> bool
{
    return RecordOfRenderTargets_Utils::Has(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_RenderTarget_UE, FCk_Handle_RenderTarget,
    ck::FFragment_RenderTarget_Current, ck::FFragment_RenderTarget_Params);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_RenderTarget_UE::
    TryGet_RenderTarget(
        const FCk_Handle& InOwnerEntity,
        FGameplayTag InSyncName)
    -> FCk_Handle_RenderTarget
{
    return RecordOfRenderTargets_Utils::Get_ValidEntry_ByTag(InOwnerEntity, InSyncName);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_RenderTarget_UE::
    Get_SyncName(
        const FCk_Handle_RenderTarget& InRenderTargetEntity)
    -> FGameplayTag
{
    return UCk_Utils_GameplayLabel_UE::Get_Label(InRenderTargetEntity);
}

auto
    UCk_Utils_RenderTarget_UE::
    Get_Target(
        const FCk_Handle_RenderTarget& InRenderTargetEntity)
    -> UTextureRenderTarget2D*
{
    return InRenderTargetEntity.Get<ck::FFragment_RenderTarget_Current>().Get_Target().Get();
}

auto
    UCk_Utils_RenderTarget_UE::
    Get_CanRenderOnThisProcess()
    -> bool
{
    return FApp::CanEverRender();
}

auto
    UCk_Utils_RenderTarget_UE::
    Get_LatestAppliedBatchSeq(
        const FCk_Handle_RenderTarget& InRenderTargetEntity)
    -> int32
{
    const auto LocallyAuthoredSeq =
        InRenderTargetEntity.Get<ck::FFragment_RenderTarget_Current>().Get_NextBatchSeq() - 1;

    const auto ReplayedSeq = InRenderTargetEntity.Has<ck::FFragment_RenderTarget_ClientReplayState>()
        ? InRenderTargetEntity.Get<ck::FFragment_RenderTarget_ClientReplayState>().Get_LastAppliedSeq()
        : 0;

    return FMath::Max(LocallyAuthoredSeq, ReplayedSeq);
}

auto
    UCk_Utils_RenderTarget_UE::
    Get_PixelStateHash(
        const FCk_Handle_RenderTarget& InRenderTargetEntity)
    -> int32
{
    const auto& Snapshot = InRenderTargetEntity.Get<ck::FFragment_RenderTarget_PixelSync>().Get_LastSyncedSnapshot();

    if (NOT Snapshot.IsEmpty())
    { return static_cast<int32>(FCrc::MemCrc32(Snapshot.GetData(), Snapshot.Num())); }

    const auto& Staging = InRenderTargetEntity.Get<ck::FFragment_RenderTarget_ClientStaging>().Get_Pixels();

    if (NOT Staging.IsEmpty())
    { return static_cast<int32>(FCrc::MemCrc32(Staging.GetData(), Staging.Num())); }

    return 0;
}

auto
    UCk_Utils_RenderTarget_UE::
    Get_LatestAppliedPixelPayloadSeq(
        const FCk_Handle_RenderTarget& InRenderTargetEntity)
    -> int32
{
    return InRenderTargetEntity.Get<ck::FFragment_RenderTarget_ClientStaging>().Get_LastAppliedPayloadSeq();
}

auto
    UCk_Utils_RenderTarget_UE::
    Get_LatestAppliedPixelPayloadKind(
        const FCk_Handle_RenderTarget& InRenderTargetEntity)
    -> ECk_RenderTarget_PixelPayloadKind
{
    return InRenderTargetEntity.Get<ck::FFragment_RenderTarget_ClientStaging>().Get_LastAppliedPayloadKind();
}

auto
    UCk_Utils_RenderTarget_UE::
    Get_BaselineInstructionWatermark(
        const FCk_Handle_RenderTarget& InRenderTargetEntity)
    -> int32
{
    return InRenderTargetEntity.Has<ck::FFragment_RenderTarget_ClientReplayState>()
        ? InRenderTargetEntity.Get<ck::FFragment_RenderTarget_ClientReplayState>().Get_BaselineInstructionWatermark()
        : 0;
}

auto
    UCk_Utils_RenderTarget_UE::
    Get_NumBaselinedClients(
        const FCk_Handle_RenderTarget& InRenderTargetEntity)
    -> int32
{
    if (NOT InRenderTargetEntity.Has<ck::FFragment_RenderTarget_HostStreams>())
    { return 0; }

    const auto& Streams = InRenderTargetEntity.Get<ck::FFragment_RenderTarget_HostStreams>().Get_Streams();

    return ck::algo::CountIf(Streams, [](const auto& InKvp) -> bool
    {
        return InKvp.Value._HasBaseline;
    });
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_RenderTarget_UE::
    Request_DrawLine(
        FCk_Handle_RenderTarget& InRenderTargetEntity,
        const FCk_Request_RenderTarget_DrawLine& InRequest)
    -> FCk_Handle_RenderTarget
{
    CK_CALLSTACK_RECORD(ck::FFragment_RenderTarget_Requests, InRenderTargetEntity);

    InRenderTargetEntity.AddOrGet<ck::FFragment_RenderTarget_Requests>()._Requests.Emplace(InRequest);

    return InRenderTargetEntity;
}

auto
    UCk_Utils_RenderTarget_UE::
    Request_DrawTexture(
        FCk_Handle_RenderTarget& InRenderTargetEntity,
        const FCk_Request_RenderTarget_DrawTexture& InRequest)
    -> FCk_Handle_RenderTarget
{
    CK_CALLSTACK_RECORD(ck::FFragment_RenderTarget_Requests, InRenderTargetEntity);

    InRenderTargetEntity.AddOrGet<ck::FFragment_RenderTarget_Requests>()._Requests.Emplace(InRequest);

    return InRenderTargetEntity;
}

auto
    UCk_Utils_RenderTarget_UE::
    Request_DrawMaterial(
        FCk_Handle_RenderTarget& InRenderTargetEntity,
        const FCk_Request_RenderTarget_DrawMaterial& InRequest)
    -> FCk_Handle_RenderTarget
{
    CK_CALLSTACK_RECORD(ck::FFragment_RenderTarget_Requests, InRenderTargetEntity);

    InRenderTargetEntity.AddOrGet<ck::FFragment_RenderTarget_Requests>()._Requests.Emplace(InRequest);

    return InRenderTargetEntity;
}

auto
    UCk_Utils_RenderTarget_UE::
    Request_DrawText(
        FCk_Handle_RenderTarget& InRenderTargetEntity,
        const FCk_Request_RenderTarget_DrawText& InRequest)
    -> FCk_Handle_RenderTarget
{
    CK_CALLSTACK_RECORD(ck::FFragment_RenderTarget_Requests, InRenderTargetEntity);

    InRenderTargetEntity.AddOrGet<ck::FFragment_RenderTarget_Requests>()._Requests.Emplace(InRequest);

    return InRenderTargetEntity;
}

auto
    UCk_Utils_RenderTarget_UE::
    Request_DrawBox(
        FCk_Handle_RenderTarget& InRenderTargetEntity,
        const FCk_Request_RenderTarget_DrawBox& InRequest)
    -> FCk_Handle_RenderTarget
{
    CK_CALLSTACK_RECORD(ck::FFragment_RenderTarget_Requests, InRenderTargetEntity);

    InRenderTargetEntity.AddOrGet<ck::FFragment_RenderTarget_Requests>()._Requests.Emplace(InRequest);

    return InRenderTargetEntity;
}

auto
    UCk_Utils_RenderTarget_UE::
    Request_DrawBorder(
        FCk_Handle_RenderTarget& InRenderTargetEntity,
        const FCk_Request_RenderTarget_DrawBorder& InRequest)
    -> FCk_Handle_RenderTarget
{
    CK_CALLSTACK_RECORD(ck::FFragment_RenderTarget_Requests, InRenderTargetEntity);

    InRenderTargetEntity.AddOrGet<ck::FFragment_RenderTarget_Requests>()._Requests.Emplace(InRequest);

    return InRenderTargetEntity;
}

auto
    UCk_Utils_RenderTarget_UE::
    Request_DrawTriangles(
        FCk_Handle_RenderTarget& InRenderTargetEntity,
        const FCk_Request_RenderTarget_DrawTriangles& InRequest)
    -> FCk_Handle_RenderTarget
{
    CK_CALLSTACK_RECORD(ck::FFragment_RenderTarget_Requests, InRenderTargetEntity);

    InRenderTargetEntity.AddOrGet<ck::FFragment_RenderTarget_Requests>()._Requests.Emplace(InRequest);

    return InRenderTargetEntity;
}

auto
    UCk_Utils_RenderTarget_UE::
    Request_DrawPolygon(
        FCk_Handle_RenderTarget& InRenderTargetEntity,
        const FCk_Request_RenderTarget_DrawPolygon& InRequest)
    -> FCk_Handle_RenderTarget
{
    CK_CALLSTACK_RECORD(ck::FFragment_RenderTarget_Requests, InRenderTargetEntity);

    InRenderTargetEntity.AddOrGet<ck::FFragment_RenderTarget_Requests>()._Requests.Emplace(InRequest);

    return InRenderTargetEntity;
}

auto
    UCk_Utils_RenderTarget_UE::
    Request_Clear(
        FCk_Handle_RenderTarget& InRenderTargetEntity,
        const FCk_Request_RenderTarget_Clear& InRequest)
    -> FCk_Handle_RenderTarget
{
    CK_CALLSTACK_RECORD(ck::FFragment_RenderTarget_Requests, InRenderTargetEntity);

    InRenderTargetEntity.AddOrGet<ck::FFragment_RenderTarget_Requests>()._Requests.Emplace(InRequest);

    return InRenderTargetEntity;
}

auto
    UCk_Utils_RenderTarget_UE::
    Request_SyncPixels(
        FCk_Handle_RenderTarget& InRenderTargetEntity,
        const FCk_Request_RenderTarget_SyncPixels& InRequest)
    -> FCk_Handle_RenderTarget
{
    CK_CALLSTACK_RECORD(ck::FFragment_RenderTarget_Requests, InRenderTargetEntity);

    InRenderTargetEntity.AddOrGet<ck::FFragment_RenderTarget_Requests>()._Requests.Emplace(InRequest);

    return InRenderTargetEntity;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_RenderTarget_UE::
    Debug_InjectCapturedPixels(
        FCk_Handle_RenderTarget& InRenderTargetEntity,
        const TArray<uint8>& InPixels,
        FIntPoint InSize)
    -> FCk_Handle_RenderTarget
{
#if !UE_BUILD_SHIPPING
    CK_ENSURE_IF_NOT(InPixels.Num() == InSize.X * InSize.Y * ck::render_target::pixel::BytesPerPixel,
        TEXT("Debug_InjectCapturedPixels buffer size [{}] does not match [{}x{}] RGBA8 ([{}] bytes expected)"),
        InPixels.Num(), InSize.X, InSize.Y, InSize.X * InSize.Y * ck::render_target::pixel::BytesPerPixel)
    { return InRenderTargetEntity; }

    CK_ENSURE_IF_NOT(NOT InRenderTargetEntity.Has<ck::FTag_RenderTarget_PixelSyncInFlight>(),
        TEXT("Debug_InjectCapturedPixels called on [{}] while a pixel pass is already in flight"),
        InRenderTargetEntity)
    { return InRenderTargetEntity; }

    auto& PixelSync = InRenderTargetEntity.Get<ck::FFragment_RenderTarget_PixelSync>();

    // Mirror the readback path's upload rule: client captures of replicating targets always
    // produce a FullSync (see FProcessor_RenderTarget_PixelSyncPump).
    const auto IsUploadBound =
        InRenderTargetEntity.Get<ck::FFragment_RenderTarget_Params>().Get_Replication() == ECk_Replication::Replicates
        && NOT UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(InRenderTargetEntity);

    PixelSync._PendingInstructionWatermark =
        InRenderTargetEntity.Get<ck::FFragment_RenderTarget_Current>().Get_NextBatchSeq() - 1;
    PixelSync._JobResult = ck::render_target::pixel::Launch_DiffAndCompressJob(
        InPixels, InSize,
        IsUploadBound ? TArray<uint8>{} : PixelSync._LastSyncedSnapshot,
        IsUploadBound ? FIntPoint::ZeroValue : PixelSync._SnapshotSize,
        UCk_Utils_RenderTarget_Settings_UE::Get_BlockSize());
    InRenderTargetEntity.Add<ck::FTag_RenderTarget_PixelSyncInFlight>();
#endif

    return InRenderTargetEntity;
}

auto
    UCk_Utils_RenderTarget_UE::
    Debug_RedrawTargetFromLastSnapshot(
        FCk_Handle_RenderTarget& InRenderTargetEntity)
    -> FCk_Handle_RenderTarget
{
#if !UE_BUILD_SHIPPING
    const auto& PixelSync = InRenderTargetEntity.Get<ck::FFragment_RenderTarget_PixelSync>();

    CK_ENSURE_IF_NOT(PixelSync.Get_LastSyncedSnapshot().Num() > 0,
        TEXT("Debug_RedrawTargetFromLastSnapshot called on [{}] with no captured snapshot — capture first"),
        InRenderTargetEntity)
    { return InRenderTargetEntity; }

    auto& Staging = InRenderTargetEntity.AddOrGet<ck::FFragment_RenderTarget_ClientStaging>();

    ck_render_target_processor::DrawPixelsToTarget(
        InRenderTargetEntity,
        InRenderTargetEntity.Get<ck::FFragment_RenderTarget_Current>(),
        PixelSync.Get_LastSyncedSnapshot(),
        PixelSync.Get_SnapshotSize(),
        Staging._UploadTexture);
#endif

    return InRenderTargetEntity;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_RenderTarget_UE::
    BindTo_OnInstructionsApplied(
        FCk_Handle_RenderTarget& InRenderTargetEntity,
        const FCk_Delegate_RenderTarget_OnInstructionsApplied& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_RenderTarget
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_RenderTarget_OnInstructionsApplied, InRenderTargetEntity, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InRenderTargetEntity;
}

auto
    UCk_Utils_RenderTarget_UE::
    UnbindFrom_OnInstructionsApplied(
        FCk_Handle_RenderTarget& InRenderTargetEntity,
        const FCk_Delegate_RenderTarget_OnInstructionsApplied& InDelegate)
    -> FCk_Handle_RenderTarget
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_RenderTarget_OnInstructionsApplied, InRenderTargetEntity, InDelegate);
    return InRenderTargetEntity;
}

auto
    UCk_Utils_RenderTarget_UE::
    BindTo_OnPixelPayloadProduced(
        FCk_Handle_RenderTarget& InRenderTargetEntity,
        const FCk_Delegate_RenderTarget_OnPixelPayloadProduced& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_RenderTarget
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_RenderTarget_OnPixelPayloadProduced, InRenderTargetEntity, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InRenderTargetEntity;
}

auto
    UCk_Utils_RenderTarget_UE::
    UnbindFrom_OnPixelPayloadProduced(
        FCk_Handle_RenderTarget& InRenderTargetEntity,
        const FCk_Delegate_RenderTarget_OnPixelPayloadProduced& InDelegate)
    -> FCk_Handle_RenderTarget
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_RenderTarget_OnPixelPayloadProduced, InRenderTargetEntity, InDelegate);
    return InRenderTargetEntity;
}

auto
    UCk_Utils_RenderTarget_UE::
    BindTo_OnPixelPayloadApplied(
        FCk_Handle_RenderTarget& InRenderTargetEntity,
        const FCk_Delegate_RenderTarget_OnPixelPayloadApplied& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_RenderTarget
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_RenderTarget_OnPixelPayloadApplied, InRenderTargetEntity, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InRenderTargetEntity;
}

auto
    UCk_Utils_RenderTarget_UE::
    UnbindFrom_OnPixelPayloadApplied(
        FCk_Handle_RenderTarget& InRenderTargetEntity,
        const FCk_Delegate_RenderTarget_OnPixelPayloadApplied& InDelegate)
    -> FCk_Handle_RenderTarget
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_RenderTarget_OnPixelPayloadApplied, InRenderTargetEntity, InDelegate);
    return InRenderTargetEntity;
}

auto
    UCk_Utils_RenderTarget_UE::
    BindTo_OnClientBaselineEstablished(
        FCk_Handle_RenderTarget& InRenderTargetEntity,
        const FCk_Delegate_RenderTarget_OnClientBaselineEstablished& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_RenderTarget
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_RenderTarget_OnClientBaselineEstablished, InRenderTargetEntity, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InRenderTargetEntity;
}

auto
    UCk_Utils_RenderTarget_UE::
    UnbindFrom_OnClientBaselineEstablished(
        FCk_Handle_RenderTarget& InRenderTargetEntity,
        const FCk_Delegate_RenderTarget_OnClientBaselineEstablished& InDelegate)
    -> FCk_Handle_RenderTarget
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_RenderTarget_OnClientBaselineEstablished, InRenderTargetEntity, InDelegate);
    return InRenderTargetEntity;
}

// --------------------------------------------------------------------------------------------------------------------
