#pragma once

#include "CkTween_Fragment_Data.h"

#include "CkEcs/Signal/CkSignal_Macros.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_Tween_NeedsSetup);
    CK_DEFINE_ECS_TAG(FTag_Tween_Playing);
    CK_DEFINE_ECS_TAG(FTag_Tween_Paused);
    CK_DEFINE_ECS_TAG(FTag_Tween_Completed);
    CK_DEFINE_ECS_TAG(FTag_Tween_InYoyoDelay);
    CK_DEFINE_ECS_TAG(FTag_Tween_QueueHead); // Marks the first tween in a queue

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_Tween_Params = FCk_Fragment_Tween_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKTWEEN_API FFragment_Tween_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_Tween_Current);

    private:
        float _CurrentTime = 0.0f;
        float _YoyoDelayTimer = 0.0f;
        ECk_TweenState _State = ECk_TweenState::Playing;
        int32 _CurrentLoop = 0;
        bool _IsReversed = false;
        FCk_TweenValue _CurrentValue;
        float _TimeMultiplier = 1.0f;

    public:
        CK_PROPERTY(_CurrentTime);
        CK_PROPERTY(_YoyoDelayTimer);
        CK_PROPERTY(_State);
        CK_PROPERTY(_CurrentLoop);
        CK_PROPERTY(_IsReversed);
        CK_PROPERTY(_CurrentValue);
        CK_PROPERTY(_TimeMultiplier);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKTWEEN_API FFragment_Tween_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_Tween_Requests);

        using RequestType = std::variant<
            FCk_Request_Tween_Pause,
            FCk_Request_Tween_Resume,
            FCk_Request_Tween_Stop,
            FCk_Request_Tween_Restart,
            FCk_Request_Tween_SetTimeMultiplier
        >;

    private:
        TArray<RequestType> _Requests;

    public:
        CK_PROPERTY(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKTWEEN_API,
        OnTweenUpdate,
        FCk_Delegate_Tween_OnUpdate_MC,
        FCk_Handle_Tween,
        FCk_Tween_Payload_OnUpdate);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKTWEEN_API,
        OnTweenComplete,
        FCk_Delegate_Tween_OnComplete_MC,
        FCk_Handle_Tween,
        FCk_Tween_Payload_OnComplete);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKTWEEN_API,
        OnTweenLoop,
        FCk_Delegate_Tween_OnLoop_MC,
        FCk_Handle_Tween,
        FCk_Tween_Payload_OnLoop);
}

// --------------------------------------------------------------------------------------------------------------------
