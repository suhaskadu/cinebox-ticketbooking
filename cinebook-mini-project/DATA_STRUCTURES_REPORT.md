# 🎓 CineBook – College Mini Project Report & Viva Guide
## Subject: Data Structures and Algorithms (Mini Project)
### Project Title: CineBook – Online Movie Ticket Booking & Seat Locking System (C++ Native Engine)

---

## 1. Project Overview & Problem Statement
In traditional manual ticket counter setups or basic web systems, movie seat reservations face critical algorithmic challenges:
1. **Concurrency Conflicts & Double-Booking**: Two customers selecting the same seat simultaneously.
2. **Abandoned Checkouts**: A customer holding a seat indefinitely without completing payment.
3. **Slow Lookup Times**: Inefficient searching across hundreds of seats during high-traffic blockbuster releases.

**CineBook** resolves these problems by implementing **5 Classical Data Structures in Native C++ (C++14)** paired with a responsive modern cinema dashboard and persistent SQLite storage:
- A **2D Matrix Grid (`seats[5][6]`)** to map physical theater rows and columns with $O(1)$ spatial coordinate indexing.
- A **Hash Map with Bucket Chaining** for $O(1)$ constant-time status checks and seat lookup by code (`"C4"`).
- A **Min-Heap Priority Queue** that automatically releases abandoned temporary locks after 300 seconds (5 minutes) in $O(\log N)$ time.
- A **FIFO Queue** to serialize booking transaction requests preventing race conditions.
- A **Doubly Linked List** to maintain an audit trail and enable instant cancellation rollbacks.

---

## 2. Theoretical & Practical Data Structure Analysis (C++ Architecture)

### A. 2D Array / Matrix (`SeatMatrix`)
- **Dimensions**: 5 Rows ('A' through 'E') $\times$ 6 Columns (1 through 6) = 30 total cinema seats.
- **C++ Memory Layout**:
  ```cpp
  class SeatMatrix {
      static const int NUM_ROWS = 5;
      static const int NUM_COLS = 6;
      Seat grid[NUM_ROWS][NUM_COLS]; // Contiguous 2D spatial memory
  };
  ```
- **Spatial Mapping Formula**:
  $$\text{Row Index} = \text{row\_char} - \text{'A'} \quad (0 \le \text{row} < 5)$$
  $$\text{Column Index} = \text{col\_number} - 1 \quad (0 \le \text{col} < 6)$$
- **Time Complexity**:
  - Direct Seat Access: $O(1)$
  - Full Matrix Scan (computing availability): $O(M \times N)$
- **College Viva Answer**:
  > *"We used a 2D Array because a cinema hall is fundamentally a 2-dimensional spatial grid. Direct indexing `grid[row_idx][col_idx]` allows instant seat retrieval in $O(1)$ without searching."*

---

### B. Hash Map with Bucket Chaining (`SeatHashMap`)
- **Key**: String seat code (e.g., `"C4"`, `"A1"`, `"E6"`).
- **Value**: `Seat` struct (code, row, col, status, price, locked_until, locked_by, booking_id).
- **Collision Resolution**: Bucket chaining with singly linked list nodes (`HashNode*`).
- **Hash Function**: DJB2 string hashing algorithm:
  ```cpp
  size_t hash_function(const std::string& key) const {
      unsigned long hash = 5381;
      for (char c : key) {
          hash = ((hash << 5) + hash) + static_cast<unsigned char>(c); // hash * 33 + c
      }
      return hash % BUCKET_COUNT;
  }
  ```
- **Time Complexity**:
  - Average Lookup, Insert, and Update: $O(1)$
  - Worst Case (high collisions): $O(K)$ where $K$ is chain length.
- **College Viva Answer**:
  > *"When the customer clicks seat 'C4', we do not need to iterate through all 30 seats. The Hash Map computes `hash(\"C4\")` and retrieves the seat state in $O(1)$ average time."*

---

### C. Min-Heap Priority Queue (`LockExpiryPriorityQueue`)
- **Structure**: Binary Min-Heap implemented over `std::vector<LockEntry>` storing `(expiry_timestamp, seat_code, session_id)`.
- **Purpose**: Implements the **5-Minute Temporary Lock**.
  - When seat `C4` is locked at $T = 100$, its expiry timestamp is set to $T_{expire} = 100 + 300 = 400$.
  - The root of the Min-Heap always stores the seat with the **earliest** expiration timestamp.
- **Heap Invariant & Navigation**:
  - Parent index: $P(i) = \lfloor (i - 1) / 2 \rfloor$
  - Left child: $L(i) = 2i + 1$, Right child: $R(i) = 2i + 2$
  - $\text{heap}[P(i)].\text{expiry\_time} \le \text{heap}[i].\text{expiry\_time}$
- **Time Complexity**:
  - Push Lock: $O(\log K)$ where $K$ is the number of active locks.
  - Peek Next Expiry: $O(1)$ (Root inspection).
  - Pop Expired Locks: $O(\log K)$
- **College Viva Answer**:
  > *"Instead of continuously scanning all seats to check whose 5 minutes expired, we use a Min-Heap. The seat whose lock is closest to expiring is at the root ($O(1)$ peek). We only evict seats whose timestamp has passed."*

---

### D. FIFO Queue (`BookingFIFOQueue`)
- **Structure**: Linked-list First-In-First-Out Queue with `front_ptr` and `rear_ptr`.
- **Purpose**: Serializes transaction requests to prevent race conditions during high-demand booking surges.
- **Time Complexity**:
  - Enqueue (push to rear): $O(1)$
  - Dequeue (pop from front): $O(1)$
- **College Viva Answer**:
  > *"When multiple users click 'Confirm Booking' at the exact same second, the requests enter a FIFO Queue. They are processed sequentially in First-Come, First-Served order to avoid race conditions and double-allocation."*

---

### E. Doubly Linked List (`BookingHistoryDoublyLinkedList`)
- **Nodes**: Struct `TransactionNode` containing `TransactionData`, `TransactionNode* prev`, and `TransactionNode* next`.
- **Pointers**: Bidirectional pointers allow full forward and reverse traversal.
- **Purpose**: Supports atomic rollback. When a booking cancellation occurs (Screen 4), the system traverses the list from `tail`, identifies the transaction, and reverts the seat status in the 2D matrix back to `AVAILABLE`.
- **Time Complexity**:
  - Append to tail: $O(1)$
  - Reverse traversal (most recent logs first): $O(N)$
- **College Viva Answer**:
  > *"A Doubly Linked List allows bidirectional traversal. We can inspect recent transactions from the tail in $O(1)$ and walk backward to audit previous states or perform rollbacks."*

---

## 3. C++ Class Hierarchy & Files

```
cinebook-mini-project/backend/cpp/
├── data_structures.hpp     # Definitions of Seat, SeatMatrix, SeatHashMap, LockExpiryPriorityQueue, BookingFIFOQueue, DoublyLinkedList
├── data_structures.cpp     # Full C++ implementations with zero external dependencies
├── main.cpp                # 3 Modes: Interactive CLI Viva Demo, Automated Test Suite, and JSON Bridge
├── test_ds.cpp             # Dedicated C++ unit test runner
├── compile.bat             # Windows 1-click compiler script (g++)
└── Makefile                # Linux / macOS / MinGW makefile
```

---

## 4. Database Schema (SQLite)

### Table 1: `movie`
| Column | Type | Description |
|---|---|---|
| `id` | INTEGER PRIMARY KEY | Movie ID (1 for Avengers: Endgame) |
| `title` | TEXT | Movie Title |
| `rating` | TEXT | IMDb & Age Rating |
| `duration` | TEXT | Duration (3h 01m) |
| `genre` | TEXT | Action / Sci-Fi / Adventure |
| `synopsis` | TEXT | Story Summary |

### Table 2: `seats`
| Column | Type | Description |
|---|---|---|
| `id` | INTEGER PRIMARY KEY | Auto-increment seat ID |
| `seat_code` | TEXT UNIQUE | e.g., 'A1', 'C4', 'E6' |
| `row_label` | TEXT | 'A', 'B', 'C', 'D', 'E' |
| `col_number` | INTEGER | 1 to 6 |
| `price` | REAL | Ticket Price (₹180 - ₹350) |
| `status` | TEXT | `'AVAILABLE'`, `'LOCKED'`, `'BOOKED'` |
| `locked_until` | REAL | Unix timestamp for 5-minute expiry |
| `locked_by` | TEXT | Session identifier |
| `booking_id` | TEXT | Linked Booking reference |

### Table 3: `bookings`
| Column | Type | Description |
|---|---|---|
| `booking_id` | TEXT PRIMARY KEY | Unique ID (e.g. `CB-84920-C4`) |
| `movie_title` | TEXT | Avengers: Endgame |
| `seat_code` | TEXT | Seat reserved |
| `show_date` | TEXT | Show Date |
| `showtime` | TEXT | Showtime |
| `ticket_price` | REAL | Base ticket cost |
| `tax_amount` | REAL | 18% GST / Cess |
| `total_price` | REAL | Final price |
| `status` | TEXT | `'CONFIRMED'` or `'CANCELLED'` |
| `created_at` | TEXT | Timestamp |
| `cancelled_at` | TEXT | Cancellation Timestamp |

---

## 5. Top Viva Questions & Answers

1. **Q: How does C++ integrate with the web dashboard?**
   - **A**: *The C++ engine is compiled into `cinebook_ds.exe`. When the web server launches, it spawns the C++ binary with `--bridge` mode. All data structure operations (matrix cell lookups, hash table insertions, priority queue eviction, FIFO queuing, and linked list audit trail) execute natively in C++ memory via high-speed JSON-IPC.*

2. **Q: Why does the system lock the seat for 5 minutes instead of booking it immediately?**
   - **A**: *When a customer starts checking out, they need time to review ticket details and submit payment. If we don't lock the seat, someone else could buy it midway. If we lock it indefinitely, an abandoned browser tab would keep the seat unavailable forever. A 5-minute temporary lock strikes the perfect balance.*

3. **Q: What happens when the 5-minute timer hits zero?**
   - **A**: *The Min-Heap Priority Queue in C++ detects that `heap[0].expiry_time <= current_time`. The root node is popped in $O(\log N)$ time, the seat status in the 2D matrix automatically reverts from `LOCKED` back to `AVAILABLE` (green), and the available seat count increases.*

4. **Q: How does the cancellation screen work?**
   - **A**: *When the user clicks 'Cancel Booking', the backend marks the booking as `CANCELLED` in the SQLite database and executes a rollback on the 2D Matrix and Hash Map. The seat changes from red (`BOOKED`) to green (`AVAILABLE`), and the available seat count increments.*

5. **Q: How do you demonstrate the project during viva?**
   - **A**: *We can demonstrate it in two ways:
     1. **Web Dashboard**: Double click `start.bat` to show the full interactive cinema dashboard with real-time 5-minute seat locking, booking pass, and cancellation rollback.
     2. **C++ Console Demonstration**: Double click `run_cpp_demo.bat` to launch the interactive terminal demo showing ASCII cinema grid rendering, O(1) hash map lookups, min-heap eviction, FIFO queues, and unit tests.*
