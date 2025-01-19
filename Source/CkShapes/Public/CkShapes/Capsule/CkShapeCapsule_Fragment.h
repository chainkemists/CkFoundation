#pragma once

#include "CkShapeCapsule_Fragment_Data.h"

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"

#include "CkShapeCapsule_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_ShapeCapsule_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_ShapeCapsule_RequiresSetup);
    CK_DEFINE_ECS_TAG(FTag_ShapeCapsule_Updated);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSHAPES_API FFragment_ShapeCapsule_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_ShapeCapsule_Params);

    public:
        using ParamsType = FCk_Fragment_ShapeCapsule_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_ShapeCapsule_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSHAPES_API FFragment_ShapeCapsule_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_ShapeCapsule_Current);

    public:
        friend class FProcessor_ShapeCapsule_Setup;
        friend class FProcessor_ShapeCapsule_HandleRequests;
        friend class FProcessor_ShapeCapsule_Teardown;
        friend class UCk_Utils_ShapeCapsule_UE;

    private:
        FCk_Fragment_ShapeCapsule_ShapeData _CurrentShape;

    public:
        CK_PROPERTY_GET(_CurrentShape);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSHAPES_API FFragment_ShapeCapsule_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_ShapeCapsule_Requests);

    public:
        friend class FProcessor_ShapeCapsule_HandleRequests;
        friend class UCk_Utils_ShapeCapsule_UE;

    public:
        using RequestType = std::variant<FCk_Request_ShapeCapsule_UpdateShape>;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_ShapeCapsule_Replicate;
}

UCLASS(Blueprintable)
class CKSHAPES_API UCk_Fragment_ShapeCapsule_Rep : public UCk_Ecs_ReplicatedObject_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY_FRAGMENT_REP(UCk_Fragment_ShapeCapsule_Rep);

public:
    friend class ck::FProcessor_ShapeCapsule_Replicate;
};

// --------------------------------------------------------------------------------------------------------------------
