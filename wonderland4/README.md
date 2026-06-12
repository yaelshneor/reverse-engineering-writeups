# Wonderland.exe – Write Up

# Level 4 – Static and Dynamic Analysis

## Objective

The objective of this level was to bypass the validation mechanism without modifying the executable file (No Patching). The solution required understanding how the program validates the user input and identifying the correct memory address expected by the comparison routine.

---
# Step 1 – Function Identification and Renaming

In the initial static analysis, I identify function calls and look for familiar patterns in the code such as input and output operations. When I recognize standard behavior, I rename the functions to make the program flow clearer.

```asm
.text:00401576 push    offset aWaitIHaveSomet          ; "Wait... I have something on the tip of "...
.text:0040157B call    sub_401DC0
.text:00401580 add     esp, 4
```

<img width="662" height="278" alt="image" src="https://github.com/user-attachments/assets/1d9fccef-605d-43dc-ac6e-d67036947829" />


# Step 2 – Understanding the User Input

 I examined how the program receives the user's input.

The following code is executed:

```asm
lea ecx, [ebp+input]
push ecx
push offset aDu
call scanf
```

This corresponds to:

```c
scanf("%u", &input);
```

From this point I concluded that the program expects a numeric value from the user.

<img width="305" height="146" alt="image" src="https://github.com/user-attachments/assets/79bbebe9-bb91-4445-9651-b928e4a3925f" />


# Step 3 – Reconstructing the Target String

After reading the input, the program builds a string manually on the stack.

The assembly instructions write the following text into a local buffer:

```text
Good luck!!
```

This means the target string exists only temporarily on the stack during execution.

<img width="385" height="157" alt="image" src="https://github.com/user-attachments/assets/3d523b4c-87fb-4dfa-ac38-0bcc84139042" />

<img width="677" height="278" alt="image" src="https://github.com/user-attachments/assets/33c57e9e-ff22-4e73-8317-07da095c43b5" />



# Step 4 – Discovering the Pointer Trick

Near the end of the function, the program calls:

```c
strncmp()
```

When analyzing the parameters passed to the function, I noticed a critical difference.

The locally created string is passed using:

```asm
lea eax, [ebp+Str]
```

which means its memory address is supplied.

However, the user input is passed differently:

```asm
mov edx, [ebp+input]
push edx
```

Instead of passing the address of the variable, the code passes the value stored inside it.

This means that the number entered by the user is interpreted as a memory address (pointer).

Effectively the comparison becomes:

```c
strncmp(
    (char*)input,
    "Good luck!!",
    strlen("Good luck!!")
);
```

At this point I concluded that the user must provide a valid memory address that points to a string beginning with:

```text
Good luck!!
```

<img width="379" height="128" alt="image" src="https://github.com/user-attachments/assets/c4f088fd-ba0e-4b28-bf53-af132202eeb8" />


# Step 5 – Identifying the Anti-Cheat Mechanism

Immediately after the comparison, the program performs another check:

```asm
lea eax, [ebp+Str]
cmp [ebp+input], eax
```

Equivalent logic:

```c
if(input == &Str)
{
    printf("Cheater.. Try again another way.");
}
```

This means that even if the user discovers the address of the stack string, using that exact address is forbidden.

The program explicitly blocks this solution.

<img width="1292" height="402" alt="image" src="https://github.com/user-attachments/assets/8b6ce498-0abb-4728-94ed-bb08386771a5" />

<img width="677" height="278" alt="image" src="https://github.com/user-attachments/assets/33c57e9e-ff22-4e73-8317-07da095c43b5" />

<img width="443" height="81" alt="image" src="https://github.com/user-attachments/assets/d65d8af3-511c-4757-a77e-e8ba41c2115b" />


# Step 6 – Searching for an Alternative String

Since the stack address cannot be used, I searched the program's data sections for another string that already contains the required text.

Inside IDA I found:

```asm
.data:00404738 aYeahGoodLuckAn db 'Yeah! Good luck!! (and good job!)'
```

This string already contains:

```text
Good luck!!
```

as part of a larger sentence.

<img width="1069" height="610" alt="image" src="https://github.com/user-attachments/assets/6b9b8311-847a-4ae9-b0c1-7a263f576c8f" />


# Step 7 – Calculating the Correct Address

The string begins at:

```text
0x00404738
```

The word:

```text
Good
```

does not start at the beginning of the string.

The first characters are:

```text
Yeah!
```

followed by a space.

Therefore:

```text
0x00404738 + 6 = 0x0040473E
```

Address:

```text
0x0040473E
```

points directly to:

```text
Good luck!! (and good job!)
```

Since `strncmp` only checks the beginning of the string, this address satisfies the validation logic.

# Step 8 – Converting the Address

The program expects input through:

```c
scanf("%u")
```

which requires a decimal number.

The address:

```text
0x0040473E
```

was converted to decimal:

```text
4212542
```

# Final Solution

The required input for Level 4 is:

```text
4212542
```

This value represents the address:

```text
0x0040473E
```

which points to:

```text
Good luck!! (and good job!)
```

The comparison succeeds and the anti-cheat check is bypassed because this address is different from the stack buffer address.

<img width="972" height="277" alt="image" src="https://github.com/user-attachments/assets/6dc5f2ad-0f11-40fd-87d0-f01c95cc3b7c" />

