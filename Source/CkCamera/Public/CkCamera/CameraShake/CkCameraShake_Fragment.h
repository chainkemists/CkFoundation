#pragma once

#include "CkCameraShake_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkRecord/Public/CkRecord/Record/CkRecord_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_CameraShake_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct CKCAMERA_API FFragment_CameraShake_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_CameraShake_Params);

    public:
        using ParamsType = FCk_Fragment_CameraShake_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_CameraShake_Params, _Params);
    };

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
        PlayRequests _PlayRequests;

    public:
        CK_PROPERTY_GET(_PlayRequests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct FFragment_RecordOfCameraShakes : public FFragment_RecordOfEntities
    {
        using FFragment_RecordOfEntities::FFragment_RecordOfEntities;
    };
}

// --------------------------------------------------------------------------------------------------------------------
