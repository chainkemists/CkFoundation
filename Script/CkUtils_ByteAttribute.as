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
        auto Params = FCk_Fragment_ByteAttribute_ParamsData();
        Params._Name = InAttributeName;
        Params._BaseValue = InBaseValue;
        Params._MinMax = InMinMax;
        Params._MinValue = InMinValue;
        Params._MaxValue = InMaxValue;
        
        return utils_byte_attribute::Add(InAttributeOwner, Params, InReplicates);
    }
}