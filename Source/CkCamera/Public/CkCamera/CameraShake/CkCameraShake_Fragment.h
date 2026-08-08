#pragma once

#include "CkCamera/CameraShake/CkCameraShake_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkRecord/Public/CkRecord/Record/CkRecord_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_CameraShake_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    using FFragment_CameraShake_Params = FCk_CameraShake_Spec;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKCAMERA_API FFragment_CameraShake_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_CameraShake_Requests);

    public:
        friend class FProcessor_CameraShake_HandleRequests;
        friend class UCk_Utils_CameraShake_UE;

    public:
        using PlayOnTargetRequestType = FCk_Request_CameraShake_PlayOnTarget;
        using PlayAtLocationRequestType = FCk_Request_CameraShake_PlayAtLocation;
        using PlayRequests = TArray<std::variant<PlayOnTargetRequestType, PlayAtLocationRequestType>>;

    private:
        PlayRequests _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES_TRANSIENT(FFragment_RecordOfCameraShakes, FCk_Handle_CameraShake);
}

// --------------------------------------------------------------------------------------------------------------------
