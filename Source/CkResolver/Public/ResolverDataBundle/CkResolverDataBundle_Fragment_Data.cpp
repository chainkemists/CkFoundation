#include "CkResolverDataBundle_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG(TAG_Label_ResolverDataBundle_BaseValue, TEXT("ResolverDataBundle.ModifierComponent.Base"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Label_ResolverDataBundle_BonusValue, TEXT("ResolverDataBundle.ModifierComponent.Bonus"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Label_ResolverDataBundle_TotalMultiplierValue, TEXT("ResolverDataBundle.ModifierComponent.TotalMultiplier"));

UE_DEFINE_GAMEPLAY_TAG(TAG_Label_ResolverDataBundle_Name_Default, TEXT("Resolver.DataBundle.Name.Default"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Label_ResolverDataBundle_Phase_Default, TEXT("Resolver.DataBundle.Phase.Default"));


// --------------------------------------------------------------------------------------------------------------------
auto
    FCk_Payload_ResolverDataBundle_Resolved::
    Set_Causer(
        const FCk_Handle& InCauser)
    -> ThisType&
{
    _ResolverCause = InCauser;
    _Causer = InCauser;

    return *this;
}
