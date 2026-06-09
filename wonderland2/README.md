# 🐇 RE 100 – Rabbit Hole XOR

**Author:** Reverse Engineering Course Challenge  
**Point Value:** 100  
**Category:** Reverse Engineering / Input Transformation  

---

## 📌 Challenge Overview

This challenge is a reverse engineering binary that validates input by applying a transformation over 4-byte chunks and then comparing the result to a constant string.

---

## ▶️ Program Behavior


Enter password:


If wrong:

Wrong password!


If correct:

Correct! you may enter..


---

## 🔍 Static Analysis

The binary was analyzed using IDA / Ghidra / Binary Ninja.

### Stack setup
```asm
push ebp
mov ebp, esp
sub esp, 40Ch
Buffer initialization
push 400h
push 0
lea ecx, [ebp+Buffer]
call memset
Input reading
call __acrt_iob_func
call fgets
Length calculation
call strlen
mov [ebp+len], eax
🔁 Core Transformation Logic

Each 4-byte block is XORed with a constant:

mov eax, [ebp+i]
mov ecx, dword ptr [ebp+eax+Buffer]
xor ecx, 0x41524241
mov dword ptr [ebp+eax+Buffer], ecx
🧠 Pseudocode
for (int i = 0; i + 3 < len; i += 4) {
    *(uint32_t*)(Buffer + i) ^= 0x41524241;
}
📌 Key Observations
Works on 4-byte (DWORD) chunks
Uses XOR (reversible operation)
x86 architecture → little-endian format
🔍 Final Comparison

The transformed buffer is compared against:

into the rabbit hole
strncmp(Buffer, "into the rabbit hole", len);
🔁 Reversing Strategy
A XOR K = B
A = B XOR K
🧪 Exploit Script
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

The key insight is recognizing that the binary operates on 32-bit DWORD blocks instead of characters.

Once understood, the problem reduces to a simple reversible XOR transformation.
