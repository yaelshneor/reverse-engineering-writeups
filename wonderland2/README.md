# Wonderland.exe – Write Up

# Level 2 – Static Analysis

## Objective

The objective of this level was to bypass the validation mechanism without modifying the executable file (No Patching). The solution required understanding how the program reads the user's input into a buffer, transforms it with a repeating XOR key on 4-byte blocks, and compares the result against a known target string.

---

# Step 1 – Function Identification and Renaming

In the initial static analysis, I identify function calls and look for familiar patterns in the code such as input and output operations. When I recognize standard behavior, I rename the functions to make the program flow clearer.

The program begins by printing an initial prompt.
```asm
push    offset aYouKnowWhatTha ; "You know what? That was too easy. *Now*"...
call    sub_401DC0
add     esp, 4
```

<img width="737" height="154" alt="image" src="https://github.com/user-attachments/assets/26bd4966-57a0-4d4c-95e3-c72c2abc48ef" />

---

## Step 2 – Renaming var_4 to i (Loop Counter Identification)

I noticed a loop structure:

```asm
mov     [ebp+length], eax
mov     [ebp+var_4], 0
jmp     short loc_4013AE
```

```asm
loc_4013A5:
mov     ecx, [ebp+var_4]
add     ecx, 4
mov     [ebp+var_4], ecx
```

`var_4` starts at 0 and is **incremented by 4 on each iteration**. This is the structure of a loop that advances 4 bytes (one DWORD) at a time.

Based on this, I renamed `var_4` to **`i`**, since it serves as the byte offset into `Buffer`.

<img width="217" height="156" alt="image" src="https://github.com/user-attachments/assets/5ceaf7fe-ee0e-4a2d-8141-e266483a2e87" />
<br><br>

<img width="330" height="95" alt="image" src="https://github.com/user-attachments/assets/1ec51f53-832c-4ef2-bab6-2ab8e2e65e3f" />


---

## Step 3 – Renaming var_C to length

After the `strlen` call, the result is stored into `var_C`:

```asm
call    strlen
add     esp, 4
mov     [ebp+var_C], eax
```

Since `strlen` returns the length of the input string and the result is saved here and used throughout the loop condition, I renamed `var_C` to **`length`**.

<img width="335" height="84" alt="image" src="https://github.com/user-attachments/assets/22134368-d6a8-4c42-8be2-a73253145b16" />

---

## Step 4 – Stack Setup, Buffer Initialization and Reading User Input

The function sets up a large local buffer, clears it before reading any input, and then reads a line from the user and calculates its length:

```asm
push    ecx             ; void *
call    memset
add     esp, 0Ch
push    0               ; Ix
call    ds:__acrt_iob_func
add     esp, 4
push    eax             ; Stream
push    3FFh            ; MaxCount
lea     edx, [ebp+Buffer]
push    edx             ; Buffer
call    ds:fgets
```
After zeroing out the buffer with memset — which can be used for both input and output buffers, and here we can tell it's input because 0 is pushed onto the stack — the program calls fgets to read the user's input into it.

<img width="331" height="496" alt="image" src="https://github.com/user-attachments/assets/fba47615-6340-47ca-b1d9-20a62bd120a0" />

---

## Step 5 – Discovering the Core Transformation (XOR per DWORD)

Inside the loop, each 4-byte block of the buffer is XORed with a fixed constant:

```asm
mov     eax, [ebp+i]
mov     ecx, dword ptr [ebp+eax+Buffer]
xor     ecx, 41524241h
mov     dword ptr [ebp+eax+Buffer], ecx
```

On each iteration, the program takes 4 bytes from the input buffer and XORs them with the value `0x41524241`.
values are stored in **little-endian** format.

<img width="321" height="153" alt="image" src="https://github.com/user-attachments/assets/7bb94ca2-75ba-49ec-9a37-f8a000b95bf0" />


---

## Step 6 – Discovering the Comparison String

After the transformation, the modified buffer is compared against a fixed string:

```asm
mov     eax, offset aIntoTheRabbitH0 ; "into the rabbit hole"
mov     edx, [ebp+Str]
push    eax
push    edx
lea     ecx, [ebp+Str]
call    strlen
push    eax             ; MaxCount
call    strncmp
```

So the **transformed** input must be equal to `"into the rabbit hole"`.

<img width="570" height="325" alt="image" src="https://github.com/user-attachments/assets/9a901e33-0f2f-45c6-a44f-931aa449aa8c" />


---

## Step 7 – Reversing Strategy

Since the transformation is a simple XOR with a fixed key:

```
A XOR K = B
```

The inverse is just XOR again with the same key:

```
A = B XOR K
```

Where:

- `B` = `"into the rabbit hole"` (the required result after transformation)
- `K` = `0x41524241`
- `A` = the raw input the user must type, **before** the program applies the XOR

So to find the correct input, I XOR each 4-byte block of the target string with `0x41524241` (respecting little-endian byte order), and that gives the bytes the user needs to type.

---

## Step 8 – Exploit Script

```python
s = "into the rabbit hole"
key = 0x41524241
res = bytearray()

for i in range(0, len(s), 4):
    chunk = s[i:i+4].encode('latin1')
    val = int.from_bytes(chunk, "little")
    val ^= key
    res.extend(val.to_bytes(4, "little"))

print(res.decode('latin1'))
```

Running this script produces the exact byte sequence that, once read into `Buffer` and XORed by the program with `0x41524241`, becomes `"into the rabbit hole"`.

<img width="767" height="353" alt="image" src="https://github.com/user-attachments/assets/5a978e3c-d6bf-4cdb-9736-023aa486ce63" />

---

## Final Solution

The required input for Level 2 is:

```
(,&.a6:$a03##+&a)->$
```
<img width="767" height="353" alt="image" src="https://github.com/user-attachments/assets/0ddd487e-7f75-4f0f-beb5-a87fabd790f8" />


