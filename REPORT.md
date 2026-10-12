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