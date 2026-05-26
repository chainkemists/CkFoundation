#include "CkEntityTagQuery_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

FCk_EntityTagQuery_Requirement::FCk_EntityTagQuery_Requirement(
    FName InTag,
    ECk_EntityTagQuery_CountMode InMode,
    int32 InCount,
    int32 InMaxAllowedEnsure)
    : _Tag(InTag)
    , _Mode(InMode)
    , _Count(InCount)
    , _MaxAllowedEnsure(InMaxAllowedEnsure)
{}

// ----

auto
    FCk_EntityTagQuery_Requirement::
    Single(
        FName InTag)
    -> FCk_EntityTagQuery_Requirement
{
    return FCk_EntityTagQuery_Requirement{InTag, ECk_EntityTagQuery_CountMode::SingleOnly, 1, NoEnsure};
}

auto
    FCk_EntityTagQuery_Requirement::
    Of(
        FName InTag,
        int32 InCount)
    -> FCk_EntityTagQuery_Requirement
{
    return FCk_EntityTagQuery_Requirement{InTag, ECk_EntityTagQuery_CountMode::Count, InCount, NoEnsure};
}

auto
    FCk_EntityTagQuery_Requirement::
    All(
        FName InTag)
    -> FCk_EntityTagQuery_Requirement
{
    return FCk_EntityTagQuery_Requirement{InTag, ECk_EntityTagQuery_CountMode::All, 0, NoEnsure};
}

// ----

auto
    FCk_EntityTagQuery_Requirement::
    WithEnsure(
        int32 InMaxAllowed) &
    -> FCk_EntityTagQuery_Requirement&
{
    _MaxAllowedEnsure = InMaxAllowed;
    return *this;
}

auto
    FCk_EntityTagQuery_Requirement::
    WithEnsure(
        int32 InMaxAllowed) &&
    -> FCk_EntityTagQuery_Requirement
{
    _MaxAllowedEnsure = InMaxAllowed;
    return MoveTemp(*this);
}

// --------------------------------------------------------------------------------------------------------------------
