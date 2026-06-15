# Wonderland.exe – Write Up

# The Queen's Maze – Static and Dynamic Analysis

## Objective

The goal of this level is to bypass the validation routine **without patching the binary**. Unlike a simple string or numeric comparison, this level's check is not a static comparison at all — it is a small **interpreter** that walks a hard-coded maze step by step according to the characters supplied by the user. The input is accepted only if every step lands on a valid floor tile and the final step lands exactly on the target tile.

---

# Step 1 – Function Identification and Renaming

In the initial static analysis, I located the function that prints the level's flavor text:

```asm
.text:00401659 push offset aYouMayEnterBut ; "You may enter, but can you find the Que"...
.text:0040165E call Printf
```

This is clearly the entry point of the challenge, so I renamed this function `MazeChallenge`.

A second function, `sub_401770`, is called later with a single buffer argument, and its return value (`0` or `1`) decides between `"You are lost!\r\n"` and `"You have found the Queen's palace!\r\n"`. I renamed it `ValidatePath`.

---

# Step 2 – Understanding the Input Flow

The first surprise of this level is that the input pipeline goes through the **filesystem**, not directly through `stdin`:

```asm
lea  ecx, [ebp+Buffer]
push ecx          ; Buffer
push 3FFh         ; MaxCount
push eax          ; Stream (stdin)
call fgets
...
call CreateFileA  ; lpFileName = Buffer
...
call ReadFile     ; reads up to 0x400 bytes into var_808
...
push var_808
call sub_401770   ; ValidatePath(var_808)
```

Putting this together:

```c
fgets(filename, 0x3FF, stdin);
filename[strlen(filename) - 1] = '\0';   // strip newline

hFile = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ,
                     NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
ReadFile(hFile, fileContent, 0x400, &bytesRead, NULL);

result = ValidatePath(fileContent);
```

In other words:

- The text typed at the prompt is **a file name**, not the answer itself.
- The program opens that file and reads up to **1024 bytes** of its content.
- The **file's content** is what gets passed to `ValidatePath`.

So the real "key" has to be written into a file first, and the program is simply told where to find it.

---

# Step 3 – Reverse Engineering `ValidatePath`

```asm
push 4Fh                  ; 'O'
mov  eax, Str
push eax
call strchr                ; pos = strchr(Str, 'O')
...
mov  var_C, 0               ; i = 0

loc_401790:
movsx edx, [arg_0 + var_C]   ; c = moves[i]
sub   edx, 31h               ; c - '1'
cmp   edx, 3
ja    default                ; if (unsigned)(c-'1') > 3 -> return 0

; switch (c - '1')
case 0 ('1'): pos += 1     ; jumptable case 49 -> right
case 1 ('2'): pos += 0x10  ; jumptable case 50 -> down
case 2 ('3'): pos -= 1     ; jumptable case 51 -> left
case 3 ('4'): pos -= 0x10  ; jumptable case 52 -> up

i++
if (moves[i] == 0) break     ; end of input
if (*pos != '.') break        ; hit a wall / non-floor tile

; otherwise loop again
...
return (*pos == 'X');
```

Two details give away the exact structure of the maze:

- The stride `0x10` (16) used for "up"/"down" means **the maze is 16 columns wide**.
- The loop keeps going only while `*pos == '.'`, and success requires the **final** position to be `'X'` — so walls are simply anything that isn't `'.'` or `'X'`.

---

# Step 4 – Reconstructing in C

```c
int ValidatePath(char *moves)
{
    char *pos = strchr(Str, 'O');
    int i = 0;

    for (;;)
    {
        int dir = moves[i] - '1';
        if ((unsigned)dir > 3)
            return 0;                  // not '1'..'4' -> invalid

        switch (dir)
        {
            case 0: pos += 1;    break; // '1' -> right
            case 1: pos += 0x10; break; // '2' -> down
            case 2: pos -= 1;    break; // '3' -> left
            case 3: pos -= 0x10; break; // '4' -> up
        }

        i++;
        if (moves[i] == '\0')
            break;
        if (*pos != '.')
            break;
    }

    return (*pos == 'X');
}
```

---

# Step 5 – Extracting the Maze From `.data`

`Str` points to `aO`, a 97-byte buffer (96 bytes of map data + a null terminator):

```asm
.data:00404020 aO db '###################O####...########.##...#.########.##.###.######'
.data:00404061    db '##....###...X##################',0
```

`96 ÷ 16 = 6` — exactly **6 rows of 16 columns**, matching the `0x10` row-stride found in Step 3.

Splitting the 96 raw bytes into 16-character rows reconstructs the full map:

```
################
###O####...#####
###.##...#.#####
###.##.###.#####
###....###...X##
################
```

- `O` → row 1, column 3 (entrance)
- `X` → row 4, column 13 (Queen's palace)
- `#` → wall
- `.` → walkable tile

---

# Step 6 – Solving the Maze

Starting at `O` and stepping only on `.` tiles (with the final step landing on `X`), there is exactly **one** path through the maze:

```
################
###↓####→.↓#####
###↓##→→↑#↓#####
###↓##↑###↓#####
###→→→↑###→→→X##
################
```

Reading the arrows in order and converting each direction back to its digit (`right=1`, `down=2`, `left=3`, `up=4`):

| Step | From      | To        | Direction | Digit |
|------|-----------|-----------|-----------|-------|
| 1    | (1,3) O   | (2,3)     | down      | 2     |
| 2    | (2,3)     | (3,3)     | down      | 2     |
| 3    | (3,3)     | (4,3)     | down      | 2     |
| 4    | (4,3)     | (4,4)     | right     | 1     |
| 5    | (4,4)     | (4,5)     | right     | 1     |
| 6    | (4,5)     | (4,6)     | right     | 1     |
| 7    | (4,6)     | (3,6)     | up        | 4     |
| 8    | (3,6)     | (2,6)     | up        | 4     |
| 9    | (2,6)     | (2,7)     | right     | 1     |
| 10   | (2,7)     | (2,8)     | right     | 1     |
| 11   | (2,8)     | (1,8)     | up        | 4     |
| 12   | (1,8)     | (1,9)     | right     | 1     |
| 13   | (1,9)     | (1,10)    | right     | 1     |
| 14   | (1,10)    | (2,10)    | down      | 2     |
| 15   | (2,10)    | (3,10)    | down      | 2     |
| 16   | (3,10)    | (4,10)    | down      | 2     |
| 17   | (4,10)    | (4,11)    | right     | 1     |
| 18   | (4,11)    | (4,12)    | right     | 1     |
| 19   | (4,12)    | (4,13) X  | right     | 1     |

Concatenating the digits gives the **19-character move sequence**:

```
2221114411411222111
```

---

# Step 7 – Delivering the Solution

Since the program reads the answer from a **file**, not directly from `stdin`:

1. Create a text file (e.g. `path.txt`) containing exactly:
   ```
   2221114411411222111
   ```
2. Run `Wonderland.exe`.
3. At the prompt *"You may enter, but can you find the Queen's..."*, type the **file name**:
   ```
   path.txt
   ```

`ValidatePath` walks the maze using these 19 moves, lands exactly on `X`, and the program prints `"You have found the Queen's palace!\r\n"`.

---

# Final Solution

The validation routine does not compare against a fixed string — it **interprets the input as a sequence of maze moves** (`1`=right, `2`=down, `3`=left, `4`=up) and walks a hard-coded 16×6 grid from `O` to `X`.

**File content (`path.txt`):**
```
2221114411411222111
```

**Input to the program (the file name):**
```
path.txt
```

The path is valid, the final tile is `X`, and the check succeeds without modifying a single byte of the executable.
