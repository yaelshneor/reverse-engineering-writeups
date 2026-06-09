# 🐇 RE 100 – Rabbit Hole XOR

**Author:** Reverse Engineering Course Challenge  
**Point Value:** 100  
**Category:** Reverse Engineering / Input Transformation  

---

## 📌 Challenge Overview

This challenge is a relatively straightforward reverse engineering binary once the main transformation applied to the input is identified.

The binary is compiled from C and performs input validation by modifying the user’s input in-place before comparing it to a constant string.

---

## ▶️ Program Behavior

When running the binary, we see:


Enter password:


If the input is incorrect:


Wrong password!


If the input is correct:


Correct! you may enter..


This indicates that the input is transformed before comparison.

---

## 🔍 Static Analysis

Using a disassembler (IDA / Ghidra / Binary Ninja), we analyze `main`.

We first observe stack setup:

```asm
push ebp
mov ebp, esp
sub esp, 40Ch

Buffer initialization:

push 400h
push 0
lea ecx, [ebp+Buffer]
call memset

Input is read using:

call __acrt_iob_func
call fgets

And length is computed via:

call strlen
mov [ebp+len], eax
🔁 Core Transformation Logic

The binary processes input in 4-byte chunks (DWORDs):

mov eax, [ebp+i]
mov ecx, dword ptr [ebp+eax+Buffer]
xor ecx, 41524241h
mov dword ptr [ebp+eax+Buffer], ecx
🧠 Pseudocode
for (int i = 0; i + 3 < len; i += 4) {
    *(uint32_t*)(Buffer + i) ^= 0x41524241;
}
📌 Key Observations
Input is processed in DWORD (4-byte) blocks
Transformation is XOR-based
XOR is reversible
Architecture is x86 → little-endian format
🔍 Final Comparison

After transformation, the buffer is compared against:

"into the rabbit hole"

Using:

strncmp(Buffer, target, len)
🔁 Reversing Strategy

Since XOR is reversible:

A XOR K = B  
A = B XOR K

We apply the inverse transformation to the known target string.

🧪 Exploit Script (Python)
s = "into the rabbit hole"
key = 0x41524241

res = bytearray()

for i in range(0, len(s), 4):
    chunk = s[i:i+4].encode('latin1')
    val = int.from_bytes(chunk, "little")
    val ^= key
    res.extend(val.to_bytes(4, "little"))

print(res)
🎯 Solution
(,&.a6:$a03##+&a)->$
🏁 Conclusion

The key insight of this challenge is recognizing that the binary operates on 32-bit DWORD chunks rather than individual characters.

Once identified, the entire challenge reduces to a simple reversible XOR operation over block-aligned memory.
