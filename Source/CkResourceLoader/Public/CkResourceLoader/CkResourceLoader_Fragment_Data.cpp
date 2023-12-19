#include "CkResourceLoader_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------
auto
    FCk_ResourceLoader_ObjectReference_Soft::
    operator==(
        const ThisType& InOther) const
    -> bool
{
    return Get_SoftObjectPath() == InOther.Get_SoftObjectPath();
}

auto
    GetTypeHash(
        const FCk_ResourceLoader_ObjectReference_Soft& InObj)
    -> uint32
{
    return GetTypeHash(InObj.Get_SoftObjectPath());
}

// --------------------------------------------------------------------------------------------------------------------

FCk_ResourceLoader_ObjectReference_Hard::
    FCk_ResourceLoader_ObjectReference_Hard(
        UObject* InObject)
    : _Object(InObject)
{
}

auto
    FCk_ResourceLoader_ObjectReference_Hard::
    operator==(
        const ThisType& InOther) const
    -> bool
{
    return Get_Object() == InOther.Get_Object();
}

auto
    GetTypeHash(
        const FCk_ResourceLoader_ObjectReference_Hard& InObj)
    -> uint32
{
    return GetTypeHash(InObj.Get_Object());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_ResourceLoader_LoadedObject::
    operator==(
        const ThisType& InOther) const
    -> bool
{
    return Get_ObjectReference_Soft() == InOther.Get_ObjectReference_Soft() &&
           Get_ObjectReference_Hard() == InOther.Get_ObjectReference_Hard();
}

auto
    GetTypeHash(
        const FCk_ResourceLoader_LoadedObject& InObj)
    -> uint32
{
    return GetTypeHash(InObj.Get_ObjectReference_Soft()) + GetTypeHash(InObj.Get_ObjectReference_Hard());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_ResourceLoader_PendingObject::
    operator==(
        const ThisType& InOther) const
    -> bool
{
    return Get_ObjectReference_Soft() == InOther.Get_ObjectReference_Soft();
}

auto
    GetTypeHash(
        const FCk_ResourceLoader_PendingObject& InObj)
    -> uint32
{
    return GetTypeHash(InObj.Get_ObjectReference_Soft());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_ResourceLoader_PendingObjectBatch::
    operator==(
        const ThisType& InOther) const
    -> bool
{
    return Get_ObjectReferences_Soft() == InOther.Get_ObjectReferences_Soft();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_ResourceLoader_LoadedObjectBatch::
    operator==(
        const ThisType& InOther) const
    -> bool
{
    return Get_LoadedObjects() == InOther.Get_LoadedObjects();
}

// --------------------------------------------------------------------------------------------------------------------
