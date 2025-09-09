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
    
    FCk_Handle_ByteAttributeModifier
    IncrementRevocable(
        const FCk_Handle &in InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current)
    {
        auto Attribute = utils_byte_attribute::TryGet(InAttributeOwnerEntity, InAttributeName);
        if (! ck::Ensure(ck::IsValid(Attribute), f"Entity [{InAttributeOwnerEntity.ToString()}] does NOT have Attribute [{InAttributeName.TagName.ToString()}]"))
        { return FCk_Handle_ByteAttributeModifier(); }
        return Attribute.IncrementRevocable(InAttributeComponent);
    }

    void
    IncrementNotRevocable(
        const FCk_Handle &in InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current)
    {
        auto Attribute = utils_byte_attribute::TryGet(InAttributeOwnerEntity, InAttributeName);
        if (! ck::Ensure(ck::IsValid(Attribute), f"Entity [{InAttributeOwnerEntity.ToString()}] does NOT have Attribute [{InAttributeName.TagName.ToString()}]"))
        { return; }
        Attribute.IncrementNotRevocable(InAttributeComponent);
    }

    FCk_Handle_ByteAttributeModifier
    DecrementRevocable(
        const FCk_Handle &in InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current)
    {
        auto Attribute = utils_byte_attribute::TryGet(InAttributeOwnerEntity, InAttributeName);
        if (! ck::Ensure(ck::IsValid(Attribute), f"Entity [{InAttributeOwnerEntity.ToString()}] does NOT have Attribute [{InAttributeName.TagName.ToString()}]"))
        { return FCk_Handle_ByteAttributeModifier(); }
        return Attribute.DecrementRevocable(InAttributeComponent);
    }

    void
    DecrementNotRevocable(
        const FCk_Handle &in InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current)
    {
        auto Attribute = utils_byte_attribute::TryGet(InAttributeOwnerEntity, InAttributeName);
        if (! ck::Ensure(ck::IsValid(Attribute), f"Entity [{InAttributeOwnerEntity.ToString()}] does NOT have Attribute [{InAttributeName.TagName.ToString()}]"))
        { return; }
        Attribute.DecrementNotRevocable(InAttributeComponent);
    }
}

mixin FCk_Handle_ByteAttributeModifier
IncrementRevocable(
    FCk_Handle_ByteAttribute &in InAttribute,
    ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current)
{
    if (! ck::Ensure(ck::IsValid(InAttribute), f"Entity [{InAttribute.ToString()}] is NOT valid!"))
    { return FCk_Handle_ByteAttributeModifier(); }
    return InAttribute.Add_Revocable(
        FGameplayTag(),
        ECk_AttributeModifier_Operation::Add,
        FCk_Fragment_ByteAttributeModifier_ParamsData(1, InAttributeComponent));
}

mixin void
IncrementNotRevocable(
    FCk_Handle_ByteAttribute &in InAttribute,
    ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current)
{
    if (! ck::Ensure(ck::IsValid(InAttribute), f"Entity [{InAttribute.ToString()}] is NOT valid!"))
    { return; }
    InAttribute.Add_NotRevocable(
        ECk_AttributeModifier_Operation::Add,
        FCk_Fragment_ByteAttributeModifier_ParamsData(1, InAttributeComponent));
}

mixin FCk_Handle_ByteAttributeModifier
DecrementRevocable(
    FCk_Handle_ByteAttribute &in InAttribute,
    ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current)
{
    if (! ck::Ensure(ck::IsValid(InAttribute), f"Entity [{InAttribute.ToString()}] is NOT valid!"))
    { return FCk_Handle_ByteAttributeModifier(); }
    return InAttribute.Add_Revocable(
        FGameplayTag(),
        ECk_AttributeModifier_Operation::Subtract,
        FCk_Fragment_ByteAttributeModifier_ParamsData(1, InAttributeComponent));
}

mixin void
DecrementNotRevocable(
    FCk_Handle_ByteAttribute &in InAttribute,
    ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current)
{
    if (! ck::Ensure(ck::IsValid(InAttribute), f"Entity [{InAttribute.ToString()}] is NOT valid!"))
    { return; }
    InAttribute.Add_NotRevocable(
        ECk_AttributeModifier_Operation::Subtract,
        FCk_Fragment_ByteAttributeModifier_ParamsData(1, InAttributeComponent));
}