#pragma once

#include <variant>

#include "CkTransform_Fragment_Params.h"

#include "CkMacros/CkMacros.h"

#include "CkTypeTraits/CkTypeTraits.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Transform_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <typename T_ConstOrNonConst>
    struct TFragment_Transform
    {
    public:
        using ThisType = TFragment_Transform<T_ConstOrNonConst>;
        using MutabilityPolicy = policy::TMutability<T_ConstOrNonConst>;

    public:
        friend class FCk_Processor_Transform_HandleRequests;
        friend UCk_Utils_Transform_UE;

    public:
        TFragment_Transform() = default;
        explicit TFragment_Transform(FTransform InTransform);

    private:
        FTransform _Transform;

    public:
        CK_PROPERTY_GET(_Transform);
    };

    // --------------------------------------------------------------------------------------------------------------------

    using FCk_Fragment_Transform_Current = TFragment_Transform<type_traits::NonConst>;

    using FCk_Fragment_ImmutableTransform_Current = TFragment_Transform<type_traits::Const>;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKECS_API FCk_Fragment_Transform_Requests
    {
        CK_GENERATED_BODY(FCk_Fragment_Transform_Requests);

    public:
        friend class FCk_Processor_Transform_HandleRequests;
        friend class UCk_Utils_Transform_UE;

    public:
        using RotationRequestType = std::variant<FCk_Request_Transform_SetRotation, FCk_Request_Transform_AddRotationOffset>;
        using RotationRequestList = TOptional<RotationRequestType>;

        using LocationRequestType = std::variant<FCk_Request_Transform_SetLocation, FCk_Request_Transform_AddLocationOffset>;
        using LocationRequestList = TOptional<LocationRequestType>;

        using ScaleRequestType    = FCk_Request_Transform_SetScale;
        using ScaleRequestList    = TOptional<ScaleRequestType>;

    private:
        RotationRequestList _RotationRequests;
        LocationRequestList _LocationRequests;
        ScaleRequestList _ScaleRequests;

    public:
        CK_PROPERTY_GET(_RotationRequests);
        CK_PROPERTY_GET(_LocationRequests);
        CK_PROPERTY_GET(_ScaleRequests);
    };
}

// --------------------------------------------------------------------------------------------------------------------
// Definitions

namespace ck
{
    template <typename T_ConstOrNonConst>
    TFragment_Transform<T_ConstOrNonConst>::
        TFragment_Transform(
            FTransform InTransform)
        : _Transform(InTransform)
    {
    }
}

// --------------------------------------------------------------------------------------------------------------------

