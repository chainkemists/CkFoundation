#include "CkVatProxy_Processor.h"

#include "CkVat/CkVat_Log.h"
#include "CkVat/CkVat_Subsystem.h"
#include "CkVat/Collection/CkVatCollection_Data.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Time/CkTime_Utils.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"
#include "CkIsmRenderer/Proxy/CkIsmProxy_Utils.h"
#include "CkIsmRenderer/Renderer/CkIsmRenderer_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_VatProxy_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_VatProxy_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_VatProxy_CancelPendingRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_VatProxy_FireSignals);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_vat_proxy_processor
{
    auto
    Get_CurrentWorldTime(
        const FCk_Handle& InHandle)
        -> FCk_Time
    {
        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        const auto TimeResult = UCk_Utils_Time_UE::Get_WorldTime(FCk_Utils_Time_GetWorldTime_Params{World});
        return TimeResult.Get_WorldTime().Get_Time();
    }

    // The 12-scalar per-instance layout the VAT looks decode (Rate == 0 => Floats[2] holds the frozen local time).
    auto
    Pack_CustomData(
        const ck::FFragment_VatProxy_Current& InCurrent,
        const UCk_VatCollection_Data& InCollection)
        -> TArray<float>
    {
        const auto& Clips = InCollection.Get_BakedData().Get_BakedClips();
        const auto RowsOf = [&](int32 InClipIndex, float& OutRowStart, float& OutRowCount) -> void
        {
            if (Clips.IsValidIndex(InClipIndex))
            {
                OutRowStart = static_cast<float>(Clips[InClipIndex].Get_FrameIndex());
                OutRowCount = static_cast<float>(Clips[InClipIndex].Get_FrameCount());
            }
            else
            {
                OutRowStart = 0.0f;
                OutRowCount = 0.0f;
            }
        };

        TArray<float> Floats;
        Floats.SetNumZeroed(UCk_Vat_Subsystem_UE::NumPerInstanceFloats);

        RowsOf(InCurrent.Get_ActiveClipIndex(), Floats[0], Floats[1]);
        Floats[2] = InCurrent.Get_PlayRate() == 0.0f
            ? InCurrent.Get_PausedLocalTime().Get_Seconds()
            : InCurrent.Get_PlaybackStartTime().Get_Seconds();
        Floats[3] = InCurrent.Get_PlayRate();

        RowsOf(InCurrent.Get_PrevClipIndex(), Floats[4], Floats[5]);
        Floats[6] = InCurrent.Get_PrevClipStartTime().Get_Seconds();
        Floats[7] = InCurrent.Get_PrevPlayRate();

        Floats[8] = InCurrent.Get_TransitionStartTime().Get_Seconds();
        Floats[9] = InCurrent.Get_TransitionDuration().Get_Seconds();
        Floats[10] = InCurrent.Get_ActiveLoopMode() == ECk_VatProxy_LoopMode::Once ? 1.0f : 0.0f;
        Floats[11] = InCurrent.Get_PrevLoopMode() == ECk_VatProxy_LoopMode::Once ? 1.0f : 0.0f;

        return Floats;
    }

    auto
    Push_CustomData(
        ck::FFragment_VatProxy_Current& InCurrent,
        const UCk_VatCollection_Data& InCollection)
        -> void
    {
        if (ck::Is_NOT_Valid(InCurrent.Get_IsmProxy()))
        { return; } // no visual composed (setup ensure fired) — playback state still advances

        auto IsmProxy = InCurrent.Get_IsmProxy();
        UCk_Utils_IsmProxy_UE::Request_SetCustomInstanceData(IsmProxy,
            FCk_Request_IsmProxy_SetCustomInstanceData{Pack_CustomData(InCurrent, InCollection)}, {});
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_VatProxy_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_VatProxy_Params& InParams,
            FFragment_VatProxy_Current& InCurrent) const
        -> void
    {
        InHandle.Remove<FTag_VatProxy_NeedsSetup>();

        const auto& Collection = InParams.Get_Collection();

        CK_ENSURE_IF_NOT(ck::IsValid(Collection),
            TEXT("Vat entity [{}] has an invalid VatCollection"), InHandle)
        { return; }

        CK_ENSURE_IF_NOT(Collection->Get_BakedData().Get_IsBaked(),
            TEXT("VatCollection [{}] on entity [{}] is not baked — bake it in-editor before composing Vat"),
            Collection, InHandle)
        { return; }

        if (NOT InParams.Get_InitialClipName().IsNone())
        {
            const auto ClipIndex = Collection->Find_BakedClipIndex_ByName(InParams.Get_InitialClipName());
            CK_ENSURE_IF_NOT(ClipIndex != INDEX_NONE,
                TEXT("Initial clip [{}] not found in the baked clip table of VatCollection [{}] (entity [{}])"),
                InParams.Get_InitialClipName(), Collection, InHandle)
            { return; }

            InCurrent._ActiveClipIndex = ClipIndex;
            InCurrent._ActiveLoopMode = InParams.Get_InitialLoopMode();
            InCurrent._PlayRate = InParams.Get_InitialPlayRate();
            InCurrent._PlaybackStartTime = ck_vat_proxy_processor::Get_CurrentWorldTime(InHandle);
            InCurrent._FinishedDispatched = false;

            if (InParams.Get_PhaseOffset() == ECk_VatProxy_PhaseOffset::RandomPerInstance)
            {
                const auto& Clips = Collection->Get_BakedData().Get_BakedClips();
                const auto ClipSeconds = Clips[ClipIndex].Get_PlayLength().Get_Seconds();
                InCurrent._PlaybackStartTime =
                    InCurrent._PlaybackStartTime - FCk_Time{FMath::FRandRange(0.0f, ClipSeconds)};
            }
        }

        // ---- rendering hookup: shared MID + transient ISM renderer + this entity's instance ----
        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        CK_ENSURE_IF_NOT(ck::IsValid(World),
            TEXT("No World for Vat entity [{}] — cannot compose the Vat rendering hookup"), InHandle)
        { return; }

        auto* Subsystem = World->GetSubsystem<UCk_Vat_Subsystem_UE>();
        CK_ENSURE_IF_NOT(ck::IsValid(Subsystem, ck::IsValid_Policy_NullptrOnly{}),
            TEXT("No UCk_Vat_Subsystem_UE for the world of entity [{}]"), InHandle)
        { return; }

        const auto RenderState = Subsystem->GetOrCreate_RenderState(Collection);
        if (NOT RenderState.IsSet())
        { return; } // GetOrCreate already ensured loudly; entity keeps its playback state, no visual

        CK_ENSURE_IF_NOT(UCk_Utils_Transform_UE::Has(InHandle),
            TEXT("Vat entity [{}] has no Transform — compose a Transform before Vat"), InHandle)
        { return; }

        auto TransformHandle = UCk_Utils_Transform_UE::Cast(InHandle);
        auto IsmProxy = UCk_Utils_IsmProxy_UE::Add(TransformHandle,
            FCk_IsmProxy_Spec{RenderState->_RendererData.Get()});
        InCurrent._IsmProxy = IsmProxy;

        ck_vat_proxy_processor::Push_CustomData(InCurrent, *Collection);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_VatProxy_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_VatProxy_Params& InParams,
            FFragment_VatProxy_Current& InCurrent,
            FFragment_VatProxy_Requests& InRequestsComp) const
        -> void
    {
        InHandle.CopyAndRemove(InRequestsComp, [&](FFragment_VatProxy_Requests& InRequests)
        {
            algo::ForEachRequest(InRequests._Requests, ck::Visitor([&](const auto& InRequest)
            {
                // Every DoHandleRequest overload below returns bool: false only on a genuine
                // (ensure-guarded) failure, true on every idempotent no-op or successful mutation.
                auto Result = ECk_Request_OperationResult::Failed;
                const auto Guard = MakeCompletionGuard(InRequest, InHandle, Result);

                if (DoHandleRequest(InHandle, InParams, InCurrent, InRequest))
                { Result = ECk_Request_OperationResult::Succeeded; }

                if (InRequest.Get_IsRequestHandleValid())
                {
                    InRequest.GetAndDestroyRequestHandle();
                }
            }));
        });

        // One custom-data push per drained batch — playback state only reaches the GPU on change.
        if (ck::IsValid(InParams.Get_Collection()))
        { ck_vat_proxy_processor::Push_CustomData(InCurrent, *InParams.Get_Collection()); }
    }

    auto
        FProcessor_VatProxy_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_VatProxy_Params& InParams,
            FFragment_VatProxy_Current& InCurrent,
            const FCk_Request_VatProxy_PlayClip& InRequest)
        -> bool
    {
        const auto& Collection = InParams.Get_Collection();

        CK_ENSURE_IF_NOT(ck::IsValid(Collection),
            TEXT("PlayClip on Vat entity [{}] with an invalid VatCollection"), InHandle)
        { return false; }

        const auto ClipIndex = Collection->Find_BakedClipIndex_ByName(InRequest.Get_ClipName());
        CK_ENSURE_IF_NOT(ClipIndex != INDEX_NONE,
            TEXT("PlayClip: clip [{}] not found in the baked clip table of VatCollection [{}] (entity [{}])"),
            InRequest.Get_ClipName(), Collection, InHandle)
        { return false; }

        const auto Now = ck_vat_proxy_processor::Get_CurrentWorldTime(InHandle);

        InCurrent._PrevClipIndex = InCurrent._ActiveClipIndex;
        InCurrent._PrevClipStartTime = InCurrent._PlaybackStartTime;
        InCurrent._PrevPlayRate = InCurrent._PlayRate;
        InCurrent._PrevLoopMode = InCurrent._ActiveLoopMode;
        InCurrent._TransitionStartTime = Now;
        InCurrent._TransitionDuration = InRequest.Get_TransitionDuration();

        InCurrent._ActiveClipIndex = ClipIndex;
        InCurrent._ActiveLoopMode = InRequest.Get_LoopMode();
        InCurrent._PlayRate = InRequest.Get_PlayRate();
        InCurrent._PlaybackStartTime = Now;
        InCurrent._PausedLocalTime = FCk_Time{};
        InCurrent._FinishedDispatched = false;

        return true;
    }

    auto
        FProcessor_VatProxy_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_VatProxy_Params& InParams,
            FFragment_VatProxy_Current& InCurrent,
            const FCk_Request_VatProxy_Stop& InRequest)
        -> bool
    {
        if (InCurrent._ActiveClipIndex == INDEX_NONE)
        { return true; } // nothing playing — reference pose is already static

        if (InCurrent._PlayRate == 0.0f)
        { return true; } // already frozen

        const auto Now = ck_vat_proxy_processor::Get_CurrentWorldTime(InHandle);
        const auto ElapsedLocalSeconds =
            (Now - InCurrent._PlaybackStartTime).Get_Seconds() * InCurrent._PlayRate;

        InCurrent._PausedLocalTime = FCk_Time{ElapsedLocalSeconds};
        InCurrent._PlayRate = 0.0f;

        return true;
    }

    auto
        FProcessor_VatProxy_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_VatProxy_Params& InParams,
            FFragment_VatProxy_Current& InCurrent,
            const FCk_Request_VatProxy_SetPlayRate& InRequest)
        -> bool
    {
        if (InCurrent._ActiveClipIndex == INDEX_NONE)
        { return true; }

        const auto NewRate = InRequest.Get_PlayRate();

        if (NewRate == InCurrent._PlayRate)
        { return true; }

        const auto Now = ck_vat_proxy_processor::Get_CurrentWorldTime(InHandle);

        if (NewRate == 0.0f)
        {
            // Rate 0 == freeze at the current position (same contract as Stop).
            const auto ElapsedLocalSeconds =
                (Now - InCurrent._PlaybackStartTime).Get_Seconds() * InCurrent._PlayRate;
            InCurrent._PausedLocalTime = FCk_Time{ElapsedLocalSeconds};
            InCurrent._PlayRate = 0.0f;
            return true;
        }

        // Rebase the start time so (Now - Start) * Rate stays continuous across the rate change.
        const auto CurrentLocalSeconds = InCurrent._PlayRate == 0.0f
            ? InCurrent._PausedLocalTime.Get_Seconds()
            : (Now - InCurrent._PlaybackStartTime).Get_Seconds() * InCurrent._PlayRate;

        InCurrent._PlaybackStartTime = Now - FCk_Time{CurrentLocalSeconds / NewRate};
        InCurrent._PausedLocalTime = FCk_Time{};
        InCurrent._PlayRate = NewRate;

        return true;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_VatProxy_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_VatProxy_Requests& InRequestsComp)
        -> void
    {
        request::FireCancelledForPending(InHandle, InRequestsComp.Get_Requests());
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_VatProxy_FireSignals::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_VatProxy_Params& InParams,
            FFragment_VatProxy_Current& InCurrent) const
        -> void
    {
        if (InCurrent.Get_ActiveClipIndex() == INDEX_NONE ||
            InCurrent.Get_ActiveLoopMode() != ECk_VatProxy_LoopMode::Once ||
            InCurrent.Get_FinishedDispatched() ||
            InCurrent.Get_PlayRate() <= 0.0f)
        { return; }

        const auto& Collection = InParams.Get_Collection();
        if (ck::Is_NOT_Valid(Collection))
        { return; } // Setup already ensured loudly

        const auto& Clips = Collection->Get_BakedData().Get_BakedClips();
        CK_ENSURE_IF_NOT(Clips.IsValidIndex(InCurrent.Get_ActiveClipIndex()),
            TEXT("Vat entity [{}]: active clip index [{}] is out of range of VatCollection [{}]'s baked clip table ([{}] clips) — stale index after a rebake?"),
            InHandle, InCurrent.Get_ActiveClipIndex(), Collection, Clips.Num())
        { return; }

        const auto Now = ck_vat_proxy_processor::Get_CurrentWorldTime(InHandle);
        const auto ElapsedLocalSeconds =
            (Now - InCurrent.Get_PlaybackStartTime()).Get_Seconds() * InCurrent.Get_PlayRate();
        const auto& Clip = Clips[InCurrent.Get_ActiveClipIndex()];

        if (ElapsedLocalSeconds < Clip.Get_PlayLength().Get_Seconds())
        { return; }

        InCurrent._FinishedDispatched = true;
        UUtils_Signal_VatProxy_OnClipFinished::Broadcast(InHandle, ck::MakePayload(InHandle, Clip.Get_Name()));
    }
}
