#ifndef CINEBOOK_DATA_STRUCTURES_HPP
#define CINEBOOK_DATA_STRUCTURES_HPP

/**
 * ============================================================================
 * CineBook - C++ Data Structures Engine for College Mini Project
 * ============================================================================
 * Implements 5 Classical Data Structures in C++:
 * 1. 2D Array / Matrix (Cinema Seating Grid: 5 Rows x 6 Cols = 30 Seats)
 * 2. Hash Map with Bucket Chaining (O(1) Instant Seat Lookup by Code)
 * 3. Min-Heap Priority Queue (5-Minute Temporary Seat Lock Expiration)
 * 4. FIFO Queue (Sequential First-Come First-Served Booking Transactions)
 * 5. Doubly Linked List (Chronological Audit Trail & Cancellation Rollback)
 * ============================================================================
 */

#include <string>
#include <vector>
#include <utility>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <ctime>

namespace cinebook {

// ============================================================================
// DATA MODELS
// ============================================================================

struct Seat {
    std::string code;         // e.g. "C4"
    char row;                 // 'A' to 'E'
    int col;                  // 1 to 6
    std::string status;       // "AVAILABLE", "LOCKED", "BOOKED"
    double locked_until;      // Unix timestamp (0.0 if not locked)
    std::string locked_by;    // session identifier
    double price;             // Tier pricing: E=350, C/D=250, A/B=180
    std::string booking_id;   // Associated booking reference

    Seat()
        : code(""), row('A'), col(1), status("AVAILABLE"),
          locked_until(0.0), locked_by(""), price(180.0), booking_id("") {}

    Seat(const std::string& c, char r, int cl, double p)
        : code(c), row(r), col(cl), status("AVAILABLE"),
          locked_until(0.0), locked_by(""), price(p), booking_id("") {}

    std::string to_json() const;
};

struct LockEntry {
    double expiry_time;
    std::string seat_code;
    std::string session_id;

    LockEntry() : expiry_time(0.0), seat_code(""), session_id("") {}
    LockEntry(double exp, const std::string& code, const std::string& sess)
        : expiry_time(exp), seat_code(code), session_id(sess) {}

    std::string to_json() const;
};

struct BookingRequest {
    std::string booking_id;
    std::string seat_code;
    std::string session_id;
    double timestamp;

    BookingRequest() : booking_id(""), seat_code(""), session_id(""), timestamp(0.0) {}
    BookingRequest(const std::string& id, const std::string& code, const std::string& sess, double t)
        : booking_id(id), seat_code(code), session_id(sess), timestamp(t) {}

    std::string to_json() const;
};

struct TransactionData {
    std::string event;          // "SEAT_TEMPORARILY_LOCKED", "BOOKING_CONFIRMED", "BOOKING_CANCELLED_ROLLBACK"
    std::string booking_id;
    std::string seat_code;
    std::string session_id;
    std::string timestamp;
    double amount;
    std::string duration;
    std::string reverted_to;

    TransactionData()
        : event(""), booking_id(""), seat_code(""), session_id(""),
          timestamp(""), amount(0.0), duration(""), reverted_to("") {}

    std::string to_json() const;
};


// ============================================================================
// 1. 2D ARRAY / MATRIX (Cinema Seating Grid: 5 Rows x 6 Columns)
// ============================================================================
/**
 * Complexity:
 * - Direct Element Access: O(1) via seats[row_idx][col_idx]
 * - Full Matrix Scan: O(R x C) = O(30)
 * 
 * Row Index Mapping:
 *   'A' -> 0, 'B' -> 1, 'C' -> 2, 'D' -> 3, 'E' -> 4 (row_char - 'A')
 * Col Index Mapping:
 *   1 -> 0, 2 -> 1, ..., 6 -> 5 (col_num - 1)
 */
class SeatMatrix {
public:
    static const int NUM_ROWS = 5;
    static const int NUM_COLS = 6;
    static const char ROW_LETTERS[NUM_ROWS];

private:
    Seat grid[NUM_ROWS][NUM_COLS];

public:
    SeatMatrix();

    void initialize_grid();

    // O(1) Index-based cell access
    Seat* get_cell(char row_char, int col_num);
    const Seat* get_cell(char row_char, int col_num) const;

    // Update seat state
    bool update_status(const std::string& seat_code, const std::string& status,
                       double locked_until = 0.0, const std::string& locked_by = "",
                       const std::string& booking_id = "");

    // Flattened list of all 30 seats
    std::vector<Seat> get_all_seats() const;

    // Get number of currently available seats (cleans expired locks)
    int get_available_count(double current_time = 0.0);

    // ASCII representation for terminal display
    void print_grid() const;
};


// ============================================================================
// 2. HASH MAP (Key-Value O(1) Seat Lookup with Bucket Chaining)
// ============================================================================
/**
 * Complexity:
 * - Average Lookup / Insert / Delete: O(1)
 * - Worst Case (collisions): O(K) where K is bucket length
 * - Collision Resolution: Bucket Chaining with Singly Linked List
 */
class SeatHashMap {
private:
    struct HashNode {
        std::string key;
        Seat value;
        HashNode* next;

        HashNode(const std::string& k, const Seat& v)
            : key(k), value(v), next(nullptr) {}
    };

    static const size_t BUCKET_COUNT = 31; // Prime number for optimal hash distribution
    HashNode* table[BUCKET_COUNT];
    size_t element_count;

    // DJB2 Hash Algorithm
    size_t hash_function(const std::string& key) const;

public:
    SeatHashMap();
    ~SeatHashMap();

    // Prevent copy to avoid double frees
    SeatHashMap(const SeatHashMap&) = delete;
    SeatHashMap& operator=(const SeatHashMap&) = delete;

    void put(const std::string& key, const Seat& seat);
    Seat* get(const std::string& key);
    const Seat* get(const std::string& key) const;
    bool contains(const std::string& key) const;
    bool remove(const std::string& key);
    void clear();

    size_t size() const { return element_count; }
    std::vector<std::string> keys() const;
};


// ============================================================================
// 3. MIN-HEAP PRIORITY QUEUE (5-Minute Temporary Seat Lock Expiration)
// ============================================================================
/**
 * Complexity:
 * - Insert (Push): O(log N)
 * - Peek Earliest Expiry: O(1)
 * - Extract (Pop): O(log N)
 * 
 * Invariant:
 *   heap[parent].expiry_time <= heap[child].expiry_time
 *   Parent(i) = (i - 1) / 2
 *   LeftChild(i) = 2*i + 1, RightChild(i) = 2*i + 2
 */
class LockExpiryPriorityQueue {
private:
    std::vector<LockEntry> heap;

    void heapify_up(size_t index);
    void heapify_down(size_t index);

public:
    LockExpiryPriorityQueue();

    void add_lock(const std::string& seat_code, double expiry_timestamp, const std::string& session_id);
    std::vector<std::pair<std::string, std::string>> pop_expired(double current_time);
    bool peek_next_expiry(LockEntry& out) const;

    size_t size() const { return heap.size(); }
    bool is_empty() const { return heap.empty(); }
    void clear() { heap.clear(); }

    std::vector<LockEntry> get_all_entries() const { return heap; }
};


// ============================================================================
// 4. FIFO QUEUE (Sequential First-Come First-Served Booking Transactions)
// ============================================================================
/**
 * Complexity:
 * - Enqueue (Tail Push): O(1)
 * - Dequeue (Head Pop): O(1)
 * - Is Empty / Size: O(1)
 * 
 * Prevents race conditions during simultaneous booking requests.
 */
class BookingFIFOQueue {
private:
    struct QueueNode {
        BookingRequest data;
        QueueNode* next;

        QueueNode(const BookingRequest& req) : data(req), next(nullptr) {}
    };

    QueueNode* front_ptr;
    QueueNode* rear_ptr;
    size_t count;

public:
    BookingFIFOQueue();
    ~BookingFIFOQueue();

    BookingFIFOQueue(const BookingFIFOQueue&) = delete;
    BookingFIFOQueue& operator=(const BookingFIFOQueue&) = delete;

    void enqueue(const BookingRequest& request);
    bool dequeue(BookingRequest& out);
    bool is_empty() const { return count == 0; }
    size_t size() const { return count; }
    void clear();
};


// ============================================================================
// 5. DOUBLY LINKED LIST (Chronological Audit Trail & Cancellation Rollback)
// ============================================================================
/**
 * Complexity:
 * - Append to Tail: O(1)
 * - Bidirectional Traversal: O(N)
 * - Rollback Inspection: O(1) from tail
 * 
 * Nodes maintain `prev` and `next` pointers to trace full event history.
 */
struct TransactionNode {
    TransactionData data;
    TransactionNode* prev;
    TransactionNode* next;

    TransactionNode(const TransactionData& d)
        : data(d), prev(nullptr), next(nullptr) {}
};

class BookingHistoryDoublyLinkedList {
private:
    TransactionNode* head;
    TransactionNode* tail;
    size_t count;

public:
    BookingHistoryDoublyLinkedList();
    ~BookingHistoryDoublyLinkedList();

    BookingHistoryDoublyLinkedList(const BookingHistoryDoublyLinkedList&) = delete;
    BookingHistoryDoublyLinkedList& operator=(const BookingHistoryDoublyLinkedList&) = delete;

    TransactionNode* append(const TransactionData& transaction_data);
    std::vector<TransactionData> to_list_reverse() const; // Most recent first
    std::vector<TransactionData> to_list_forward() const; // Oldest first
    
    size_t size() const { return count; }
    void clear();
};

} // namespace cinebook

#endif // CINEBOOK_DATA_STRUCTURES_HPP
