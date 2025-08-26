namespace utils_float_attribute
{
    FCk_Handle_FloatAttribute Add(
        FCk_Handle InAttributeOwner,
        FGameplayTag InAttributeName,
        float InBaseValue,
        ECk_Replication InReplicates,
        ECk_MinMax InMinMax = ECk_MinMax::None,
        float InMinValue = 0.0f,
        float InMaxValue = 0.0f,
        FCk_Fragment_FloatAttributeRefill_ParamsData InRefillParams = FCk_Fragment_FloatAttributeRefill_ParamsData())
    {
        auto Params = FCk_Fragment_FloatAttribute_ParamsData(InAttributeName, InBaseValue);
        Params
        .Set_MinMax(InMinMax)
        .Set_MaxValue(InMaxValue)
        .Set_MinValue(InMinValue)
        .Set_RefillParams(InRefillParams)
        .Set_EnableRefill(InRefillParams.Get_RefillAttributeName().IsValid());

        return utils_float_attribute::Add(InAttributeOwner, Params, InReplicates);
    }

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

    float32 Get_FinalValue_ByName(const FCk_Handle &in InAttributeOwnerEntity, FGameplayTag InAttributeName, ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current)
    {
        auto Attribute = utils_float_attribute::TryGet(InAttributeOwnerEntity, InAttributeName);
        return Attribute.Get_FinalValue(InAttributeComponent);
    }

    float32 Get_BonusValue_ByName(const FCk_Handle &in InAttributeOwnerEntity, FGameplayTag InAttributeName, ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current)
    {
        auto Attribute = utils_float_attribute::TryGet(InAttributeOwnerEntity, InAttributeName);
        return Attribute.Get_BonusValue(InAttributeComponent);
    }

    float32 Get_BaseValue_ByName(const FCk_Handle &in InAttributeOwnerEntity, FGameplayTag InAttributeName, ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current)
    {
        auto Attribute = utils_float_attribute::TryGet(InAttributeOwnerEntity, InAttributeName);
        return Attribute.Get_BaseValue(InAttributeComponent);
    }
}