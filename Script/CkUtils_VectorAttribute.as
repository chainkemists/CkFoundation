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
        auto Params = FCk_Fragment_VectorAttribute_ParamsData(InAttributeName, InBaseValue);
        Params.Set_MinMax(InMinMax).Set_MinValue(InMinValue).Set_MaxValue(InMaxValue);

        return utils_vector_attribute::Add(InAttributeOwner, Params, InReplicates);
    }

    FVector Get_FinalValueOr(
        const FCk_Handle &in InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        FVector InDefault = FVector::ZeroVector,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current)
    {
        const auto Attribute = utils_vector_attribute::TryGet(InAttributeOwnerEntity, InAttributeName);
        return Attribute.IsValid() ?
            Attribute.Get_FinalValue(InAttributeComponent) :
            InDefault;
    }
}