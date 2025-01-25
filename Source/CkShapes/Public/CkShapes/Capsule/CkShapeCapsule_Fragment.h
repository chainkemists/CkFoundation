#pragma once

#include "CkShapeCapsule_Fragment_Data.h"

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_ShapeCapsule_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    using FFragment_ShapeCapsule_Params = FCk_Fragment_ShapeCapsule_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSHAPES_API FFragment_ShapeCapsule_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_ShapeCapsule_Current);

    public:
        friend class FProcessor_ShapeCapsule_HandleRequests;
        friend class UCk_Utils_ShapeCapsule_UE;

    private:
        FCk_ShapeCapsule_Dimensions _Dimensions;

    public:
        CK_PROPERTY_GET(_Dimensions);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_ShapeCapsule_Current, _Dimensions);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSHAPES_API FFragment_ShapeCapsule_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_ShapeCapsule_Requests);

    public:
        friend class FProcessor_ShapeCapsule_HandleRequests;
        friend class UCk_Utils_ShapeCapsule_UE;

    public:
        using RequestType = std::variant<FCk_Request_ShapeCapsule_UpdateDimensions>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };
}

// --------------------------------------------------------------------------------------------------------------------
