#pragma once

#include "Vfx/CkVfx_Fragment_Data.h"

#include "CkRecord/Record/CkRecord_Fragment.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "CkResourceLoader/CkResourceLoader_Fragment_Data.h"

#include <variant>

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Vfx_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_Vfx_NeedsSetup);

    // Present while the vfx's particle-system preload batch is still loading (Setup re-polls,
    // keeping NeedsSetup). Observability only — nothing gates on it.
    CK_DEFINE_ECS_TAG(FTag_Vfx_PendingAssetLoad);

    // --------------------------------------------------------------------------------------------------------------------

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
        friend class FProcessor_Vfx_Setup;
        friend class FProcessor_Vfx_HandleRequests;
        friend class FProcessor_Vfx_EndPlay;
        friend class UCk_Utils_Vfx_UE;

    private:
        // Roots the resolved particle system from Setup's kick until EndPlay — GC does not trace
        // fragments, and spawned Niagara components are fire-and-forget (world-rooted), so this
        // batch is what keeps the soft-authored system resident between plays.
        FCk_ResourceLoader_RootedAssetBatch _LoadedAssets;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKFX_API FFragment_Vfx_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_Vfx_Requests);

    public:
        friend class FProcessor_Vfx_HandleRequests;
        friend class FProcessor_Vfx_CancelPendingRequests;
        friend class UCk_Utils_Vfx_UE;

    public:
        using RequestType = std::variant<
            FCk_Request_Vfx_PlayAttached,
            FCk_Request_Vfx_PlayAtLocation>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES_TRANSIENT(FFragment_RecordOfVfx, FCk_Handle_Vfx);

    // --------------------------------------------------------------------------------------------------------------------

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_Vfx_Requests);
}

// --------------------------------------------------------------------------------------------------------------------
