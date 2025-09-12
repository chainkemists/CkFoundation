namespace utils_byte_attribute
{
    FCk_Handle_ByteAttribute Add(
        FCk_Handle InAttributeOwner,
        FGameplayTag InAttributeName,
        uint8 InBaseValue,
        ECk_Replication InReplicates,
        ECk_MinMax InMinMax = ECk_MinMax::None,
        uint8 InMinValue = 0,
        uint8 InMaxValue = 0)
    {
        auto Params = FCk_Fragment_ByteAttribute_ParamsData(InAttributeName, InBaseValue);
        Params
        .Set_MinMax(InMinMax)
        .Set_MinValue(InMinValue)
        .Set_MaxValue(InMaxValue);

        return utils_byte_attribute::Add(InAttributeOwner, Params, InReplicates);
    }

    uint8 Get_FinalValueOr(
        const FCk_Handle &in InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        uint8 InDefault = 0,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current)
    {
        const auto Attribute = utils_byte_attribute::TryGet(InAttributeOwnerEntity, InAttributeName);
        return Attribute.IsValid() ?
            Attribute.Get_FinalValue(InAttributeComponent) :
            InDefault;
    }

    uint8 Get_FinalValue_ByName(const FCk_Handle &in InAttributeOwnerEntity, FGameplayTag InAttributeName, ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current)
    {
        auto Attribute = utils_byte_attribute::TryGet(InAttributeOwnerEntity, InAttributeName);
        return Attribute.Get_FinalValue(InAttributeComponent);
    }

    uint8 Get_BonusValue_ByName(const FCk_Handle &in InAttributeOwnerEntity, FGameplayTag InAttributeName, ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current)
    {
        auto Attribute = utils_byte_attribute::TryGet(InAttributeOwnerEntity, InAttributeName);
        return Attribute.Get_BonusValue(InAttributeComponent);
    }

    uint8 Get_BaseValue_ByName(const FCk_Handle &in InAttributeOwnerEntity, FGameplayTag InAttributeName, ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current)
    {
        auto Attribute = utils_byte_attribute::TryGet(InAttributeOwnerEntity, InAttributeName);
        return Attribute.Get_BaseValue(InAttributeComponent);
    }
}