#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"
#include "CkEcs/Tag/CkTag.h"
#include "CkEcs/Signal/CkSignal_Fragment.h"
#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkEcs/Signal/CkSignal_Utils.h"

#include "CkVat/CkVat_Fragment_Data.h"

#include "CkIsmRenderer/Proxy/CkIsmProxy_Fragment_Data.h"

#include <variant>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Vat_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_Vat_NeedsSetup);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_Vat_Params = FCk_Fragment_Vat_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    // Playback state under the GPU-time-driven model: the GPU derives the frame from
    // (WorldTime - _PlaybackStartTime) * _PlayRate — state is written on CHANGE only, never per frame.
    // Gate 3 packs this into per-instance custom data; the crossfade pair (_PrevClip*) feeds the shader's
    // 2-state blend.
    struct CKVAT_API FFragment_Vat_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_Vat_Current);

    public:
        friend class FProcessor_Vat_Setup;
        friend class FProcessor_Vat_HandleRequests;
        friend class FProcessor_Vat_FireSignals;

    private:
        // Index into the collection's SERIALIZED baked clip table; INDEX_NONE = reference pose (row 0).
        int32 _ActiveClipIndex = INDEX_NONE;
        ECk_Vat_LoopMode _ActiveLoopMode = ECk_Vat_LoopMode::Loop;
        // A rate change rebases _PlaybackStartTime so the playback position is preserved (Gate 3 packing
        // contract) — consumers must not assume _PlaybackStartTime is the original play call's timestamp.
        float _PlayRate = 1.0f;
        FCk_Time _PlaybackStartTime;

        // Crossfade source state (valid while a transition is in flight). The source keeps playing
        // at ITS rate/loop-mode during the fade, so those are captured alongside the start time.
        int32 _PrevClipIndex = INDEX_NONE;
        FCk_Time _PrevClipStartTime;
        float _PrevPlayRate = 1.0f;
        ECk_Vat_LoopMode _PrevLoopMode = ECk_Vat_LoopMode::Loop;
        FCk_Time _TransitionStartTime;
        FCk_Time _TransitionDuration;

        // The ISM instance rendering this entity (composed by Setup; per-instance custom-data
        // pushes go through it).
        FCk_Handle_IsmProxy _IsmProxy;

        // Clip-local playback position captured by Stop (meaningful only while _PlayRate == 0); the GPU
        // packing holds the frame via this constant while the rate multiplier is zero.
        FCk_Time _PausedLocalTime;

        // OnClipFinished (Once clips) dispatched for the active clip.
        bool _FinishedDispatched = false;

    public:
        CK_PROPERTY_GET(_ActiveClipIndex);
        CK_PROPERTY_GET(_ActiveLoopMode);
        CK_PROPERTY_GET(_PlayRate);
        CK_PROPERTY_GET(_PlaybackStartTime);
        CK_PROPERTY_GET(_PrevClipIndex);
        CK_PROPERTY_GET(_PrevClipStartTime);
        CK_PROPERTY_GET(_PrevPlayRate);
        CK_PROPERTY_GET(_PrevLoopMode);
        CK_PROPERTY_GET(_TransitionStartTime);
        CK_PROPERTY_GET(_TransitionDuration);
        CK_PROPERTY_GET(_IsmProxy);
        CK_PROPERTY_GET(_PausedLocalTime);
        CK_PROPERTY_GET(_FinishedDispatched);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKVAT_API FFragment_Vat_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_Vat_Requests);

    public:
        friend class FProcessor_Vat_HandleRequests;
        friend class UCk_Utils_Vat_UE;

    public:
        using PlayClipRequestType = FCk_Request_Vat_PlayClip;
        using StopRequestType = FCk_Request_Vat_Stop;
        using SetPlayRateRequestType = FCk_Request_Vat_SetPlayRate;

        using RequestType = std::variant<PlayClipRequestType, StopRequestType, SetPlayRateRequestType>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKVAT_API,
        Vat_OnClipFinished,
        FCk_Delegate_Vat_OnClipFinished,
        FCk_Handle_Vat,
        FName);

    // --------------------------------------------------------------------------------------------------------------------

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_Vat_Requests);
}

// --------------------------------------------------------------------------------------------------------------------
