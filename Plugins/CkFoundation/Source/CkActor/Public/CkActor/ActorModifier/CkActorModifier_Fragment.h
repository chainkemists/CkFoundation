#pragma once

#include <variant>

#include "CkActorModifier_Fragment_Params.h"

#include "CkMacros/CkMacros.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_ActorModifier_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct CKACTOR_API FCk_Fragment_ActorModifier_LocationRequests
    {
    public:
        CK_GENERATED_BODY(FCk_Fragment_ActorModifier_LocationRequests);

    public:
        friend class FCk_Processor_ActorModifier_Location_HandleRequests;
        friend class UCk_Utils_ActorModifier_UE;

    public:
        using RequestType = std::variant<
            FCk_Request_ActorModifier_SetLocation,
            FCk_Request_ActorModifier_AddLocationOffset>;

        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKACTOR_API FCk_Fragment_ActorModifier_RotationRequests
    {
    public:
        CK_GENERATED_BODY(FCk_Fragment_ActorModifier_RotationRequests);

    public:
        friend class FCk_Processor_ActorModifier_Rotation_HandleRequests;
        friend class UCk_Utils_ActorModifier_UE;

    public:
        using RequestType = std::variant<
            FCk_Request_ActorModifier_SetRotation,
            FCk_Request_ActorModifier_AddRotationOffset>;

        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKACTOR_API FCk_Fragment_ActorModifier_ScaleRequests
    {
    public:
        CK_GENERATED_BODY(FCk_Fragment_ActorModifier_ScaleRequests);

    public:
        friend class FCk_Processor_ActorModifier_Scale_HandleRequests;
        friend class UCk_Utils_ActorModifier_UE;

    public:
        using RequestType = FCk_Request_ActorModifier_SetScale;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKACTOR_API FCk_Fragment_ActorModifier_TransformRequests
    {
    public:
        CK_GENERATED_BODY(FCk_Fragment_ActorModifier_TransformRequests);

    public:
        friend class FCk_Processor_ActorModifier_Transform_HandleRequests;
        friend class UCk_Utils_ActorModifier_UE;

    public:
        using RequestType = FCk_Request_ActorModifier_SetTransform;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKACTOR_API FCk_Fragment_ActorModifier_SpawnActorRequests
    {
    public:
        CK_GENERATED_BODY(FCk_Fragment_ActorModifier_SpawnActorRequests);

    public:
        friend class FCk_Processor_ActorModifier_SpawnActor_HandleRequests;
        friend class UCk_Utils_ActorModifier_UE;

    public:
        using RequestType = FCk_Request_ActorModifier_SpawnActor;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    private:
        CK_PROPERTY_GET_NON_CONST(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKACTOR_API FCk_Fragment_ActorModifier_AddActorComponentRequests
    {
    public:
        CK_GENERATED_BODY(FCk_Fragment_ActorModifier_AddActorComponentRequests);

    public:
        friend class FCk_Processor_ActorModifier_AddActorComponent_HandleRequests;
        friend class UCk_Utils_ActorModifier_UE;

    public:
        using RequestType = FCk_Request_ActorModifier_AddActorComponent;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    private:
        CK_PROPERTY_GET_NON_CONST(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

}

// --------------------------------------------------------------------------------------------------------------------
