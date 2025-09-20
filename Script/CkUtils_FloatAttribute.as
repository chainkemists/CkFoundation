namespace utils_float_attribute
{
	/*
	┌──────────────────────────────────────────────────────────────────────────────┐
	│                           Attribute Creation Helpers                         │
	└──────────────────────────────────────────────────────────────────────────────┘
	*/

	FCk_Handle_FloatAttribute
	Add(
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

	/*
	┌──────────────────────────────────────────────────────────────────────────────┐
	│                            Value Getters by Name                             │
	└──────────────────────────────────────────────────────────────────────────────┘
	*/

	float32
	Get_BaseValue(
		const FCk_Handle &in InAttributeOwnerEntity,
		FGameplayTag InAttributeName,
		ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current)
	{
		auto Attribute = utils_float_attribute::TryGet(InAttributeOwnerEntity, InAttributeName);
		return Attribute.Get_BaseValue(InAttributeComponent);
	}

	float32
	Get_BonusValue(
		const FCk_Handle &in InAttributeOwnerEntity,
		FGameplayTag InAttributeName,
		ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current)
	{
		auto Attribute = utils_float_attribute::TryGet(InAttributeOwnerEntity, InAttributeName);
		return Attribute.Get_BonusValue(InAttributeComponent);
	}

	float32
	Get_Current(
		FCk_Handle InOwner,
		FGameplayTag InName)
	{
		auto Attribute = TryGet(InOwner, InName);
		if (! ck::Ensure(Attribute.IsValid(), f"Attribute not found: {InName.ToString()}"))
		{ return 0.0f; }
		return Get_FinalValue(Attribute, ECk_MinMaxCurrent::Current);
	}

	float32
	Get_FinalValue(
		const FCk_Handle &in InAttributeOwnerEntity,
		FGameplayTag InAttributeName,
		ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current)
	{
		auto Attribute = utils_float_attribute::TryGet(InAttributeOwnerEntity, InAttributeName);
		return Attribute.Get_FinalValue(InAttributeComponent);
	}

	float32
	Get_FinalValueOr(
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

	float32
	Get_Max(
		FCk_Handle InOwner,
		FGameplayTag InName)
	{
		auto Attribute = TryGet(InOwner, InName);
		if (! ck::Ensure(Attribute.IsValid(), f"Attribute not found: {InName.ToString()}"))
		{ return 0.0f; }
		if (! ck::Ensure(Has_Component(Attribute, ECk_MinMaxCurrent::Max), "Attribute does not have Max component"))
		{ return 0.0f; }
		return Get_FinalValue(Attribute, ECk_MinMaxCurrent::Max);
	}

	float32
	Get_Min(
		FCk_Handle InOwner,
		FGameplayTag InName)
	{
		auto Attribute = TryGet(InOwner, InName);
		if (! ck::Ensure(Attribute.IsValid(), f"Attribute not found: {InName.ToString()}"))
		{ return 0.0f; }
		if (! ck::Ensure(Has_Component(Attribute, ECk_MinMaxCurrent::Min), "Attribute does not have Min component"))
		{ return 0.0f; }
		return Get_FinalValue(Attribute, ECk_MinMaxCurrent::Min);
	}

	/*
	┌──────────────────────────────────────────────────────────────────────────────┐
	│                           Increment/Decrement Helpers                        │
	└──────────────────────────────────────────────────────────────────────────────┘
	*/

	void
	DecrementNotRevocable(
		const FCk_Handle &in InAttributeOwnerEntity,
		FGameplayTag InAttributeName,
		ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current)
	{
		auto Attribute = utils_float_attribute::TryGet(InAttributeOwnerEntity, InAttributeName);
		if (! ck::Ensure(ck::IsValid(Attribute), f"Entity [{InAttributeOwnerEntity.ToString()}] does NOT have Attribute [{InAttributeName.TagName.ToString()}]"))
		{ return; }
		Attribute.DecrementNotRevocable(InAttributeComponent);
	}

	FCk_Handle_FloatAttributeModifier
	DecrementRevocable(
		const FCk_Handle &in InAttributeOwnerEntity,
		FGameplayTag InAttributeName,
		ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current)
	{
		auto Attribute = utils_float_attribute::TryGet(InAttributeOwnerEntity, InAttributeName);
		if (! ck::Ensure(ck::IsValid(Attribute), f"Entity [{InAttributeOwnerEntity.ToString()}] does NOT have Attribute [{InAttributeName.TagName.ToString()}]"))
		{ return FCk_Handle_FloatAttributeModifier(); }
		return Attribute.DecrementRevocable(InAttributeComponent);
	}

	void
	IncrementNotRevocable(
		const FCk_Handle &in InAttributeOwnerEntity,
		FGameplayTag InAttributeName,
		ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current)
	{
		auto Attribute = utils_float_attribute::TryGet(InAttributeOwnerEntity, InAttributeName);
		if (! ck::Ensure(ck::IsValid(Attribute), f"Entity [{InAttributeOwnerEntity.ToString()}] does NOT have Attribute [{InAttributeName.TagName.ToString()}]"))
		{ return; }
		Attribute.IncrementNotRevocable(InAttributeComponent);
	}

	FCk_Handle_FloatAttributeModifier
	IncrementRevocable(
		const FCk_Handle &in InAttributeOwnerEntity,
		FGameplayTag InAttributeName,
		ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current)
	{
		auto Attribute = utils_float_attribute::TryGet(InAttributeOwnerEntity, InAttributeName);
		if (! ck::Ensure(ck::IsValid(Attribute), f"Entity [{InAttributeOwnerEntity.ToString()}] does NOT have Attribute [{InAttributeName.TagName.ToString()}]"))
		{ return FCk_Handle_FloatAttributeModifier(); }
		return Attribute.IncrementRevocable(InAttributeComponent);
	}

	/*
	┌──────────────────────────────────────────────────────────────────────────────┐
	│                            Percentage & Calculations                         │
	└──────────────────────────────────────────────────────────────────────────────┘
	*/

	float32
	Calculate_Attribute_Magnitude(FCk_Handle_FloatAttribute InAttribute)
	{
		if (! ck::Ensure(InAttribute.IsValid(), "InAttribute must be valid"))
		{ return 0.0f; }
		if (! ck::Ensure(Has_Component(InAttribute, ECk_MinMaxCurrent::Min), "Attribute must have Min component"))
		{ return 0.0f; }
		if (! ck::Ensure(Has_Component(InAttribute, ECk_MinMaxCurrent::Max), "Attribute must have Max component"))
		{ return 0.0f; }

		float32 Min = Get_FinalValue(InAttribute, ECk_MinMaxCurrent::Min);
		float32 Max = Get_FinalValue(InAttribute, ECk_MinMaxCurrent::Max);
		return Max - Min;
	}

	float32
	Get_Value_AsPercentage(FCk_Handle_FloatAttribute InAttribute)
	{
		if (! ck::Ensure(InAttribute.IsValid(), "InAttribute must be valid"))
		{ return 0.0f; }
		if (! ck::Ensure(Has_Component(InAttribute, ECk_MinMaxCurrent::Min), "Attribute must have Min component for percentage calculation"))
		{ return 0.0f; }
		if (! ck::Ensure(Has_Component(InAttribute, ECk_MinMaxCurrent::Max), "Attribute must have Max component for percentage calculation"))
		{ return 0.0f; }

		float32 Current = Get_FinalValue(InAttribute, ECk_MinMaxCurrent::Current);
		float32 Min = Get_FinalValue(InAttribute, ECk_MinMaxCurrent::Min);
		float32 Max = Get_FinalValue(InAttribute, ECk_MinMaxCurrent::Max);

		float32 Range = Max - Min;
		if (Range <= 0.0f)
		{
			return 0.0f;
		}

		return (Current - Min) / Range;
	}

	float32
	Get_Value_AsPercentage(
		FCk_Handle InOwner,
		FGameplayTag InName)
	{
		auto Attribute = TryGet(InOwner, InName);
		if (! ck::Ensure(Attribute.IsValid(), f"Attribute not found: {InName.ToString()}"))
		{ return 0.0f; }
		return Get_Value_AsPercentage(Attribute);
	}

	/*
	┌──────────────────────────────────────────────────────────────────────────────┐
	│                              Override Helpers                                │
	└──────────────────────────────────────────────────────────────────────────────┘
	*/

	FCk_Handle_FloatAttribute
	Request_Override_Current(
		FCk_Handle InOwner,
		FGameplayTag InName,
		float32 InNewValue)
	{
		auto Attribute = TryGet(InOwner, InName);
		if (! ck::Ensure(Attribute.IsValid(), f"Attribute not found for override: {InName.ToString()}"))
		{ return FCk_Handle_FloatAttribute(); }
		return Request_Override(Attribute, InNewValue, ECk_MinMaxCurrent::Current);
	}

	FCk_Handle_FloatAttribute
	Request_Override_Current(
		FCk_Handle_FloatAttribute InAttribute,
		float32 InNewValue)
	{
		if (! ck::Ensure(InAttribute.IsValid(), "InAttribute must be valid"))
		{ return FCk_Handle_FloatAttribute(); }
		return Request_Override(InAttribute, InNewValue, ECk_MinMaxCurrent::Current);
	}

	FCk_Handle_FloatAttribute
	Request_Override_Max(
		FCk_Handle InOwner,
		FGameplayTag InName,
		float32 InNewValue)
	{
		auto Attribute = TryGet(InOwner, InName);
		if (! ck::Ensure(Attribute.IsValid(), f"Attribute not found for override: {InName.ToString()}"))
		{ return FCk_Handle_FloatAttribute(); }
		if (! ck::Ensure(Has_Component(Attribute, ECk_MinMaxCurrent::Max), "Attribute must have Max component to override it"))
		{ return FCk_Handle_FloatAttribute(); }
		return Request_Override(Attribute, InNewValue, ECk_MinMaxCurrent::Max);
	}

	FCk_Handle_FloatAttribute
	Request_Override_Max(
		FCk_Handle_FloatAttribute InAttribute,
		float32 InNewValue)
	{
		if (! ck::Ensure(InAttribute.IsValid(), "InAttribute must be valid"))
		{ return FCk_Handle_FloatAttribute(); }
		if (! ck::Ensure(Has_Component(InAttribute, ECk_MinMaxCurrent::Max), "Attribute must have Max component to override it"))
		{ return FCk_Handle_FloatAttribute(); }
		return Request_Override(InAttribute, InNewValue, ECk_MinMaxCurrent::Max);
	}

	FCk_Handle_FloatAttribute
	Request_Override_Min(
		FCk_Handle InOwner,
		FGameplayTag InName,
		float32 InNewValue)
	{
		auto Attribute = TryGet(InOwner, InName);
		if (! ck::Ensure(Attribute.IsValid(), f"Attribute not found for override: {InName.ToString()}"))
		{ return FCk_Handle_FloatAttribute(); }
		if (! ck::Ensure(Has_Component(Attribute, ECk_MinMaxCurrent::Min), "Attribute must have Min component to override it"))
		{ return FCk_Handle_FloatAttribute(); }
		return Request_Override(Attribute, InNewValue, ECk_MinMaxCurrent::Min);
	}

	FCk_Handle_FloatAttribute
	Request_Override_Min(
		FCk_Handle_FloatAttribute InAttribute,
		float32 InNewValue)
	{
		if (! ck::Ensure(InAttribute.IsValid(), "InAttribute must be valid"))
		{ return FCk_Handle_FloatAttribute(); }
		if (! ck::Ensure(Has_Component(InAttribute, ECk_MinMaxCurrent::Min), "Attribute must have Min component to override it"))
		{ return FCk_Handle_FloatAttribute(); }
		return Request_Override(InAttribute, InNewValue, ECk_MinMaxCurrent::Min);
	}

	/*
	┌──────────────────────────────────────────────────────────────────────────────┐
	│                              Utility Functions                               │
	└──────────────────────────────────────────────────────────────────────────────┘
	*/

	bool
	Has_Attribute(
		FCk_Handle InOwner,
		FGameplayTag InName)
	{
		auto Attribute = TryGet(InOwner, InName);
		return Attribute.IsValid();
	}
}

/*
┌──────────────────────────────────────────────────────────────────────────────┐
│                              Mixin Functions                                 │
└──────────────────────────────────────────────────────────────────────────────┘
*/

mixin void
DecrementNotRevocable(
	FCk_Handle_FloatAttribute &in InAttribute,
	ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current)
{
	if (! ck::Ensure(ck::IsValid(InAttribute), f"Entity [{InAttribute.ToString()}] is NOT valid!"))
	{ return; }
	InAttribute.Add_NotRevocable(
		ECk_AttributeModifier_Operation::Subtract,
		FCk_Fragment_FloatAttributeModifier_ParamsData(1.f, InAttributeComponent));
}

mixin FCk_Handle_FloatAttributeModifier
DecrementRevocable(
	FCk_Handle_FloatAttribute &in InAttribute,
	ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current)
{
	if (! ck::Ensure(ck::IsValid(InAttribute), f"Entity [{InAttribute.ToString()}] is NOT valid!"))
	{ return FCk_Handle_FloatAttributeModifier(); }
	return InAttribute.Add_Revocable(
		FGameplayTag(),
		ECk_AttributeModifier_Operation::Subtract,
		FCk_Fragment_FloatAttributeModifier_ParamsData(1.f, InAttributeComponent));
}

mixin void
IncrementNotRevocable(
	FCk_Handle_FloatAttribute &in InAttribute,
	ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current)
{
	if (! ck::Ensure(ck::IsValid(InAttribute), f"Entity [{InAttribute.ToString()}] is NOT valid!"))
	{ return; }
	InAttribute.Add_NotRevocable(
		ECk_AttributeModifier_Operation::Add,
		FCk_Fragment_FloatAttributeModifier_ParamsData(1.f, InAttributeComponent));
}

mixin FCk_Handle_FloatAttributeModifier
IncrementRevocable(
	FCk_Handle_FloatAttribute &in InAttribute,
	ECk_MinMaxCurrent InAttributeComponent = ECk_MinMaxCurrent::Current)
{
	if (! ck::Ensure(ck::IsValid(InAttribute), f"Entity [{InAttribute.ToString()}] is NOT valid!"))
	{ return FCk_Handle_FloatAttributeModifier(); }
	return InAttribute.Add_Revocable(
		FGameplayTag(),
		ECk_AttributeModifier_Operation::Add,
		FCk_Fragment_FloatAttributeModifier_ParamsData(1.f, InAttributeComponent));
}