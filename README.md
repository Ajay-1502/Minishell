# 🐚 MiniShell (Linux Shell in C)

A simple Unix-like **MiniShell implemented in C** that replicates core functionalities of a Linux shell. This project demonstrates key operating system concepts such as **process creation, command execution, piping, signal handling, and job control**.

---

## 🚀 Features

- ✅ Custom shell prompt (`PS1` support)
- ✅ Execution of **external commands** (`ls`, `cat`, `ps`, etc.)
- ✅ Support for **built-in commands**
  - `cd`, `pwd`, `exit`
  - `jobs`, `fg`, `bg`
- ✅ **Pipe (`|`) support**
  - Example: `ls | grep txt`
- ✅ **Signal handling**
  - `Ctrl + C` → terminate process
  - `Ctrl + Z` → stop process (job control)


---

## 🧠 Concepts Used

This project is built using core **Linux system calls**:

- `fork()` → process creation  
- `execvp()` → command execution  
- `wait()` / `waitpid()` → process synchronization  
- `pipe()` → inter-process communication  
- `dup2()` → file descriptor redirection  
- `signal()` → signal handling  

---

