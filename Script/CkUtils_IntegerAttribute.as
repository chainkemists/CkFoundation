namespace utils_integer_attribute
{
    FCk_Handle_IntegerAttribute Add(
        FCk_Handle InAttributeOwner,
        FGameplayTag InAttributeName,
        int32 InBaseValue,
        ECk_Replication InReplicates,
        ECk_MinMax InMinMax = ECk_MinMax::None,
        int32 InMinValue = 0,
        int32 InMaxValue = 0)
    {
        auto Params = FCk_Fragment_IntegerAttribute_ParamsData(InAttributeName, InBaseValue);
        Params
        .Set_MinMax(InMinMax)
        .Set_MinValue(InMinValue)
        .Set_MaxValue(InMaxValue);

        return utils_integer_attribute::Add(InAttributeOwner, Params, InReplicates);
    }

    int32 Get_FinalValueOr(
        const FCk_Handle &in InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        int32 InDefault = 0,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current)
    {
        const auto Attribute = utils_integer_attribute::TryGet(InAttributeOwnerEntity, InAttributeName);
        return Attribute.IsValid() ?
            Attribute.Get_FinalValue(InAttributeComponent) :
            InDefault;
    }

    int32 Get_FinalValue(const FCk_Handle &in InAttributeOwnerEntity, FGameplayTag InAttributeName, ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current)
    {
        auto Attribute = utils_integer_attribute::TryGet(InAttributeOwnerEntity, InAttributeName);
        return Attribute.Get_FinalValue(InAttributeComponent);
    }

    int32 Get_BonusValue(const FCk_Handle &in InAttributeOwnerEntity, FGameplayTag InAttributeName, ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current)
    {
        auto Attribute = utils_integer_attribute::TryGet(InAttributeOwnerEntity, InAttributeName);
        return Attribute.Get_BonusValue(InAttributeComponent);
    }

    int32 Get_BaseValue(const FCk_Handle &in InAttributeOwnerEntity, FGameplayTag InAttributeName, ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current)
    {
        auto Attribute = utils_integer_attribute::TryGet(InAttributeOwnerEntity, InAttributeName);
        return Attribute.Get_BaseValue(InAttributeComponent);
    }
}