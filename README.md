# cinebox-ticketbooking
# 🎬 CineBook – Movie Ticket Booking System
### College Data Structures Mini Project (C++ Native Engine + Web Dashboard)

A full-stack cinema ticket booking system and interactive desktop dashboard powered by **5 Core Data Structures implemented in C++ (C++14)** for college Data Structures mini project demonstration.

---

## 🌟 Highlights & Design Features

- **C++ Native Data Structures Engine**: All 5 core data structures (`SeatMatrix`, `SeatHashMap`, `LockExpiryPriorityQueue`, `BookingFIFOQueue`, `BookingHistoryDoublyLinkedList`) are written in clean, modern C++ with zero external dependencies.
- **Cinema Visual Theme**: Dark navy and black background (`#050811`), glowing violet/purple (`#9333ea`) and crimson red (`#e50914`) neon accents.
- **Desktop Website Layout**: Tailored for high-resolution desktop project presentations and college projector screens.
- **4 Complete Screens**:
  1. **Screen 1: Home Page**:
     - CineBook glowing logo & project branding.
     - Custom cinematic poster for **Avengers: Endgame**.
     - Rating (8.4/10 IMDb), duration (3h 01m), and genre badges.
     - Show date selector (Today, Tomorrow, Tue, Wed) and showtimes (10:30 AM, 02:15 PM, 06:00 PM, 09:45 PM).
     - Glowing "Book Tickets" call to action.
  2. **Screen 2: Seat Selection Page**:
     - Curved cinema screen at the top with light projection illumination.
     - 5 rows labelled **A, B, C, D, and E**.
     - 6 numbered seats per row (**30 seats in total**).
     - Color-coded seat states:
       - 🟢 **Green**: Available seats
       - 🔴 **Red**: Booked seats
       - 🟡 **Yellow**: Currently selected seat
       - 🟣 **Violet**: **Temporary Blocked / Locked seat** (5-minute hold)
     - **5-Minute Temporary Seat Lock**: When a user selects a seat, it locks for 300 seconds (`05:00` countdown timer) backed by a C++ Min-Heap Priority Queue so nobody else can take it while booking.
     - Real-time available seat counter (`Available: 27 / 30`).
     - "Confirm Booking" and "Cancel Booking" controls.
  3. **Screen 3: Booking Confirmation Page**:
     - Animated checkmark & booking confirmation notice.
     - Digital cinema ticket pass with barcode, reference code, movie name, showtime, and selected row/seat (e.g. `Row C • Seat 4 [C4]`).
     - Itemized price breakdown with 18% Cinema GST & total amount.
     - "Cancel Booking" button and "Back to Home" button.
  4. **Screen 4: Booking Cancelled Page**:
     - Cancellation confirmation notice.
     - **Live Rollback Visualizer**: Demonstrates the booked seat transitioning back to Green (Available).
     - Dynamically incremented available seat count.
     - "Book Again" button.
- **College Viva Modal**: In-app architectural breakdown with time complexities and algorithmic explanations for college presentations.

---

## 📊 Data Structures Implemented in C++

| Data Structure | C++ Class | Purpose & Time Complexity |
|---|---|---|
| **2D Array / Matrix** | `SeatMatrix` | Represents the 5 rows $\times$ 6 columns cinema theater (`grid[5][6]`). Fast $O(1)$ spatial coordinate access. |
| **Hash Map with Chaining** | `SeatHashMap` | Custom bucket-chaining hash table mapping seat codes (`"C4"`) $\to$ seat metadata. $O(1)$ average time. |
| **Min-Heap Priority Queue** | `LockExpiryPriorityQueue` | Binary min-heap storing `(expiry_time, seat_code)` to evict expired 5-minute temporary locks in $O(\log N)$ time. |
| **FIFO Queue** | `BookingFIFOQueue` | First-In-First-Out request pipeline preventing race conditions during simultaneous booking transactions in $O(1)$ time. |
| **Doubly Linked List** | `BookingHistoryDoublyLinkedList` | Nodes with `prev` and `next` pointers maintaining chronological audit trails and instant cancellation rollbacks. |

---

## 🚀 How to Run the Project

### Option A: 1-Click Launch Web Dashboard (Windows)
Double-click `start.bat` in this folder. It auto-compiles the C++ engine (if needed), starts the server, and opens your browser at:
```
http://localhost:8000
```

### Option B: 1-Click Interactive C++ Console & Viva Demo
Double-click `run_cpp_demo.bat` in this folder. It launches an interactive terminal console showing:
- Visual ASCII cinema matrix
- O(1) hash map lookups
- 5-minute lock scheduling & eviction
- FIFO transaction queuing
- Doubly linked list audit log & rollback
- Full C++ automated test suite

### Option C: Compile & Run C++ Directly with GCC/MinGW
```bash
cd backend/cpp
g++ -std=c++14 -O2 data_structures.cpp main.cpp -o cinebook_ds.exe
./cinebook_ds.exe --demo
```
To run the automated C++ test suite:
```bash
./cinebook_ds.exe --test
```

### Option D: Run with Python (Full Stack)
```bash
python backend/server.py
```
Open [http://localhost:8000](http://localhost:8000) in your web browser.

### Option E: Run with Node.js
```bash
node backend/server.js
```
Open [http://localhost:8000](http://localhost:8000) in your web browser.

---

## 📂 Project Directory Structure

```
cinebook-mini-project/
├── backend/
│   ├── cpp/                           # Core C++ Data Structures Engine
│   │   ├── data_structures.hpp        # C++ Header (Matrix, HashMap, Min-Heap, FIFO Queue, Doubly Linked List)
│   │   ├── data_structures.cpp        # Full C++ Implementation (Zero external dependencies)
│   │   ├── main.cpp                   # 3 Modes: Interactive CLI Viva Demo, Test Suite, JSON Bridge
│   │   ├── test_ds.cpp                # Dedicated C++ unit test runner
│   │   ├── compile.bat                # 1-Click C++ compilation script
│   │   ├── Makefile                   # Linux / macOS / MinGW makefile
│   │   ├── cinebook_ds.exe            # Compiled native C++ binary
│   │   └── test_ds.exe                # Compiled C++ test runner
│   ├── data_structures.py             # Python native bridge connecting to C++ engine
│   ├── database.py                    # SQLite database schema, seeding & queries
│   ├── server.py                      # Python HTTP & REST API server
│   ├── server.js                      # Node.js REST API server alternative
│   ├── test_api.py                    # Integration test suite for REST API
│   └── test_ds_and_db.py              # Data structures & database test suite
├── database/
│   └── cinebook.db                    # SQLite database storing movies, seats & bookings
├── frontend/
│   ├── index.html                     # Presentation file containing all 4 screens & screen switcher
│   ├── style.css                      # Dark cinema navy/black styling with purple & red neon
│   ├── app.js                         # UI controller, 5-minute timer, REST API client, DS modal
│   └── assets/
│       ├── logo.svg                   # CineBook brand logo with filmstrip emblem
│       └── avengers_poster.svg        # Theatrical poster for Avengers: Endgame
├── run_cpp_demo.bat                   # 1-Click interactive C++ Terminal Demo for College Viva
├── start.bat                          # 1-Click Windows launcher for Web Dashboard
├── DATA_STRUCTURES_REPORT.md          # College mini project report & viva Q&A
└── README.md                          # Project documentation
```
