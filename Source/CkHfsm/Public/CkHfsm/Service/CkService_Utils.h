#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkHFSM/Service/CkService_Fragment.h"

#include "CkService_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_Service_Setup;
    class FProcessor_Service_Enter;
    class FProcessor_Service_Exit;
    class FProcessor_Service_Update;
}

// --------------------------------------------------------------------------------------------------------------------

// NOT Blueprint-exposed - internal use only
UCLASS(NotBlueprintable)
class CKHFSM_API UCk_Utils_Service_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Service_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Service);

public:
    friend class ck::FProcessor_Service_Setup;
    friend class ck::FProcessor_Service_Enter;
    friend class ck::FProcessor_Service_Exit;
    friend class ck::FProcessor_Service_Update;

public:
    // Feature Management (C++ only)
    static FCk_Handle_Service
    Create(
        FCk_Handle_State& InStateHandle);

    static bool
    Has(
        const FCk_Handle& InHandle);

    static FCk_Handle_Service
    Cast(
        FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    static FCk_Handle_Service
    CastChecked(
        FCk_Handle InHandle);

public:
    // Commands (C++ only)
    static FCk_Handle_Service
    Request_Start(
        FCk_Handle_Service& InHandle);

    static FCk_Handle_Service
    Request_Stop(
        FCk_Handle_Service& InHandle);

public:
    // Query (C++ only)
    static bool
    Get_IsWorkDone(
        const FCk_Handle_Service& InHandle);

public:
    // Signals (C++ only)
    static FCk_Handle_Service
    BindTo_OnStart(
        FCk_Handle_Service& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Service& InDelegate);

    static FCk_Handle_Service
    BindTo_OnStop(
        FCk_Handle_Service& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Service& InDelegate);

    static FCk_Handle_Service
    BindTo_OnWorkDone(
        FCk_Handle_Service& InHandle,
        ECk_Signal_BindingPolicy InBindingPolicy,
        const FCk_Delegate_Service& InDelegate);

    static FCk_Handle_Service
    UnbindFrom_OnStart(
        FCk_Handle_Service& InHandle,
        const FCk_Delegate_Service& InDelegate);

    static FCk_Handle_Service
    UnbindFrom_OnStop(
        FCk_Handle_Service& InHandle,
        const FCk_Delegate_Service& InDelegate);

    static FCk_Handle_Service
    UnbindFrom_OnWorkDone(
        FCk_Handle_Service& InHandle,
        const FCk_Delegate_Service& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------