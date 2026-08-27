#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "CkQueue/Coordinator/CkQueueCoordinator_Fragment_Data.h"

#include <variant>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_QueueCoordinator_UE;

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_QueueCoordinator_NeedsSetup);
    CK_DEFINE_ECS_TAG(FTag_QueueCoordinator_NeedsReconcile);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_QueueCoordinator_Params = FCk_Fragment_QueueCoordinator_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKQUEUE_API FFragment_QueueCoordinator_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_QueueCoordinator_Current);

    public:
        friend class FProcessor_QueueCoordinator_Setup;
        friend class FProcessor_QueueCoordinator_HandleRequests;
        friend class FProcessor_QueueCoordinator_Reconcile;
        friend class FProcessor_QueueCoordinator_EndPlay;

    private:
        TArray<FCk_QueueCoordinator_Service> _Services;
        int32 _Revision = 0;
        int32 _NextRegistrationOrdinal = 1;

    public:
        CK_PROPERTY_GET(_Services);
        CK_PROPERTY_GET(_Revision);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKQUEUE_API FFragment_QueueCoordinator_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_QueueCoordinator_Requests);

    public:
        friend class FProcessor_QueueCoordinator_HandleRequests;
        friend class UCk_Utils_QueueCoordinator_UE;

    public:
        using RequestType = std::variant<
            FCk_Request_QueueCoordinator_RegisterQueue,
            FCk_Request_QueueCoordinator_UnregisterQueue,
            FCk_Request_QueueCoordinator_SelectQueue>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_QueueCoordinator_Requests);
}

// --------------------------------------------------------------------------------------------------------------------
