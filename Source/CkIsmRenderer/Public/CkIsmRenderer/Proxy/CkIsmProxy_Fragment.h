#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <NativeGameplayTags.h>

#include "CkIsmRenderer/Proxy/CkIsmProxy_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_IsmProxy_UE;

// --------------------------------------------------------------------------------------------------------------------

UE_DECLARE_GAMEPLAY_TAG_EXTERN(FTag_Ism_Renderer_Defaults_Gnomon);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(FTag_Ism_Renderer_Defaults_Entity);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_IsmProxy_NeedsSetup);
    CK_DEFINE_ECS_TAG(FTag_IsmProxy_NeedsInstanceAdded);
    CK_DEFINE_ECS_TAG(FTag_IsmProxy_Disabled);
    CK_DEFINE_ECS_TAG(FTag_IsmProxy_Movable);

    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_IsmProxy_Params = FCk_Fragment_IsmProxy_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    struct CKISMRENDERER_API FFragment_IsmProxy_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_IsmProxy_Current);

    public:
        friend class FProcessor_IsmProxy_Setup;
        friend class FProcessor_IsmProxy_EndPlay;
        friend class FProcessor_IsmProxy_AddInstance;
        friend class FProcessor_IsmProxy_HandleRequests;

    private:
        FPrimitiveInstanceId _IsmInstanceIndex;
        TArray<float> _CustomInstanceDataValues;

    public:
        CK_PROPERTY_GET(_IsmInstanceIndex);
        CK_PROPERTY_GET(_CustomInstanceDataValues);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKISMRENDERER_API FFragment_IsmProxy_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_IsmProxy_Requests);

    public:
        using SetCustomInstanceDataRequestType = FCk_Request_IsmProxy_SetCustomInstanceData;
        using SetCustomInstanceDataValueRequestType = FCk_Request_IsmProxy_SetCustomInstanceDataValue;
        using SetCustomPrimitiveDataRequestType = FCk_Request_IsmProxy_SetCustomPrimitiveData;
        using EnableDisableRequestType = FCk_Request_IsmProxy_EnableDisable;

        using RequestType = std::variant<
            SetCustomInstanceDataRequestType,
            SetCustomInstanceDataValueRequestType,
            SetCustomPrimitiveDataRequestType,
            EnableDisableRequestType>;
        using RequestList = TArray<RequestType>;

    public:
        friend class FProcessor_IsmProxy_HandleRequests;
        friend class UCk_Utils_IsmProxy_UE;

    private:
        RequestList _Requests;
    };
}

// --------------------------------------------------------------------------------------------------------------------
