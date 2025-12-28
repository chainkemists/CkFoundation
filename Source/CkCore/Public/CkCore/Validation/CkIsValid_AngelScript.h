#pragma once

#include "CkCore/Validation/CkIsValid.h"

// --------------------------------------------------------------------------------------------------------------------

#if WITH_ANGELSCRIPT_CK

#include "AngelscriptBinds.h"
#include "AngelscriptManager.h"

// Static tracking to prevent duplicate registration of global IsValid functions.
// HasFunction() doesn't work reliably because all Late-order FBind callbacks run in the same phase,
// so the function isn't registered to the engine yet when parallel callbacks check.
class CKCORE_API FCkAngelScriptIsValidRegistrationTracker
{
public:
    static auto
    TryRegister(
        const FString& InTypeName) -> bool
    {
        auto& RegisteredTypes = Get_RegisteredTypes();
        if (RegisteredTypes.Contains(InTypeName))
        {
            return false;
        }
        RegisteredTypes.Add(InTypeName);
        return true;
    }

private:
    static auto
    Get_RegisteredTypes() -> TSet<FString>&
    {
        static TSet<FString> RegisteredTypes;
        return RegisteredTypes;
    }
};

#define CK_DEFINE_ANGELSCRIPT_IS_VALID(_type_, _type_no_ptr_)                              \
inline auto CK_UNIQUE_NAME(Register_IsValid_##_type_no_ptr_)() -> void                     \
{                                                                                          \
    auto TypeBind = FAngelscriptBinds::ExistingClass(#_type_no_ptr_);                      \
    if (TypeBind.GetTypeInfo() == nullptr)                                                 \
    {                                                                                      \
        return;                                                                            \
    }                                                                                      \
                                                                                           \
    /* Use static tracking instead of HasFunction since timing is unreliable */            \
    if (NOT FCkAngelScriptIsValidRegistrationTracker::TryRegister(TEXT(#_type_no_ptr_)))   \
    {                                                                                      \
        return;                                                                            \
    }                                                                                      \
                                                                                           \
    FAngelscriptBinds::FNamespace Ns(FString(TEXT("ck")));                                 \
    FAngelscriptBinds::BindGlobalFunction("bool IsValid("#_type_no_ptr_" In)",             \
    [](ck::type_traits::Binding_Param_T<_type_> InObj) -> bool                             \
    {                                                                                      \
        return ck::IsValid(InObj);                                                         \
    });                                                                                    \
    FAngelscriptBinds::BindGlobalFunction("bool Is_NOT_Valid("#_type_no_ptr_" In)",        \
    [](ck::type_traits::Binding_Param_T<_type_> InObj) -> bool                             \
    {                                                                                      \
        return ck::Is_NOT_Valid(InObj);                                                    \
    });                                                                                    \
};                                                                                         \
AS_FORCE_LINK const FAngelscriptBinds::FBind CK_UNIQUE_NAME(Bind_IsValid_)(                \
    FAngelscriptBinds::EOrder::Late,                                                       \
    [] { CK_UNIQUE_NAME(Register_IsValid_##_type_no_ptr_)(); }                             \
);

#endif // WITH_ANGELSCRIPT_CK

// --------------------------------------------------------------------------------------------------------------------
