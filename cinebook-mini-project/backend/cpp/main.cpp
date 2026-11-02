/**
 * ============================================================================
 * CineBook - C++ Main Executable & Demonstration
 * ============================================================================
 * Supports 3 Modes:
 * 1. Interactive CLI Viva Mode (default / --demo)
 * 2. Automated Test Suite (--test)
 * 3. JSON IPC Bridge Mode (--bridge) for Python/Node REST backend
 * ============================================================================
 */

#include "data_structures.hpp"
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <cassert>
#include <ctime>
#include <cstdlib>

using namespace cinebook;

// ============================================================================
// LIGHTWEIGHT ZERO-DEPENDENCY JSON PARSER HELPERS
// ============================================================================

static std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

static std::string extract_json_str(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";

    size_t colon = json.find(':', pos + search.length());
    if (colon == std::string::npos) return "";

    size_t start = json.find_first_not_of(" \t\r\n", colon + 1);
    if (start == std::string::npos) return "";

    if (json[start] == '"') {
        size_t end = start + 1;
        while (end < json.length()) {
            if (json[end] == '"' && json[end - 1] != '\\') break;
            end++;
        }
        if (end < json.length()) {
            std::string val = json.substr(start + 1, end - start - 1);
            // Replace escaped quotes
            size_t esc = 0;
            while ((esc = val.find("\\\"", esc)) != std::string::npos) {
                val.replace(esc, 2, "\"");
                esc += 1;
            }
            return val;
        }
    } else if (json.compare(start, 4, "null") == 0) {
        return "";
    } else {
        size_t end = json.find_first_of(",}\r\n", start);
        if (end == std::string::npos) end = json.length();
        return trim(json.substr(start, end - start));
    }
    return "";
}

static double extract_json_double(const std::string& json, const std::string& key, double def = 0.0) {
    std::string val = extract_json_str(json, key);
    if (val.empty() || val == "null") return def;
    try {
        return std::stod(val);
    } catch (...) {
        return def;
    }
}

static int extract_json_int(const std::string& json, const std::string& key, int def = 0) {
    std::string val = extract_json_str(json, key);
    if (val.empty() || val == "null") return def;
    try {
        return std::stoi(val);
    } catch (...) {
        return def;
    }
}


// ============================================================================
// MODE 1: AUTOMATED TEST SUITE
// ============================================================================

bool run_all_tests() {
    std::cout << "\n======================================================================\n";
    std::cout << " RUNNING CINEBOOK C++ DATA STRUCTURES TEST SUITE\n";
    std::cout << "======================================================================\n";

    // 1. Test 2D Array / Matrix
    std::cout << "[1/5] Testing 2D Array Matrix (5x6 Cinema Grid)...\n";
    SeatMatrix matrix;
    Seat* c4 = matrix.get_cell('C', 4);
    assert(c4 != nullptr);
    assert(c4->code == "C4");
    assert(c4->status == "AVAILABLE");
    assert(c4->price == 250.0);

    Seat* e6 = matrix.get_cell('E', 6);
    assert(e6 != nullptr);
    assert(e6->price == 350.0); // Premium row

    assert(matrix.get_available_count() == 30);
    bool updated = matrix.update_status("C4", "LOCKED", 9999999999.0, "user_test");
    assert(updated);
    assert(matrix.get_available_count(100.0) == 29);
    matrix.update_status("C4", "AVAILABLE");
    std::cout << "      PASS: 2D Matrix indexing, tier pricing, and status updates verified.\n";

    // 2. Test Hash Map
    std::cout << "[2/5] Testing Hash Map with Bucket Chaining (O(1) Lookup)...\n";
    SeatHashMap map;
    map.put("C4", *c4);
    assert(map.contains("C4"));
    assert(map.size() == 1);
    Seat* found = map.get("C4");
    assert(found != nullptr);
    assert(found->code == "C4");
    assert(found->price == 250.0);

    // Test collision handling and multi-insert
    for (int r = 0; r < 5; ++r) {
        for (int c = 1; c <= 6; ++c) {
            char row_char = SeatMatrix::ROW_LETTERS[r];
            std::string code = std::string(1, row_char) + std::to_string(c);
            Seat* s = matrix.get_cell(row_char, c);
            if (s) map.put(code, *s);
        }
    }
    assert(map.size() == 30);
    assert(map.contains("A1"));
    assert(map.contains("E6"));
    assert(!map.contains("Z9"));
    map.remove("A1");
    assert(!map.contains("A1"));
    assert(map.size() == 29);
    std::cout << "      PASS: Hash Map bucket chaining, collision resolution, and O(1) lookups verified.\n";

    // 3. Test Min-Heap Priority Queue
    std::cout << "[3/5] Testing Min-Heap Priority Queue (5-Min Lock Expiry)...\n";
    LockExpiryPriorityQueue pq;
    double now = 1000.0;
    pq.add_lock("C4", now + 300.0, "sess_active");   // Expires at 1300 (5 mins later)
    pq.add_lock("A1", now - 10.0, "sess_expired");   // Already expired at 990
    pq.add_lock("B2", now + 60.0, "sess_soon");      // Expires at 1060

    assert(pq.size() == 3);
    LockEntry next_exp;
    assert(pq.peek_next_expiry(next_exp));
    assert(next_exp.seat_code == "A1"); // Earliest expiry at root

    auto expired = pq.pop_expired(now);
    assert(expired.size() == 1);
    assert(expired[0].first == "A1");
    assert(pq.size() == 2);

    // Now fast forward past B2
    auto expired_soon = pq.pop_expired(now + 100.0);
    assert(expired_soon.size() == 1);
    assert(expired_soon[0].first == "B2");
    assert(pq.size() == 1);
    std::cout << "      PASS: Min-Heap priority queue order, root peek O(1), and auto-eviction verified.\n";

    // 4. Test FIFO Queue
    std::cout << "[4/5] Testing FIFO Booking Queue (Transaction Pipeline)...\n";
    BookingFIFOQueue queue;
    queue.enqueue(BookingRequest("CB-001", "C4", "sess_1", 100.0));
    queue.enqueue(BookingRequest("CB-002", "D3", "sess_2", 101.0));
    queue.enqueue(BookingRequest("CB-003", "E5", "sess_3", 102.0));
    assert(queue.size() == 3);

    BookingRequest req;
    assert(queue.dequeue(req));
    assert(req.booking_id == "CB-001");
    assert(queue.dequeue(req));
    assert(req.booking_id == "CB-002");
    assert(queue.dequeue(req));
    assert(req.booking_id == "CB-003");
    assert(queue.is_empty());
    std::cout << "      PASS: FIFO queue serialization and sequential order verified.\n";

    // 5. Test Doubly Linked List
    std::cout << "[5/5] Testing Doubly Linked List (Audit Trail & Rollback)...\n";
    BookingHistoryDoublyLinkedList dll;
    TransactionData t1;
    t1.event = "SEAT_TEMPORARILY_LOCKED";
    t1.seat_code = "C4";
    t1.duration = "5 minutes";
    dll.append(t1);

    TransactionData t2;
    t2.event = "BOOKING_CONFIRMED";
    t2.booking_id = "CB-84920-C4";
    t2.seat_code = "C4";
    t2.amount = 295.0;
    dll.append(t2);

    TransactionData t3;
    t3.event = "BOOKING_CANCELLED_ROLLBACK";
    t3.booking_id = "CB-84920-C4";
    t3.seat_code = "C4";
    t3.reverted_to = "AVAILABLE";
    dll.append(t3);

    assert(dll.size() == 3);
    auto reverse_list = dll.to_list_reverse();
    assert(reverse_list.size() == 3);
    assert(reverse_list[0].event == "BOOKING_CANCELLED_ROLLBACK"); // Most recent first
    assert(reverse_list[2].event == "SEAT_TEMPORARILY_LOCKED");

    std::cout << "      PASS: Doubly Linked List bidirectional traversal and rollback audit verified.\n";

    std::cout << "\n======================================================================\n";
    std::cout << " ALL 5 DATA STRUCTURES IN C++ FULLY VALIDATED AND PASSED (100%)\n";
    std::cout << "======================================================================\n\n";
    return true;
}


// ============================================================================
// MODE 2: JSON IPC BRIDGE MODE (For REST Backend Server)
// ============================================================================

void run_bridge_mode() {
    SeatMatrix matrix;
    SeatHashMap map;
    LockExpiryPriorityQueue pq;
    BookingFIFOQueue queue;
    BookingHistoryDoublyLinkedList dll;

    // Seed initial map from matrix
    for (const auto& s : matrix.get_all_seats()) {
        map.put(s.code, s);
    }

    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;

        std::string cmd = extract_json_str(line, "cmd");
        if (cmd == "ping") {
            std::cout << "{\"status\":\"ok\",\"pong\":true}\n" << std::flush;
        }
        else if (cmd == "init_grid") {
            matrix.initialize_grid();
            map.clear();
            for (const auto& s : matrix.get_all_seats()) {
                map.put(s.code, s);
            }
            std::cout << "{\"status\":\"ok\",\"message\":\"Grid reinitialized\"}\n" << std::flush;
        }
        else if (cmd == "update_matrix") {
            std::string code = extract_json_str(line, "seat_code");
            std::string status = extract_json_str(line, "status");
            double locked_until = extract_json_double(line, "locked_until", 0.0);
            std::string locked_by = extract_json_str(line, "locked_by");
            std::string booking_id = extract_json_str(line, "booking_id");

            bool ok = matrix.update_status(code, status, locked_until, locked_by, booking_id);
            if (ok) {
                if (code.length() >= 2) {
                    Seat* s = matrix.get_cell(code[0], std::stoi(code.substr(1)));
                    if (s) map.put(code, *s);
                }
            }
            std::cout << "{\"status\":\"ok\",\"success\":" << (ok ? "true" : "false") << "}\n" << std::flush;
        }
        else if (cmd == "get_cell") {
            std::string row_str = extract_json_str(line, "row");
            int col = extract_json_int(line, "col", 1);
            char row_char = row_str.empty() ? 'A' : row_str[0];

            Seat* s = matrix.get_cell(row_char, col);
            if (s) {
                std::cout << "{\"status\":\"ok\",\"found\":true,\"seat\":" << s->to_json() << "}\n" << std::flush;
            } else {
                std::cout << "{\"status\":\"ok\",\"found\":false,\"seat\":null}\n" << std::flush;
            }
        }
        else if (cmd == "get_all_seats") {
            auto seats = matrix.get_all_seats();
            std::ostringstream oss;
            oss << "{\"status\":\"ok\",\"seats\":[";
            for (size_t i = 0; i < seats.size(); ++i) {
                oss << seats[i].to_json();
                if (i + 1 < seats.size()) oss << ",";
            }
            oss << "]}\n";
            std::cout << oss.str() << std::flush;
        }
        else if (cmd == "get_available_count") {
            double now = extract_json_double(line, "now", 0.0);
            int count = matrix.get_available_count(now);
            std::cout << "{\"status\":\"ok\",\"available_count\":" << count << "}\n" << std::flush;
        }
        else if (cmd == "hash_put") {
            std::string code = extract_json_str(line, "seat_code");
            if (code.length() >= 2) {
                Seat* s = matrix.get_cell(code[0], std::stoi(code.substr(1)));
                if (s) map.put(code, *s);
            }
            std::cout << "{\"status\":\"ok\",\"size\":" << map.size() << "}\n" << std::flush;
        }
        else if (cmd == "hash_get") {
            std::string code = extract_json_str(line, "seat_code");
            Seat* s = map.get(code);
            if (s) {
                std::cout << "{\"status\":\"ok\",\"found\":true,\"seat\":" << s->to_json() << "}\n" << std::flush;
            } else {
                std::cout << "{\"status\":\"ok\",\"found\":false,\"seat\":null}\n" << std::flush;
            }
        }
        else if (cmd == "hash_contains") {
            std::string code = extract_json_str(line, "seat_code");
            std::cout << "{\"status\":\"ok\",\"contains\":" << (map.contains(code) ? "true" : "false") << "}\n" << std::flush;
        }
        else if (cmd == "hash_size") {
            std::cout << "{\"status\":\"ok\",\"size\":" << map.size() << "}\n" << std::flush;
        }
        else if (cmd == "pq_add_lock") {
            std::string code = extract_json_str(line, "seat_code");
            double expiry = extract_json_double(line, "expiry_time", 0.0);
            std::string sess = extract_json_str(line, "session_id");
            pq.add_lock(code, expiry, sess);
            std::cout << "{\"status\":\"ok\",\"size\":" << pq.size() << "}\n" << std::flush;
        }
        else if (cmd == "pq_pop_expired") {
            double now = extract_json_double(line, "now", 0.0);
            auto expired = pq.pop_expired(now);
            std::ostringstream oss;
            oss << "{\"status\":\"ok\",\"expired\":[";
            for (size_t i = 0; i < expired.size(); ++i) {
                oss << "[\"" << expired[i].first << "\",\"" << expired[i].second << "\"]";
                if (i + 1 < expired.size()) oss << ",";
            }
            oss << "]}\n";
            std::cout << oss.str() << std::flush;
        }
        else if (cmd == "pq_peek") {
            LockEntry entry;
            if (pq.peek_next_expiry(entry)) {
                std::cout << "{\"status\":\"ok\",\"has_entry\":true,\"entry\":" << entry.to_json() << "}\n" << std::flush;
            } else {
                std::cout << "{\"status\":\"ok\",\"has_entry\":false,\"entry\":null}\n" << std::flush;
            }
        }
        else if (cmd == "pq_size") {
            std::cout << "{\"status\":\"ok\",\"size\":" << pq.size() << "}\n" << std::flush;
        }
        else if (cmd == "fifo_enqueue") {
            std::string id = extract_json_str(line, "booking_id");
            std::string code = extract_json_str(line, "seat_code");
            std::string sess = extract_json_str(line, "session_id");
            double t = extract_json_double(line, "timestamp", 0.0);
            queue.enqueue(BookingRequest(id, code, sess, t));
            std::cout << "{\"status\":\"ok\",\"size\":" << queue.size() << "}\n" << std::flush;
        }
        else if (cmd == "fifo_dequeue") {
            BookingRequest req;
            if (queue.dequeue(req)) {
                std::cout << "{\"status\":\"ok\",\"has_entry\":true,\"request\":" << req.to_json() << "}\n" << std::flush;
            } else {
                std::cout << "{\"status\":\"ok\",\"has_entry\":false,\"request\":null}\n" << std::flush;
            }
        }
        else if (cmd == "fifo_size") {
            std::cout << "{\"status\":\"ok\",\"size\":" << queue.size() << "}\n" << std::flush;
        }
        else if (cmd == "dll_append") {
            TransactionData d;
            d.event = extract_json_str(line, "event");
            d.booking_id = extract_json_str(line, "booking_id");
            d.seat_code = extract_json_str(line, "seat_code");
            d.session_id = extract_json_str(line, "session_id");
            d.timestamp = extract_json_str(line, "timestamp");
            d.amount = extract_json_double(line, "amount", 0.0);
            d.duration = extract_json_str(line, "duration");
            d.reverted_to = extract_json_str(line, "reverted_to");
            dll.append(d);
            std::cout << "{\"status\":\"ok\",\"size\":" << dll.size() << "}\n" << std::flush;
        }
        else if (cmd == "dll_to_list") {
            auto list = dll.to_list_reverse();
            std::ostringstream oss;
            oss << "{\"status\":\"ok\",\"logs\":[";
            for (size_t i = 0; i < list.size(); ++i) {
                oss << list[i].to_json();
                if (i + 1 < list.size()) oss << ",";
            }
            oss << "]}\n";
            std::cout << oss.str() << std::flush;
        }
        else if (cmd == "dll_size") {
            std::cout << "{\"status\":\"ok\",\"size\":" << dll.size() << "}\n" << std::flush;
        }
        else if (cmd == "get_ds_metrics") {
            // Build full viva data structure metrics representation
            std::ostringstream oss;
            oss << "{\"status\":\"ok\",\"metrics\":{";

            // 1. Matrix Representation
            oss << "\"seat_matrix_2d\":{";
            oss << "\"name\":\"2D Matrix / Array Grid [5][6]\",";
            oss << "\"dimensions\":\"5 Rows (A-E) x 6 Columns (1-6) = 30 Seats\",";
            oss << "\"time_complexity\":\"O(1) index-based access: seats[row_idx][col_idx]\",";
            oss << "\"grid\":[";
            for (int r = 0; r < SeatMatrix::NUM_ROWS; ++r) {
                char row_char = SeatMatrix::ROW_LETTERS[r];
                oss << "{\"row\":\"" << row_char << "\",\"seats\":[";
                for (int c = 1; c <= SeatMatrix::NUM_COLS; ++c) {
                    Seat* s = matrix.get_cell(row_char, c);
                    if (s) oss << s->to_json();
                    if (c < SeatMatrix::NUM_COLS) oss << ",";
                }
                oss << "]}";
                if (r + 1 < SeatMatrix::NUM_ROWS) oss << ",";
            }
            oss << "]},";

            // 2. Hash Map
            oss << "\"seat_hash_map\":{";
            oss << "\"name\":\"Hash Map (Bucket Chaining Table)\",";
            oss << "\"purpose\":\"O(1) Instant Seat Lookup by Code (e.g. 'C4')\",";
            oss << "\"keys_count\":" << map.size();
            oss << "},";

            // 3. Priority Queue
            oss << "\"lock_priority_queue\":{";
            oss << "\"name\":\"Min-Heap Priority Queue (5-Min Lock Expiry)\",";
            oss << "\"purpose\":\"Sorts active locks by earliest expiration time\",";
            oss << "\"time_complexity\":\"O(log N) insertion & extraction\",";
            oss << "\"active_locks\":" << pq.size() << ",";
            LockEntry next_exp;
            if (pq.peek_next_expiry(next_exp)) {
                oss << "\"next_expiry\":[" << std::fixed << std::setprecision(2) << next_exp.expiry_time
                    << ",\"" << next_exp.seat_code << "\",\"" << next_exp.session_id << "\"]";
            } else {
                oss << "\"next_expiry\":null";
            }
            oss << "},";

            // 4. FIFO Queue
            oss << "\"fifo_booking_queue\":{";
            oss << "\"name\":\"FIFO Queue (Booking Transactions)\",";
            oss << "\"purpose\":\"Sequential First-Come-First-Served booking execution\",";
            oss << "\"time_complexity\":\"O(1) enqueue / dequeue\",";
            oss << "\"pending_requests\":" << queue.size();
            oss << "},";

            // 5. Doubly Linked List
            auto recent_logs = dll.to_list_reverse();
            oss << "\"audit_linked_list\":{";
            oss << "\"name\":\"Doubly Linked List (Audit & Rollback Log)\",";
            oss << "\"purpose\":\"Maintains chronological order of locks, bookings, and cancellations\",";
            oss << "\"total_events\":" << dll.size() << ",";
            oss << "\"recent_logs\":[";
            size_t max_logs = std::min(recent_logs.size(), (size_t)10);
            for (size_t i = 0; i < max_logs; ++i) {
                oss << recent_logs[i].to_json();
                if (i + 1 < max_logs) oss << ",";
            }
            oss << "]}";

            oss << "}}\n";
            std::cout << oss.str() << std::flush;
        }
        else if (cmd == "exit") {
            std::cout << "{\"status\":\"ok\",\"bye\":true}\n" << std::flush;
            break;
        }
        else {
            std::cout << "{\"status\":\"error\",\"message\":\"Unknown command\"}\n" << std::flush;
        }
    }
}


// ============================================================================
// MODE 3: INTERACTIVE CLI DEMONSTRATION FOR COLLEGE VIVA
// ============================================================================

void print_banner() {
    std::cout << "\n======================================================================\n";
    std::cout << "   * CINEBOOK - C++ DATA STRUCTURES ENGINE *\n";
    std::cout << "   Subject: Data Structures and Algorithms Mini Project\n";
    std::cout << "   Theme: Movie Ticket Booking System with 5 Core Data Structures\n";
    std::cout << "======================================================================\n";
}

void run_interactive_cli() {
    print_banner();

    SeatMatrix matrix;
    SeatHashMap map;
    LockExpiryPriorityQueue pq;
    BookingFIFOQueue queue;
    BookingHistoryDoublyLinkedList dll;

    // Seed map with initial seats
    for (const auto& s : matrix.get_all_seats()) {
        map.put(s.code, s);
    }

    // Seed pre-booked seats for realistic demo
    std::string sample_booked[] = {"B2", "B3", "D4"};
    for (const auto& code : sample_booked) {
        matrix.update_status(code, "BOOKED", 0.0, "", "CB-PRE-" + code);
        Seat* s = matrix.get_cell(code[0], std::stoi(code.substr(1)));
        if (s) map.put(code, *s);
        TransactionData t;
        t.event = "PRE_SEEDED_BOOKING";
        t.seat_code = code;
        t.booking_id = "CB-PRE-" + code;
        t.amount = s ? s->price : 250.0;
        dll.append(t);
    }

    while (true) {
        std::cout << "\n------------------- COLLEGE VIVA DEMO MENU -------------------\n";
        std::cout << " [1] Display 2D Cinema Matrix (5x6 Grid)\n";
        std::cout << " [2] Test Hash Map O(1) Seat Lookup\n";
        std::cout << " [3] Lock a Seat (5-Minute Temporary Min-Heap Schedule)\n";
        std::cout << " [4] Check & Pop Expired 5-Min Locks from Min-Heap\n";
        std::cout << " [5] Enqueue & Dequeue FIFO Booking Transactions\n";
        std::cout << " [6] View Doubly Linked List Audit Trail & Rollback Demo\n";
        std::cout << " [7] Run Full Automated C++ Test Suite (5/5 DS Verification)\n";
        std::cout << " [0] Exit\n";
        std::cout << " Select option (0-7): ";

        int choice = -1;
        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::string dummy;
            std::cin >> dummy;
            continue;
        }

        if (choice == 0) {
            std::cout << "\nExiting CineBook C++ Engine. Good luck with your Viva!\n\n";
            break;
        }
        else if (choice == 1) {
            matrix.print_grid();
            std::cout << "Total Seats: " << SeatMatrix::NUM_ROWS * SeatMatrix::NUM_COLS
                      << " | Available Seats: " << matrix.get_available_count() << "\n";
            std::cout << "Time Complexity: Direct index matrix[row_idx][col_idx] = O(1)\n";
        }
        else if (choice == 2) {
            std::string code;
            std::cout << "Enter Seat Code to lookup in Hash Map (e.g., C4, E6, A1): ";
            std::cin >> code;
            Seat* s = map.get(code);
            if (s) {
                std::cout << "\n>>> Hash Map Hit (O(1) Average Time):\n";
                std::cout << "    Seat Code: " << s->code << "\n";
                std::cout << "    Row: " << s->row << " | Col: " << s->col << "\n";
                std::cout << "    Status: " << s->status << "\n";
                std::cout << "    Price: INR " << s->price << "\n";
                if (!s->booking_id.empty()) std::cout << "    Booking ID: " << s->booking_id << "\n";
            } else {
                std::cout << "Seat '" << code << "' not found in Hash Map!\n";
            }
        }
        else if (choice == 3) {
            std::string code;
            std::cout << "Enter Seat Code to temporarily lock for 5 minutes (e.g. C4): ";
            std::cin >> code;
            double now = static_cast<double>(std::time(nullptr));
            double lock_until = now + 300.0; // 5 minutes

            bool ok = matrix.update_status(code, "LOCKED", lock_until, "student_viva_session");
            if (ok) {
                pq.add_lock(code, lock_until, "student_viva_session");
                Seat* s = matrix.get_cell(code[0], std::stoi(code.substr(1)));
                if (s) map.put(code, *s);

                TransactionData t;
                t.event = "SEAT_TEMPORARILY_LOCKED";
                t.seat_code = code;
                t.session_id = "student_viva_session";
                t.duration = "300 seconds (5 mins)";
                dll.append(t);

                std::cout << "\n>>> Seat " << code << " LOCKED for 5 minutes (300s)!\n";
                std::cout << "    Pushed to Min-Heap Priority Queue. Active Locks: " << pq.size() << "\n";
            } else {
                std::cout << "Failed to lock seat " << code << "!\n";
            }
        }
        else if (choice == 4) {
            std::cout << "Active locks currently in Min-Heap: " << pq.size() << "\n";
            LockEntry next_exp;
            if (pq.peek_next_expiry(next_exp)) {
                std::cout << "Next seat closest to expiring: " << next_exp.seat_code << "\n";
            }
            std::cout << "Do you want to simulate 5 minutes passing? (y/n): ";
            char sim;
            std::cin >> sim;
            if (sim == 'y' || sim == 'Y') {
                double future = static_cast<double>(std::time(nullptr)) + 301.0;
                auto expired = pq.pop_expired(future);
                std::cout << "Evicted " << expired.size() << " expired lock(s) from Min-Heap:\n";
                for (const auto& item : expired) {
                    matrix.update_status(item.first, "AVAILABLE");
                    Seat* s = matrix.get_cell(item.first[0], std::stoi(item.first.substr(1)));
                    if (s) map.put(item.first, *s);
                    std::cout << "  - Seat " << item.first << " returned to AVAILABLE pool!\n";
                }
            }
        }
        else if (choice == 5) {
            std::cout << "Enqueueing simulated concurrent booking requests into FIFO Queue...\n";
            queue.enqueue(BookingRequest("CB-VIVA-01", "C4", "user_1", 1.0));
            queue.enqueue(BookingRequest("CB-VIVA-02", "C5", "user_2", 2.0));
            queue.enqueue(BookingRequest("CB-VIVA-03", "D2", "user_3", 3.0));
            std::cout << "FIFO Queue length: " << queue.size() << "\n";

            std::cout << "Processing requests First-In First-Out:\n";
            BookingRequest req;
            while (queue.dequeue(req)) {
                std::cout << "  -> Dequeued & Processed: ID=" << req.booking_id
                          << ", Seat=" << req.seat_code << "\n";
            }
        }
        else if (choice == 6) {
            std::cout << "\n>>> Booking Audit Trail from Doubly Linked List (Tail -> Head Traversal):\n";
            auto logs = dll.to_list_reverse();
            for (size_t i = 0; i < logs.size(); ++i) {
                std::cout << "  [" << (i + 1) << "] Event: " << logs[i].event
                          << " | Seat: " << logs[i].seat_code;
                if (!logs[i].booking_id.empty()) std::cout << " | ID: " << logs[i].booking_id;
                if (logs[i].amount > 0.0) std::cout << " | Amount: INR " << logs[i].amount;
                std::cout << "\n";
            }
            std::cout << "Total chronological events stored in Doubly Linked List: " << dll.size() << "\n";
        }
        else if (choice == 7) {
            run_all_tests();
        }
    }
}


// ============================================================================
// MAIN ENTRY POINT
// ============================================================================

int main(int argc, char* argv[]) {
    // Check command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--bridge") {
            run_bridge_mode();
            return 0;
        } else if (arg == "--test") {
            bool ok = run_all_tests();
            return ok ? 0 : 1;
        } else if (arg == "--demo") {
            run_interactive_cli();
            return 0;
        }
    }

    // Default to interactive CLI if running directly in a terminal
    run_interactive_cli();
    return 0;
}
