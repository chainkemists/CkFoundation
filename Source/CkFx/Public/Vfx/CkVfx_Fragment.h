#pragma once

#include "CkVfx_Fragment_Data.h"

#include "CkRecord/Record/CkRecord_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Vfx_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct CKFX_API FFragment_Vfx_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Vfx_Params);

    public:
        using ParamsType = FCk_Fragment_Vfx_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Vfx_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKFX_API FFragment_Vfx_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_Vfx_Current);

    public:
        friend class FProcessor_Vfx_HandleRequests;
        friend class UCk_Utils_Vfx_UE;

    private:
        // Add your properties here
        int32 _DummyProperty = 0;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKFX_API FFragment_Vfx_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_Vfx_Requests);

    public:
        friend class FProcessor_Vfx_HandleRequests;
        friend class UCk_Utils_Vfx_UE;

    private:
        // Add your requests here
        int32 _DummyProperty = 0;
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfVfx, FCk_Handle_Vfx);
}

// --------------------------------------------------------------------------------------------------------------------
