#include "CkResourceLoader_Fragment_Data.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Validation/CkIsValid.h"

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
    return Get_UniqueLoadedObjects() == InOther.Get_UniqueLoadedObjects() &&
        Get_AllOrderedLoadedObjects() == InOther.Get_AllOrderedLoadedObjects();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_ResourceLoader_RootedAssetBatch::
    Get_IsRequested() const
    -> bool
{
    return _Requested;
}

auto
    FCk_ResourceLoader_RootedAssetBatch::
    Get_IsReady() const
    -> bool
{
    if (NOT Get_IsRequested())
    { return false; }

    if (NOT _StreamableHandle.IsValid())
    { return true; }

    return _StreamableHandle->HasLoadCompleted() || _StreamableHandle->WasCanceled();
}

auto
    FCk_ResourceLoader_RootedAssetBatch::
    Get_HasFailed() const
    -> bool
{
    if (NOT Get_IsRequested())
    { return false; }

    if (NOT _StreamableHandle.IsValid())
    { return true; }

    if (_StreamableHandle->WasCanceled())
    { return true; }

    if (NOT _StreamableHandle->HasLoadCompleted())
    { return false; }

    return ck::algo::AnyOf(_RequestedPaths, [](const FSoftObjectPath& InPath)
    {
        return ck::Is_NOT_Valid(InPath.ResolveObject(), ck::IsValid_Policy_NullptrOnly{});
    });
}

auto
    FCk_ResourceLoader_RootedAssetBatch::
    Get_ResolvedObject(
        const FSoftObjectPath& InPath) const
    -> UObject*
{
    if (NOT _StreamableHandle.IsValid() || NOT _StreamableHandle->HasLoadCompleted())
    { return nullptr; }

    return InPath.ResolveObject();
}

// --------------------------------------------------------------------------------------------------------------------
