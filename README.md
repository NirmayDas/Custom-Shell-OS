# My Custom Shell

A lightweight command-line shell built in C to showcase systems programming and operating system concepts. This project demonstrates core OS skills including process management, signals, job control, and command execution.

## Features

* **Command Execution**

  * Runs external programs by searching the system `$PATH`.
  * Supports foreground and background execution.

* **Redirection**

  * Input (`<`), output (`>`), and error (`2>`) redirection.
  * Creates new files when needed and handles permission bits.
  * Supports combining multiple types of redirection in a single command.

* **Pipes**

  * One level of piping (`|`) to connect two commands.
  * Manages process groups for pipeline execution.

* **Signals**

  * Handles common signals (`SIGINT`, `SIGTSTP`, `SIGCHLD`).
  * `Ctrl-C` interrupts only the running process, not the shell.
  * `Ctrl-Z` suspends the current foreground process.

* **Job Control**

  * Run commands in the background using `&`.
  * Built-in commands:

    * `jobs` – List active and stopped jobs.
    * `fg` – Resume the most recent job in the foreground.
    * `bg` – Resume the most recent job in the background.
  * Tracks up to 20 concurrent jobs.

* **Robust Behavior**

  * Ignores invalid commands gracefully.
  * Ensures child processes are terminated on shell exit.
  * Stable prompt and error handling.

## Technical Highlights

* Implemented in **C (GNU99/C11)** using only OS-level and standard GNU C library calls.
* Demonstrates key OS concepts: process creation, I/O redirection, pipes, signals, and process groups.
* Reinforces safe memory management and parsing strategies.

## Build & Run

```bash
# Clone and build
git clone https://github.com/<your-username>/<repo-name>.git
cd <repo-name>
make

# Run the shell
./shell
```

## Example Usage

```bash
# Run a program
ls -l

# Redirect output
ls > files.txt

# Pipe commands
cat input.txt | grep keyword

# Background job
sleep 10 &

# Manage jobs
jobs
fg
bg
```

## Purpose

This project was created to strengthen and showcase low-level programming skills in process management, job control, and system calls. It serves as a foundation for deeper operating system and systems programming work.

---

