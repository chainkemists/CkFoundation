#include "CkReflection_Utils.h"

#include "CkCore/CkCoreLog.h"
#include "CkCore/Ensure/CkEnsure.h"

#if WITH_EDITOR
#include "Kismet2/BlueprintEditorUtils.h"
#endif

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Reflection_UE::
    Get_SanitizedUserDefinedPropertyName(
        const FProperty* InProperty)
    -> FString
{
    CK_ENSURE_IF_NOT(ck::IsValid(InProperty), TEXT("Invalid Property"))
    { return {}; }

    const auto& PropertyName = SlugStringForValidName(InProperty->GetName());

    int32 LastUnderscoreIndex = INDEX_NONE;
    int32 SecondLastUnderscoreIndex = INDEX_NONE;

    if (PropertyName.FindLastChar(TEXT('_'), LastUnderscoreIndex))
    {
        if (constexpr auto GuidLength = 32;
            PropertyName.Len() - LastUnderscoreIndex - 1 == GuidLength)
        {
            if (const auto& BeforeLastUnderscore = PropertyName.Left(LastUnderscoreIndex);
                BeforeLastUnderscore.FindLastChar(TEXT('_'), SecondLastUnderscoreIndex))
            {
                return BeforeLastUnderscore.Left(SecondLastUnderscoreIndex);
            }
        }
    }

    return PropertyName;
}

auto
    UCk_Utils_Reflection_UE::
    Get_PropertyBySanitizedName(
        UObject* InObject,
        const FString& InSanitizedPropertyName)
    -> FProperty*
{
    CK_ENSURE_IF_NOT(ck::IsValid(InObject), TEXT("Invalid Object"))
    { return nullptr; }

    if (InSanitizedPropertyName.IsEmpty())
    { return nullptr; }

    const UClass* ObjectClass = InObject->GetClass();

    // Iterate through all properties in the class hierarchy
    for (TFieldIterator<FProperty> PropertyIt(ObjectClass, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
    {
        FProperty* Property = *PropertyIt;

        if (ck::Is_NOT_Valid(Property, ck::IsValid_Policy_NullptrOnly{}))
        { continue; }

        // Compare the sanitized version of this property's name with the target
        const FString SanitizedCurrentPropertyName = Get_SanitizedUserDefinedPropertyName(Property);

        if (SanitizedCurrentPropertyName.Equals(InSanitizedPropertyName, ESearchCase::IgnoreCase))
        {
            return Property;
        }
    }

    return nullptr;
}

auto
    UCk_Utils_Reflection_UE::
    Get_UserDefinedPropertyGuid(
        const FProperty* InProperty)
    -> FString
{
    CK_ENSURE_IF_NOT(ck::IsValid(InProperty), TEXT("Invalid Property"))
    { return {}; }

    const auto& PropertyName = SlugStringForValidName(InProperty->GetName());

    int32 LastUnderscoreIndex = INDEX_NONE;
    if (PropertyName.FindLastChar(TEXT('_'), LastUnderscoreIndex))
    {
        return PropertyName.Right(LastUnderscoreIndex);
    }

    return PropertyName;
}

auto
    UCk_Utils_Reflection_UE::
    Get_ArePropertiesCompatible(
        const FProperty* InPropertyA,
        const FProperty* InPropertyB)
    -> bool
{
    if (ck::Is_NOT_Valid(InPropertyA, ck::IsValid_Policy_NullptrOnly{}) ||
        ck::Is_NOT_Valid(InPropertyB, ck::IsValid_Policy_NullptrOnly{}))
    { return {}; }

    if (InPropertyA->GetClass() != InPropertyB->GetClass())
    { return {}; }

    if (const auto& ObjectPropA = CastField<FObjectPropertyBase>(InPropertyA))
    {
        if (const auto& ObjectPropB = CastField<FObjectPropertyBase>(InPropertyB))
        {
            return ObjectPropA->PropertyClass == ObjectPropB->PropertyClass;
        }
    }

    if (const auto& SoftObjectPropA = CastField<FSoftObjectProperty>(InPropertyA))
    {
        if (const auto& SoftObjectPropB = CastField<FSoftObjectProperty>(InPropertyB))
        {
            return SoftObjectPropA->PropertyClass == SoftObjectPropB->PropertyClass;
        }
    }

    if (const auto& WeakObjectPropA = CastField<FWeakObjectProperty>(InPropertyA))
    {
        if (const auto& WeakObjectPropB = CastField<FWeakObjectProperty>(InPropertyB))
        {
            return WeakObjectPropA->PropertyClass == WeakObjectPropB->PropertyClass;
        }
    }

    if (const auto& ClassPropA = CastField<FClassProperty>(InPropertyA))
    {
        if (const auto& ClassPropB = CastField<FClassProperty>(InPropertyB))
        {
            return ClassPropA->MetaClass == ClassPropB->MetaClass;
        }
    }

    if (const auto& SoftClassPropA = CastField<FSoftClassProperty>(InPropertyA))
    {
        if (const auto& SoftClassPropB = CastField<FSoftClassProperty>(InPropertyB))
        {
            return SoftClassPropA->MetaClass == SoftClassPropB->MetaClass;
        }
    }

    if (const auto& InterfacePropA = CastField<FInterfaceProperty>(InPropertyA))
    {
        if (const auto& InterfacePropB = CastField<FInterfaceProperty>(InPropertyB))
        {
            return InterfacePropA->InterfaceClass == InterfacePropB->InterfaceClass;
        }
    }

    if (const auto& StructPropA = CastField<FStructProperty>(InPropertyA))
    {
        if (const auto& StructPropB = CastField<FStructProperty>(InPropertyB))
        {
            return StructPropA->Struct == StructPropB->Struct;
        }
    }

    if (const auto& EnumPropA = CastField<FEnumProperty>(InPropertyA))
    {
        if (const auto& EnumPropB = CastField<FEnumProperty>(InPropertyB))
        {
            return EnumPropA->GetEnum() == EnumPropB->GetEnum();
        }
    }

    if (const auto& BytePropA = CastField<FByteProperty>(InPropertyA))
    {
        if (const auto& BytePropB = CastField<FByteProperty>(InPropertyB))
        {
            return BytePropA->Enum == BytePropB->Enum;
        }
    }

    if (const auto& ArrayPropA = CastField<FArrayProperty>(InPropertyA))
    {
        if (const auto& ArrayPropB = CastField<FArrayProperty>(InPropertyB))
        {
            return Get_ArePropertiesCompatible(ArrayPropA->Inner, ArrayPropB->Inner);
        }
    }

    if (const auto& SetPropA = CastField<FSetProperty>(InPropertyA))
    {
        if (const auto& SetPropB = CastField<FSetProperty>(InPropertyB))
        {
            return Get_ArePropertiesCompatible(SetPropA->ElementProp, SetPropB->ElementProp);
        }
    }

    if (const auto& MapPropA = CastField<FMapProperty>(InPropertyA))
    {
        if (const auto& MapPropB = CastField<FMapProperty>(InPropertyB))
        {
            return Get_ArePropertiesCompatible(MapPropA->KeyProp, MapPropB->KeyProp) &&
                   Get_ArePropertiesCompatible(MapPropA->ValueProp, MapPropB->ValueProp);
        }
    }

    if (const auto& DelegatePropA = CastField<FDelegateProperty>(InPropertyA))
    {
        if (const auto& DelegatePropB = CastField<FDelegateProperty>(InPropertyB))
        {
            return DelegatePropA->SignatureFunction == DelegatePropB->SignatureFunction;
        }
    }

    if (const auto& MulticastPropA = CastField<FMulticastDelegateProperty>(InPropertyA))
    {
        if (const auto& MulticastPropB = CastField<FMulticastDelegateProperty>(InPropertyB))
        {
            return MulticastPropA->SignatureFunction == MulticastPropB->SignatureFunction;
        }
    }

    if (const auto& MulticastSparsePropA = CastField<FMulticastSparseDelegateProperty>(InPropertyA))
    {
        if (const auto& MulticastSparsePropB = CastField<FMulticastSparseDelegateProperty>(InPropertyB))
        {
            return MulticastSparsePropA->SignatureFunction == MulticastSparsePropB->SignatureFunction;
        }
    }

    return true;
}

auto
    UCk_Utils_Reflection_UE::
    Get_ArePropertiesDifferent(
        const TArray<FProperty*>& InPropertiesA,
        const TArray<FProperty*>& InPropertiesB)
    -> bool
{
    if (InPropertiesA.Num() != InPropertiesB.Num())
    { return true; }

    auto PropertiesA_Map = TMap<FName, FProperty*>{};

    for (auto* Property : InPropertiesA)
    {
        if (ck::IsValid(Property, ck::IsValid_Policy_NullptrOnly{}))
        {
            PropertiesA_Map.Add(*Get_SanitizedUserDefinedPropertyName(Property), Property);
        }
    }

    for (const auto* NewProperty : InPropertiesB)
    {
        const auto& FoundExistingProperty = PropertiesA_Map.Find(*Get_SanitizedUserDefinedPropertyName(NewProperty));

        if (ck::Is_NOT_Valid(FoundExistingProperty, ck::IsValid_Policy_NullptrOnly{}))
        { return true; }

        if (const auto ExistingProperty = *FoundExistingProperty;
            NOT Get_ArePropertiesCompatible(ExistingProperty, NewProperty))
        { return true; }
    }

    return false;
}

auto
    UCk_Utils_Reflection_UE::
    Get_ExposedPropertiesOfClass(
        const UClass* InClass)
    -> TArray<FProperty*>
{
    auto ExposedProperties = TArray<FProperty*>{};

#if WITH_EDITOR
    for (TFieldIterator<FProperty> PropertyIt(InClass, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
    {
        auto* Property = *PropertyIt;

        const auto& IsDelegate          = Property->IsA(FMulticastDelegateProperty::StaticClass());
        const auto& IsExposedToSpawn    = UEdGraphSchema_K2::IsPropertyExposedOnSpawn(Property);
        const auto& IsSettableExternally = NOT Property->HasAnyPropertyFlags(CPF_DisableEditOnInstance);
        const auto& IsParm              = Property->HasAnyPropertyFlags(CPF_Parm);
        const auto& StillExists         = FBlueprintEditorUtils::PropertyStillExists(Property);
        const auto& IsBpVisible         = Property->HasAllPropertyFlags(CPF_BlueprintVisible);

        const auto WouldBeIncluded = NOT IsParm && StillExists && IsBpVisible && IsSettableExternally && IsExposedToSpawn && NOT IsDelegate;
        if (NOT WouldBeIncluded)
        {
            ck::core::Warning(
                TEXT("[ExposedProps] SKIP Class=[{}] Prop=[{}] | IsParm={} StillExists={} BpVisible={} Settable={} ExposedToSpawn={} IsDelegate={}"),
                InClass->GetName(),
                Property->GetName(),
                IsParm, StillExists, IsBpVisible, IsSettableExternally, IsExposedToSpawn, IsDelegate);
            continue;
        }

        ExposedProperties.Add(Property);
    }
#endif

    return ExposedProperties;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Reflection_UE::
    Get_IsDelegateProperty(
        const FProperty* InProperty)
    -> bool
{
    if (ck::Is_NOT_Valid(InProperty, ck::IsValid_Policy_NullptrOnly{}))
    { return false; }

    // FDelegateProperty covers single-cast delegates.
    // FMulticastDelegateProperty covers all multicast variants
    //   (FMulticastInlineDelegateProperty, FMulticastSparseDelegateProperty).
    return CastField<FDelegateProperty>(InProperty) != nullptr
        || CastField<FMulticastDelegateProperty>(InProperty) != nullptr;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Reflection_UE::
    ClonePropertyForStruct(
        const FProperty* InSourceProperty,
        UScriptStruct* InTargetStruct)
    -> FProperty*
{
    if (ck::Is_NOT_Valid(InSourceProperty, ck::IsValid_Policy_NullptrOnly{}) ||
        ck::Is_NOT_Valid(InTargetStruct, ck::IsValid_Policy_NullptrOnly{}))
    { return nullptr; }

    const auto Name = InSourceProperty->GetFName();
    constexpr auto ObjectFlags = RF_Public;
    auto* Result = static_cast<FProperty*>(nullptr);

    // ---- Delegate types ----

    if (const auto* DelegateSource = CastField<FDelegateProperty>(InSourceProperty))
    {
        auto* Prop = new FDelegateProperty(InTargetStruct, Name, ObjectFlags);
        Prop->SignatureFunction = DelegateSource->SignatureFunction;
        Result = Prop;
    }
    else if (const auto* MulticastSource = CastField<FMulticastInlineDelegateProperty>(InSourceProperty))
    {
        auto* Prop = new FMulticastInlineDelegateProperty(InTargetStruct, Name, ObjectFlags);
        Prop->SignatureFunction = MulticastSource->SignatureFunction;
        Result = Prop;
    }

    // ---- Object types (order matters: derived before base) ----

    else if (const auto* ClassSource = CastField<FClassProperty>(InSourceProperty))
    {
        auto* Prop = new FClassProperty(InTargetStruct, Name, ObjectFlags);
        Prop->PropertyClass = UClass::StaticClass();
        Prop->MetaClass = ClassSource->MetaClass;
        Result = Prop;
    }
    else if (const auto* SoftClassSource = CastField<FSoftClassProperty>(InSourceProperty))
    {
        auto* Prop = new FSoftClassProperty(InTargetStruct, Name, ObjectFlags);
        Prop->PropertyClass = UClass::StaticClass();
        Prop->MetaClass = SoftClassSource->MetaClass;
        Result = Prop;
    }
    else if (const auto* ObjectSource = CastField<FObjectProperty>(InSourceProperty))
    {
        auto* Prop = new FObjectProperty(InTargetStruct, Name, ObjectFlags);
        Prop->PropertyClass = ObjectSource->PropertyClass;
        Result = Prop;
    }
    else if (const auto* WeakObjSource = CastField<FWeakObjectProperty>(InSourceProperty))
    {
        auto* Prop = new FWeakObjectProperty(InTargetStruct, Name, ObjectFlags);
        Prop->PropertyClass = WeakObjSource->PropertyClass;
        Result = Prop;
    }
    else if (const auto* SoftObjSource = CastField<FSoftObjectProperty>(InSourceProperty))
    {
        auto* Prop = new FSoftObjectProperty(InTargetStruct, Name, ObjectFlags);
        Prop->PropertyClass = SoftObjSource->PropertyClass;
        Result = Prop;
    }
    else if (const auto* InterfaceSource = CastField<FInterfaceProperty>(InSourceProperty))
    {
        auto* Prop = new FInterfaceProperty(InTargetStruct, Name, ObjectFlags);
        Prop->InterfaceClass = InterfaceSource->InterfaceClass;
        Result = Prop;
    }

    // ---- Struct type ----

    else if (const auto* StructSource = CastField<FStructProperty>(InSourceProperty))
    {
        auto* Prop = new FStructProperty(InTargetStruct, Name, ObjectFlags);
        Prop->Struct = StructSource->Struct;
        Result = Prop;
    }

    // ---- Enum types ----

    else if (const auto* EnumSource = CastField<FEnumProperty>(InSourceProperty))
    {
        auto* Prop = new FEnumProperty(InTargetStruct, Name, ObjectFlags);
        Prop->SetEnum(EnumSource->GetEnum());
        auto* UnderlyingProp = new FByteProperty(Prop, TEXT("UnderlyingType"), ObjectFlags);
        UnderlyingProp->Enum = EnumSource->GetEnum();
        Prop->AddCppProperty(UnderlyingProp);
        Result = Prop;
    }
    else if (const auto* ByteSource = CastField<FByteProperty>(InSourceProperty))
    {
        auto* Prop = new FByteProperty(InTargetStruct, Name, ObjectFlags);
        Prop->Enum = ByteSource->Enum;
        Result = Prop;
    }

    // ---- Bool ----

    else if (CastField<FBoolProperty>(InSourceProperty))
    {
        auto* Prop = new FBoolProperty(InTargetStruct, Name, ObjectFlags);
        Prop->SetBoolSize(sizeof(bool), true);
        Result = Prop;
    }

    // ---- Container types ----

    else if (const auto* ArraySource = CastField<FArrayProperty>(InSourceProperty))
    {
        auto* Prop = new FArrayProperty(InTargetStruct, Name, ObjectFlags);
        if (auto* Inner = ClonePropertyForStruct(ArraySource->Inner, InTargetStruct))
        {
            Prop->Inner = Inner;
        }
        Result = Prop;
    }
    else if (const auto* SetSource = CastField<FSetProperty>(InSourceProperty))
    {
        auto* Prop = new FSetProperty(InTargetStruct, Name, ObjectFlags);
        if (auto* ElementProp = ClonePropertyForStruct(SetSource->ElementProp, InTargetStruct))
        {
            Prop->ElementProp = ElementProp;
        }
        Result = Prop;
    }
    else if (const auto* MapSource = CastField<FMapProperty>(InSourceProperty))
    {
        auto* Prop = new FMapProperty(InTargetStruct, Name, ObjectFlags);
        if (auto* KeyProp = ClonePropertyForStruct(MapSource->KeyProp, InTargetStruct))
        {
            Prop->KeyProp = KeyProp;
        }
        if (auto* ValueProp = ClonePropertyForStruct(MapSource->ValueProp, InTargetStruct))
        {
            Prop->ValueProp = ValueProp;
        }
        Result = Prop;
    }

    // ---- Primitive types ----

    else if (CastField<FIntProperty>(InSourceProperty))
    { Result = new FIntProperty(InTargetStruct, Name, ObjectFlags); }
    else if (CastField<FInt64Property>(InSourceProperty))
    { Result = new FInt64Property(InTargetStruct, Name, ObjectFlags); }
    else if (CastField<FFloatProperty>(InSourceProperty))
    { Result = new FFloatProperty(InTargetStruct, Name, ObjectFlags); }
    else if (CastField<FDoubleProperty>(InSourceProperty))
    { Result = new FDoubleProperty(InTargetStruct, Name, ObjectFlags); }
    else if (CastField<FNameProperty>(InSourceProperty))
    { Result = new FNameProperty(InTargetStruct, Name, ObjectFlags); }
    else if (CastField<FStrProperty>(InSourceProperty))
    { Result = new FStrProperty(InTargetStruct, Name, ObjectFlags); }
    else if (CastField<FTextProperty>(InSourceProperty))
    { Result = new FTextProperty(InTargetStruct, Name, ObjectFlags); }

    // ---- Fallback ----

    if (ck::Is_NOT_Valid(Result, ck::IsValid_Policy_NullptrOnly{}))
    {
        ck::core::Warning(TEXT("[ClonePropertyForStruct] Unsupported property type [{}] for property [{}]"),
            InSourceProperty->GetClass()->GetName(), Name);
        return nullptr;
    }

    // ---- Copy common flags ----

    constexpr auto RelevantFlags = CPF_BlueprintVisible | CPF_HasGetValueTypeHash
        | CPF_ContainsInstancedReference | CPF_InstancedReference | CPF_UObjectWrapper;

    Result->SetPropertyFlags(InSourceProperty->GetPropertyFlags() & RelevantFlags);

    return Result;
}

// --------------------------------------------------------------------------------------------------------------------
