#pragma once

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <typename T_DerivedTag>
    struct TTag { };
}

#define CK_DEFINE_ECS_TAG(_TagName_)\
    struct _TagName_ : public ck::TTag<_TagName_> { };\
    static_assert(std::is_empty_v<_TagName_>, "Tags must not carry any data")

// --------------------------------------------------------------------------------------------------------------------