#include "CkUnrealVariables_Utils.h"
#include "CkVariables_Utils.h"

#if WITH_ANGELSCRIPT_CK

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Format/CkFormat_AngelScript.h"
#include "CkCore/AngelScript/CkAngelScript_TypeValidation.h"

#include <AngelscriptBinds.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::variables_mixin
{
    template <typename T_UtilsClass>
    auto
    BindVariableMixin(
        FAngelscriptBinds& Handle,
        const FString& InSuffix)
    -> void
    {
        using UtilsType = typename T_UtilsClass::UtilsType;
        using ValueType = typename T_UtilsClass::FragmentType::ValueType;
        using ArgType   = typename T_UtilsClass::FragmentType::ArgType;

        const auto AsTypeName = ck::Get_RuntimeTypeToString_AngelScript<ValueType>();

        if (NOT ck::angelscript::IsTypeValidForAngelScript(AsTypeName))
        { return; }

        // ---- StoreVar ----

        Handle.Method(
            ck::Format_ANSI(TEXT("void StoreVar_{}(FGameplayTag InName, {} InValue)"),
                InSuffix, AsTypeName).c_str(),
            [](FCk_Handle& Self, FGameplayTag InName, ArgType InValue) -> void
            { T_UtilsClass::Set(Self, InName, InValue); });

        // ---- GetVar ----

        Handle.Method(
            ck::Format_ANSI(TEXT("TOptional<{}> GetVar_{}(FGameplayTag InName,"
                " ECk_Recursion InRecursion = ECk_Recursion::NotRecursive) const"),
                AsTypeName, InSuffix).c_str(),
            [](const FCk_Handle& Self, FGameplayTag InName,
               ECk_Recursion InRecursion) -> TOptional<ValueType>
            { return T_UtilsClass::Get_Optional(Self, InName, InRecursion); });

        // ---- HasVar ----

        Handle.Method(
            ck::Format_ANSI(TEXT("bool HasVar_{}(FGameplayTag InName) const"),
                InSuffix).c_str(),
            [](const FCk_Handle& Self, FGameplayTag InName) -> bool
            { return UtilsType::Has(Self, InName); });
    }
}

// --------------------------------------------------------------------------------------------------------------------

AS_FORCE_LINK const FAngelscriptBinds::FBind Bind_CkVariablesMixin(
    static_cast<int32>(FAngelscriptBinds::EOrder::Late), []
{
    using namespace ck::variables_mixin;

    auto Handle = FAngelscriptBinds::ExistingClass("FCk_Handle");

    // ---- Standard types ----

    BindVariableMixin<UCk_Utils_Variables_Bool_UE>                  (Handle, TEXT("Bool"));
    BindVariableMixin<UCk_Utils_Variables_Byte_UE>                  (Handle, TEXT("Byte"));
    BindVariableMixin<UCk_Utils_Variables_Int32_UE>                 (Handle, TEXT("Int32"));
    BindVariableMixin<UCk_Utils_Variables_Int64_UE>                 (Handle, TEXT("Int64"));
    BindVariableMixin<UCk_Utils_Variables_Float_UE>                 (Handle, TEXT("Float"));
    BindVariableMixin<UCk_Utils_Variables_Name_UE>                  (Handle, TEXT("Name"));
    BindVariableMixin<UCk_Utils_Variables_String_UE>                (Handle, TEXT("String"));
    BindVariableMixin<UCk_Utils_Variables_Text_UE>                  (Handle, TEXT("Text"));
    BindVariableMixin<UCk_Utils_Variables_Vector_UE>                (Handle, TEXT("Vector"));
    BindVariableMixin<UCk_Utils_Variables_Vector2D_UE>              (Handle, TEXT("Vector2D"));
    BindVariableMixin<UCk_Utils_Variables_Rotator_UE>               (Handle, TEXT("Rotator"));
    BindVariableMixin<UCk_Utils_Variables_Transform_UE>             (Handle, TEXT("Transform"));
    BindVariableMixin<UCk_Utils_Variables_GameplayTag_UE>           (Handle, TEXT("GameplayTag"));
    BindVariableMixin<UCk_Utils_Variables_GameplayTagContainer_UE>  (Handle, TEXT("GameplayTagContainer"));
    BindVariableMixin<UCk_Utils_Variables_Entity_UE>                (Handle, TEXT("Entity"));
    BindVariableMixin<UCk_Utils_Variables_LinearColor_UE>           (Handle, TEXT("LinearColor"));
    BindVariableMixin<UCk_Utils_Variables_InstancedStruct_UE>       (Handle, TEXT("InstancedStruct"));
    BindVariableMixin<UCk_Utils_Variables_UObject_SubclassOf_UE>    (Handle, TEXT("SubclassOf"));

    // ---- UObject (TWeakObjectPtr<UObject> storage, exposed as UObject pointer) ----

    Handle.Method(
        "void StoreVar_UObject(FGameplayTag InName, UObject InValue)",
        [](FCk_Handle& Self, FGameplayTag InName, UObject* InValue) -> void
        { UCk_Utils_Variables_UObject_UE::Set(Self, InName, InValue); });

    Handle.Method(
        "TOptional<UObject> GetVar_UObject(FGameplayTag InName,"
        " ECk_Recursion InRecursion = ECk_Recursion::NotRecursive) const",
        [](const FCk_Handle& Self, FGameplayTag InName,
           ECk_Recursion InRecursion) -> TOptional<UObject*>
        { return UCk_Utils_Variables_UObject_UE::Get_Optional(Self, InName, InRecursion); });

    Handle.Method(
        "bool HasVar_UObject(FGameplayTag InName) const",
        [](const FCk_Handle& Self, FGameplayTag InName) -> bool
        { return UCk_Utils_Variables_UObject_UE::UtilsType::Has(Self, InName); });

    // ---- Material (TWeakObjectPtr<UMaterialInterface> storage, exposed as pointer) ----

    Handle.Method(
        "void StoreVar_Material(FGameplayTag InName, UMaterialInterface InValue)",
        [](FCk_Handle& Self, FGameplayTag InName, UMaterialInterface* InValue) -> void
        { UCk_Utils_Variables_Material_UE::Set(Self, InName, InValue); });

    Handle.Method(
        "TOptional<UMaterialInterface> GetVar_Material(FGameplayTag InName,"
        " ECk_Recursion InRecursion = ECk_Recursion::NotRecursive) const",
        [](const FCk_Handle& Self, FGameplayTag InName,
           ECk_Recursion InRecursion) -> TOptional<UMaterialInterface*>
        { return UCk_Utils_Variables_Material_UE::Get_Optional(Self, InName, InRecursion); });

    Handle.Method(
        "bool HasVar_Material(FGameplayTag InName) const",
        [](const FCk_Handle& Self, FGameplayTag InName) -> bool
        { return UCk_Utils_Variables_Material_UE::UtilsType::Has(Self, InName); });
});

// --------------------------------------------------------------------------------------------------------------------

#endif // WITH_ANGELSCRIPT_CK
