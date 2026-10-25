# VineYard: Mount Opsys Vineyard Database 🍇

A command-line program in C for managing vineyard plots during an outbreak of *Golden Yellowness* (flavescence dorée), a grapevine disease. It stores records in a binary file. It also runs a "spring campaign" that uses **child processes, pipes and signals** to send out destruction or treatment orders.

## Features

- **CRUD on vineyard records.** Each record has a location, site, grape type, spread and damage (0–100%). Location + site + type together identify a record.
- **List** all records, or filter by location or by grape type
- **Input validation:** text fields can't be empty, spread must be positive, and damage must be between 0 and 100. Inputs are length-limited to prevent buffer overflows.
- **Persistent storage.** Records are saved as raw structs in `vineyards.dat` using low-level `open`/`read`/`write` system calls. The file is created automatically on the first save.
- **Dynamic array** that doubles its capacity with `realloc` as records are added
- **Spring campaign** (menu option 7), which uses process handling:
  - Plots are split by damage. **≥ 30%** means the plot must be destroyed, and **< 30%** means it gets chemical treatment.
  - The parent process `fork`s two **commissioner** child processes, one for destruction and one for defense.
  - The records are sent to each commissioner through its own **pipe**. Each commissioner prints an official letter for every plot it receives.
  - When a commissioner finishes, it signals the parent with **`SIGUSR1`**. The parent waits for both and then prints a summary report.

## Building and running

This uses POSIX system calls (`fork`, `pipe`, `signal`), so it runs on **Linux or macOS**. On Windows, use **WSL**.

```
gcc vineyard.c -o vineyard
./vineyard
```

## Example menu

```
Welcome to Mount Opsys Vineyard database!

1. Add record
2. Modify record
3. Delete record
4. List all
5. List by location
6. List by type
7. Start spring campaign
8. Exit
```

## About

Made for the Operating Systems course at ELTE (2025). It practises file handling with system calls, process creation, inter-process communication over pipes, and signal handling.
