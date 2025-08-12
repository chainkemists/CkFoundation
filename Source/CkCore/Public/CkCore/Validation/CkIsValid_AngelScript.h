#pragma once

#include "CkCore/Validation/CkIsValid.h"

// --------------------------------------------------------------------------------------------------------------------

#if WITH_ANGELSCRIPT_CK

#include "AngelscriptBinds.h"
#include "AngelscriptManager.h"

#define CK_DEFINE_ANGELSCRIPT_IS_VALID(_type_, _type_no_ptr_)                             \
inline auto CK_UNIQUE_NAME(Register_IsValid_)() -> void                                    \
{                                                                                          \
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
    [] { CK_UNIQUE_NAME(Register_IsValid_)(); }                                            \
);

#endif // WITH_ANGELSCRIPT_CK

// --------------------------------------------------------------------------------------------------------------------
