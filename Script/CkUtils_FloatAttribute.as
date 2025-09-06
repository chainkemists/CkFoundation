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
        Params.Set_MinMax(InMinMax).Set_MaxValue(InMaxValue).Set_RefillParams(InRefillParams);

        return utils_float_attribute::Add(InAttributeOwner, Params, InReplicates);
    }
}