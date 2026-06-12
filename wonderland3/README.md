# Wonderland.exe – Write Up

# Level 3 – Static Analysis

## Objective

The objective of this level was to bypass the validation mechanism without modifying the executable file (No Patching). The solution required understanding how the program reads multiple user inputs, transforms them through a global lookup table, and validates them based on an ordering condition.

---

# Step 1 – Function Identification and Renaming

In the initial static analysis, I identify function calls and look for familiar patterns in the code such as input and output operations. When I recognize standard behavior, I rename the functions to make the program flow clearer.

The program begins by printing an initial prompt.

```asm
push    offset aWaitCanYouHelp ; "..Wait! Can you help me first with some"...
call    sub_401DC0
add     esp, 4
```

<img width="748" height="400" alt="image" src="https://github.com/user-attachments/assets/5ac4b343-e561-4a91-8f88-c97f9dc94c0c" />


---

# Step 2 – Renaming var_4 to i (Loop Counter Identification)

While examining the initialization code, I noticed the following:

```asm
mov     [ebp+var_8], 0
mov     [ebp+var_4], 0
jmp     short loc_401469
```

Following the jump forward to see how `var_4` is used, I found this block:

```asm
loc_401460:
mov     ecx, [ebp+var_4]
add     ecx, 1
mov     [ebp+var_4], ecx

loc_401469:
cmp     [ebp+var_4], 8
```

This is a classic loop pattern:

- `var_4` is initialized to **0**.
- At the end of each iteration, `var_4` is **incremented by 1**.
- The loop condition checks `var_4` against **8**.

This is exactly the structure of:

```c
for (i = 0; i < 8; i++) { ... }
```

Based on this, I renamed `var_4` to **`i`**, since it clearly serves as the loop counter that tracks how many numbers the user has entered so far.
<img width="379" height="257" alt="image" src="https://github.com/user-attachments/assets/bfd10894-4052-4594-9e62-5c3071335563" />
<br><br>
<img width="187" height="142" alt="image" src="https://github.com/user-attachments/assets/e7c65b2d-ce10-473f-a900-922801d706d1" />

---

# Step 3 – Identifying the Number of Required Inputs

The loop condition checks the counter against a fixed value:

```asm
cmp     [ebp+i], 8
jge     short loc_4014AE
```

This means the program enforces **exactly 8 iterations**, so the user must provide 8 separate values.

<img width="240" height="129" alt="image" src="https://github.com/user-attachments/assets/b06e6727-ea34-4cc0-af3f-a483f4fbaa02" />

---

# Step 4 – Understanding How Input is Read

Each iteration reads one value using `scanf`:

```asm
lea     ecx, [ebp+input]
push    ecx
push    offset aHu
call    scanf
add     esp, 8
```

This corresponds to:

```c
scanf("%hu", &input[i]);
```

From this I concluded that each value is read as an **unsigned short**.

<img width="289" height="189" alt="image" src="https://github.com/user-attachments/assets/c222b051-1a03-484a-b28e-33d75e352689" />

---

# Step 5 – Reconstructing the Range Validation

After reading each value, the program performs a range check:

```asm
cmp     edx, 8
jb      short loc_4014AC
```

This corresponds to:

```c
if (input[i] >= 8) {
    // invalid input, request again
}
```

Equivalent constraint:

```text
0 ≤ input[i] < 8
```

So each of the 8 values must be a number between 0 and 7.

<img width="323" height="97" alt="image" src="https://github.com/user-attachments/assets/ef6f36f5-3f6c-481f-9419-bb1ce6aa7144" />

---

# Step 6 – Discovering the Storage Layout

The accepted values are stored in a local array using 2-byte (word) indexing:

```asm
movzx   edx, word ptr [ebp+ecx*2+var_18]
```

This confirms the array holds `unsigned short` elements, consistent with the `%hu` format used in `scanf`.

<img width="294" height="102" alt="image" src="https://github.com/user-attachments/assets/12755b54-5766-4f2f-8bb3-570969ca12a4" />

---

# Step 7 – Locating the Validation Function

Once all 8 values are collected, the program passes the array to a separate validation routine:

```asm
lea     eax, [ebp+var_18]
push    eax
call    sub_4014F0
```

This means the actual validation logic is performed inside `sub_4014F0`, using the array of 8 user-provided values.

<img width="181" height="159" alt="image" src="https://github.com/user-attachments/assets/ca8c598c-cf51-4031-9434-0cb5eaa5f415" />

---

# Step 8 – Discovering the Lookup Table Trick

Inside `sub_4014F0`, I found another loop counter (which I renamed to `i`) controlling iteration over the 8 input values — i.e., the function goes over each of the 8 elements of the input array it received as a parameter.

Each input value is used as an **index** into a global lookup table, due to the `*2` scaling for a word-sized table:

```asm
mov     ecx, [ebp+arg_0]
movzx   edx, word ptr [ecx+eax*2]
mov     ax, word_404000[edx*2]
```
In other words:
```c
mapped[i] = word_404000[input[i]];
```
`input[i]` is not compared directly — it only serves as a **key/index** to fetch the actual value (`word_404000[input[i]]`) that takes part in the validation.

<img width="327" height="240" alt="image" src="https://github.com/user-attachments/assets/6f5d64c9-88d0-48bb-9f8e-0ef85d321c3c" />

---

# Step 9 – Identifying the Final Validation Condition

After the transformation, the program checks the resulting mapped values:

```asm
cmp     [ebp+i], 8
jnb     short loc_401559
```

Combined with surrounding logic, this enforces that the sequence of mapped values must be **strictly increasing**.

Equivalent logic:

```c
for (i = 0; i < 7; i++) {
    if (mapped[i] >= mapped[i+1]) {
        // invalid sequence
    }
}
```
<img width="707" height="582" alt="image" src="https://github.com/user-attachments/assets/fdbe4e13-be24-4273-acd8-95ea603f0851" />

---

# Step 10 – The Lookup Table Data

Examining the `.data` section, I found the actual contents of `word_404000`:

```asm
.data:00404000                 ;org 404000h
.data:00404000 word_404000     dw 7                    ; DATA XREF: sub_4014F0+15↑r
.data:00404000                                         ; sub_4014F0+43↑r
.data:00404002                 db  21h ; !
.data:00404003                 db    0
.data:00404004                 db    1
.data:00404005                 db    0
.data:00404006                 db 0A8h
.data:00404007                 db 0FDh
.data:00404008                 db  78h ; x
.data:00404009                 db 0ECh
.data:0040400A                 db 0F1h
.data:0040400B                 db    6
.data:0040400C                 db  0Dh
.data:0040400D                 db    0
.data:0040400E                 db  45h ; E
.data:0040400F                 db    0
```

Grouping these bytes into 16-bit little-endian words gives the 8 entries of `word_404000`:

| Index `i` | Bytes | Value (`word_404000[i]`) |
|-----------|-------|---------------------------|
| 0 | `07 00` | 7 |
| 1 | `21 00` | 33 |
| 2 | `01 00` | 1 |
| 3 | `A8 FD` | -600 (signed short) |
| 4 | `78 EC` | -5000 (signed short) |
| 5 | `F1 06` | 1777 |
| 6 | `0D 00` | 13 |
| 7 | `45 00` | 69 |

---
<img width="473" height="345" alt="image" src="https://github.com/user-attachments/assets/347bdcba-b2e5-4fe2-a69f-6e4dc07cc639" />


# Step 11 – Solving the Challenge

To pass this level, the user must provide 8 numbers, each in the range `0–7`, such that after applying `word_404000[input[i]]`, the resulting values form a **strictly increasing sequence**.

---

# Final Solution

The required input sequence for Level 3 is:

```text
4 3 2 0 6 1 7 5
```
<img width="954" height="215" alt="image" src="https://github.com/user-attachments/assets/20e05a47-2b28-4afc-a0c8-20a5bbe8093f" />



