#include "CkAttribute_Fragment.h"

#include <GameplayTagsManager.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    FAttributeModifier_ReplicationTags FAttributeModifier_ReplicationTags::_Tags;

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FAttributeModifier_ReplicationTags::
        AddTags()
        -> void
    {
        auto& Manager = UGameplayTagsManager::Get();

        _Base = Manager.AddNativeGameplayTag(TEXT("Ck.AttributeModifier.Replication.Base"));
        _Final = Manager.AddNativeGameplayTag(TEXT("Ck.AttributeModifier.Replication.Final"));
    }

    auto
        FAttributeModifier_ReplicationTags::
        Get_BaseTag()
        -> FGameplayTag
    {
        return _Tags._Base;
    }

    auto
        FAttributeModifier_ReplicationTags::
        Get_FinalTag()
        -> FGameplayTag
    {
        return _Tags._Final;
    }

    // --------------------------------------------------------------------------------------------------------------------

    FAttributeModifier_Tags FAttributeModifier_Tags::_Tags;

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FAttributeModifier_Tags::
        AddTags()
        -> void
    {
        auto& Manager = UGameplayTagsManager::Get();

        _Override = Manager.AddNativeGameplayTag(TEXT("Ck.AttributeModifier.Override"));
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FAttributeModifier_Tags::
        Get_Override()
        -> FGameplayTag
    {
        return _Tags._Override;
    }
}

// --------------------------------------------------------------------------------------------------------------------
