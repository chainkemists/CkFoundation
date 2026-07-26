#pragma once

#include "CkEcs/Archetype/CkArchetype_Data.h"
#include "CkEcs/Archetype/CkArchetype_Registry.h"
#include "CkEcs/Handle/CkHandle.h"

#include "Misc/DelayedAutoRegister.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// Typed archetype amalgamation — authoring example and rationale in CkEcs/CLAUDE.md. Each listed name X is
// simultaneously the member name, the typesafe handle type FCk_Handle_X and the utils class UCk_Utils_X_UE;
// the macros are purely textual and resolve at the EXPANSION site. Up to 8 features per archetype.
// --------------------------------------------------------------------------------------------------------------------

#define CK_ARCHETYPE_BODY(_StructType_)                                                    \
    static auto TryCast(const FCk_Handle& InHandle) -> TOptional<_StructType_>

// ---- FOR_EACH plumbing (up to 8) -----------------------------------------------------

#define CK_ARCHETYPE_EXPAND(x) x
#define CK_ARCHETYPE_FE_1(M, A1) M(A1)
#define CK_ARCHETYPE_FE_2(M, A1, A2) M(A1) M(A2)
#define CK_ARCHETYPE_FE_3(M, A1, A2, A3) M(A1) M(A2) M(A3)
#define CK_ARCHETYPE_FE_4(M, A1, A2, A3, A4) M(A1) M(A2) M(A3) M(A4)
#define CK_ARCHETYPE_FE_5(M, A1, A2, A3, A4, A5) M(A1) M(A2) M(A3) M(A4) M(A5)
#define CK_ARCHETYPE_FE_6(M, A1, A2, A3, A4, A5, A6) M(A1) M(A2) M(A3) M(A4) M(A5) M(A6)
#define CK_ARCHETYPE_FE_7(M, A1, A2, A3, A4, A5, A6, A7) M(A1) M(A2) M(A3) M(A4) M(A5) M(A6) M(A7)
#define CK_ARCHETYPE_FE_8(M, A1, A2, A3, A4, A5, A6, A7, A8) M(A1) M(A2) M(A3) M(A4) M(A5) M(A6) M(A7) M(A8)
#define CK_ARCHETYPE_GET_FE(_1, _2, _3, _4, _5, _6, _7, _8, NAME, ...) NAME
#define CK_ARCHETYPE_FOR_EACH(M, ...)                                                      \
    CK_ARCHETYPE_EXPAND(CK_ARCHETYPE_GET_FE(__VA_ARGS__,                                   \
        CK_ARCHETYPE_FE_8, CK_ARCHETYPE_FE_7, CK_ARCHETYPE_FE_6, CK_ARCHETYPE_FE_5,        \
        CK_ARCHETYPE_FE_4, CK_ARCHETYPE_FE_3, CK_ARCHETYPE_FE_2, CK_ARCHETYPE_FE_1)(M, __VA_ARGS__))

// ---- Per-member expansions -------------------------------------------------------------

#define CK_ARCHETYPE_MEMBER_HAS_GATE(_Feature_)                                            \
    if (NOT UCk_Utils_##_Feature_##_UE::Has(InHandle))                                     \
    { return {}; }

#define CK_ARCHETYPE_MEMBER_ASSIGN(_Feature_)                                              \
    Result._Feature_ = UCk_Utils_##_Feature_##_UE::Cast(InHandle);

#define CK_ARCHETYPE_MEMBER_FEATUREID(_Feature_)                                           \
    FName{TEXT(#_Feature_)},

// ---- The definition macro ---------------------------------------------------------------

#define CK_DEFINE_ARCHETYPE(_StructType_, _NameStr_, ...)                                  \
auto                                                                                       \
    _StructType_::                                                                         \
    TryCast(                                                                               \
        const FCk_Handle& InHandle)                                                        \
    -> TOptional<_StructType_>                                                             \
{                                                                                          \
    if (ck::Is_NOT_Valid(InHandle))                                                        \
    { return {}; }                                                                         \
                                                                                           \
    CK_ARCHETYPE_FOR_EACH(CK_ARCHETYPE_MEMBER_HAS_GATE, __VA_ARGS__)                       \
                                                                                           \
    auto Result = _StructType_{};                                                          \
    CK_ARCHETYPE_FOR_EACH(CK_ARCHETYPE_MEMBER_ASSIGN, __VA_ARGS__)                         \
    return Result;                                                                         \
}                                                                                          \
                                                                                           \
static FDelayedAutoRegisterHelper GCkArchetypeRegistrar_##_StructType_{                    \
    EDelayedRegisterRunPhase::EndOfEngineInit,                                             \
    []()                                                                                   \
    {                                                                                      \
        auto Descriptor = FCk_ArchetypeDescriptor{FName{TEXT(_NameStr_)}};                 \
        Descriptor.Set_FeatureIds(TArray<FName>{                                           \
            CK_ARCHETYPE_FOR_EACH(CK_ARCHETYPE_MEMBER_FEATUREID, __VA_ARGS__)              \
        });                                                                                \
                                                                                           \
        ck::archetype_registry::Register(Descriptor,                                       \
            [](const FCk_Handle& InHandle) -> bool                                         \
            {                                                                              \
                return _StructType_::TryCast(InHandle).IsSet();                            \
            });                                                                            \
    }}
