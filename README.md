# POSIX Shell Implementation in C++

A custom Unix shell written from scratch in C++, exploring operating system internals. 
This project implements core shell functionality including process management, I/O redirection, and signal handling.

## Features
* **Command Execution:** Supports running external programs and custom built-ins.
* **Built-in Commands:** `cd`, `pwd`, `echo`, `type`, `history`.
* **I/O Redirection:** Handles standard output (`>`) and append (`>>`) redirection.
* **Pipelines:** Supports chaining commands using pipes (`|`).
* **Signal Handling:** Custom implementation of `SIGINT` (Ctrl+C) and `SIGTSTP`.
* **History Management:** Persistent history storage with append-only logic.

## Technical Details
* **Language:** C++ (Standard Library + POSIX System Calls)
* **Key Concepts:** `fork()`, `exec()`, `pipe()`, `dup2()`, Raw Mode terminal manipulation.
