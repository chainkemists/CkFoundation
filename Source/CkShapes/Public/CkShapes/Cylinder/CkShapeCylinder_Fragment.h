#pragma once

#include "CkShapeCylinder_Fragment_Data.h"

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_ShapeCylinder_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    using FFragment_ShapeCylinder_Params = FCk_Fragment_ShapeCylinder_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSHAPES_API FFragment_ShapeCylinder_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_ShapeCylinder_Current);

    public:
        friend class FProcessor_ShapeCylinder_HandleRequests;
        friend class UCk_Utils_ShapeCylinder_UE;

    private:
        FCk_ShapeCylinder_Dimensions _Dimensions;

    public:
        CK_PROPERTY_GET(_Dimensions);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_ShapeCylinder_Current, _Dimensions);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSHAPES_API FFragment_ShapeCylinder_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_ShapeCylinder_Requests);

    public:
        friend class FProcessor_ShapeCylinder_HandleRequests;
        friend class UCk_Utils_ShapeCylinder_UE;

    public:
        using RequestType = std::variant<FCk_Request_ShapeCylinder_UpdateDimensions>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };
}

// --------------------------------------------------------------------------------------------------------------------
