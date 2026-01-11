# cpp-shell

A POSIX-inspired Unix-like shell implementation in C++17.
Built from scratch to understand process lifecycle, IPC, and terminal driver interaction.

This project prioritizes correctness, clarity, and architectural understanding over feature bloat.

## Core Implementation Details

### Process Management
* **Pipeline Execution**: Uses `pipe()` and `dup2()` to chain `stdin`/`stdout` across multiple processes.
* **Execution**: Manages process creation via `fork()` and `execvp()`.
* **Wait Logic**: Implements blocking `waitpid()` to synchronize parent/child process lifecycles.

### Terminal & Input
* **Raw Mode**: Directly manages `termios` attributes (disabling `ICANON` and `ECHO`) to handle byte-level input.
* **Custom Line Editing**: Implements backspace handling, arrow-key navigation (ANSI escape sequences `^[[A`, `^[[B`), and interactive history cycling.
* **Autocompletion**: Scans `$PATH` using `std::filesystem` to resolve executable matches on `TAB` press.

### I/O Redirection
* Supports standard redirection operators:
  * `>` (truncate stdout)
  * `>>` (append stdout)
  * `2>` / `2>>` (stderr redirection)

### Builtins
* `cd`: Modifies the shell’s working directory (parent process execution).
* `history`: Persistent command history with `HISTFILE` synchronization.
* `type`: Resolves builtins versus external commands via `$PATH`.

## Architecture

The shell is structured into distinct, modular components:
- **Shell**: Owns interactive state (history, terminal mode, prompt).
- **Parser**: Converts raw input strings into structured pipelines and command objects.
- **Executor**: Handles low-level process creation, pipe management, redirections, and waiting.
- **Utils**: Provides pipe-safe builtin implementations and redirection helpers.

This separation mirrors production shell design, ensuring clean isolation between shell state management and child process execution.

## Build

**Requirements:** `g++` (C++17) or compatible compiler, CMake 3.10+, POSIX-compatible system (Linux/macOS).

### Build with CMake
```bash
mkdir build
cd build
cmake ..
make
./shell
