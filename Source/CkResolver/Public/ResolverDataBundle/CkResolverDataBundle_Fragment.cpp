#include "CkResolverDataBundle_Fragment.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

// --------------------------------------------------------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG(TAG_ResolverDataBundle_InvalidPhase, TEXT("ResolverPhase.InvalidPhase"));
UE_DEFINE_GAMEPLAY_TAG(TAG_ResolverDataBundle_Initial, TEXT("Resolver.DataBundle.Name.Initial"));
UE_DEFINE_GAMEPLAY_TAG(TAG_ResolverDataBundle_Final, TEXT("Resolver.DataBundle.Name.Final"));

// --------------------------------------------------------------------------------------------------------------------

CK_ECS_DEFINE_CALLSTACK_ANGELSCRIPT_UTILS(CKRESOLVER_API, resolver_data_bundle, ck::FFragment_ResolverDataBundle_Requests);

// --------------------------------------------------------------------------------------------------------------------
