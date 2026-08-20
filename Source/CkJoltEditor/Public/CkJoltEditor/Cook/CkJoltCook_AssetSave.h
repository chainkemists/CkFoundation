#pragma once

// --------------------------------------------------------------------------------------------------------------------

class UObject;

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt::cook
{
    /** Writes a freshly-cooked asset over whatever is already on disk. Wraps UPackage::SavePackage with the
     *  two things the editor's own save path does for free and a cook does not get:
     *   - the target is made WRITABLE first. `*.uasset` is marked `lockable` in .gitattributes, so a Git-LFS
     *     (or Perforce) workspace checks every previously-cooked asset out read-only and the overwrite fails
     *     with "Cannot remove '<file>' as it is read only!".
     *   - save errors are kept OFF GError. FSavePackageArgs::Error defaults to the fatal error device, so on
     *     Windows a failed overwrite calls appError and takes the whole editor down rather than returning
     *     false to the caller's ensure. */
    CKJOLTEDITOR_API auto
        Save_CookedAsset(
            UObject& InAsset)
        -> bool;
}

// --------------------------------------------------------------------------------------------------------------------
