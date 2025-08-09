namespace utils_vector_attribute
{
    FCk_Handle_VectorAttribute Add(
        FCk_Handle InAttributeOwner,
        FGameplayTag InAttributeName,
        FVector InBaseValue,
        ECk_Replication InReplicates,
        ECk_MinMax InMinMax = ECk_MinMax::None,
        FVector InMinValue = FVector::ZeroVector,
        FVector InMaxValue = FVector::ZeroVector)
    {
        auto Params = FCk_Fragment_VectorAttribute_ParamsData();
        Params._Name = InAttributeName;
        Params._BaseValue = InBaseValue;
        Params._MinMax = InMinMax;
        Params._MinValue = InMinValue;
        Params._MaxValue = InMaxValue;
        
        return utils_vector_attribute::Add(InAttributeOwner, Params, InReplicates);
    }
}