#pragma once

#include "CkRecord/Record/CkRecord_Fragment.h"

#include "CkTargetable_Fragment_Data.h"

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_Targetable_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_Targetable_NeedsSetup);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKTARGETING_API FFragment_Targetable_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_Targetable_Params);

    public:
        using ParamsType = FCk_Fragment_Targetable_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Targetable_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKTARGETING_API FFragment_Targetable_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_Targetable_Current);

    public:
        friend class FProcessor_Targetable_Setup;
        friend class UCk_Utils_Targetable_UE;

    private:
        TWeakObjectPtr<USceneComponent> _AttachmentNode;

    public:
        CK_PROPERTY_GET(_AttachmentNode);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfTargetables, FCk_Handle_Targetable);
}

// --------------------------------------------------------------------------------------------------------------------
