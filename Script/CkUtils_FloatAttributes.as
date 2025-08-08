
namespace utils_float_attribute
{
    float32 Get_FinalValueOr(
        const FCk_Handle &in InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        float32 InDefault = 0.f,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current)
    {
        const auto Attribute = utils_float_attribute::TryGet(InAttributeOwnerEntity, InAttributeName);
        return Attribute.IsValid() ?
            Attribute.Get_FinalValue(InAttributeComponent) :
            InDefault;
    }
}
