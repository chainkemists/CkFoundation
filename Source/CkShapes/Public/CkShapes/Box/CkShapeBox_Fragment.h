#pragma once

#include "CkShapeBox_Fragment_Data.h"

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"
#include "CkEcs/Signal/CkSignal_Macros.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_ShapeBox_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    using FFragment_ShapeBox_Params = FCk_Fragment_ShapeBox_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSHAPES_API FFragment_ShapeBox_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_ShapeBox_Current);

    public:
        friend class FProcessor_ShapeBox_HandleRequests;
        friend class UCk_Utils_ShapeBox_UE;

    private:
        FCk_ShapeBox_Dimensions _Dimensions;

    public:
        CK_PROPERTY_GET(_Dimensions);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_ShapeBox_Current, _Dimensions);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSHAPES_API FFragment_ShapeBox_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_ShapeBox_Requests);

    public:
        friend class FProcessor_ShapeBox_HandleRequests;
        friend class UCk_Utils_ShapeBox_UE;

    public:
        using RequestType = std::variant<FCk_Request_ShapeBox_UpdateDimensions>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKSHAPES_API, OnShapeBoxDimensionsChanged,
        FCk_Delegate_ShapeBox_OnDimensionsChanged, FCk_Handle_ShapeBox, FCk_ShapeBox_Dimensions);
}

// --------------------------------------------------------------------------------------------------------------------