#pragma once

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"
#include "CkMacros/CkMacros.h"
#include "CkRecord/Record/CkRecord_Fragment.h"

#include "CkSignal/Public/CkSignal/CkSignal_Macros.h"
#include "CkSignal/Public/CkSignal/CkSignal_Utils.h"
#include "CkSignal/Public/CkSignal/CkSignal_Fragment.h"

#include "CkTimer/CkTimer_Fragment_Data.h"

#include "CkTimer_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Timer_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct FTag_Timer_NeedsUpdate {};
    struct FTag_Timer_Updated {};

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

    public:
        friend class FProcessor_Timer_HandleRequests;
        friend class FProcessor_Timer_Update;
        friend class FProcessor_Timer_Replicate;

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
        friend class FProcessor_Timer_HandleRequests;
        friend class UCk_Utils_Timer_UE;

    public:
        using RequestType = FCk_Request_Timer_Manipulate;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _ManipulateRequests;

    public:
        CK_PROPERTY_GET(_ManipulateRequests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct FFragment_RecordOfTimers : public FFragment_RecordOfEntities {};

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKTIMER_API, OnTimerReset, FCk_Delegate_Timer_MC, FCk_Handle, FCk_Chrono);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKTIMER_API, OnTimerStop, FCk_Delegate_Timer_MC, FCk_Handle, FCk_Chrono);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKTIMER_API, OnTimerPause, FCk_Delegate_Timer_MC, FCk_Handle, FCk_Chrono);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKTIMER_API, OnTimerResume, FCk_Delegate_Timer_MC, FCk_Handle, FCk_Chrono);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKTIMER_API, OnTimerDone, FCk_Delegate_Timer_MC, FCk_Handle, FCk_Chrono);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKTIMER_API, OnTimerUpdate, FCk_Delegate_Timer_MC, FCk_Handle, FCk_Chrono);
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Blueprintable)
class CKTIMER_API UCk_Fragment_Timer_Rep : public UCk_Ecs_ReplicatedObject_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY_FRAGMENT_REP(UCk_Fragment_Timer_Rep);

public:
    friend class ck::FProcessor_Timer_Replicate;

public:
    virtual auto GetLifetimeReplicatedProps(TArray<FLifetimeProperty>&) const -> void override;

public:
    UFUNCTION()
    void OnRep_Chrono();

private:
    UPROPERTY(ReplicatedUsing = OnRep_Chrono)
    FCk_Chrono _Chrono;
};

// --------------------------------------------------------------------------------------------------------------------
