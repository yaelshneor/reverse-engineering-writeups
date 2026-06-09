# 🐇 RE 100 – Rabbit Hole XOR

**Author:** Reverse Engineering Course Challenge  
**Point Value:** 100  
**Category:** Reverse Engineering / Input Transformation  

---

## 📌 Challenge Overview

This challenge is a reverse engineering binary that validates user input by applying an in-place transformation before comparing it to a constant string.

The goal is to understand the transformation logic and recover the correct input.

---

## ▶️ Program Behavior

The program prompts:

Enter password:

Wrong input:
Wrong password!

Correct input:
Correct! you may enter..

This indicates that the input is transformed before comparison.

---

## 🔍 Static Analysis

The binary was analyzed using IDA / Ghidra / Binary Ninja.

Stack setup:
push ebp
mov ebp, esp
sub esp, 40Ch

Buffer initialization:
push 400h
push 0
lea ecx, [ebp+Buffer]
call memset

Input reading:
call __acrt_iob_func
call fgets

Length calculation:
call strlen
mov [ebp+len], eax

---

## 🔁 Core Transformation Logic

The input is processed in 4-byte (DWORD) chunks:

mov eax, [ebp+i]
mov ecx, dword ptr [ebp+eax+Buffer]
xor ecx, 0x41524241
mov dword ptr [ebp+eax+Buffer], ecx

---

## 🧠 Pseudocode

for (int i = 0; i + 3 < len; i += 4) {
    *(uint32_t*)(Buffer + i) ^= 0x41524241;
}

---

## 📌 Key Observations

- Input is processed in 4-byte blocks (DWORD)
- Transformation is XOR-based
- XOR is reversible
- Architecture is x86 (little-endian)

---

## 🔍 Final Comparison

After transformation, the buffer is compared to:

into the rabbit hole

strncmp(Buffer, "into the rabbit hole", len);

---

## 🔁 Reversing Strategy

A XOR K = B  
A = B XOR K  

We apply the same XOR operation on the target string to recover the original input.

---

## 🧪 Exploit Script (Python)

s = "into the rabbit hole"
key = 0x41524241

res = bytearray()

for i in range(0, len(s), 4):
    chunk = s[i:i+4].encode('latin1')
    val = int.from_bytes(chunk, "little")
    val ^= key
    res.extend(val.to_bytes(4, "little"))

print(res)

---

## 🎯 Solution

(,&.a6:$a03##+&a)->$

---

## 🏁 Conclusion

The key insight is that the binary operates on 32-bit DWORD blocks instead of individual characters.

Once this is identified, the challenge reduces to a simple reversible XOR operation at block level.
