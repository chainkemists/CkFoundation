#include "CkArchetype_Data.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ArchetypeDefinition::
    Get_Descriptor() const
    -> FCk_ArchetypeDescriptor
{
    return FCk_ArchetypeDescriptor{Name}
        .Set_DisplayName(DisplayName)
        .Set_FeatureIds(FeatureIds)
        .Set_RequiredLabel(RequiredLabel)
        .Set_NamePattern(NamePattern)
        .Set_IconSvgPath(IconSvgPath)
        .Set_Color(Color)
        .Set_Priority(Priority);
}

auto
    UCk_ArchetypeDefinition::
    IsValidDefinition() const
    -> bool
{
    return Name.IsNone() == false;
}
