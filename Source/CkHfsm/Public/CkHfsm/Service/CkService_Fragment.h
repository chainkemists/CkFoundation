#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Signal/CkSignal_Macros.h"
#include "CkRecord/Record/CkRecord_Fragment.h"

#include "CkService_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Lifecycle tags
    CK_DEFINE_ECS_TAG(FTag_Service_Setup);
    CK_DEFINE_ECS_TAG(FTag_Service_Enter);
    CK_DEFINE_ECS_TAG(FTag_Service_Exit);
    CK_DEFINE_ECS_TAG(FTag_Service_Update);
    CK_DEFINE_ECS_TAG(FTag_Service_WorkDone);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKHFSM_API FFragment_Service_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_Service_Current);

    public:
        friend class FProcessor_Service_Setup;
        friend class FProcessor_Service_Enter;
        friend class FProcessor_Service_Exit;
        friend class FProcessor_Service_Update;

    private:
        // Derived service implementations can extend this
        int32 _ReservedForFutureUse = 0;

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Service_Current);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKHFSM_API FFragment_Service_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_Service_Requests);

    public:
        friend class FProcessor_Service_HandleRequests;
        friend class UCk_Utils_Service_UE;

    public:
        using RequestType = std::variant<FCk_Request_Service_Command>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Signals
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKHFSM_API, OnServiceStart, FCk_Delegate_Service_MC, 
        FCk_Handle_Service, FCk_Time);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKHFSM_API, OnServiceStop, FCk_Delegate_Service_MC, 
        FCk_Handle_Service, FCk_Time);
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKHFSM_API, OnServiceWorkDone, FCk_Delegate_Service_MC, 
        FCk_Handle_Service, FCk_Time);
}

// --------------------------------------------------------------------------------------------------------------------