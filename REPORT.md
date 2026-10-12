# Report: Programming Assignment 02

## Feature 2: Long Listing Format (`ls-v1.1.0`)

### Question 1
**What is the crucial difference between the `stat()` and `lstat()` system calls? In the context of the `ls` command, when is it more appropriate to use `lstat()`?**

**Answer:**
* **`stat()` System Call:** Dereferences symbolic links. If the file path supplied is a symbolic link, `stat()` follows the link to the target file and retrieves the metadata of that target file.
* **`lstat()` System Call:** Does **not** dereference symbolic links. If the file path supplied is a symbolic link, `lstat()` returns the metadata of the symbolic link itself (e.g., its link size, link permissions, and modification time).

**Context in `ls`:**
In implementing the `ls` utility, **`lstat()` is the appropriate choice**. Using `stat()` would cause symbolic links to report the attributes of their target files rather than the link files themselves. Additionally, if a symbolic link points to a nonexistent file (a broken link), `stat()` fails and returns an `ENOENT` error, whereas `lstat()` successfully reads the metadata of the link itself.

---

### Question 2
**The `st_mode` field in `struct stat` is an integer that contains both the file type (e.g., regular file, directory) and the permission bits. Explain how you can use bitwise operators (like `&`) and predefined macros (like `S_IFDIR` or `S_IRUSR`) to extract this information.**

**Answer:**
The `st_mode` field stores both file classification and permission flags combined into a single integer bitmask.

#### 1. Extracting File Type
To determine the file type, apply a bitwise AND operator (`&`) between `st_mode` and the bitmask macro `S_IFMT` to clear all permission bits and isolate the file-type bits. The result is then compared against constants like `S_IFDIR` or `S_IFREG`:

```c
if ((info.st_mode & S_IFMT) == S_IFDIR) {
    // File is a directory
}



## Feature 3: Column Display (`ls-v1.2.0`)

### Question 1
**Explain the general logic for printing items in a "down then across" columnar format. Why is a simple single loop through the list of filenames insufficient for this task?**

**Answer:**
A simple single loop iterates sequentially through an array ($0, 1, 2, \dots, N$). Because standard terminal output prints line-by-line horizontally, a single loop produces either a single vertical column or a left-to-right horizontal grid.

To print "down then across", the total entries are split into calculated grid dimensions: $rows$ and $cols$. The output loop executes row by row ( outer loop $r$ from $0$ to $rows-1$) and column by column (inner loop $c$ from $0$ to $cols-1$). The file corresponding to position $(r, c)$ is located at array index:
$$\text{index} = r + (c \times \text{rows})$$
This mapping forces items in the same column to increment vertically before moving to the next column.

---

### Question 2
**What is the purpose of the `ioctl` system call in this context? What would be the limitations of your program if you only used a fixed-width fallback (e.g., 80 columns) instead of detecting the terminal size?**

**Answer:**
* **Purpose of `ioctl`:** The `ioctl(STDOUT_FILENO, TIOCGWINSZ, &w)` system call queries the terminal driver to retrieve the active window size (`ws_col` and `ws_row`) dynamically.
* **Limitations of a Fixed-Width Fallback:**
  1. **Wasted Space on Large Displays:** On wide modern terminal windows (e.g., 140+ columns), an 80-column fixed layout leaves significant unused horizontal space and forces unnecessary vertical scrolling.
  2. **Line Wrapping on Small Displays:** On narrow terminal windows (e.g., split-screen pane of 60 columns), an 80-column layout exceeds display limits, causing lines to wrap awkwardly and ruin vertical alignment.


  ## Feature 4: Horizontal Column Display (`ls-v1.3.0`)

### Question 1
**Compare the algorithmic complexity and implementation difficulty of the default column display (down then across) vs. the `-x` horizontal column display (across then down). Why is the horizontal layout simpler to implement?**

**Answer:**
* **Algorithmic Complexity:** Both implementations operate in $O(N)$ time complexity, where $N$ is the number of directory entries.
* **Implementation Simplicity of Horizontal Layout (`-x`):**
  The horizontal layout prints entries in natural array order ($0, 1, 2, \dots, N-1$). Since terminal output Streams line-by-line horizontally, printing item $i$ followed by item $i+1$ directly matches standard terminal streaming. The program only needs a single loop with a modulus check `(i + 1) % cols == 0` to print a newline `\n`.
* **Complexity of Default Vertical Layout:**
  The vertical layout requires mapping line-by-line output to vertical columns using formula calculations $\text{index} = r + (c \times \text{rows})$ across nested row and column loops. It also requires edge-case handling for incomplete final columns.

---

### Question 2
**How does flag precedence work when conflicting flags (such as `-l` and `-x`) are passed together? How is this handled in your implementation?**

**Answer:**
In standard GNU `ls`, when conflicting options are provided, the last specified option on the command line typically takes precedence, or flags combine depending on functional scope.

In this implementation:
1. `-l` overrides column formatting (`-x` or default) because long listing requires one detailed item per line.
2. If `-l` is present (`l_flag == 1`), `do_ls` executes `print_long_format`.
3. If `-l` is not present, `do_ls` checks `x_flag`: calling `print_horizontal_format` if `-x` is set, or falling back to `print_column_format` (default vertical layout) otherwise.


## Feature 5: Alphabetical Sort (`ls-v1.4.0`)

### Question 1
**Why is it necessary to read all directory entries into memory before you can sort them? What are the potential drawbacks of this approach for directories containing millions of files?**

**Answer:**
* **Necessity:** File systems store directory structures in underlying disk blocks or tree indices in non-alphabetical or creation-based order. `readdir()` yields entries sequentially as stored on disk. Sorting requires global knowledge of the complete list of elements, so all items must be loaded into memory before applying a sorting algorithm like `qsort()`.
* **Drawbacks for Large Directories:**
  1. **High Memory Overhead:** Storing millions of string paths consumes significant heap memory ($O(N)$ memory complexity), leading to potential RAM exhaustion or out-of-memory crashes.
  2. **Latency & Execution Delays:** Reading and sorting millions of files in memory causes noticeable UI blocking before any output can be rendered to the user.

---

### Question 2
**Explain the purpose and signature of the comparison function required by `qsort()`. How does it work, and why must it take `const void *` arguments?**

**Answer:**
* **Function Signature:** `int compare_filenames(const void *a, const void *b)`
* **Purpose:** `qsort()` is a generic sorting algorithm designed to sort arrays of any data type. Because `qsort()` operates on raw byte pointers, it relies on a user-provided comparison function to evaluate whether element $A$ is smaller than, equal to, or greater than element $B$.
* **How It Works:** Inside the comparator function, the arguments are cast from generic generic pointers (`const void *`) to string pointers (`const char **`). Deriving the underlying strings allows `strcmp()` to perform lexicographical comparison:
  * Returns `< 0` if string $A$ precedes string $B$.
  * Returns `0` if string $A$ equals string $B$.
  * Returns `> 0` if string $A$ follows string $B$.
* **Why `const void *` arguments:** Generic C interface design requires `void *` so `qsort()` works on arbitrary structures. The `const` modifier ensures `qsort()` does not modify the element memory contents while evaluating comparisons.