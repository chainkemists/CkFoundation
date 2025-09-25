#pragma once

#include <NativeGameplayTags.h>
#include <GameplayEffectTypes.h>

#include "CkEntityTag_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Utils_EntityTag_UE;

// --------------------------------------------------------------------------------------------------------------------

CKENTITYTAG_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_EntityTag_Root);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    using FFragment_EntityTag_StorageParams = FCk_Fragment_EntityTag_ParamsData;

    // --------------------------------------------------------------------------------------------------------------------

    struct FEntityTagCount
    {
        FName _Name;
        int32 _Count = 0;

        FEntityTagCount(FName _Name, int32 _Count)
            : _Name(_Name)
            , _Count(_Count)
        {}
    };

    struct CKENTITYTAG_API FFragment_EntityTag_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_EntityTag_Current);

    private:
        TArray<FEntityTagCount> _Tags;
        FGameplayTagCountContainer _GameplayTags;

    public:
        CK_PROPERTY(_Tags)
        CK_PROPERTY(_GameplayTags)

    public:
        FFragment_EntityTag_Current() = default;
    };

    // --------------------------------------------------------------------------------------------------------------------

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKENTITYTAG_API,
        EntityTag_OnTagUpdated,
        FCk_Delegate_EntityTag_OnTagUpdated_MC,
        FCk_Handle,
        FName,
        ECk_EntityTagUpdate);

    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKENTITYTAG_API,
        EntityTag_OnGameplayTagUpdated,
        FCk_Delegate_EntityTag_OnGameplayTagUpdated_MC,
        FCk_Handle,
        FGameplayTag,
        ECk_EntityTagUpdate);

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------