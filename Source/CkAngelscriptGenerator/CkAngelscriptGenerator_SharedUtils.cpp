#include "CkAngelscriptGenerator_SharedUtils.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat.h"

#include <UObject/PropertyOptional.h>
#include <UObject/UnrealType.h>

#if WITH_ANGELSCRIPT_CK
#include <AngelscriptType.h>
#include <AngelscriptManager.h>
#include <ClassGenerator/ASClass.h>
#endif

// --------------------------------------------------------------------------------------------------------------------

namespace
{
    struct FReflectedObjectWrapperType
    {
        FString WrapperName;
        FString Declaration;
    };

    auto Make_ReflectedObjectWrapperType(
        const TCHAR* InWrapperName,
        const UClass* InObjectClass) -> TOptional<FReflectedObjectWrapperType>
    {
        if (InObjectClass == nullptr)
        { return {}; }

        return FReflectedObjectWrapperType
        {
            InWrapperName,
            ck::Format_UE(
                TEXT("{}<{}{}>"),
                InWrapperName,
                InObjectClass->GetPrefixCPP(),
                InObjectClass->GetName())
        };
    }

    // FSoftClassProperty derives from FSoftObjectProperty, so the class case
    // must be checked first or it collapses to TSoftObjectPtr<UClass>.
    auto Get_ReflectedObjectWrapperType(
        FProperty* InProperty) -> TOptional<FReflectedObjectWrapperType>
    {
        if (const auto* SoftClassProperty = CastField<FSoftClassProperty>(InProperty))
        {
            return Make_ReflectedObjectWrapperType(
                TEXT("TSoftClassPtr"),
                SoftClassProperty->MetaClass.Get());
        }

        if (const auto* WeakObjectProperty = CastField<FWeakObjectProperty>(InProperty))
        {
            const auto* WrapperName = WeakObjectProperty->HasAnyPropertyFlags(CPF_AutoWeak)
                ? TEXT("TAutoWeakObjectPtr")
                : TEXT("TWeakObjectPtr");
            return Make_ReflectedObjectWrapperType(
                WrapperName,
                WeakObjectProperty->PropertyClass.Get());
        }

        if (const auto* SoftObjectProperty = CastField<FSoftObjectProperty>(InProperty))
        {
            return Make_ReflectedObjectWrapperType(
                TEXT("TSoftObjectPtr"),
                SoftObjectProperty->PropertyClass.Get());
        }

        return {};
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAngelscriptGenerator_SharedUtils::
    Get_ConvertedToAngelscriptType(
        const FString& UnrealType)
    -> FString
{
    auto Result = UnrealType;

    if (Result.StartsWith(TEXT("TArray<")) && Result.EndsWith(TEXT(">")))
    {
        auto StartIndex = int32{7}; // Length of "TArray<"
        auto EndIndex = Result.Len() - 1; // Remove the closing ">"
        auto InnerType = Result.Mid(StartIndex, EndIndex - StartIndex);

        auto CleanInnerType = Get_ConvertedToAngelscriptType(InnerType);
        Result = ck::Format_UE(TEXT("TArray<{}>"), CleanInnerType);
        return Result;
    }

    if (Result.StartsWith(TEXT("TMap<")) && Result.EndsWith(TEXT(">")))
    {
        auto StartIndex = int32{5}; // Length of "TMap<"
        auto EndIndex = Result.Len() - 1;
        auto InnerTypes = Result.Mid(StartIndex, EndIndex - StartIndex);

        auto CommaIndex = InnerTypes.Find(TEXT(","));
        if (CommaIndex != INDEX_NONE)
        {
            auto KeyType = InnerTypes.Left(CommaIndex).TrimStartAndEnd();
            auto ValueType = InnerTypes.Mid(CommaIndex + 1).TrimStartAndEnd();

            auto CleanKeyType = Get_ConvertedToAngelscriptType(KeyType);
            auto CleanValueType = Get_ConvertedToAngelscriptType(ValueType);
            Result = ck::Format_UE(TEXT("TMap<{}, {}>"), CleanKeyType, CleanValueType);
            return Result;
        }
    }

    if (Result.StartsWith(TEXT("TSet<")) && Result.EndsWith(TEXT(">")))
    {
        auto StartIndex = int32{5}; // Length of "TSet<"
        auto EndIndex = Result.Len() - 1;
        auto InnerType = Result.Mid(StartIndex, EndIndex - StartIndex);

        auto CleanInnerType = Get_ConvertedToAngelscriptType(InnerType);
        Result = ck::Format_UE(TEXT("TSet<{}>"), CleanInnerType);
        return Result;
    }

    if (Result.StartsWith(TEXT("TEnumAsByte<")) && Result.EndsWith(TEXT(">")))
    {
        Result = Result.Mid(12); // Remove "TEnumAsByte<"
        Result = Result.Left(Result.Len() - 1); // Remove ">"
    }

    if (Result.StartsWith(TEXT("TOptional<")) && Result.EndsWith(TEXT(">")))
    {
        auto StartIndex = int32{10}; // Length of "TOptional<"
        auto EndIndex = Result.Len() - 1;
        auto InnerType = Result.Mid(StartIndex, EndIndex - StartIndex);

        auto CleanInnerType = Get_ConvertedToAngelscriptType(InnerType);
        Result = ck::Format_UE(TEXT("TOptional<{}>"), CleanInnerType);
        return Result;
    }

    // AngelScript has no pointers.
    Result = Result.Replace(TEXT("*"), TEXT(""));

    if (Result.EndsWith(TEXT(" const")))
    {
        Result = Result.Left(Result.Len() - 6);
    }

    Result = Result.TrimStartAndEnd();

    return Result;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAngelscriptGenerator_SharedUtils::
    Get_DetailedPropertyType(
        FProperty* InProperty)
    -> FString
{
    if (ck::Is_NOT_Valid(InProperty))
    { return TEXT("void"); }

    // Independent source of truth for retention: in some reinstancing states a runtime-generated
    // AS property reports a raw UObject/TObjectPtr declaration even though its FProperty
    // subclass still records the weak/soft wrapper.
    const auto ReflectedObjectWrapperType = Get_ReflectedObjectWrapperType(InProperty);

#if WITH_ANGELSCRIPT_CK
    // An AS-declared UPROPERTY keeps its narrow type (e.g. a typesafe FCk_Handle_Trigger) only in
    // AS's asITypeInfo — the FProperty collapses it to FCk_Handle. Walk UClass -> UASClass ->
    // asITypeInfo to recover the declaration as written in source.
    if ((InProperty->AngelscriptPropertyFlags & APF_RuntimeGenerated) != 0)
    {
        if (auto* OwnerClass = InProperty->GetOwner<UClass>())
        {
            if (auto* ASClass = UASClass::GetFirstASClass(OwnerClass))
            {
                if (auto* ScriptType = static_cast<asITypeInfo*>(ASClass->ScriptTypePtr))
                {
                    const auto PropName = InProperty->GetName();
                    const auto PropCount = ScriptType->GetPropertyCount();
                    for (asUINT i = 0; i < PropCount; ++i)
                    {
                        const char* AsPropName = nullptr;
                        auto AsTypeId = int{0};
                        ScriptType->GetProperty(i, &AsPropName, &AsTypeId);
                        if (AsPropName != nullptr && PropName.Equals(UTF8_TO_TCHAR(AsPropName)))
                        {
                            const auto Usage = FAngelscriptTypeUsage::FromProperty(ScriptType, i);
                            if (Usage.IsValid() && Usage.Type.IsValid())
                            {
                                auto Decl = Usage.GetAngelscriptDeclaration();
                                if (NOT Decl.IsEmpty())
                                {
                                    if (ReflectedObjectWrapperType.IsSet())
                                    {
                                        const auto WrapperPrefix = ReflectedObjectWrapperType->WrapperName + TEXT("<");
                                        const auto ConstWrapperPrefix = TEXT("const ") + WrapperPrefix;
                                        if (NOT Decl.StartsWith(WrapperPrefix)
                                            && NOT Decl.StartsWith(ConstWrapperPrefix))
                                        {
                                            return ReflectedObjectWrapperType->Declaration;
                                        }
                                    }

                                    // FAngelscriptTypeUsage::FromProperty never populates
                                    // bIsConst, so `const UFoo` loses its qualifier — recover it
                                    // from the flag AS encodes into the property's TypeId.
                                    const auto bIsConstHandle = (AsTypeId & asTYPEID_HANDLETOCONST) != 0;
                                    if (bIsConstHandle && NOT Decl.StartsWith(TEXT("const ")))
                                    {
                                        Decl = TEXT("const ") + Decl;
                                    }
                                    return Decl;
                                }
                            }
                            break;
                        }
                    }
                }
            }
        }
    }
#endif

    if (ReflectedObjectWrapperType.IsSet())
    { return ReflectedObjectWrapperType->Declaration; }

    if (auto ArrayProp = CastField<FArrayProperty>(InProperty))
    {
        auto InnerType = Get_DetailedPropertyType(ArrayProp->Inner);
        return ck::Format_UE(TEXT("TArray<{}>"), InnerType);
    }

    if (auto MapProp = CastField<FMapProperty>(InProperty))
    {
        auto KeyType = Get_DetailedPropertyType(MapProp->KeyProp);
        auto ValueType = Get_DetailedPropertyType(MapProp->ValueProp);
        return ck::Format_UE(TEXT("TMap<{}, {}>"), KeyType, ValueType);
    }

    if (auto SetProp = CastField<FSetProperty>(InProperty))
    {
        auto ElementType = Get_DetailedPropertyType(SetProp->ElementProp);
        return ck::Format_UE(TEXT("TSet<{}>"), ElementType);
    }

    if (auto OptionalProp = CastField<FOptionalProperty>(InProperty))
    {
        auto ValueType = Get_DetailedPropertyType(OptionalProp->GetValueProperty());
        return ck::Format_UE(TEXT("TOptional<{}>"), ValueType);
    }

    if (auto EnumProp = CastField<FEnumProperty>(InProperty))
    {
        return EnumProp->GetEnum()->GetName();
    }

    if (auto ByteProp = CastField<FByteProperty>(InProperty))
    {
        if (ck::IsValid(ByteProp->Enum))
        {
            return ByteProp->Enum->GetName();
        }
    }

    if (CastField<FFloatProperty>(InProperty))
    {
        return TEXT("float32");
    }

    if (CastField<FDoubleProperty>(InProperty))
    {
        return TEXT("float64");
    }

    auto CppType = InProperty->GetCPPType();
    return Get_ConvertedToAngelscriptType(CppType);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAngelscriptGenerator_SharedUtils::
    Get_ConvertedClassNameToNamespace(
        const FString& ClassName)
    -> FString
{
    auto Result = ClassName;

    if (Result.StartsWith(TEXT("U")))
    {
        Result = Result.Mid(1);
    }

    if (Result.StartsWith(TEXT("Ck_")))
    {
        Result = Result.Mid(3);
    }

    if (Result.EndsWith(TEXT("_UE")))
    {
        Result = Result.Left(Result.Len() - 3);
    }
    else if (Result.EndsWith(TEXT("FunctionLibrary")))
    {
        Result = Result.Left(Result.Len() - 15);
    }

    auto Converted = FString{};
    for (auto I = int32{0}; I < Result.Len(); I++)
    {
        auto Char = Result[I];

        if (Char == TEXT('_'))
        {
            Converted += TEXT("_");
        }
        else if (I > 0 && FChar::IsUpper(Char))
        {
            if (Result[I - 1] != TEXT('_'))
            {
                Converted += TEXT("_");
            }
            Converted += FChar::ToLower(Char);
        }
        else
        {
            Converted += FChar::ToLower(Char);
        }
    }

    return Converted;
}

// --------------------------------------------------------------------------------------------------------------------
