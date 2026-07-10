#include "CkVat_Processor.h"

#include "CkVat/CkVat_Log.h"
#include "CkVat/Collection/CkVatCollection_Data.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Time/CkTime_Utils.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_Vat_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Vat_HandleRequests);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_vat_processor
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
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Vat_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Vat_Params& InParams,
            FFragment_Vat_Current& InCurrent) const
        -> void
    {
        InHandle.Remove<FTag_Vat_NeedsSetup>();

        const auto& Collection = InParams.Get_Collection();

        CK_ENSURE_IF_NOT(ck::IsValid(Collection),
            TEXT("Vat entity [{}] has an invalid VatCollection"), InHandle)
        { return; }

        CK_ENSURE_IF_NOT(Collection->Get_IsBaked(),
            TEXT("VatCollection [{}] on entity [{}] is not baked — bake it in-editor before composing Vat"),
            Collection, InHandle)
        { return; }

        if (InParams.Get_InitialClipName().IsNone())
        { return; } // reference pose (texture row 0)

        const auto ClipIndex = Collection->Find_BakedClipIndex_ByName(InParams.Get_InitialClipName());
        CK_ENSURE_IF_NOT(ClipIndex != INDEX_NONE,
            TEXT("Initial clip [{}] not found in the baked clip table of VatCollection [{}] (entity [{}])"),
            InParams.Get_InitialClipName(), Collection, InHandle)
        { return; }

        InCurrent._ActiveClipIndex = ClipIndex;
        InCurrent._ActiveLoopMode = InParams.Get_InitialLoopMode();
        InCurrent._PlayRate = InParams.Get_InitialPlayRate();
        InCurrent._PlaybackStartTime = ck_vat_processor::Get_CurrentWorldTime(InHandle);
        InCurrent._FinishedDispatched = false;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Vat_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Vat_Params& InParams,
            FFragment_Vat_Current& InCurrent,
            FFragment_Vat_Requests& InRequestsComp) const
        -> void
    {
        InHandle.CopyAndRemove(InRequestsComp, [&](FFragment_Vat_Requests& InRequests)
        {
            algo::ForEachRequest(InRequests._Requests, ck::Visitor([&](const auto& InRequest)
            {
                DoHandleRequest(InHandle, InParams, InCurrent, InRequest);

                if (InRequest.Get_IsRequestHandleValid())
                {
                    InRequest.GetAndDestroyRequestHandle();
                }
            }));
        });
    }

    auto
        FProcessor_Vat_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Vat_Params& InParams,
            FFragment_Vat_Current& InCurrent,
            const FCk_Request_Vat_PlayClip& InRequest)
        -> void
    {
        const auto& Collection = InParams.Get_Collection();

        CK_ENSURE_IF_NOT(ck::IsValid(Collection),
            TEXT("PlayClip on Vat entity [{}] with an invalid VatCollection"), InHandle)
        { return; }

        const auto ClipIndex = Collection->Find_BakedClipIndex_ByName(InRequest.Get_ClipName());
        CK_ENSURE_IF_NOT(ClipIndex != INDEX_NONE,
            TEXT("PlayClip: clip [{}] not found in the baked clip table of VatCollection [{}] (entity [{}])"),
            InRequest.Get_ClipName(), Collection, InHandle)
        { return; }

        const auto Now = ck_vat_processor::Get_CurrentWorldTime(InHandle);

        // Crossfade source = whatever was active; the shader's 2-state blend (Gate 2/3) reads the pair.
        InCurrent._PrevClipIndex = InCurrent._ActiveClipIndex;
        InCurrent._PrevClipStartTime = InCurrent._PlaybackStartTime;
        InCurrent._TransitionStartTime = Now;
        InCurrent._TransitionDuration = InRequest.Get_TransitionDuration();

        InCurrent._ActiveClipIndex = ClipIndex;
        InCurrent._ActiveLoopMode = InRequest.Get_LoopMode();
        InCurrent._PlayRate = InRequest.Get_PlayRate();
        InCurrent._PlaybackStartTime = Now;
        InCurrent._PausedLocalTime = FCk_Time{};
        InCurrent._FinishedDispatched = false;
    }

    auto
        FProcessor_Vat_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Vat_Params& InParams,
            FFragment_Vat_Current& InCurrent,
            const FCk_Request_Vat_Stop& InRequest)
        -> void
    {
        if (InCurrent._ActiveClipIndex == INDEX_NONE)
        { return; } // nothing playing — reference pose is already static

        if (InCurrent._PlayRate == 0.0f)
        { return; } // already frozen

        const auto Now = ck_vat_processor::Get_CurrentWorldTime(InHandle);
        const auto ElapsedLocalSeconds =
            (Now - InCurrent._PlaybackStartTime).Get_Seconds() * InCurrent._PlayRate;

        InCurrent._PausedLocalTime = FCk_Time{ElapsedLocalSeconds};
        InCurrent._PlayRate = 0.0f;
    }

    auto
        FProcessor_Vat_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Vat_Params& InParams,
            FFragment_Vat_Current& InCurrent,
            const FCk_Request_Vat_SetPlayRate& InRequest)
        -> void
    {
        if (InCurrent._ActiveClipIndex == INDEX_NONE)
        { return; }

        const auto NewRate = InRequest.Get_PlayRate();

        if (NewRate == InCurrent._PlayRate)
        { return; }

        const auto Now = ck_vat_processor::Get_CurrentWorldTime(InHandle);

        if (NewRate == 0.0f)
        {
            // Rate 0 == freeze at the current position (same contract as Stop).
            const auto ElapsedLocalSeconds =
                (Now - InCurrent._PlaybackStartTime).Get_Seconds() * InCurrent._PlayRate;
            InCurrent._PausedLocalTime = FCk_Time{ElapsedLocalSeconds};
            InCurrent._PlayRate = 0.0f;
            return;
        }

        // Rebase the start time so the playback position is preserved across the rate change
        // (frame = (Now - Start) * Rate must be continuous).
        const auto CurrentLocalSeconds = InCurrent._PlayRate == 0.0f
            ? InCurrent._PausedLocalTime.Get_Seconds()
            : (Now - InCurrent._PlaybackStartTime).Get_Seconds() * InCurrent._PlayRate;

        InCurrent._PlaybackStartTime = Now - FCk_Time{CurrentLocalSeconds / NewRate};
        InCurrent._PausedLocalTime = FCk_Time{};
        InCurrent._PlayRate = NewRate;
    }
}
