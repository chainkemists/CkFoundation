#pragma once

#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment_Params.h"

#include "CkRecord/Record/CkRecord_Fragment.h"

#include "CkEcs/Signal/CkSignal_Macros.h"

#include "ResolverDataBundle/CkResolverDataBundle_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_ResolverDataBundle_UE;

// --------------------------------------------------------------------------------------------------------------------

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_ResolverDataBundle_InvalidPhase);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_ResolverDataBundle_Name_Initial);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_ResolverDataBundle_Name_Final);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ECS_TAG(FTag_ResolverDataBundle_StartNewPhase);
    CK_DEFINE_ECS_TAG(FTag_ResolverDataBundle_OperationsResolved);
    CK_DEFINE_ECS_TAG(FTag_ResolverDataBundle_CalculateDone);

    // --------------------------------------------------------------------------------------------------------------------

    struct CKRESOLVER_API FFragment_ResolverDataBundle_Params
    {
    public:
        CK_GENERATED_BODY(FFragment_ResolverDataBundle_Params);

    public:
        using ParamsType = FCk_Fragment_ResolverDataBundle_ParamsData;

    private:
        ParamsType _Params;

    public:
        CK_PROPERTY_GET(_Params);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_ResolverDataBundle_Params, _Params);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKRESOLVER_API FFragment_ResolverDataBundle_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_ResolverDataBundle_Current);

    public:
        friend class FProcessor_ResolverDataBundle_StartNewPhase;
        friend class FProcessor_ResolverDataBundle_HandleRequests;
        friend class FProcessor_ResolverDataBundle_ResolveOperations;
        friend class FProcessor_ResolverDataBundle_Calculate;
        friend class UCk_Utils_ResolverDataBundle_UE;

    private:
        FGameplayTagContainer _MetadataTags;
        float _FinalValue = 0.0f;
        int32 _CurrentPhaseIndex = INDEX_NONE;

    public:
        CK_PROPERTY_GET(_MetadataTags);
        CK_PROPERTY_GET(_FinalValue);
        CK_PROPERTY_GET(_CurrentPhaseIndex);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKRESOLVER_API FFragment_ResolverDataBundle_PendingOperations
    {
    public:
        CK_GENERATED_BODY(FFragment_ResolverDataBundle_PendingOperations);

    public:
        friend class UCk_Utils_ResolverDataBundle_UE;

    private:
        TArray<FCk_ResolverDataBundle_ModifierOperation> _PendingModifiersOperations;
        TArray<FCk_ResolverDataBundle_ModifierOperation_Conditional> _PendingModifiersOperations_Conditionals;

        TArray<FCk_ResolverDataBundle_MetadataOperation> _PendingMetadataOperations;
        TArray<FCk_ResolverDataBundle_MetadataOperation_Conditional> _PendingMetadataOperations_Conditionals;

    public:
        CK_PROPERTY_GET(_PendingModifiersOperations);
        CK_PROPERTY_GET(_PendingModifiersOperations_Conditionals);
        CK_PROPERTY_GET(_PendingMetadataOperations);
        CK_PROPERTY_GET(_PendingMetadataOperations_Conditionals);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKRESOLVER_API FFragment_ResolverDataBundle_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_ResolverDataBundle_Requests);

    public:
        friend class UCk_Utils_ResolverDataBundle_UE;
        friend class FProcessor_ResolverDataBundle_HandleRequests;

    public:
        using ModifierOperation_RequestType = FCk_Request_ResolverDataBundle_ModifierOperation;
        using ModifierOperator_RequestList = TArray<ModifierOperation_RequestType>;

        using MetadataOperation_RequestType = FCk_Request_ResolverDataBundle_MetadataOperation;
        using MetadataOperation_RequestList = TArray<MetadataOperation_RequestType>;

    private:
        ModifierOperator_RequestList _MutateModifierRequests;
        MetadataOperation_RequestList _MutateMetadataRequests;

    public:
        CK_PROPERTY_GET(_MutateModifierRequests);
        CK_PROPERTY_GET(_MutateMetadataRequests);
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKRESOLVER_API, ResolverDataBundle_PhaseStart,
        FCk_Delegate_ResolverDataBundle_OnPhaseStart_MC,
        FCk_Handle_ResolverDataBundle,
        FGameplayTag);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKRESOLVER_API, ResolverDataBundle_PhaseComplete,
        FCk_Delegate_ResolverDataBundle_OnPhaseComplete_MC,
        FCk_Handle_ResolverDataBundle,
        FGameplayTag,
        FCk_Payload_ResolverDataBundle_Resolved);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(CKRESOLVER_API, ResolverDataBundle_AllPhasesComplete,
        FCk_Delegate_ResolverDataBundle_OnAllPhasesComplete_MC,
        FCk_Handle_ResolverDataBundle,
        FCk_Payload_ResolverDataBundle_Resolved);

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfDataBundles, FCk_Handle_ResolverDataBundle);
}

// --------------------------------------------------------------------------------------------------------------------
