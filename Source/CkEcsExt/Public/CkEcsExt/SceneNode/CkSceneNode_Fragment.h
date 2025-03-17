#pragma once

#include "CkSceneNode_Fragment_Data.h"

#include "CkEcsExt/EntityHolder/CkEntityHolder_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_SceneNode_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_SceneNode_Layer0);
    CK_DEFINE_ECS_TAG(FTag_SceneNode_Layer1);
    CK_DEFINE_ECS_TAG(FTag_SceneNode_Layer2);
    CK_DEFINE_ECS_TAG(FTag_SceneNode_Layer3);
    CK_DEFINE_ECS_TAG(FTag_SceneNode_Layer4);
    CK_DEFINE_ECS_TAG(FTag_SceneNode_Layer5);
    CK_DEFINE_ECS_TAG(FTag_SceneNode_Layer6);
    CK_DEFINE_ECS_TAG(FTag_SceneNode_Layer7);
    CK_DEFINE_ECS_TAG(FTag_SceneNode_Layer8);
    CK_DEFINE_ECS_TAG(FTag_SceneNode_Layer9);

    // --------------------------------------------------------------------------------------------------------------------

    // --------------------------------------------------------------------------------------------------------------------

    struct CKECSEXT_API FFragment_SceneNode_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_SceneNode_Current);

    public:
        friend class FProcessor_SceneNode_Setup;
        friend class FProcessor_SceneNode_HandleRequests;
        friend class FProcessor_SceneNode_Teardown;
        friend class UCk_Utils_SceneNode_UE;

    private:
        FTransform _RelativeTransform;

    public:
        CK_PROPERTY_GET(_RelativeTransform);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKECSEXT_API FFragment_SceneNode_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_SceneNode_Requests);

    public:
        friend class FProcessor_SceneNode_HandleRequests;
        friend class UCk_Utils_SceneNode_UE;

    public:
        using RequestType = std::variant
        <
            FCk_Request_SceneNode_UpdateRelativeTransform
        >;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_ENTITY_HOLDER_AND_UTILS(USceneNodeParent_Utils, SceneNodeParent);

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------