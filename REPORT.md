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
