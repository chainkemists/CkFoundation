#pragma once

#include "CkMacros/CkMacros.h"

#include "CkSignal/Public/CkSignal/CkSignal_Macros.h"
#include "CkSignal/Public/CkSignal/CkSignal_Utils.h"
#include "CkSignal/Public/CkSignal/CkSignal_Fragment.h"

#include "CkTimer/CkTimer_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct FTag_Timer_Update {};

    // --------------------------------------------------------------------------------------------------------------------

    struct CKTIMER_API FFragment_Timer_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Timer_Params);

    public:
        using ParamsType = FCk_Fragment_Timer_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Timer_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKTIMER_API FFragment_Timer_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_Timer_Current);

    private:
        FCk_Chrono _Chrono;

    public:
        CK_PROPERTY_GET(_Chrono);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Timer_Current, _Chrono);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKTIMER_API FFragment_Timer_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_Timer_Requests);

    public:
        using RequestType = FCk_Request_Timer_Manipulate;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _ManipulateRequests;

    public:
        CK_PROPERTY_GET(_ManipulateRequests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKTIMER_API, OnTimerReset, FCk_Delegate_Timer_MC, FCk_Handle, FCk_Chrono);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKTIMER_API, OnTimerStop, FCk_Delegate_Timer_MC, FCk_Handle, FCk_Chrono);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKTIMER_API, OnTimerPause, FCk_Delegate_Timer_MC, FCk_Handle, FCk_Chrono);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKTIMER_API, OnTimerResume, FCk_Delegate_Timer_MC, FCk_Handle, FCk_Chrono);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKTIMER_API, OnTimerDone, FCk_Delegate_Timer_MC, FCk_Handle, FCk_Chrono);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKTIMER_API, OnTimerUpdate, FCk_Delegate_Timer_MC, FCk_Handle, FCk_Chrono);
}