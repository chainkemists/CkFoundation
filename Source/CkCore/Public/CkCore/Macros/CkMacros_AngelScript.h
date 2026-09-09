#pragma once

#include <Containers/Map.h>
#include <Containers/Set.h>

#include <Delegates/IDelegateInstance.h>

#include <HAL/UnrealMemory.h>

#include <Templates/Function.h>

#include <CkCore/AngelScript/CkAngelScript_TypeValidation.h>

// Get_InternedBindName takes the std::string ck::Format_ANSI produces.
#include <string>

#if WITH_ANGELSCRIPT_CK

// --------------------------------------------------------------------------------------------------------------------

#define CK_ANGELSCRIPT_BIND_OPERATOR_EQUALS(_Type_)                                                                                        \
AS_FORCE_LINK const FAngelscriptBinds::FBind opEquals_##_Type_ (FAngelscriptBinds::EOrder::Early, []                                    \
{                                                                                                                                       \
    const FBindFlags Flags;                                                                                                             \
    auto Bind = FAngelscriptBinds::ValueClass<_Type_>(#_Type_, Flags);                                                                  \
    Bind.Method("bool opEquals(const "#_Type_"& Other) const",                                                                          \
        METHODPR_TRIVIAL(bool, _Type_, operator==, (const _Type_&) const));                                                             \
});

#define CK_ANGELSCRIPT_BIND_OPERATOR_COMPARISON(_Type_)                                                                                    \
AS_FORCE_LINK const FAngelscriptBinds::FBind opCmp_##_Type_ (FAngelscriptBinds::EOrder::Early, []                                       \
{                                                                                                                                       \
    const FBindFlags Flags;                                                                                                             \
    auto Bind = FAngelscriptBinds::ValueClass<_Type_>(#_Type_, Flags);                                                                  \
    Bind.Method("int opCmp(const "#_Type_"& Other) const",                                                                              \
        METHODPR_TRIVIAL(int, _Type_, OpCmp, (const _Type_&) const));                                                                   \
});

#define CK_ANGELSCRIPT_BIND_OPERATOR_ASSIGNMENT(_Type_, _Operator_, _AS_Op_Name_)                                                          \
AS_FORCE_LINK const FAngelscriptBinds::FBind _AS_Op_Name_##_##_Type_ (FAngelscriptBinds::EOrder::Early, []                              \
{                                                                                                                                       \
    const FBindFlags Flags;                                                                                                             \
    auto Bind = FAngelscriptBinds::ValueClass<_Type_>(#_Type_, Flags);                                                                  \
    Bind.Method(#_Type_" "#_AS_Op_Name_"(const "#_Type_"& Other) const",                                                                \
        METHODPR_TRIVIAL(_Type_, _Type_, operator _Operator_, (const _Type_&) const));                                                  \
});

#define CK_ANGELSCRIPT_BIND_OPERATOR_UNARY(_Type_, _Ret_Type_, _Ret_Type__AsParam_, _Operator_, _AS_Op_Name_)                       \
AS_FORCE_LINK const FAngelscriptBinds::FBind _AS_Op_Name_##_##_Type_##_Ret_Type_ (FAngelscriptBinds::EOrder::Early, []           \
{                                                                                                                                \
    const FBindFlags Flags;                                                                                                      \
    auto Bind = FAngelscriptBinds::ValueClass<_Type_>(#_Type_, Flags);                                                           \
    Bind.Method(#_Ret_Type_" "#_AS_Op_Name_"() const",                                                                           \
        METHODPR_TRIVIAL(_Ret_Type__AsParam_, _Type_, operator _Operator_, () const));                                           \
});

// The "_Ret_Type__AsParam_" is needed because float in the bind needs to be "float32" for some weird reason, but it returns a float in c++
#define CK_ANGELSCRIPT_BIND_OPERATOR_BINARY(_Type_, _Other_Type_, _Other_Type_AsParam_, _Ret_Type_, _Ret_Type__AsParam_, _Operator_, _AS_Op_Name_)      \
AS_FORCE_LINK const FAngelscriptBinds::FBind _AS_Op_Name_##_##_Type_##_Other_Type_##_Ret_Type_ (FAngelscriptBinds::EOrder::Early, []                 \
{                                                                                                                                                    \
    const FBindFlags Flags;                                                                                                                          \
    auto Bind = FAngelscriptBinds::ValueClass<_Type_>(#_Type_, Flags);                                                                               \
    Bind.Method(#_Ret_Type_" "#_AS_Op_Name_"("#_Other_Type_AsParam_" Other) const",                                                                  \
        METHODPR_TRIVIAL(_Ret_Type__AsParam_, _Type_, operator _Operator_, (_Other_Type_AsParam_) const));                                           \
});

// --------------------------------------------------------------------------------------------------------------------

#define CK_ANGELSCRIPT_LAMBDA_PARAM_1(_ClassType_, _1) _ClassType_* Address, decltype(_1) _1
#define CK_ANGELSCRIPT_LAMBDA_PARAM_2(_ClassType_, _1, _2) CK_ANGELSCRIPT_LAMBDA_PARAM_1(_ClassType_, _1), decltype(_2) _2
#define CK_ANGELSCRIPT_LAMBDA_PARAM_3(_ClassType_, _1, _2, _3) CK_ANGELSCRIPT_LAMBDA_PARAM_2(_ClassType_, _1, _2), decltype(_3) _3
#define CK_ANGELSCRIPT_LAMBDA_PARAM_4(_ClassType_, _1, _2, _3, _4) CK_ANGELSCRIPT_LAMBDA_PARAM_3(_ClassType_, _1, _2, _3), decltype(_4) _4
#define CK_ANGELSCRIPT_LAMBDA_PARAM_5(_ClassType_, _1, _2, _3, _4, _5) CK_ANGELSCRIPT_LAMBDA_PARAM_4(_ClassType_, _1, _2, _3, _4), decltype(_5) _5
#define CK_ANGELSCRIPT_LAMBDA_PARAM_6(_ClassType_, _1, _2, _3, _4, _5, _6) CK_ANGELSCRIPT_LAMBDA_PARAM_5(_ClassType_, _1, _2, _3, _4, _5), decltype(_6) _6
#define CK_ANGELSCRIPT_LAMBDA_PARAM_7(_ClassType_, _1, _2, _3, _4, _5, _6, _7) CK_ANGELSCRIPT_LAMBDA_PARAM_6(_ClassType_, _1, _2, _3, _4, _5, _6), decltype(_7) _7
#define CK_ANGELSCRIPT_LAMBDA_PARAM_8(_ClassType_, _1, _2, _3, _4, _5, _6, _7, _8) CK_ANGELSCRIPT_LAMBDA_PARAM_7(_ClassType_, _1, _2, _3, _4, _5, _6, _7), decltype(_8) _8
#define CK_ANGELSCRIPT_LAMBDA_PARAM_9(_ClassType_, _1, _2, _3, _4, _5, _6, _7, _8, _9) CK_ANGELSCRIPT_LAMBDA_PARAM_8(_ClassType_, _1, _2, _3, _4, _5, _6, _7, _8), decltype(_9) _9

#define CK_ANGELSCRIPT_LAMBDA_PARAM_VARIADIC(_ClassType_, M, ...) EXPAND(M(_ClassType_, __VA_ARGS__))
#define CK_ANGELSCRIPT_LAMBDA_PARAMS(_ClassType_, ...) CK_ANGELSCRIPT_LAMBDA_PARAM_VARIADIC(_ClassType_, CK_CONCAT(CK_ANGELSCRIPT_LAMBDA_PARAM_, EXPAND(NARG_(__VA_ARGS__))), __VA_ARGS__)

#define CK_ANGELSCRIPT_CTOR_ARG_1(_1) std::move(_1)
#define CK_ANGELSCRIPT_CTOR_ARG_2(_1, _2) CK_ANGELSCRIPT_CTOR_ARG_1(_1), std::move(_2)
#define CK_ANGELSCRIPT_CTOR_ARG_3(_1, _2, _3) CK_ANGELSCRIPT_CTOR_ARG_2(_1, _2), std::move(_3)
#define CK_ANGELSCRIPT_CTOR_ARG_4(_1, _2, _3, _4) CK_ANGELSCRIPT_CTOR_ARG_3(_1, _2, _3), std::move(_4)
#define CK_ANGELSCRIPT_CTOR_ARG_5(_1, _2, _3, _4, _5) CK_ANGELSCRIPT_CTOR_ARG_4(_1, _2, _3, _4), std::move(_5)
#define CK_ANGELSCRIPT_CTOR_ARG_6(_1, _2, _3, _4, _5, _6) CK_ANGELSCRIPT_CTOR_ARG_5(_1, _2, _3, _4, _5), std::move(_6)
#define CK_ANGELSCRIPT_CTOR_ARG_7(_1, _2, _3, _4, _5, _6, _7) CK_ANGELSCRIPT_CTOR_ARG_6(_1, _2, _3, _4, _5, _6), std::move(_7)
#define CK_ANGELSCRIPT_CTOR_ARG_8(_1, _2, _3, _4, _5, _6, _7, _8) CK_ANGELSCRIPT_CTOR_ARG_7(_1, _2, _3, _4, _5, _6, _7), std::move(_8)
#define CK_ANGELSCRIPT_CTOR_ARG_9(_1, _2, _3, _4, _5, _6, _7, _8, _9) CK_ANGELSCRIPT_CTOR_ARG_8(_1, _2, _3, _4, _5, _6, _7, _8), std::move(_9)

#define CK_ANGELSCRIPT_CTOR_ARG_VARIADIC(M, ...) EXPAND(M(__VA_ARGS__))
#define CK_ANGELSCRIPT_CTOR_ARGS(...) CK_ANGELSCRIPT_CTOR_ARG_VARIADIC(CK_CONCAT(CK_ANGELSCRIPT_CTOR_ARG_, EXPAND(NARG_(__VA_ARGS__))), __VA_ARGS__)

// Helper macros to create unique names based on parameter names (which should reflect their types)
#define CK_ANGELSCRIPT_UNIQUE_SUFFIX_1(_1) CK_CONCAT(_Params_, _1)
#define CK_ANGELSCRIPT_UNIQUE_SUFFIX_2(_1, _2) CK_CONCAT(CK_CONCAT(_Params_, _1), CK_CONCAT(_, _2))
#define CK_ANGELSCRIPT_UNIQUE_SUFFIX_3(_1, _2, _3) CK_CONCAT(CK_ANGELSCRIPT_UNIQUE_SUFFIX_2(_1, _2), CK_CONCAT(_, _3))
#define CK_ANGELSCRIPT_UNIQUE_SUFFIX_4(_1, _2, _3, _4) CK_CONCAT(CK_ANGELSCRIPT_UNIQUE_SUFFIX_3(_1, _2, _3), CK_CONCAT(_, _4))
#define CK_ANGELSCRIPT_UNIQUE_SUFFIX_5(_1, _2, _3, _4, _5) CK_CONCAT(CK_ANGELSCRIPT_UNIQUE_SUFFIX_4(_1, _2, _3, _4), CK_CONCAT(_, _5))
#define CK_ANGELSCRIPT_UNIQUE_SUFFIX_6(_1, _2, _3, _4, _5, _6) CK_CONCAT(CK_ANGELSCRIPT_UNIQUE_SUFFIX_5(_1, _2, _3, _4, _5), CK_CONCAT(_, _6))
#define CK_ANGELSCRIPT_UNIQUE_SUFFIX_7(_1, _2, _3, _4, _5, _6, _7) CK_CONCAT(CK_ANGELSCRIPT_UNIQUE_SUFFIX_6(_1, _2, _3, _4, _5, _6), CK_CONCAT(_, _7))
#define CK_ANGELSCRIPT_UNIQUE_SUFFIX_8(_1, _2, _3, _4, _5, _6, _7, _8) CK_CONCAT(CK_ANGELSCRIPT_UNIQUE_SUFFIX_7(_1, _2, _3, _4, _5, _6, _7), CK_CONCAT(_, _8))
#define CK_ANGELSCRIPT_UNIQUE_SUFFIX_9(_1, _2, _3, _4, _5, _6, _7, _8, _9) CK_CONCAT(CK_ANGELSCRIPT_UNIQUE_SUFFIX_8(_1, _2, _3, _4, _5, _6, _7, _8), CK_CONCAT(_, _9))

#define CK_ANGELSCRIPT_UNIQUE_SUFFIX_VARIADIC(M, ...) EXPAND(M(__VA_ARGS__))
#define CK_ANGELSCRIPT_UNIQUE_SUFFIX(...) CK_ANGELSCRIPT_UNIQUE_SUFFIX_VARIADIC(CK_CONCAT(CK_ANGELSCRIPT_UNIQUE_SUFFIX_, EXPAND(NARG_(__VA_ARGS__))), __VA_ARGS__)

#define CK_ANGELSCRIPT_GET_TYPE_1(_1) auto Param1 = ck::Get_RuntimeTypeToString_AngelScript<decltype(_1)>();
#define CK_ANGELSCRIPT_GET_TYPE_2(_1, _2) CK_ANGELSCRIPT_GET_TYPE_1(_1) auto Param2 = ck::Get_RuntimeTypeToString_AngelScript<decltype(_2)>();
#define CK_ANGELSCRIPT_GET_TYPE_3(_1, _2, _3) CK_ANGELSCRIPT_GET_TYPE_2(_1, _2) auto Param3 = ck::Get_RuntimeTypeToString_AngelScript<decltype(_3)>();
#define CK_ANGELSCRIPT_GET_TYPE_4(_1, _2, _3, _4) CK_ANGELSCRIPT_GET_TYPE_3(_1, _2, _3) auto Param4 = ck::Get_RuntimeTypeToString_AngelScript<decltype(_4)>();
#define CK_ANGELSCRIPT_GET_TYPE_5(_1, _2, _3, _4, _5) CK_ANGELSCRIPT_GET_TYPE_4(_1, _2, _3, _4) auto Param5 = ck::Get_RuntimeTypeToString_AngelScript<decltype(_5)>();
#define CK_ANGELSCRIPT_GET_TYPE_6(_1, _2, _3, _4, _5, _6) CK_ANGELSCRIPT_GET_TYPE_5(_1, _2, _3, _4, _5) auto Param6 = ck::Get_RuntimeTypeToString_AngelScript<decltype(_6)>();
#define CK_ANGELSCRIPT_GET_TYPE_7(_1, _2, _3, _4, _5, _6, _7) CK_ANGELSCRIPT_GET_TYPE_6(_1, _2, _3, _4, _5, _6) auto Param7 = ck::Get_RuntimeTypeToString_AngelScript<decltype(_7)>();
#define CK_ANGELSCRIPT_GET_TYPE_8(_1, _2, _3, _4, _5, _6, _7, _8) CK_ANGELSCRIPT_GET_TYPE_7(_1, _2, _3, _4, _5, _6, _7) auto Param8 = ck::Get_RuntimeTypeToString_AngelScript<decltype(_8)>();
#define CK_ANGELSCRIPT_GET_TYPE_9(_1, _2, _3, _4, _5, _6, _7, _8, _9) CK_ANGELSCRIPT_GET_TYPE_8(_1, _2, _3, _4, _5, _6, _7, _8) auto Param9 = ck::Get_RuntimeTypeToString_AngelScript<decltype(_9)>();

#define CK_ANGELSCRIPT_GET_TYPE_VARIADIC(M, ...) EXPAND(M(__VA_ARGS__))
#define CK_ANGELSCRIPT_GET_TYPES(...) CK_ANGELSCRIPT_GET_TYPE_VARIADIC(CK_CONCAT(CK_ANGELSCRIPT_GET_TYPE_, EXPAND(NARG_(__VA_ARGS__))), __VA_ARGS__)

#define CK_ANGELSCRIPT_FORMAT_SIG_WITH_NAMES_1(_1) TEXT("void f({} " #_1 ")")
#define CK_ANGELSCRIPT_FORMAT_SIG_WITH_NAMES_2(_1, _2) TEXT("void f({} " #_1 ", {} " #_2 ")")
#define CK_ANGELSCRIPT_FORMAT_SIG_WITH_NAMES_3(_1, _2, _3) TEXT("void f({} " #_1 ", {} " #_2 ", {} " #_3 ")")
#define CK_ANGELSCRIPT_FORMAT_SIG_WITH_NAMES_4(_1, _2, _3, _4) TEXT("void f({} " #_1 ", {} " #_2 ", {} " #_3 ", {} " #_4 ")")
#define CK_ANGELSCRIPT_FORMAT_SIG_WITH_NAMES_5(_1, _2, _3, _4, _5) TEXT("void f({} " #_1 ", {} " #_2 ", {} " #_3 ", {} " #_4 ", {} " #_5 ")")
#define CK_ANGELSCRIPT_FORMAT_SIG_WITH_NAMES_6(_1, _2, _3, _4, _5, _6) TEXT("void f({} " #_1 ", {} " #_2 ", {} " #_3 ", {} " #_4 ", {} " #_5 ", {} " #_6 ")")
#define CK_ANGELSCRIPT_FORMAT_SIG_WITH_NAMES_7(_1, _2, _3, _4, _5, _6, _7) TEXT("void f({} " #_1 ", {} " #_2 ", {} " #_3 ", {} " #_4 ", {} " #_5 ", {} " #_6 ", {} " #_7 ")")
#define CK_ANGELSCRIPT_FORMAT_SIG_WITH_NAMES_8(_1, _2, _3, _4, _5, _6, _7, _8) TEXT("void f({} " #_1 ", {} " #_2 ", {} " #_3 ", {} " #_4 ", {} " #_5 ", {} " #_6 ", {} " #_7 ", {} " #_8 ")")
#define CK_ANGELSCRIPT_FORMAT_SIG_WITH_NAMES_9(_1, _2, _3, _4, _5, _6, _7, _8, _9) TEXT("void f({} " #_1 ", {} " #_2 ", {} " #_3 ", {} " #_4 ", {} " #_5 ", {} " #_6 ", {} " #_7 ", {} " #_8 ", {} " #_9 ")")

#define CK_ANGELSCRIPT_FORMAT_SIG_WITH_NAMES_VARIADIC(M, ...) EXPAND(M(__VA_ARGS__))
#define CK_ANGELSCRIPT_FORMAT_SIG_WITH_NAMES(...) CK_ANGELSCRIPT_FORMAT_SIG_WITH_NAMES_VARIADIC(CK_CONCAT(CK_ANGELSCRIPT_FORMAT_SIG_WITH_NAMES_, EXPAND(NARG_(__VA_ARGS__))), __VA_ARGS__)

#define CK_ANGELSCRIPT_FORMAT_ARGS_1() Param1
#define CK_ANGELSCRIPT_FORMAT_ARGS_2() Param1, Param2
#define CK_ANGELSCRIPT_FORMAT_ARGS_3() Param1, Param2, Param3
#define CK_ANGELSCRIPT_FORMAT_ARGS_4() Param1, Param2, Param3, Param4
#define CK_ANGELSCRIPT_FORMAT_ARGS_5() Param1, Param2, Param3, Param4, Param5
#define CK_ANGELSCRIPT_FORMAT_ARGS_6() Param1, Param2, Param3, Param4, Param5, Param6
#define CK_ANGELSCRIPT_FORMAT_ARGS_7() Param1, Param2, Param3, Param4, Param5, Param6, Param7
#define CK_ANGELSCRIPT_FORMAT_ARGS_8() Param1, Param2, Param3, Param4, Param5, Param6, Param7, Param8
#define CK_ANGELSCRIPT_FORMAT_ARGS_9() Param1, Param2, Param3, Param4, Param5, Param6, Param7, Param8, Param9

#define CK_ANGELSCRIPT_FORMAT_ARGS(...) CK_CONCAT(CK_ANGELSCRIPT_FORMAT_ARGS_, EXPAND(NARG_(__VA_ARGS__)))()

#define CK_ANGELSCRIPT_VALIDATE_TYPE_1() \
    if (NOT ck::angelscript::IsTypeValidForAngelScript(Param1)) { return; }
#define CK_ANGELSCRIPT_VALIDATE_TYPE_2() \
    if (NOT ck::angelscript::IsTypeValidForAngelScript(Param1)) { return; } \
    if (NOT ck::angelscript::IsTypeValidForAngelScript(Param2)) { return; }
#define CK_ANGELSCRIPT_VALIDATE_TYPE_3() \
    if (NOT ck::angelscript::IsTypeValidForAngelScript(Param1)) { return; } \
    if (NOT ck::angelscript::IsTypeValidForAngelScript(Param2)) { return; } \
    if (NOT ck::angelscript::IsTypeValidForAngelScript(Param3)) { return; }
#define CK_ANGELSCRIPT_VALIDATE_TYPE_4() \
    if (NOT ck::angelscript::IsTypeValidForAngelScript(Param1)) { return; } \
    if (NOT ck::angelscript::IsTypeValidForAngelScript(Param2)) { return; } \
    if (NOT ck::angelscript::IsTypeValidForAngelScript(Param3)) { return; } \
    if (NOT ck::angelscript::IsTypeValidForAngelScript(Param4)) { return; }
#define CK_ANGELSCRIPT_VALIDATE_TYPE_5() \
    if (NOT ck::angelscript::IsTypeValidForAngelScript(Param1)) { return; } \
    if (NOT ck::angelscript::IsTypeValidForAngelScript(Param2)) { return; } \
    if (NOT ck::angelscript::IsTypeValidForAngelScript(Param3)) { return; } \
    if (NOT ck::angelscript::IsTypeValidForAngelScript(Param4)) { return; } \
    if (NOT ck::angelscript::IsTypeValidForAngelScript(Param5)) { return; }
#define CK_ANGELSCRIPT_VALIDATE_TYPE_6() \
    if (NOT ck::angelscript::IsTypeValidForAngelScript(Param1)) { return; } \
    if (NOT ck::angelscript::IsTypeValidForAngelScript(Param2)) { return; } \
    if (NOT ck::angelscript::IsTypeValidForAngelScript(Param3)) { return; } \
    if (NOT ck::angelscript::IsTypeValidForAngelScript(Param4)) { return; } \
    if (NOT ck::angelscript::IsTypeValidForAngelScript(Param5)) { return; } \
    if (NOT ck::angelscript::IsTypeValidForAngelScript(Param6)) { return; }
#define CK_ANGELSCRIPT_VALIDATE_TYPE_7() \
    if (NOT ck::angelscript::IsTypeValidForAngelScript(Param1)) { return; } \
    if (NOT ck::angelscript::IsTypeValidForAngelScript(Param2)) { return; } \
    if (NOT ck::angelscript::IsTypeValidForAngelScript(Param3)) { return; } \
    if (NOT ck::angelscript::IsTypeValidForAngelScript(Param4)) { return; } \
    if (NOT ck::angelscript::IsTypeValidForAngelScript(Param5)) { return; } \
    if (NOT ck::angelscript::IsTypeValidForAngelScript(Param6)) { return; } \
    if (NOT ck::angelscript::IsTypeValidForAngelScript(Param7)) { return; }
#define CK_ANGELSCRIPT_VALIDATE_TYPE_8() \
    if (NOT ck::angelscript::IsTypeValidForAngelScript(Param1)) { return; } \
    if (NOT ck::angelscript::IsTypeValidForAngelScript(Param2)) { return; } \
    if (NOT ck::angelscript::IsTypeValidForAngelScript(Param3)) { return; } \
    if (NOT ck::angelscript::IsTypeValidForAngelScript(Param4)) { return; } \
    if (NOT ck::angelscript::IsTypeValidForAngelScript(Param5)) { return; } \
    if (NOT ck::angelscript::IsTypeValidForAngelScript(Param6)) { return; } \
    if (NOT ck::angelscript::IsTypeValidForAngelScript(Param7)) { return; } \
    if (NOT ck::angelscript::IsTypeValidForAngelScript(Param8)) { return; }
#define CK_ANGELSCRIPT_VALIDATE_TYPE_9() \
    if (NOT ck::angelscript::IsTypeValidForAngelScript(Param1)) { return; } \
    if (NOT ck::angelscript::IsTypeValidForAngelScript(Param2)) { return; } \
    if (NOT ck::angelscript::IsTypeValidForAngelScript(Param3)) { return; } \
    if (NOT ck::angelscript::IsTypeValidForAngelScript(Param4)) { return; } \
    if (NOT ck::angelscript::IsTypeValidForAngelScript(Param5)) { return; } \
    if (NOT ck::angelscript::IsTypeValidForAngelScript(Param6)) { return; } \
    if (NOT ck::angelscript::IsTypeValidForAngelScript(Param7)) { return; } \
    if (NOT ck::angelscript::IsTypeValidForAngelScript(Param8)) { return; } \
    if (NOT ck::angelscript::IsTypeValidForAngelScript(Param9)) { return; }

#define CK_ANGELSCRIPT_VALIDATE_TYPES(...) CK_CONCAT(CK_ANGELSCRIPT_VALIDATE_TYPE_, EXPAND(NARG_(__VA_ARGS__)))()

#define CK_ANGELSCRIPT_CTOR_REGISTRATION(_ClassType_, ...)\
    static void CK_CONCAT(RegisterAngelScriptCtor, CK_ANGELSCRIPT_UNIQUE_SUFFIX(__VA_ARGS__))()\
    {\
        auto ExistingClass = FAngelscriptBinds::ExistingClass(#_ClassType_);\
        /* Skip if type not registered in AngelScript */\
        if (ExistingClass.GetTypeInfo() == nullptr)\
        { return; }\
        \
        /* Get runtime type strings for each parameter */\
        CK_ANGELSCRIPT_GET_TYPES(__VA_ARGS__)\
        \
        /* Skip if any parameter type contains unsupported patterns */\
        CK_ANGELSCRIPT_VALIDATE_TYPES(__VA_ARGS__)\
        \
        /* Format the constructor signature with actual parameter names */\
        auto CtorSignature = ck::Format_ANSI(CK_ANGELSCRIPT_FORMAT_SIG_WITH_NAMES(__VA_ARGS__), CK_ANGELSCRIPT_FORMAT_ARGS(__VA_ARGS__));\
        \
        /* Track to prevent duplicate registration */\
        auto CtorKey = ck::Format_UE(TEXT("{}::{}"), TEXT(#_ClassType_), FString{CtorSignature.c_str()});\
        if (NOT FCkAngelScriptCtorFunctionRegistration::TryRegisterCtor(CtorKey))\
        { return; }\
        \
        ExistingClass.Constructor(CtorSignature.c_str(), []( CK_ANGELSCRIPT_LAMBDA_PARAMS(_ClassType_, __VA_ARGS__) )\
        {\
            new(Address) _ClassType_( CK_ANGELSCRIPT_CTOR_ARGS(__VA_ARGS__) );\
        });\
        FAngelscriptBinds::SetPreviousBindNoDiscard(true);\
        SCRIPT_NATIVE_CONSTRUCTOR(ExistingClass, #_ClassType_);\
    }\
    \
    static inline bool CK_CONCAT(AngelScriptRegistered, CK_ANGELSCRIPT_UNIQUE_SUFFIX(__VA_ARGS__)) = []() -> bool\
    {\
        FCkAngelScriptCtorFunctionRegistration::RegisterCtorFunction(\
            &CK_CONCAT(RegisterAngelScriptCtor, CK_ANGELSCRIPT_UNIQUE_SUFFIX(__VA_ARGS__)));\
        return true;\
    }();

// --------------------------------------------------------------------------------------------------------------------

class CKCORE_API FCkAngelScriptPropertyFunctionRegistration
{
public:
    using FPropertyFunction = TFunction<void()>;

    static auto
    RegisterPropertyFunction(
        const FPropertyFunction& InPropertyFunc) -> void;

    // BindNativeMethod stores this pointer RAW (FScriptNativeMethod::Name) and only dereferences it
    // much later - when the StaticJIT transpiler emits C++, in a separate pass. A local std::string
    // from ck::Format_ANSI is long dead by then, so the transpiler reads freed memory and emits
    // either an invalid identifier or, where the allocation has been reused by the NEXT property's
    // name, a real-but-wrong method that can compile and call the wrong thing.
    //
    // The engine guards the FBindString path with ToCString_EnsureConstant(); the raw
    // BindNativeMethod overload has no such guard, so the lifetime is the caller's to get right.
    // Interning is the fix: the pointer must outlive the process, because generation happens at an
    // arbitrary later point. Bind registration is game-thread-only at startup, so no
    // synchronisation is needed, and the pool is bounded by the number of DISTINCT property names.
    static auto
    Get_InternedBindName(
        const std::string& InName) -> const ANSICHAR*
    {
        static TMap<FString, const ANSICHAR*> Pool;

        const auto Key = FString{InName.c_str()};

        if (const auto* const Found = Pool.Find(Key))
        { return *Found; }

        const auto LengthWithNull = InName.size() + 1;
        auto* const Copy = new ANSICHAR[LengthWithNull];
        FMemory::Memcpy(Copy, InName.c_str(), LengthWithNull);

        Pool.Add(Key, Copy);
        return Copy;
    }

    static auto
    TryRegisterProperty(
        const FString& InPropertyKey) -> bool
    {
        auto& RegisteredProperties = Get_RegisteredProperties();
        if (RegisteredProperties.Contains(InPropertyKey))
        {
            return false;
        }
        RegisteredProperties.Add(InPropertyKey);
        return true;
    }

private:
    static auto
    EnsureCallbackRegistered() -> void;

    static auto
    RegisterAllPropertyFunctions() -> void;

    static auto
    Get_AllPropertyFunctions() -> TArray<FPropertyFunction>&;

    static auto
    Get_RegisteredProperties() -> TSet<FString>&
    {
        static TSet<FString> RegisteredProperties;
        return RegisteredProperties;
    }

private:
    static inline FDelegateHandle _PreCompileDelegateHandle;
};

// --------------------------------------------------------------------------------------------------------------------

// Const getter + setter only — registering the non-const getter too collides on overload resolution.
#define CK_ANGELSCRIPT_PROPERTY_REGISTRATION_GETTER_SETTER(_InVar_)\
    static void CK_CONCAT(RegisterAngelScriptProperty_, CK_CONCAT(__LINE__, CK_CONCAT(_, _InVar_)))()\
    {\
        /* Get the class type from ThisType (defined by CK_GENERATED_BODY) */\
        using ClassType = ThisType;\
        auto ClassTypeStr = ck::Get_RuntimeTypeToString_AngelScript<ClassType>();\
        auto ExistingClass = FAngelscriptBinds::ExistingClass(FBindString(ClassTypeStr));\
        /* Skip if type not registered in AngelScript */\
        if (ExistingClass.GetTypeInfo() == nullptr)\
        { return; }\
        \
        /* Get runtime type string for the property */\
        auto PropertyTypeStr = ck::Get_RuntimeTypeToString_AngelScript<decltype(_InVar_)>();\
        \
        /* Skip if property type contains unsupported patterns */\
        if (NOT ck::angelscript::IsTypeValidForAngelScript(PropertyTypeStr))\
        { return; }\
        \
        /* Strip leading "const " — signature templates below pre-pend "const" for the ref form. */\
        /* Property fields of type `const T*` come back here as `"const T"`; without the strip, the */\
        /* template `"const {}& Get_X() const"` would emit invalid `"const const T&"` (asINVALID_DECLARATION). */\
        if (PropertyTypeStr.StartsWith(TEXT("const ")))\
        { PropertyTypeStr = PropertyTypeStr.RightChop(6); }\
        \
        /* Remove leading underscore from variable name for method signatures */\
        auto FullVarName = FString(TEXT(#_InVar_));\
        auto CleanPropertyName = FullVarName.StartsWith(TEXT("_")) ? FullVarName.RightChop(1) : FullVarName;\
        \
        /* Track to prevent duplicate registration */\
        auto PropertyKey = ck::Format_UE(TEXT("{}::Get_{}"), ClassTypeStr, CleanPropertyName);\
        if (NOT FCkAngelScriptPropertyFunctionRegistration::TryRegisterProperty(PropertyKey))\
        { return; }\
        \
        /* Check if methods already exist (UHT may have registered them) */\
        auto TypeInfo = ExistingClass.GetTypeInfo();\
        auto GetterMethodName = ck::Format_ANSI(TEXT("Get_{}"), CleanPropertyName);\
        auto SetterMethodName = ck::Format_ANSI(TEXT("Set_{}"), CleanPropertyName);\
        auto GetterExists = TypeInfo->GetMethodByName(GetterMethodName.c_str()) != nullptr;\
        auto SetterExists = TypeInfo->GetMethodByName(SetterMethodName.c_str()) != nullptr;\
        \
        /* Register getter only if it doesn't exist */\
        if (NOT GetterExists)\
        {\
            auto GetterSignature = ck::Format_ANSI(TEXT("const {}& Get_{}() const"), PropertyTypeStr, CleanPropertyName);\
            ExistingClass.Method(GetterSignature.c_str(), [](const ClassType* Self) -> const decltype(_InVar_)&\
            {\
                return Self->CK_CONCAT(Get, _InVar_)();\
            });\
            FScriptFunctionNativeForm::BindNativeMethod(ExistingClass, FCkAngelScriptPropertyFunctionRegistration::Get_InternedBindName(GetterMethodName), true);\
        }\
        \
        /* Register setter only if it doesn't exist */\
        if (NOT SetterExists)\
        {\
            auto SetterSignature = ck::Format_ANSI(TEXT("{}& Set_{}(const {}& InValue)"), ClassTypeStr, CleanPropertyName, PropertyTypeStr);\
            ExistingClass.Method(SetterSignature.c_str(),\
                METHODPR_TRIVIAL(ClassType&, ClassType, CK_CONCAT(Set, _InVar_), (const decltype(_InVar_)&)));\
            FScriptFunctionNativeForm::BindNativeMethod(ExistingClass, FCkAngelScriptPropertyFunctionRegistration::Get_InternedBindName(SetterMethodName), true);\
        }\
        \
        FAngelscriptBinds::SetPreviousBindNoDiscard(false);\
    }\
    \
    static inline bool CK_CONCAT(AngelScriptPropertyRegistered_, CK_CONCAT(__LINE__, CK_CONCAT(_, _InVar_))) = []() -> bool\
    {\
        FCkAngelScriptPropertyFunctionRegistration::RegisterPropertyFunction(\
            &CK_CONCAT(RegisterAngelScriptProperty_, CK_CONCAT(__LINE__, CK_CONCAT(_, _InVar_))));\
        return true;\
    }();

#define CK_ANGELSCRIPT_PROPERTY_REGISTRATION_GETTER_CONSTREF(_InVar_)\
    static void CK_CONCAT(RegisterAngelScriptPropertyGetterConstRef_, CK_CONCAT(__LINE__, CK_CONCAT(_, _InVar_)))()\
    {\
        /* Get the class type from ThisType (defined by CK_GENERATED_BODY) */\
        using ClassType = ThisType;\
        auto ClassTypeStr = ck::Get_RuntimeTypeToString_AngelScript<ClassType>();\
        auto ExistingClass = FAngelscriptBinds::ExistingClass(FBindString(ClassTypeStr));\
        /* Skip if type not registered in AngelScript */\
        if (ExistingClass.GetTypeInfo() == nullptr)\
        { return; }\
        \
        /* Get runtime type string for the property */\
        auto PropertyTypeStr = ck::Get_RuntimeTypeToString_AngelScript<decltype(_InVar_)>();\
        \
        /* Skip if property type contains unsupported patterns */\
        if (NOT ck::angelscript::IsTypeValidForAngelScript(PropertyTypeStr))\
        { return; }\
        \
        /* Strip leading "const " — see GETTER_SETTER variant above for rationale. */\
        if (PropertyTypeStr.StartsWith(TEXT("const ")))\
        { PropertyTypeStr = PropertyTypeStr.RightChop(6); }\
        \
        /* Remove leading underscore from variable name for method signatures */\
        auto FullVarName = FString(TEXT(#_InVar_));\
        auto CleanPropertyName = FullVarName.StartsWith(TEXT("_")) ? FullVarName.RightChop(1) : FullVarName;\
        \
        /* Track to prevent duplicate registration */\
        auto PropertyKey = ck::Format_UE(TEXT("{}::Get_{}"), ClassTypeStr, CleanPropertyName);\
        if (NOT FCkAngelScriptPropertyFunctionRegistration::TryRegisterProperty(PropertyKey))\
        { return; }\
        \
        /* Check if method already exists (UHT may have registered it) */\
        auto TypeInfo = ExistingClass.GetTypeInfo();\
        auto GetterMethodName = ck::Format_ANSI(TEXT("Get_{}"), CleanPropertyName);\
        if (TypeInfo->GetMethodByName(GetterMethodName.c_str()) != nullptr)\
        { return; }\
        \
        /* Format the getter signature - use lambda to avoid static_cast issues with auto return types */\
        auto GetterSignature = ck::Format_ANSI(TEXT("const {}& Get_{}() const"), PropertyTypeStr, CleanPropertyName);\
        ExistingClass.Method(GetterSignature.c_str(), [](const ClassType* Self) -> const decltype(_InVar_)&\
        {\
            return Self->CK_CONCAT(Get, _InVar_)();\
        });\
        \
        FAngelscriptBinds::SetPreviousBindNoDiscard(true);\
        FScriptFunctionNativeForm::BindNativeMethod(ExistingClass, FCkAngelScriptPropertyFunctionRegistration::Get_InternedBindName(GetterMethodName), true);\
    }\
    \
    static inline bool CK_CONCAT(AngelScriptPropertyGetterConstRefRegistered_, CK_CONCAT(__LINE__, CK_CONCAT(_, _InVar_))) = []() -> bool\
    {\
        FCkAngelScriptPropertyFunctionRegistration::RegisterPropertyFunction(\
            &CK_CONCAT(RegisterAngelScriptPropertyGetterConstRef_, CK_CONCAT(__LINE__, CK_CONCAT(_, _InVar_))));\
        return true;\
    }();

#define CK_ANGELSCRIPT_PROPERTY_REGISTRATION_SETTER_ONLY(_InVar_)\
    static void CK_CONCAT(RegisterAngelScriptPropertySetterOnly_, CK_CONCAT(__LINE__, CK_CONCAT(_, _InVar_)))()\
    {\
        /* Get the class type from ThisType (defined by CK_GENERATED_BODY) */\
        using ClassType = ThisType;\
        auto ClassTypeStr = ck::Get_RuntimeTypeToString_AngelScript<ClassType>();\
        auto ExistingClass = FAngelscriptBinds::ExistingClass(FBindString(ClassTypeStr));\
        /* Skip if type not registered in AngelScript */\
        if (ExistingClass.GetTypeInfo() == nullptr)\
        { return; }\
        \
        /* Get runtime type string for the property */\
        auto PropertyTypeStr = ck::Get_RuntimeTypeToString_AngelScript<decltype(_InVar_)>();\
        \
        /* Skip if property type contains unsupported patterns */\
        if (NOT ck::angelscript::IsTypeValidForAngelScript(PropertyTypeStr))\
        { return; }\
        \
        /* Strip leading "const " — see GETTER_SETTER variant above for rationale. */\
        if (PropertyTypeStr.StartsWith(TEXT("const ")))\
        { PropertyTypeStr = PropertyTypeStr.RightChop(6); }\
        \
        /* Remove leading underscore from variable name for method signatures */\
        auto FullVarName = FString(TEXT(#_InVar_));\
        auto CleanPropertyName = FullVarName.StartsWith(TEXT("_")) ? FullVarName.RightChop(1) : FullVarName;\
        \
        /* Track to prevent duplicate registration */\
        auto PropertyKey = ck::Format_UE(TEXT("{}::Set_{}"), ClassTypeStr, CleanPropertyName);\
        if (NOT FCkAngelScriptPropertyFunctionRegistration::TryRegisterProperty(PropertyKey))\
        { return; }\
        \
        /* Check if method already exists (UHT may have registered it) */\
        auto TypeInfo = ExistingClass.GetTypeInfo();\
        auto SetterMethodName = ck::Format_ANSI(TEXT("Set_{}"), CleanPropertyName);\
        if (TypeInfo->GetMethodByName(SetterMethodName.c_str()) != nullptr)\
        { return; }\
        \
        /* Register setter only */\
        auto SetterSignature = ck::Format_ANSI(TEXT("{}& Set_{}(const {}& InValue)"), ClassTypeStr, CleanPropertyName, PropertyTypeStr);\
        ExistingClass.Method(SetterSignature.c_str(),\
            METHODPR_TRIVIAL(ClassType&, ClassType, CK_CONCAT(Set, _InVar_), (const decltype(_InVar_)&)));\
        \
        FAngelscriptBinds::SetPreviousBindNoDiscard(false);\
        FScriptFunctionNativeForm::BindNativeMethod(ExistingClass, FCkAngelScriptPropertyFunctionRegistration::Get_InternedBindName(SetterMethodName), true);\
    }\
    \
    static inline bool CK_CONCAT(AngelScriptPropertySetterOnlyRegistered_, CK_CONCAT(__LINE__, CK_CONCAT(_, _InVar_))) = []() -> bool\
    {\
        FCkAngelScriptPropertyFunctionRegistration::RegisterPropertyFunction(\
            &CK_CONCAT(RegisterAngelScriptPropertySetterOnly_, CK_CONCAT(__LINE__, CK_CONCAT(_, _InVar_))));\
        return true;\
    }();

// --------------------------------------------------------------------------------------------------------------------

class CKCORE_API FCkAngelScriptCtorFunctionRegistration
{
public:
    using FCtorFunction = TFunction<void()>;

    static auto
    RegisterCtorFunction(
        const FCtorFunction& InCtorFunc) -> void;

    static auto
    TryRegisterCtor(
        const FString& InCtorKey) -> bool
    {
        auto& RegisteredCtors = Get_RegisteredCtors();
        if (RegisteredCtors.Contains(InCtorKey))
        {
            return false;
        }
        RegisteredCtors.Add(InCtorKey);
        return true;
    }

private:
    static auto
    EnsureCallbackRegistered() -> void;

    static auto
    RegisterAllCtorFunctions() -> void;

    static auto
    Get_AllCtorFunctions() -> TArray<FCtorFunction>&;

    static auto
    Get_RegisteredCtors() -> TSet<FString>&
    {
        static TSet<FString> RegisteredCtors;
        return RegisteredCtors;
    }

private:
    static inline FDelegateHandle _PreCompileDelegateHandle;
};

#endif

// --------------------------------------------------------------------------------------------------------------------
