namespace utils_variables_bool
{
    TOptional<bool> Get(const FCk_Handle& InHandle, FGameplayTag InVariableName, ECk_Recursion InRecursion = ECk_Recursion::NotRecursive)
    {
        ECk_SucceededFailed OutSuccessFail = ECk_SucceededFailed::Failed;
        auto Result = UCk_Utils_Variables_Bool_UE::Get(InHandle, InVariableName, InRecursion, OutSuccessFail);
        
        if (OutSuccessFail == ECk_SucceededFailed::Succeeded)
        {
            return TOptional<bool>(Result);
        }
        return TOptional<bool>();
    }
}

//--------------------------------------------------------------------------------------------------------------------------

namespace utils_variables_byte
{
    TOptional<uint8> Get(const FCk_Handle& InHandle, FGameplayTag InVariableName, ECk_Recursion InRecursion = ECk_Recursion::NotRecursive)
    {
        ECk_SucceededFailed OutSuccessFail = ECk_SucceededFailed::Failed;
        auto Result = UCk_Utils_Variables_Byte_UE::Get(InHandle, InVariableName, InRecursion, OutSuccessFail);
        
        if (OutSuccessFail == ECk_SucceededFailed::Succeeded)
        {
            return TOptional<uint8>(Result);
        }
        return TOptional<uint8>();
    }
}

//--------------------------------------------------------------------------------------------------------------------------

namespace utils_variables_entity
{
    TOptional<FCk_Handle> Get(const FCk_Handle& InHandle, FGameplayTag InVariableName, ECk_Recursion InRecursion = ECk_Recursion::NotRecursive)
    {
        ECk_SucceededFailed OutSuccessFail = ECk_SucceededFailed::Failed;
        auto Result = UCk_Utils_Variables_Entity_UE::Get(InHandle, InVariableName, InRecursion, OutSuccessFail);
        
        if (OutSuccessFail == ECk_SucceededFailed::Succeeded)
        {
            return TOptional<FCk_Handle>(Result);
        }
        return TOptional<FCk_Handle>();
    }
}

//--------------------------------------------------------------------------------------------------------------------------

namespace utils_variables_float
{
    TOptional<float> Get(const FCk_Handle& InHandle, FGameplayTag InVariableName, ECk_Recursion InRecursion = ECk_Recursion::NotRecursive)
    {
        ECk_SucceededFailed OutSuccessFail = ECk_SucceededFailed::Failed;
        auto Result = UCk_Utils_Variables_Float_UE::Get(InHandle, InVariableName, InRecursion, OutSuccessFail);
        
        if (OutSuccessFail == ECk_SucceededFailed::Succeeded)
        {
            return TOptional<float>(Result);
        }
        return TOptional<float>();
    }
}

//--------------------------------------------------------------------------------------------------------------------------

namespace utils_variables_gameplay_tag
{
    TOptional<FGameplayTag> Get(const FCk_Handle& InHandle, FGameplayTag InVariableName, ECk_Recursion InRecursion = ECk_Recursion::NotRecursive)
    {
        ECk_SucceededFailed OutSuccessFail = ECk_SucceededFailed::Failed;
        auto Result = UCk_Utils_Variables_GameplayTag_UE::Get(InHandle, InVariableName, InRecursion, OutSuccessFail);
        
        if (OutSuccessFail == ECk_SucceededFailed::Succeeded)
        {
            return TOptional<FGameplayTag>(Result);
        }
        return TOptional<FGameplayTag>();
    }
}

//--------------------------------------------------------------------------------------------------------------------------

namespace utils_variables_gameplay_tag_container
{
    TOptional<FGameplayTagContainer> Get(const FCk_Handle& InHandle, FGameplayTag InVariableName, ECk_Recursion InRecursion = ECk_Recursion::NotRecursive)
    {
        ECk_SucceededFailed OutSuccessFail = ECk_SucceededFailed::Failed;
        auto Result = UCk_Utils_Variables_GameplayTagContainer_UE::Get(InHandle, InVariableName, InRecursion, OutSuccessFail);
        
        if (OutSuccessFail == ECk_SucceededFailed::Succeeded)
        {
            return TOptional<FGameplayTagContainer>(Result);
        }
        return TOptional<FGameplayTagContainer>();
    }
}

//--------------------------------------------------------------------------------------------------------------------------

namespace utils_variables_int32
{
    TOptional<int32> Get(const FCk_Handle& InHandle, FGameplayTag InVariableName, ECk_Recursion InRecursion = ECk_Recursion::NotRecursive)
    {
        ECk_SucceededFailed OutSuccessFail = ECk_SucceededFailed::Failed;
        auto Result = UCk_Utils_Variables_Int32_UE::Get(InHandle, InVariableName, InRecursion, OutSuccessFail);
        
        if (OutSuccessFail == ECk_SucceededFailed::Succeeded)
        {
            return TOptional<int32>(Result);
        }
        return TOptional<int32>();
    }
}

//--------------------------------------------------------------------------------------------------------------------------

namespace utils_variables_int64
{
    TOptional<int64> Get(const FCk_Handle& InHandle, FGameplayTag InVariableName, ECk_Recursion InRecursion = ECk_Recursion::NotRecursive)
    {
        ECk_SucceededFailed OutSuccessFail = ECk_SucceededFailed::Failed;
        auto Result = UCk_Utils_Variables_Int64_UE::Get(InHandle, InVariableName, InRecursion, OutSuccessFail);
        
        if (OutSuccessFail == ECk_SucceededFailed::Succeeded)
        {
            return TOptional<int64>(Result);
        }
        return TOptional<int64>();
    }
}

//--------------------------------------------------------------------------------------------------------------------------

namespace utils_variables_linear_color
{
    TOptional<FLinearColor> Get(const FCk_Handle& InHandle, FGameplayTag InVariableName, ECk_Recursion InRecursion = ECk_Recursion::NotRecursive)
    {
        ECk_SucceededFailed OutSuccessFail = ECk_SucceededFailed::Failed;
        auto Result = UCk_Utils_Variables_LinearColor_UE::Get(InHandle, InVariableName, InRecursion, OutSuccessFail);
        
        if (OutSuccessFail == ECk_SucceededFailed::Succeeded)
        {
            return TOptional<FLinearColor>(Result);
        }
        return TOptional<FLinearColor>();
    }
}

//--------------------------------------------------------------------------------------------------------------------------

namespace utils_variables_material
{
    TOptional<UMaterialInterface> Get(const FCk_Handle& InHandle, FGameplayTag InVariableName, ECk_Recursion InRecursion = ECk_Recursion::NotRecursive)
    {
        ECk_SucceededFailed OutSuccessFail = ECk_SucceededFailed::Failed;
        auto Result = UCk_Utils_Variables_Material_UE::Get(InHandle, InVariableName, InRecursion, OutSuccessFail);
        
        if (OutSuccessFail == ECk_SucceededFailed::Succeeded)
        {
            return TOptional<UMaterialInterface>(Result);
        }
        return TOptional<UMaterialInterface>();
    }
}

//--------------------------------------------------------------------------------------------------------------------------

namespace utils_variables_name
{
    TOptional<FName> Get(const FCk_Handle& InHandle, FGameplayTag InVariableName, ECk_Recursion InRecursion = ECk_Recursion::NotRecursive)
    {
        ECk_SucceededFailed OutSuccessFail = ECk_SucceededFailed::Failed;
        auto Result = UCk_Utils_Variables_Name_UE::Get(InHandle, InVariableName, InRecursion, OutSuccessFail);
        
        if (OutSuccessFail == ECk_SucceededFailed::Succeeded)
        {
            return TOptional<FName>(Result);
        }
        return TOptional<FName>();
    }
}

//--------------------------------------------------------------------------------------------------------------------------

namespace utils_variables_string
{
    TOptional<FString> Get(const FCk_Handle& InHandle, FGameplayTag InVariableName, ECk_Recursion InRecursion = ECk_Recursion::NotRecursive)
    {
        ECk_SucceededFailed OutSuccessFail = ECk_SucceededFailed::Failed;
        auto Result = UCk_Utils_Variables_String_UE::Get(InHandle, InVariableName, InRecursion, OutSuccessFail);
        
        if (OutSuccessFail == ECk_SucceededFailed::Succeeded)
        {
            return TOptional<FString>(Result);
        }
        return TOptional<FString>();
    }
}

//--------------------------------------------------------------------------------------------------------------------------

namespace utils_variables_text
{
    TOptional<FText> Get(const FCk_Handle& InHandle, FGameplayTag InVariableName, ECk_Recursion InRecursion = ECk_Recursion::NotRecursive)
    {
        ECk_SucceededFailed OutSuccessFail = ECk_SucceededFailed::Failed;
        auto Result = UCk_Utils_Variables_Text_UE::Get(InHandle, InVariableName, InRecursion, OutSuccessFail);
        
        if (OutSuccessFail == ECk_SucceededFailed::Succeeded)
        {
            return TOptional<FText>(Result);
        }
        return TOptional<FText>();
    }
}

//--------------------------------------------------------------------------------------------------------------------------

namespace utils_variables_vector
{
    TOptional<FVector> Get(const FCk_Handle& InHandle, FGameplayTag InVariableName, ECk_Recursion InRecursion = ECk_Recursion::NotRecursive)
    {
        ECk_SucceededFailed OutSuccessFail = ECk_SucceededFailed::Failed;
        auto Result = UCk_Utils_Variables_Vector_UE::Get(InHandle, InVariableName, InRecursion, OutSuccessFail);
        
        if (OutSuccessFail == ECk_SucceededFailed::Succeeded)
        {
            return TOptional<FVector>(Result);
        }
        return TOptional<FVector>();
    }
}

//--------------------------------------------------------------------------------------------------------------------------

namespace utils_variables_vector2_d
{
    TOptional<FVector2D> Get(const FCk_Handle& InHandle, FGameplayTag InVariableName, ECk_Recursion InRecursion = ECk_Recursion::NotRecursive)
    {
        ECk_SucceededFailed OutSuccessFail = ECk_SucceededFailed::Failed;
        auto Result = UCk_Utils_Variables_Vector2D_UE::Get(InHandle, InVariableName, InRecursion, OutSuccessFail);
        
        if (OutSuccessFail == ECk_SucceededFailed::Succeeded)
        {
            return TOptional<FVector2D>(Result);
        }
        return TOptional<FVector2D>();
    }
}

//--------------------------------------------------------------------------------------------------------------------------

namespace utils_variables_rotator
{
    TOptional<FRotator> Get(const FCk_Handle& InHandle, FGameplayTag InVariableName, ECk_Recursion InRecursion = ECk_Recursion::NotRecursive)
    {
        ECk_SucceededFailed OutSuccessFail = ECk_SucceededFailed::Failed;
        auto Result = UCk_Utils_Variables_Rotator_UE::Get(InHandle, InVariableName, InRecursion, OutSuccessFail);
        
        if (OutSuccessFail == ECk_SucceededFailed::Succeeded)
        {
            return TOptional<FRotator>(Result);
        }
        return TOptional<FRotator>();
    }
}

//--------------------------------------------------------------------------------------------------------------------------

namespace utils_variables_transform
{
    TOptional<FTransform> Get(const FCk_Handle& InHandle, FGameplayTag InVariableName, ECk_Recursion InRecursion = ECk_Recursion::NotRecursive)
    {
        ECk_SucceededFailed OutSuccessFail = ECk_SucceededFailed::Failed;
        auto Result = UCk_Utils_Variables_Transform_UE::Get(InHandle, InVariableName, InRecursion, OutSuccessFail);
        
        if (OutSuccessFail == ECk_SucceededFailed::Succeeded)
        {
            return TOptional<FTransform>(Result);
        }
        return TOptional<FTransform>();
    }
}

//--------------------------------------------------------------------------------------------------------------------------

namespace utils_variables_u_object_subclass_of
{
    TOptional<TSubclassOf<UObject>> Get(const FCk_Handle& InHandle, FGameplayTag InVariableName, TSubclassOf<UObject> InObject, ECk_Recursion InRecursion = ECk_Recursion::NotRecursive)
    {
        ECk_SucceededFailed OutSuccessFail = ECk_SucceededFailed::Failed;;
        auto Result =  UCk_Utils_Variables_UObject_SubclassOf_UE::Get(InHandle, InVariableName, InObject, InRecursion, OutSuccessFail);
        
        if (OutSuccessFail == ECk_SucceededFailed::Succeeded)
        {
            return TOptional<TSubclassOf<UObject>>(Result);
        }
        return TOptional<TSubclassOf<UObject>>();
    }


}

//--------------------------------------------------------------------------------------------------------------------------
