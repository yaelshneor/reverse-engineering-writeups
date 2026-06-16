# Wonderland.exe – Write Up

# The Queen's Maze – Static and Dynamic Analysis

## Objective

The objective of this level was to bypass the validation mechanism **without patching the binary**. Unlike previous levels where the check was a straightforward string or numeric comparison, this time the validation routine turned out to be a small **maze interpreter** — it walks a hard-coded grid step by step according to the characters supplied by the user, and only accepts the input if every step lands on a valid floor tile and the final position is exactly the target tile.

---

# Step 1 – Function Identification and Renaming

In the initial static analysis, I identify function calls and look for familiar patterns in the code such as input and output operations. When I recognize standard behavior, I rename the functions to make the program flow clearer.

```asm
.text:00401659 push    offset aYouMayEnterBut ; "You may enter, but can you find the Que"...
.text:0040165E call    sub_401650
.text:00401663 add     esp, 4
```

From this I concluded that `sub_401650` is the main entry point of the challenge and renamed it `MazeChallenge`. I also noticed that `sub_401DC0` was being called every time a string was printed to the console — a classic `printf` wrapper — so I renamed it `Printf`. Finally, later in the same function I spotted `sub_401770` being called with a single buffer argument, with its return value immediately tested to decide between `"You are lost!\r\n"` and `"You have found the Queen's palace!\r\n"`. That pattern made it clear this was the validation routine, so I renamed it `ValidatePath`.

<img width="692" height="136" alt="image" src="https://github.com/user-attachments/assets/eaf80c9d-0263-436c-9345-3725cc5b27cb" />

---

# Step 2 – Understanding the Input Expectation and File Opening

I then examined how the program actually receives input from the user. The following code runs first:

```asm
lea  ecx, [ebp+Buffer]
push ecx
push 3FFh
push eax            ; stdin
call fgets
```

This is a straightforward `fgets` call reading up to 1023 characters from stdin. My first instinct was that this was reading the answer directly — but looking at what happens next changed that assumption immediately:

```asm
call    CreateFileA     ; lpFileName = Buffer
mov     [ebp+hFile], eax
cmp     [ebp+hFile], 0FFFFFFFFh
jnz     short loc_4016E2
push    offset aWrong_0 ; "Wrong!\r\n"
```

The buffer is not being compared — it is being passed as a **file name** to `CreateFileA`. If the file does not exist on disk, the program prints `"Wrong!\r\n"` and loops back. This was the first key insight: the user is not expected to type the answer, but rather the **name of a file** that contains it.

---
<img width="466" height="583" alt="image" src="https://github.com/user-attachments/assets/8986a176-6b7f-45d3-8585-102cf5a2a040" />


# Step 3 – Reading the File Contents and Passing Them to the Validator

Once the file is opened successfully, the program reads its contents:

```asm
push    400h            ; nNumberOfBytesToRead
lea     edx, [ebp+var_808]
push    edx             ; lpBuffer
mov     eax, [ebp+hFile]
push    eax
call    ReadFile
```

After reading, the handle is closed and the buffer is passed directly to `ValidatePath`:

```asm
call    ds:CloseHandle
lea     eax, [ebp+var_808]
push    eax
call    sub_401770      ; ValidatePath(fileContent)
```

This confirmed the picture: it is the **contents of the file** — not its name — that get validated. The real key needs to be written inside the file, and the program is simply told where to find it.

<img width="438" height="316" alt="image" src="https://github.com/user-attachments/assets/e5181a39-5a05-4730-80fe-a690a5d11d68" />

---

# Step 4 – Recognizing the Maze Logic

Diving into `ValidatePath`, the very first thing the function does is search for the character `'O'` inside a global string (`Str`) and saves its address as the starting position:

```asm
push    4Fh         ; 'O'
mov     eax, Str
push    eax
call    strchr      ; pos = address of 'O' in the map
```

That immediately suggested a maze: there is a defined starting point. Scrolling to the end of the function confirmed the win condition:

```asm
cmp     eax, 58h    ; 'X'
```

The function returns success only if the final position holds `'X'`. Between start and end, each character in the input is processed through a **switch with 4 cases** — digits `'1'` through `'4'` — where each case moves the current position pointer in a different direction within the map. Cells containing `'.'` are valid floor tiles; anything else is treated as a wall and breaks the loop.

<img width="1520" height="271" alt="image" src="https://github.com/user-attachments/assets/257c5c32-fa3f-4996-8371-f8201a8f7c00" />

---

# Step 5 – Extracting the Maze from `.data`

With the logic understood, I turned to the `.data` section to recover the actual map. `Str` is a pointer to `aO`, a 96-byte buffer (plus a null terminator) defined in the binary:

```asm
.data:00404010 Str             dd offset aO            ; "###################O####...########.##."...
.data:00404020 aO              db '###################O####...########.##...#.########.##.###.######'
.data:00404061                 db '##....###...X##################',0
```

The stride used for vertical movement inside `ValidatePath` is `0x10` (16), which means the map is **16 columns wide**. Dividing 96 bytes by 16 gives exactly 6 rows. Splitting the raw string into 16-character rows reconstructs the full grid:

```
################
###O####...#####
###.##...#.#####
###.##.###.#####
###....###...X##
################
```

- `O` → row 1, column 3 — entrance
- `X` → row 4, column 13 — Queen's palace
- `#` → wall
- `.` → walkable floor tile

<img width="943" height="167" alt="image" src="https://github.com/user-attachments/assets/156576ca-61a0-473a-a006-53d981250a8c" />

---

# Step 6 – Solving the Maze

Starting at `O` and stepping only onto `'.'` tiles, with the final step landing on `'X'`, I traced the single valid path through the grid:

```
################
###↓####→.↓#####
###↓##→→↑#↓#####
###↓##↑###↓#####
###→→→↑###→→→X##
################
```

Converting each direction to its corresponding digit (`right=1`, `down=2`, `left=3`, `up=4`):

| Step | From       | To         | Direction | Digit |
|------|------------|------------|-----------|-------|
| 1    | (1,3) O    | (2,3)      | down      | 2     |
| 2    | (2,3)      | (3,3)      | down      | 2     |
| 3    | (3,3)      | (4,3)      | down      | 2     |
| 4    | (4,3)      | (4,4)      | right     | 1     |
| 5    | (4,4)      | (4,5)      | right     | 1     |
| 6    | (4,5)      | (4,6)      | right     | 1     |
| 7    | (4,6)      | (3,6)      | up        | 4     |
| 8    | (3,6)      | (2,6)      | up        | 4     |
| 9    | (2,6)      | (2,7)      | right     | 1     |
| 10   | (2,7)      | (2,8)      | right     | 1     |
| 11   | (2,8)      | (1,8)      | up        | 4     |
| 12   | (1,8)      | (1,9)      | right     | 1     |
| 13   | (1,9)      | (1,10)     | right     | 1     |
| 14   | (1,10)     | (2,10)     | down      | 2     |
| 15   | (2,10)     | (3,10)     | down      | 2     |
| 16   | (3,10)     | (4,10)     | down      | 2     |
| 17   | (4,10)     | (4,11)     | right     | 1     |
| 18   | (4,11)     | (4,12)     | right     | 1     |
| 19   | (4,12)     | (4,13) X   | right     | 1     |

Concatenating the digits produces the **19-character move sequence**:

```
2221114411411222111
```

---

# Step 7 – Delivering the Solution

Since the program reads the answer from a **file** rather than directly from stdin, the final step was straightforward:

1. Create a text file — I named it `path.txt` — containing exactly:
   ```
   2221114411411222111
   ```
2. Run `Wonderland.exe`.
3. At the prompt *"You may enter, but can you find the Queen's..."*, type the **file name**:
   ```
   solution.txt
   ```

`ValidatePath` walks the maze using these 19 moves, lands exactly on `'X'`, and the program prints `"You have found the Queen's palace!\r\n"`.

---

# Final Solution

The validation routine does not compare against a fixed string — it **interprets the file content as a sequence of maze moves** (`1`=right, `2`=down, `3`=left, `4`=up) and walks a hard-coded 16×6 grid from `O` to `X`.

**File content (`solution.txt`):**
```
2221114411411222111
```

**Input to the program (the file name):**
```
solution.txt
```

The path is valid, the final tile is `X`, and the check passes without modifying a single byte of the executable.
<img width="997" height="302" alt="image" src="https://github.com/user-attachments/assets/cf9f1a0b-8c57-4aca-8645-360b3e40b72e" />
