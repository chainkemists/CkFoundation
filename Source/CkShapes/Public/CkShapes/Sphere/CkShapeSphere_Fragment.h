#pragma once

#include "CkShapeSphere_Fragment_Data.h"

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"

#include "CkShapeSphere_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_ShapeSphere_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_ShapeSphere_RequiresSetup);
    CK_DEFINE_ECS_TAG(FTag_ShapeSphere_Updated);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSHAPES_API FFragment_ShapeSphere_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_ShapeSphere_Params);

    public:
        using ParamsType = FCk_Fragment_ShapeSphere_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_ShapeSphere_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSHAPES_API FFragment_ShapeSphere_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_ShapeSphere_Current);

    public:
        friend class FProcessor_ShapeSphere_Setup;
        friend class FProcessor_ShapeSphere_HandleRequests;
        friend class FProcessor_ShapeSphere_Teardown;
        friend class UCk_Utils_ShapeSphere_UE;

    private:
        FCk_Fragment_ShapeSphere_ShapeData _CurrentShape;

    public:
        CK_PROPERTY_GET(_CurrentShape);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKSHAPES_API FFragment_ShapeSphere_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_ShapeSphere_Requests);

    public:
        friend class FProcessor_ShapeSphere_HandleRequests;
        friend class UCk_Utils_ShapeSphere_UE;

    public:
        using RequestType = std::variant<FCk_Request_ShapeSphere_UpdateShape>;
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
    class FProcessor_ShapeSphere_Replicate;
}

UCLASS(Blueprintable)
class CKSHAPES_API UCk_Fragment_ShapeSphere_Rep : public UCk_Ecs_ReplicatedObject_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY_FRAGMENT_REP(UCk_Fragment_ShapeSphere_Rep);

public:
    friend class ck::FProcessor_ShapeSphere_Replicate;
};

// --------------------------------------------------------------------------------------------------------------------
