#pragma once

#include "CkVfxCue_Fragment_Data.h"

#include "CkEcs/Signal/CkSignal_Macros.h"

#include <NiagaraComponent.h>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_VfxCue_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_VfxCue_NeedsSetup);
    CK_DEFINE_ECS_TAG(FTag_VfxCue_IsPlaying);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKVFX_API FFragment_VfxCue_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_VfxCue_Current);

    public:
        friend class FProcessor_VfxCue_Setup;
        friend class FProcessor_VfxCue_HandleRequests;
        friend class FProcessor_VfxCue_EndPlay;
        friend class FProcessor_VfxCue_EffectLifetimeMonitor;
        friend class UCk_Utils_VfxCue_UE;

    private:
        TStrongObjectPtr<UNiagaraComponent> _NiagaraComponent;
        FCk_Time _EffectStartTime = FCk_Time{0.0f};
        FCk_Time _EffectDuration = FCk_Time{0.0f};
        bool _HasFiredFinished = false;

    public:
        CK_PROPERTY_GET(_NiagaraComponent);
        CK_PROPERTY_GET(_EffectStartTime);
        CK_PROPERTY_GET(_EffectDuration);
        CK_PROPERTY_GET(_HasFiredFinished);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKVFX_API FFragment_VfxCue_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_VfxCue_Requests);

    public:
        friend class FProcessor_VfxCue_HandleRequests;
        friend class UCk_Utils_VfxCue_UE;

    public:
        using RequestType = std::variant<
            FCk_Request_VfxCue_Play,
            FCk_Request_VfxCue_Stop
        >;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKVFX_API,
        OnVfxCue_Started,
        FCk_Delegate_VfxCue_OnStarted_MC,
        FCk_Handle_VfxCue);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKVFX_API,
        OnVfxCue_Finished,
        FCk_Delegate_VfxCue_OnFinished_MC,
        FCk_Handle_VfxCue);
}

// --------------------------------------------------------------------------------------------------------------------
