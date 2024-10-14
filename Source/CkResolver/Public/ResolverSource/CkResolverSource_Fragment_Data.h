#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkProvider/Public/CkProvider/CkProvider_Data.h"

#include "ResolverDataBundle/CkResolverDataBundle_Fragment_Data.h"

#include "CkResolverSource_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Resolver_Variable_InstancedStruct_HitResult);

// common damage tags
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Resolver_DataBundle_Phase_MainStats);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Resolver_DataBundle_Phase_GlobalModifiers);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Resolver_DataBundle_Phase_FinalResults);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKRESOLVER_API FCk_Fragment_ResolverSource_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_ResolverSource_ParamsData);

private:
    // this list can be unique per ResolverSource but most often uses a shared set of phases
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    TArray<FCk_Fragment_ResolverDataBundle_PhaseInfo> _ResolutionPhases;

public:
    CK_PROPERTY_GET(_ResolutionPhases);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKRESOLVER_API FCk_Request_ResolverSource_InitiateNewResolution : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_ResolverSource_InitiateNewResolution);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "Resolver.DataBundle.Name"))
    FGameplayTag _BundleName;

    // target must always be present - if there is no valid target, create one through the Resolver Utils
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FCk_Handle_ResolverTarget _Target;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FCk_Handle _Causer;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_ResolverDataBundle_MetadataOperation_Conditional> _InitialMetadata;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_ResolverDataBundle_ModifierOperation_Conditional> _InitialModifierOperations;

public:
    CK_PROPERTY_GET(_BundleName);
    CK_PROPERTY_GET(_Target);
    CK_PROPERTY_GET(_Causer);
    CK_PROPERTY_GET(_InitialMetadata);
    CK_PROPERTY_GET(_InitialModifierOperations);

    CK_DEFINE_CONSTRUCTORS(FCk_Request_ResolverSource_InitiateNewResolution, _BundleName, _Target, _Causer, _InitialMetadata, _InitialModifierOperations);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_ResolverSource_OnNewResolverDataBundle,
    FCk_Handle_ResolverSource, InSource,
    FCk_Handle, InCause,
    FCk_Handle_ResolverDataBundle, InDataBundle);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FCk_Delegate_ResolverSource_OnNewResolverDataBundle_MC,
    FCk_Handle_ResolverSource, InSource,
    FCk_Handle, InCause,
    FCk_Handle_ResolverDataBundle, InDataBundle);

// --------------------------------------------------------------------------------------------------------------------
