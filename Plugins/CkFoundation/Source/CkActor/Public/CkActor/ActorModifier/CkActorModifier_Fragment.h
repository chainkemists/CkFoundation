#pragma once

#include <variant>

#include "CkActorModifier_Fragment_Params.h"

#include "CkActor/ActorInfo/CkActorInfo_Fragment_Params.h"

#include "CkMacros/CkMacros.h"

#include "CkActorModifier_Fragment.generated.h"

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

}

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKACTOR_API UCk_Fragment_ActorModifier_Rep : public UCk_Ecs_ReplicatedObject
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Fragment_ActorModifier_Rep);

public:
    UFUNCTION(NetMulticast, Reliable)
    void
    Request_SetLocation(
        const FCk_Request_ActorModifier_SetLocation& InRequest);

    UFUNCTION(NetMulticast, Reliable)
    void
    Request_AddLocationOffset(
        const FCk_Request_ActorModifier_AddLocationOffset& InRequest);

public:
    UFUNCTION(NetMulticast, Reliable)
    void
    Request_SetRotation(
        const FCk_Request_ActorModifier_SetRotation& InRequest);

    UFUNCTION(NetMulticast, Reliable)
    void
    Request_AddRotationOffset(
        const FCk_Request_ActorModifier_AddRotationOffset& InRequest);

public:
    UFUNCTION(NetMulticast, Reliable)
    void
    Request_SetScale(
        const FCk_Request_ActorModifier_SetScale& InRequest);

    UFUNCTION(NetMulticast, Reliable)
    void
    Request_SetTransform(
        const FCk_Request_ActorModifier_SetTransform& InRequest);
};

// --------------------------------------------------------------------------------------------------------------------
