#!/usr/bin/env python3
"""Fix CarPlay Maps VC core.so (791 build) for MU1427's AirPlay 210.81.

core.so derives the alt-screen (stream type 111) AES key/IV with the stock
AirPlay_DeriveAESKeySHA512ForScreen(session + OFFSET, 16, streamConnectionID, key, iv).
The shared 791 build uses OFFSET 0x1c8 (layout of the newer libairplay in MU1102/1367).
On MU1427 (libairplay.so 857,735 B, sourceVersion 210.81) the stock receiver itself
passes session + 0x1c0 for the main screen stream (AirPlayReceiverSessionSetup @0x23188..0x231e8,
call @0x23310), so with 0x1c8 every alt frame decrypts to garbage
("alt-aes-decrypt-invalid-avcc" in /ramdisk/carplaymaps-video.log).

Patch: vaddr 0x5284  add r0, r8, #0x1c8 (e2880f72) -> add r0, r8, #0x1c0 (e2880f70).
Usage: patch-core-mu1427.py core.so   (idempotent, verifies the original bytes)
"""
import hashlib, struct, sys

ORIG_SHA = "00b48a8fe876e6ff405124b87ed9eace1bcb079220ff7f02b866f42676477a4a"
FIXED_SHA = "46da26893c38367baa483f27ada72f601cc3d418e5b0fee21f7fe3cee88a6a35"
FILE_OFF = 0x5284  # vaddr == file offset in this text segment (p_offset 0, p_vaddr 0)
OLD, NEW = 0xE2880F72, 0xE2880F70

p = sys.argv[1]
d = bytearray(open(p, "rb").read())
h = hashlib.sha256(d).hexdigest()
if h == FIXED_SHA:
    print("already patched"); sys.exit(0)
if h != ORIG_SHA:
    sys.exit("unexpected core.so (sha256 %s), not patching" % h)
assert struct.unpack_from("<I", d, FILE_OFF)[0] == OLD
struct.pack_into("<I", d, FILE_OFF, NEW)
assert hashlib.sha256(d).hexdigest() == FIXED_SHA
open(p, "wb").write(d)
print("patched", p)
