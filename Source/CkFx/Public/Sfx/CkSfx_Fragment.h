#pragma once

#include "Sfx/CkSfx_Fragment_Data.h"

#include "CkRecord/Record/CkRecord_Fragment.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "CkResourceLoader/CkResourceLoader_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Sfx_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_Sfx_NeedsSetup);
    CK_DEFINE_ECS_TAG(FTag_Sfx_PendingAssetLoad);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKFX_API FFragment_Sfx_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Sfx_Params);

    public:
        using ParamsType = FCk_Sfx_Spec;

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
        friend class FProcessor_Sfx_Setup;
        friend class FProcessor_Sfx_HandleRequests;
        friend class FProcessor_Sfx_EndPlay;
        friend class UCk_Utils_Sfx_UE;

    private:
        // The GC root for the sfx's resolved cue + settings: the batch's streamable handle keeps
        // them loaded for exactly as long as this fragment holds it (reset at EndPlay).
        FCk_ResourceLoader_RootedAssetBatch _LoadedAssets;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKFX_API FFragment_Sfx_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_Sfx_Requests);

    public:
        friend class FProcessor_Sfx_HandleRequests;
        friend class UCk_Utils_Sfx_UE;

    public:
        using RequestType = std::variant<
            FCk_Request_Sfx_PlayAttached,
            FCk_Request_Sfx_PlayAtLocation
        >;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES_TRANSIENT(FFragment_RecordOfSfx, FCk_Handle_Sfx);

    // --------------------------------------------------------------------------------------------------------------------

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_Sfx_Requests);
}

// --------------------------------------------------------------------------------------------------------------------
