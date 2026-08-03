# P0 Spike Memo — Unreliable unicast Client-direction RPCs under the fork's Iris stack

> **Date:** 2026-08-03 · **Status:** DRAFT — empirical sections pending the first spike run.
> **Artifact of:** Gate 0, work item 4 ([Gate_0.md](Gate_0.md)). Spike code (throwaway):
> CkTests `feature/voice-chat` — `CkAutoTest_VoiceSpike.{h,cpp}`,
> `CkVoiceChat_TransportSpike.spec.cpp` (`Ck.VoiceChat.Spike.*`).
> **Evidence discipline:** engine claims below are [CONFIRMED] against the fork's source
> (UnrealEngine-Angelscript, UE 5.7.4, Iris root `Engine/Source/Runtime/Net/Iris/`) — the three
> load-bearing ones re-verified by hand, the rest by a dedicated source-reading pass; empirical
> claims cite the spike run's log lines.

## What was unproven

ADR-4 (blessed with constraints) rides on one [INFERRED] composite nothing in production
exercises: **sustained-rate (~25 Hz) Unreliable unicast Client-direction RPCs on a per-player
relay actor under Iris** (`net.Iris.UseIrisReplication=1`, BusterBlock `DefaultEngine.ini:6`).
CkCue proves Unreliable Server/Multicast at event rates; CkRenderTarget proves per-player unicast
Client RPCs but Reliable. The CTO review sharpened the spike to three questions:
(a) delivery under packet-fill pressure — does Iris coalesce/starve unreliable attachments;
(b) target channel actor not yet resolved on the client — silent vanish, or ensure/log storm;
(c) per-RPC overhead at ~25 Hz — should the bundle grow past 3 frames.

## Headline finding — "Unreliable unicast" is not what the design assumed

**A unicast RPC is flagged `Ordered` even when Unreliable, and `Ordered` routes it into the
reliable-queue machinery.** [CONFIRMED — read by hand]

- `NetRPCHandler.cpp:24-33` (`CreateRPC`): `Reliable` is set only for `FUNC_NetReliable`, but
  every non-multicast RPC gets `ENetBlobFlags::Ordered`, with the engine's own comment: *"Unicast
  RPCs should be ordered with respect to other reliable and unicast RPCs."*
- `AttachmentReplication.cpp:215-226` (`FNetObjectAttachmentSendQueue::Enqueue`): flags
  `Reliable | Ordered` → the per-object `FReliableSendQueue`; only multicast/non-ordered blobs
  reach the 10-deep drop-oldest unreliable circular queue.

Consequences (all [CONFIRMED] by the source pass unless noted):

| Property | Behavior on the ordered-unreliable path | Voice impact |
|---|---|---|
| Loss semantics | Sent **once**, never resent; on packet loss the sequence slot is re-serialized as an empty stub (`ReliableNetBlobQueue.cpp:208-216`, `:473-485`, `:127-128`) | ✅ exactly voice semantics — silent loss, no stale retransmit, and the receiver still sees the hole |
| Queue depth | 1024-entry in-flight window (`ReliableNetBlobQueue.h:83`) + 4096-entry pre-queue (`net.ReliableRPCQueueSize`, `AttachmentReplication.cpp:26-27`) | ⚠️ saturation manifests as **queueing latency**, not drops — the spike measures lag, not just delivery ratio |
| Overflow policy | Pre-queue full → enqueue **rejected silently** (drop-newest, no log/ensure: `AttachmentReplication.cpp:86-89` → `ReplicationWriter.cpp:317-320`) | ⚠️ the P3 pacing processor is the real freshness guard — the transport will happily queue ~5 minutes of voice at 25 Hz before rejecting |
| Ordering coupling | Reliable attachments on the **same object** are written first; if they don't fit, the unreliable section is never attempted (`AttachmentReplication.cpp:326`, `:360-372`) | 🔒 **design rule: no reliable RPCs on the voice relay actor** — a stalled reliable RPC head-of-line-blocks voice on that channel |
| Packet-full handling | Rolled back per-blob and retried next packet — never dropped by packet pressure (`ReplicationWriter.cpp:2665-2692`; `AttachmentReplication.cpp:429-493`) | ✅ no coalescing/starvation drop; ⚠️ but see latency above |
| State vs attachments | Attachments ride in the same batch as their object's state, written after it; state changemask clears once written, so state cannot starve attachments indefinitely (`ReplicationWriter.cpp:2460-2703`, `:3572`, `:3607-3622`) | ✅ (the voice relay actor has near-zero state anyway) |

## Answers to the three spike questions

### (a) Delivery under packet-fill pressure

**Mechanism [CONFIRMED]:** no starvation-drop exists on this path. Unfitting attachments roll
back and reschedule; the object stays dirty with accumulating priority
(`ReplicationWriter.cpp:1096-1101`, threshold 1.0). The two real blockers are transient:
`WaitOnCreateConfirmation` (~1 RTT after creation, `ReplicationWriter.cpp:2180-2192`) and
replication-record starvation (below 1000 free records only OOB/huge objects write,
`ReplicationWriter.cpp:90-92`, `:3409`). Pressure converts to latency through the deep ordered
queue.

**Empirical [PENDING — fill from `Ck.VoiceChat.Spike.UnreliableUnicast_DeliveryUnderLoad`]:**
- Phase A (1 bundle/tick × 300, no pressure): delivery %, lag min/avg/max = …
- Phase B (8 bundles/tick × 300): …
- Phase C (8 bundles/tick × 300 + 64 KB/tick state churn): delivery %, lag delta vs Phase A = …
- Reliable-control yardstick delivered in full: …

### (b) Client has not resolved the target channel actor

**Mechanism [CONFIRMED]:** four distinct cases, and "silent vanish" is only one of them:

1. **Target out of scope for that connection → silent server-side drop.** No log by default
   (`ReplicationWriter.cpp:295-301`; warning gated behind
   `net.Iris.WarnAboutDroppedAttachmentsToObjectsNotInScope`, default false, `:60-65`).
   Relay channels are `bAlwaysRelevant`, so this is the join/travel-window case only.
2. **In scope, creation not yet sent → RPC rides with/behind the creation data** (scope updates
   before attachment forwarding each frame: `ReplicationSystem.cpp:948-956`; batch write
   `ReplicationWriter.cpp:2472+`).
3. **Creation sent, unconfirmed → queued ~1 RTT, then burst-delivered** (`:2188-2192`).
4. **Root handle unresolvable at send → dropped with a warning throttled per RPC type**
   (`NetBlobManager.cpp:257-270`, `net.Iris.ThrottleRPCWarnings` default true).

**The log-storm risk is real but lives client-side, in a different failure class than the design
worried about:** an RPC arriving for an object the client cannot resolve emits **unthrottled
`LogIrisRpc` Errors** (`NetRPC.cpp:145-152`, `:242-248`, `:488-505`) — at 25 Hz that is a
25-line/s storm, and it is the path a client hits when the channel actor has been torn down
locally (travel/teardown windows) while frames are still in flight. Separately, several
malformed-stream paths **close the connection** (`ReplicationReader.cpp:1377-1397`, `:3304-3309`;
`AttachmentReplication.cpp:1041-1046` — the last one is unreachable for ordered blobs).

**Empirical [PENDING — fill from `Ck.VoiceChat.Spike.UnresolvedTarget_SilentDrop`]:** burst of
20 unreliable + 5 reliable fired the same frame as the spawn → received counts, LogNet/LogIris
warning-or-worse count, samples = …

### (c) Per-RPC overhead at ~25 Hz; bundle sizing

**Mechanism [CONFIRMED]:** per-attachment framing ≈ 53-79 bits (~7-10 bytes) on the with-object
path the server always uses (`ReliableNetBlobQueue.cpp:87-183` field walk;
`ReplicationSystem.cpp:302`), plus ~5-6 bytes of per-object batch overhead amortized across all
attachments to that object in the packet (`ReplicationWriter.cpp:2329-2393`). Byte totals are
arithmetic from field widths, not measured.

**The binding constraint is not overhead — it is the split threshold.** [CONFIRMED — read by
hand] A server→client unreliable RPC whose serialized form exceeds
`ServerUnreliableBitCountSplitThreshold` = **256 bytes**
(`PartialNetObjectAttachmentHandler.h:30`, selected at `.cpp:37-44`) is split into ≤128-byte
ordered parts that must ALL arrive or the whole RPC is discarded (`NetBlobAssembler.cpp:57-65`),
adding ~8-10 bytes per part. **Ruling for the spec: the 3-frame bundle (~190 B payload + ~10 B
framing) sits safely under the threshold; do NOT grow the bundle past 3 × 20 ms frames at
24 kbps.** Higher bitrates or bigger bundles must either stay under 256 B serialized or raise
`[/Script/IrisCore.PartialNetObjectAttachmentHandlerConfig]` in config (it is `UPROPERTY(Config)`).

**Empirical [PENDING]:** wire-bytes per phase vs idle baseline → measured per-RPC cost = …

## Incidental findings

- **`net.Iris.SaturateBandwidth=0` in BusterBlock's `DefaultEngine.ini:7` is a no-op** — the
  cvar was deleted upstream in the 5.6 timeframe (commit `8f1adbd52cbf` removing the
  `IsNetReady(bSaturate)` argument). Dead config; flag to the BB repo owner, not this campaign.
- **Iris has zero VOIP special-casing** (exhaustive grep) — the engine's own VOIP still rides
  the legacy `UVoiceChannel` outside Iris entirely. This module is untrodden ground w.r.t.
  Epic's own usage; nothing to copy, nothing to collide with.
- Diagnostics worth enabling while developing P3:
  `net.Iris.WarnAboutDroppedAttachmentsToObjectsNotInScope 1` (the only signal for the silent
  out-of-scope drop) and `net.Iris.ReplicationWriterCannotSendWarningInterval` (blocked-object
  warnings).

## Design amendments this memo feeds into P3 (recorded in PROMPT.md ledger via Gate_0 close)

1. **No reliable RPCs on the voice relay actor, ever** — head-of-line coupling is per-object.
   Acks/control ride the replicated control plane or a different actor.
2. **The pacing processor must bound FRESHNESS, not just bytes**: the transport queues silently
   (4096 deep) — pace + drop stale frames server-side before enqueue, or latency replaces loss.
3. **Bundle stays ≤3 × 20 ms frames**; serialized RPC must stay under 256 B (split threshold).
4. **Teardown windows must stop sends before destroying channel actors** (client-side
   unresolvable-target path = unthrottled Error storm — N1's drop-counter alone does not cover
   the transport-level log noise).

## Verdict

**[PENDING the empirical run.]** The STOP condition (pathological drop/starvation) has no
mechanism to occur for ordered-unreliable attachments per the source read; the failure mode to
judge instead is queueing latency under saturation, bounded by design amendment 2. Final verdict
after the spike run's numbers land below.

## Run record

[PENDING — commands, exit codes, and the `[VoiceSpike]` log lines verbatim.]
