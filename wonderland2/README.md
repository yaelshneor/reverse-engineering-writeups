# RE 100 – rabbit_hole_xor

Author: Reverse Engineering Course Challenge  
Point Value: 100  
Description: C input transformation cipher checker using DWORD XOR obfuscation

---

This challenge is a relatively straightforward reverse engineering binary once we identify the main transformation applied to the input. The binary is compiled from C code and performs input validation by modifying the user’s input in-place before comparing it to a constant string.

At first glance, running the binary gives us a simple prompt:

> Enter password:

If we provide incorrect input, we immediately get:

> Wrong password!

and on success:

> Correct! you may enter..

So clearly, there is some transformation happening before the comparison stage.

---

Let’s start by opening the binary in a disassembler (IDA / Ghidra / Binary Ninja). Even though the binary is stripped and optimized, the control flow is still very readable once we locate `main`.

Inside `main`, we immediately see a classic stack-based buffer setup:

push ebp  
mov ebp, esp  
sub esp, 40Ch  

followed by buffer initialization:

push 400h  
push 0  
lea ecx, [ebp+Buffer]  
call memset  

and input reading using `fgets`:

push 0  
call __acrt_iob_func  

push eax  
push 3FFh  
lea edx, [ebp+Buffer]  
call fgets  

then computing the input length:

lea eax, [ebp+Buffer]  
call strlen  
mov [ebp+len], eax  

At this point, we enter the interesting part: a loop over the input buffer.

---

The program processes the input in chunks of 4 bytes (DWORDs), which is a very important observation. The loop structure looks like:

mov ecx, [ebp+i]  
add ecx, 4  
mov [ebp+i], ecx  

with loop bounds checking:

mov edx, [ebp+i]  
add edx, 3  
cmp edx, [ebp+len]  
jnb end_loop  

Now we reach the core transformation logic of the binary:

mov eax, [ebp+i]  
mov ecx, dword ptr [ebp+eax+Buffer]  
xor ecx, 41524241h  
mov dword ptr [ebp+eax+Buffer], ecx  

This single block is the entire “cipher” of the challenge.

---

What is happening here is that every 4-byte segment of the input is treated as a 32-bit integer, loaded into a register, XORed with a constant key, and then written back into the same buffer location.

In pseudocode, this is equivalent to:

for (int i = 0; i + 3 < len; i += 4) {
    *(uint32_t*)(Buffer + i) ^= 0x41524241;
}

---

This immediately tells us two important things:

1. The transformation is block-based (DWORD = 4 bytes), not character-based.  
2. The operation is XOR, which is reversible.

Another crucial detail is that the architecture is x86, meaning the system uses little-endian representation. So each 4-character block is interpreted as a 32-bit integer in little-endian order before applying XOR.

---

After the transformation loop completes, the binary performs a comparison against a constant string:

mov [ebp+Str], offset aIntoTheRabbitH_0  
push eax  
push ecx  
lea edx, [ebp+Buffer]  
call strncmp  

The target string is:

"into the rabbit hole"

So the program is effectively checking whether:

transformed_input == "into the rabbit hole"

---

Since XOR is reversible, we can invert the transformation:

A XOR K = B  
A = B XOR K  

This allows us to reconstruct the original input by applying the same XOR operation to the target string, but in reverse (block by block, respecting little-endian format).

---

We can implement the solution directly in Python:

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

Running the script produces the correct input:

(,&.a6:$a03##+&a)->$

Feeding this into the binary successfully passes the check and prints the success message.

---

To summarize, the key insight in this challenge is recognizing that the binary does not operate on characters, but rather on 32-bit DWORD chunks of memory. Each chunk is XORed with a constant key (0x41524241), and the comparison is performed only after this transformation.

Once this is understood, the entire challenge reduces to a simple reversible XOR operation applied at the block level.

---