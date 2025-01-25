#pragma once

#include "CkShapeSphere_Fragment_Data.h"

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_ShapeSphere_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    using FFragment_ShapeSphere_Params = FCk_Fragment_ShapeSphere_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSHAPES_API FFragment_ShapeSphere_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_ShapeSphere_Current);

    public:
        friend class FProcessor_ShapeSphere_HandleRequests;
        friend class UCk_Utils_ShapeSphere_UE;

    private:
        FCk_ShapeSphere_Dimensions _Dimensions;

    public:
        CK_PROPERTY_GET(_Dimensions);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_ShapeSphere_Current, _Dimensions);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSHAPES_API FFragment_ShapeSphere_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_ShapeSphere_Requests);

    public:
        friend class FProcessor_ShapeSphere_HandleRequests;
        friend class UCk_Utils_ShapeSphere_UE;

    public:
        using RequestType = std::variant<FCk_Request_ShapeSphere_UpdateDimensions>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };
}

// --------------------------------------------------------------------------------------------------------------------
