Q1: What is the crucial difference between the stat() and lstat() system calls? When is it more appropriate to use lstat()?
Ans:
stat() follows symbolic links and gives info about the target file, while lstat() gives info about the link itself.
In the ls command, lstat() is used when we need to detect symbolic links instead of the file they point to.

Q2: The st_mode field in struct stat contains both file type and permissions. How can you use bitwise operators and macros to extract this information?
Ans:
We use bitwise AND (&) with macros to check specific bits.
For example:
if (st.st_mode & S_IFDIR)   // check if directory  
if (st.st_mode & S_IRUSR)   // check if user has read permission
This extracts only the required bits from st_mode.

Q1: Explain the logic for printing items in a “down then across” columnar format. Why is a simple single loop insufficient?
Ans:
In “down then across,” items fill vertically first, then move to the next column.
A single loop prints left-to-right only, so we need to calculate rows and columns, then print items using index jumps (e.g., i + rows * j).

Q2: What is the purpose of the ioctl system call in this context? What are the limitations of using a fixed width?
Ans:
ioctl() is used to get terminal width dynamically so columns fit the screen.
If we use a fixed width (like 80 columns), output might look broken or misaligned on terminals with different sizes.

Q1: Compare the complexity of vertical vs horizontal (across) display logic. Which needs more pre-calculation and why?
Ans:
The vertical (down-then-across) format needs more pre-calculation because it must compute the number of rows and columns and index mapping.
Horizontal printing just loops left-to-right, making it simpler.

Q2: How did your program manage the display modes (-l, -x, and default)?
Ans:
I used flags from getopt() to detect which mode was selected.
Based on the flag, the program calls the correct function — long_listing(), horizontal_display(), or column_display().

Q1: Compare the complexity of vertical vs horizontal (across) display logic. Which needs more pre-calculation and why?
Ans:
The vertical (down-then-across) format needs more pre-calculation because it must compute the number of rows and columns and index mapping.
Horizontal printing just loops left-to-right, making it simpler.

Q2: How did your program manage the display modes (-l, -x, and default)?
Ans:
I used flags from getopt() to detect which mode was selected.
Based on the flag, the program calls the correct function — long_listing(), horizontal_display(), or column_display().

Q1: Why must all directory entries be read into memory before sorting? What are drawbacks for huge directories?
Ans:
Sorting needs access to all entries at once, so we store them in an array.
For directories with millions of files, it increases memory usage and can slow down the system.

Q2: Explain the comparison function for qsort(). Why must it use const void * arguments?
Ans:
qsort() expects a function like:
int compare(const void *a, const void *b)
We cast the void* to the correct type and compare values.
const void * is used for generic sorting of any data type safely.

Q1: How do ANSI escape codes produce color in a Linux terminal? Show the code for printing text in green.
Ans:
ANSI escape codes tell the terminal to change text color.
For green text:
printf("\033[0;32mHello\033[0m");
033[ starts the escape sequence, 0;32m sets green, and 0m resets the color.

Q2: How to detect if a file is executable using st_mode bits?
Ans:
Check the execute bits in st_mode:
if (st.st_mode & S_IXUSR) // user execute
if (st.st_mode & S_IXGRP) // group execute
if (st.st_mode & S_IXOTH) // others execute
If any is set, the file is executable.

Q1: What is a “base case” in recursion? What is the base case in recursive ls?
Ans:
A base case stops recursion from running forever.
In recursive ls, the base case occurs when there are no subdirectories left (or the entry is . or ..).

Q2: Why must you construct a full path before recursive call? What happens if you call with only the subdir name?
Ans:
The full path (parent/subdir) ensures correct navigation into nested directories.
If we only pass "subdir", recursion will fail because it looks for the folder in the wrong directory context.

