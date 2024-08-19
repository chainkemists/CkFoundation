#pragma once

#include "CkGeometryCollection_Fragment_Data.h"

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"

#include "CkRecord/Record/CkRecord_Fragment.h"
#include "CkRecord/Record/CkRecord_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_GeometryCollection_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    struct CKCHAOS_API FFragment_GeometryCollection_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_GeometryCollection_Params);

    public:
        using ParamsType = FCk_Fragment_GeometryCollection_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_GeometryCollection_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKCHAOS_API FFragment_GeometryCollection_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_GeometryCollection_Current);

    public:
        friend class FProcessor_GeometryCollection_HandleRequests;
        friend class UCk_Utils_GeometryCollection_UE;

    private:
        // Add your properties here
        int32 _DummyProperty = 0;
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKCHAOS_API FFragment_GeometryCollection_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_GeometryCollection_Requests);

    public:
        friend class FProcessor_GeometryCollection_HandleRequests;
        friend class UCk_Utils_GeometryCollection_UE;

    public:
        using RequestType = std::variant
        <
            FCk_Request_GeometryCollection_ApplyStrain,
            FCk_Request_GeometryCollection_ApplyAoE
        >;
        using RequestList = TArray<RequestType>;

    private:
        RequestList _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES_AND_UTILS(FUtils_RecordOfGeometryCollections, FFragment_RecordOfGeometryCollections, FCk_Handle_GeometryCollection);
}

// --------------------------------------------------------------------------------------------------------------------
