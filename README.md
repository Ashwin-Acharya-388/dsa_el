# 📍 Entertainment Venue Finder (K-D Tree Implementation)

A full-stack project demonstrating specific DSA concepts: using a **K-D Tree** (2D Tree) for efficient Nearest Neighbor Search.

![Project Status](https://img.shields.io/badge/Status-Active-success)
![Backend](https://img.shields.io/badge/Backend-C-blue)
![Frontend](https://img.shields.io/badge/Frontend-HTML%2FJS-orange)

## 🚀 Overview

This project implements a **K-Dimensional Tree** in C to perform spatial hashing and fast nearest neighbor lookups. It includes a visualization frontend where users can click on a map, and the C backend efficiently calculates the closest entertainment venue (Theater, Mall, Cinema, etc.).

**Key Features:**
*   **O(log n)** Search Complexity using K-D Trees.
*   **C Backend** handling core data structures and logic.
*   **Custom HTTP Server** written in C to serve API requests.
*   **Interactive Frontend** with HTML5 Canvas for visualization.
*   **Comparison Mode**: Benchmark K-D Tree performance vs Brute Force (O(n)).

## 🛠️ Tech Stack

*   **Backend:** C (Sockets, Pthreads, Structs)
*   **Frontend:** HTML5, CSS3, Vanilla JavaScript
*   **Data Structure:** K-D Tree (2-Dimensional)
*   **Build Tool:** Make

## ⚙️ Installation & Run

### Prerequisites
*   GCC Compiler
*   Make

### Steps
1.  **Clone the repository**
    ```bash
    git clone https://github.com/Ashwin-Acharya-388/dsa_el.git
    cd dsa_el
    ```

2.  **Compile the Backend**
    ```bash
    make server
    ```

3.  **Start the Server**
    ```bash
    ./server
    ```
    *Output should confirm: `C Backend Server running on port 8080`*

4.  **Launch the App**
    Open `index.html` in your web browser.

## 🔌 API Endpoints

The C server exposes the following JSON endpoints:

| Method | Endpoint | Description |
| :--- | :--- | :--- |
| `GET` | `/api/venues` | Returns all venue data points. |
| `GET` | `/api/nearest?x=100&y=200` | Returns closest venue using the selected algorithm. |

## 📂 Project Structure

```
├── backend.c       # Core Logic: K-D Tree implementation searching
├── server.c        # HTTP Server: Handles requests and routing
├── kdtree.h        # Headers: Shared structs and prototypes
├── index.html      # Frontend: Visualization and UI
└── makefile        # Build: Compilation scripts
```

## 🧠 Efficiency

Why use a K-D Tree? 
*   **Brute Force:** Checks every single point. **O(n)**.
*   **K-D Tree:** Prunes search space by eliminating half the tree at each step. **Average: O(log n)**.

---
*Created by Ashwin Acharya*