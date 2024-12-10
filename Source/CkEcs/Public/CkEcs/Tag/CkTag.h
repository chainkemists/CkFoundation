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

#define CK_DEFINE_ECS_TAG_COUNTED(_TagName_)\
    struct _TagName_ : public ck::FTag_CountedTag { };\
    static_assert(sizeof(_TagName_) == sizeof(int32), "Counted Tags must NOT carry any additional data")

// --------------------------------------------------------------------------------------------------------------------