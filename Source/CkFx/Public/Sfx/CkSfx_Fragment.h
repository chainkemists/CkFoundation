#pragma once

#include "Sfx/CkSfx_Fragment_Data.h"

#include "CkEcs/Record/CkRecord_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Sfx_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct CKFX_API FFragment_Sfx_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Sfx_Params);

    public:
        using ParamsType = FCk_Fragment_Sfx_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Sfx_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKFX_API FFragment_Sfx_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_Sfx_Current);

    public:
        friend class FProcessor_Sfx_HandleRequests;
        friend class UCk_Utils_Sfx_UE;

    private:
        // Add your properties here
        int32 _DummyProperty = 0;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKFX_API FFragment_Sfx_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_Sfx_Requests);

    public:
        friend class FProcessor_Sfx_HandleRequests;
        friend class UCk_Utils_Sfx_UE;

    private:
        // Add your requests here
        int32 _DummyProperty = 0;
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfSfx, FCk_Handle_Sfx);
}

// --------------------------------------------------------------------------------------------------------------------
