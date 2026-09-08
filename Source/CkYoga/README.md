# CkYoga

`CkYoga` packages Yoga's native flex-layout core for CkFoundation runtime consumers.
It provides the upstream C ABI through `<yoga/Yoga.h>` and intentionally contains
no Slate or framework adapter.

The vendored sources are isolated in this module because Yoga enables C++ exceptions
while the existing CkThirdParty module compiles Jolt and other dependencies.

See [UPSTREAM.md](UPSTREAM.md) for the imported source identity.

## Consuming the native library

Add `CkYoga` to the consumer's `PrivateDependencyModuleNames` when its public
headers do not expose Yoga types, then include `<yoga/Yoga.h>`. Native integration
tests live in CkTests under `Private/UnitTests/CkYoga/` and the automation family
`Ck.Yoga.Integration`.

Configure `YGConfigRef` before creating nodes. A node tree must not outlive its
configuration; free owned nodes before freeing the config. Measured content
changes require `YGNodeMarkDirty` on the measured leaf. The consumer owns context
pointers and keeps their targets alive for all node callbacks.

Yoga's native API requires valid inputs and may terminate or throw on violated
preconditions. It is not a recoverable CK validation boundary. The future Slate
adapter must validate declarations atomically before calling it; do not expose
raw nodes or fatal preconditions directly through BP/AS bindings.

Module registration follows CkFoundation's Win64/Mac/Linux scope. Actual build and
test evidence, and the proposed native Slate vertical, are tracked in
[PROGRESS.md](PROGRESS.md) and [PLAN.md](PLAN.md).
