/**
 * ============================================================================
 * CineBook - Standalone C++ Data Structures Unit Tests
 * ============================================================================
 */

#include "data_structures.hpp"
#include <iostream>
#include <cassert>

using namespace cinebook;

int main() {
    std::cout << "========================================================\n";
    std::cout << " CINEBOOK C++ DATA STRUCTURES UNIT TESTS\n";
    std::cout << "========================================================\n";

    // 1. Matrix
    std::cout << "1. Testing SeatMatrix...\n";
    SeatMatrix m;
    assert(m.get_available_count() == 30);
    Seat* c4 = m.get_cell('C', 4);
    assert(c4 != nullptr && c4->code == "C4" && c4->price == 250.0);
    m.update_status("C4", "LOCKED", 5000.0, "sess_1");
    assert(m.get_available_count(100.0) == 29);
    std::cout << "   [PASS] 2D Array Matrix verified.\n";

    // 2. Hash Map
    std::cout << "2. Testing SeatHashMap...\n";
    SeatHashMap map;
    map.put("C4", *c4);
    assert(map.contains("C4"));
    assert(map.get("C4")->code == "C4");
    map.remove("C4");
    assert(!map.contains("C4"));
    std::cout << "   [PASS] Hash Map with bucket chaining verified.\n";

    // 3. Priority Queue
    std::cout << "3. Testing LockExpiryPriorityQueue...\n";
    LockExpiryPriorityQueue pq;
    pq.add_lock("C4", 1000.0, "sess_1");
    pq.add_lock("A1", 800.0, "sess_2");
    LockEntry top;
    assert(pq.peek_next_expiry(top));
    assert(top.seat_code == "A1");
    auto exp = pq.pop_expired(900.0);
    assert(exp.size() == 1 && exp[0].first == "A1");
    std::cout << "   [PASS] Min-Heap Priority Queue verified.\n";

    // 4. FIFO Queue
    std::cout << "4. Testing BookingFIFOQueue...\n";
    BookingFIFOQueue q;
    q.enqueue(BookingRequest("CB-1", "C4", "u1", 10.0));
    q.enqueue(BookingRequest("CB-2", "C5", "u2", 11.0));
    BookingRequest req;
    assert(q.dequeue(req) && req.booking_id == "CB-1");
    assert(q.dequeue(req) && req.booking_id == "CB-2");
    assert(q.is_empty());
    std::cout << "   [PASS] FIFO Queue verified.\n";

    // 5. Doubly Linked List
    std::cout << "5. Testing BookingHistoryDoublyLinkedList...\n";
    BookingHistoryDoublyLinkedList dll;
    TransactionData t1; t1.event = "LOCK"; t1.seat_code = "C4";
    TransactionData t2; t2.event = "CONFIRM"; t2.seat_code = "C4";
    dll.append(t1);
    dll.append(t2);
    auto logs = dll.to_list_reverse();
    assert(logs.size() == 2 && logs[0].event == "CONFIRM");
    std::cout << "   [PASS] Doubly Linked List verified.\n";

    std::cout << "\n>>> ALL 5 C++ DATA STRUCTURES TESTS PASSED (100%) <<<\n";
    return 0;
}
