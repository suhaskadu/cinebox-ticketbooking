#include "data_structures.hpp"
#include <algorithm>
#include <iostream>

namespace cinebook {

const char SeatMatrix::ROW_LETTERS[SeatMatrix::NUM_ROWS] = {'A', 'B', 'C', 'D', 'E'};

// Helper to format float with fixed precision
static std::string format_double(double val, int precision = 2) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(precision) << val;
    return ss.str();
}

static std::string escape_json_str(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\b') out += "\\b";
        else if (c == '\f') out += "\\f";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else out += c;
    }
    return out;
}

// ============================================================================
// DATA MODELS IMPLEMENTATION
// ============================================================================

std::string Seat::to_json() const {
    std::ostringstream oss;
    oss << "{"
        << "\"code\":\"" << escape_json_str(code) << "\","
        << "\"row\":\"" << row << "\","
        << "\"col\":" << col << ","
        << "\"status\":\"" << escape_json_str(status) << "\",";

    if (locked_until > 0.0) {
        oss << "\"locked_until\":" << format_double(locked_until, 2) << ",";
    } else {
        oss << "\"locked_until\":null,";
    }

    if (!locked_by.empty()) {
        oss << "\"locked_by\":\"" << escape_json_str(locked_by) << "\",";
    } else {
        oss << "\"locked_by\":null,";
    }

    oss << "\"price\":" << format_double(price, 2) << ",";

    if (!booking_id.empty()) {
        oss << "\"booking_id\":\"" << escape_json_str(booking_id) << "\"";
    } else {
        oss << "\"booking_id\":null";
    }

    oss << "}";
    return oss.str();
}

std::string LockEntry::to_json() const {
    std::ostringstream oss;
    oss << "{"
        << "\"expiry_time\":" << format_double(expiry_time, 2) << ","
        << "\"seat_code\":\"" << escape_json_str(seat_code) << "\","
        << "\"session_id\":\"" << escape_json_str(session_id) << "\""
        << "}";
    return oss.str();
}

std::string BookingRequest::to_json() const {
    std::ostringstream oss;
    oss << "{"
        << "\"booking_id\":\"" << escape_json_str(booking_id) << "\","
        << "\"seat_code\":\"" << escape_json_str(seat_code) << "\","
        << "\"session_id\":\"" << escape_json_str(session_id) << "\","
        << "\"timestamp\":" << format_double(timestamp, 2)
        << "}";
    return oss.str();
}

std::string TransactionData::to_json() const {
    std::ostringstream oss;
    oss << "{"
        << "\"event\":\"" << escape_json_str(event) << "\",";
    if (!booking_id.empty()) oss << "\"booking_id\":\"" << escape_json_str(booking_id) << "\",";
    if (!seat_code.empty()) oss << "\"seat_code\":\"" << escape_json_str(seat_code) << "\",";
    if (!session_id.empty()) oss << "\"session_id\":\"" << escape_json_str(session_id) << "\",";
    if (!timestamp.empty()) oss << "\"timestamp\":\"" << escape_json_str(timestamp) << "\",";
    if (amount > 0.0) oss << "\"amount\":" << format_double(amount, 2) << ",";
    if (!duration.empty()) oss << "\"duration\":\"" << escape_json_str(duration) << "\",";
    if (!reverted_to.empty()) oss << "\"reverted_to\":\"" << escape_json_str(reverted_to) << "\",";
    
    // Remove trailing comma if present
    std::string s = oss.str();
    if (s.back() == ',') s.pop_back();
    s += "}";
    return s;
}


// ============================================================================
// 1. SEAT MATRIX IMPLEMENTATION
// ============================================================================

SeatMatrix::SeatMatrix() {
    initialize_grid();
}

void SeatMatrix::initialize_grid() {
    for (int r = 0; r < NUM_ROWS; ++r) {
        char row_char = ROW_LETTERS[r];
        // Pricing policy: Row E = Premium (350), Rows C/D = Standard (250), Rows A/B = Economy (180)
        double price = (row_char == 'E') ? 350.0 : ((row_char == 'C' || row_char == 'D') ? 250.0 : 180.0);

        for (int c = 0; c < NUM_COLS; ++c) {
            int col_num = c + 1;
            std::string code = std::string(1, row_char) + std::to_string(col_num);
            grid[r][c] = Seat(code, row_char, col_num, price);
        }
    }
}

Seat* SeatMatrix::get_cell(char row_char, int col_num) {
    if (row_char < 'A' || row_char > 'E' || col_num < 1 || col_num > NUM_COLS) {
        return nullptr;
    }
    int r_idx = row_char - 'A';
    int c_idx = col_num - 1;
    return &grid[r_idx][c_idx];
}

const Seat* SeatMatrix::get_cell(char row_char, int col_num) const {
    if (row_char < 'A' || row_char > 'E' || col_num < 1 || col_num > NUM_COLS) {
        return nullptr;
    }
    int r_idx = row_char - 'A';
    int c_idx = col_num - 1;
    return &grid[r_idx][c_idx];
}

bool SeatMatrix::update_status(const std::string& seat_code, const std::string& status,
                              double locked_until, const std::string& locked_by,
                              const std::string& booking_id) {
    if (seat_code.length() < 2) return false;
    char row_char = seat_code[0];
    int col_num = 0;
    try {
        col_num = std::stoi(seat_code.substr(1));
    } catch (...) {
        return false;
    }

    Seat* cell = get_cell(row_char, col_num);
    if (!cell) return false;

    cell->status = status;
    cell->locked_until = locked_until;
    cell->locked_by = locked_by;
    if (!booking_id.empty() || status == "AVAILABLE") {
        cell->booking_id = booking_id;
    }
    return true;
}

std::vector<Seat> SeatMatrix::get_all_seats() const {
    std::vector<Seat> seats;
    seats.reserve(NUM_ROWS * NUM_COLS);
    for (int r = 0; r < NUM_ROWS; ++r) {
        for (int c = 0; c < NUM_COLS; ++c) {
            seats.push_back(grid[r][c]);
        }
    }
    return seats;
}

int SeatMatrix::get_available_count(double current_time) {
    if (current_time <= 0.0) {
        current_time = static_cast<double>(std::time(nullptr));
    }

    int count = 0;
    for (int r = 0; r < NUM_ROWS; ++r) {
        for (int c = 0; c < NUM_COLS; ++c) {
            Seat& s = grid[r][c];
            // Auto-revert expired temporary locks
            if (s.status == "LOCKED" && s.locked_until > 0.0 && s.locked_until < current_time) {
                s.status = "AVAILABLE";
                s.locked_until = 0.0;
                s.locked_by = "";
            }
            if (s.status == "AVAILABLE") {
                count++;
            }
        }
    }
    return count;
}

void SeatMatrix::print_grid() const {
    std::cout << "\n   --- CINEMA SEATING MATRIX (5x6) --- \n";
    std::cout << "        [SCREEN THIS WAY]\n";
    std::cout << "      1     2     3     4     5     6\n";
    std::cout << "   +-----+-----+-----+-----+-----+-----+\n";
    for (int r = 0; r < NUM_ROWS; ++r) {
        std::cout << " " << ROW_LETTERS[r] << " |";
        for (int c = 0; c < NUM_COLS; ++c) {
            const Seat& s = grid[r][c];
            char st = (s.status == "AVAILABLE") ? 'O' : ((s.status == "LOCKED") ? 'L' : 'X');
            std::cout << " " << s.code << st << " |";
        }
        std::cout << "\n   +-----+-----+-----+-----+-----+-----+\n";
    }
    std::cout << " Legend: O = Available, L = 5-Min Locked, X = Booked\n\n";
}


// ============================================================================
// 2. SEAT HASH MAP IMPLEMENTATION (Bucket Chaining)
// ============================================================================

SeatHashMap::SeatHashMap() : element_count(0) {
    for (size_t i = 0; i < BUCKET_COUNT; ++i) {
        table[i] = nullptr;
    }
}

SeatHashMap::~SeatHashMap() {
    clear();
}

size_t SeatHashMap::hash_function(const std::string& key) const {
    // DJB2 Hash Algorithm
    unsigned long hash = 5381;
    for (char c : key) {
        hash = ((hash << 5) + hash) + static_cast<unsigned char>(c); // hash * 33 + c
    }
    return hash % BUCKET_COUNT;
}

void SeatHashMap::put(const std::string& key, const Seat& seat) {
    size_t bucket_idx = hash_function(key);
    HashNode* curr = table[bucket_idx];

    // Check if key already exists in bucket chain
    while (curr) {
        if (curr->key == key) {
            curr->value = seat;
            return;
        }
        curr = curr->next;
    }

    // Insert new node at head of chain
    HashNode* new_node = new HashNode(key, seat);
    new_node->next = table[bucket_idx];
    table[bucket_idx] = new_node;
    element_count++;
}

Seat* SeatHashMap::get(const std::string& key) {
    size_t bucket_idx = hash_function(key);
    HashNode* curr = table[bucket_idx];
    while (curr) {
        if (curr->key == key) {
            return &curr->value;
        }
        curr = curr->next;
    }
    return nullptr;
}

const Seat* SeatHashMap::get(const std::string& key) const {
    size_t bucket_idx = hash_function(key);
    HashNode* curr = table[bucket_idx];
    while (curr) {
        if (curr->key == key) {
            return &curr->value;
        }
        curr = curr->next;
    }
    return nullptr;
}

bool SeatHashMap::contains(const std::string& key) const {
    return get(key) != nullptr;
}

bool SeatHashMap::remove(const std::string& key) {
    size_t bucket_idx = hash_function(key);
    HashNode* curr = table[bucket_idx];
    HashNode* prev = nullptr;

    while (curr) {
        if (curr->key == key) {
            if (prev) {
                prev->next = curr->next;
            } else {
                table[bucket_idx] = curr->next;
            }
            delete curr;
            element_count--;
            return true;
        }
        prev = curr;
        curr = curr->next;
    }
    return false;
}

void SeatHashMap::clear() {
    for (size_t i = 0; i < BUCKET_COUNT; ++i) {
        HashNode* curr = table[i];
        while (curr) {
            HashNode* next = curr->next;
            delete curr;
            curr = next;
        }
        table[i] = nullptr;
    }
    element_count = 0;
}

std::vector<std::string> SeatHashMap::keys() const {
    std::vector<std::string> all_keys;
    all_keys.reserve(element_count);
    for (size_t i = 0; i < BUCKET_COUNT; ++i) {
        HashNode* curr = table[i];
        while (curr) {
            all_keys.push_back(curr->key);
            curr = curr->next;
        }
    }
    return all_keys;
}


// ============================================================================
// 3. MIN-HEAP PRIORITY QUEUE IMPLEMENTATION
// ============================================================================

LockExpiryPriorityQueue::LockExpiryPriorityQueue() {}

void LockExpiryPriorityQueue::heapify_up(size_t index) {
    while (index > 0) {
        size_t parent = (index - 1) / 2;
        if (heap[index].expiry_time < heap[parent].expiry_time) {
            std::swap(heap[index], heap[parent]);
            index = parent;
        } else {
            break;
        }
    }
}

void LockExpiryPriorityQueue::heapify_down(size_t index) {
    size_t n = heap.size();
    while (true) {
        size_t smallest = index;
        size_t left = 2 * index + 1;
        size_t right = 2 * index + 2;

        if (left < n && heap[left].expiry_time < heap[smallest].expiry_time) {
            smallest = left;
        }
        if (right < n && heap[right].expiry_time < heap[smallest].expiry_time) {
            smallest = right;
        }

        if (smallest != index) {
            std::swap(heap[index], heap[smallest]);
            index = smallest;
        } else {
            break;
        }
    }
}

void LockExpiryPriorityQueue::add_lock(const std::string& seat_code, double expiry_timestamp, const std::string& session_id) {
    heap.push_back(LockEntry(expiry_timestamp, seat_code, session_id));
    heapify_up(heap.size() - 1);
}

std::vector<std::pair<std::string, std::string>> LockExpiryPriorityQueue::pop_expired(double current_time) {
    std::vector<std::pair<std::string, std::string>> expired;
    while (!heap.empty() && heap[0].expiry_time <= current_time) {
        expired.push_back({heap[0].seat_code, heap[0].session_id});
        if (heap.size() == 1) {
            heap.pop_back();
        } else {
            heap[0] = heap.back();
            heap.pop_back();
            heapify_down(0);
        }
    }
    return expired;
}

bool LockExpiryPriorityQueue::peek_next_expiry(LockEntry& out) const {
    if (heap.empty()) return false;
    out = heap[0];
    return true;
}


// ============================================================================
// 4. FIFO BOOKING QUEUE IMPLEMENTATION
// ============================================================================

BookingFIFOQueue::BookingFIFOQueue() : front_ptr(nullptr), rear_ptr(nullptr), count(0) {}

BookingFIFOQueue::~BookingFIFOQueue() {
    clear();
}

void BookingFIFOQueue::enqueue(const BookingRequest& request) {
    QueueNode* new_node = new QueueNode(request);
    if (!rear_ptr) {
        front_ptr = rear_ptr = new_node;
    } else {
        rear_ptr->next = new_node;
        rear_ptr = new_node;
    }
    count++;
}

bool BookingFIFOQueue::dequeue(BookingRequest& out) {
    if (!front_ptr) return false;
    
    QueueNode* temp = front_ptr;
    out = temp->data;
    front_ptr = front_ptr->next;
    if (!front_ptr) {
        rear_ptr = nullptr;
    }
    delete temp;
    count--;
    return true;
}

void BookingFIFOQueue::clear() {
    while (front_ptr) {
        QueueNode* temp = front_ptr;
        front_ptr = front_ptr->next;
        delete temp;
    }
    rear_ptr = nullptr;
    count = 0;
}


// ============================================================================
// 5. DOUBLY LINKED LIST IMPLEMENTATION (Audit Trail & Rollback)
// ============================================================================

BookingHistoryDoublyLinkedList::BookingHistoryDoublyLinkedList()
    : head(nullptr), tail(nullptr), count(0) {}

BookingHistoryDoublyLinkedList::~BookingHistoryDoublyLinkedList() {
    clear();
}

TransactionNode* BookingHistoryDoublyLinkedList::append(const TransactionData& transaction_data) {
    TransactionNode* new_node = new TransactionNode(transaction_data);
    if (!head) {
        head = tail = new_node;
    } else {
        tail->next = new_node;
        new_node->prev = tail;
        tail = new_node;
    }
    count++;
    return new_node;
}

std::vector<TransactionData> BookingHistoryDoublyLinkedList::to_list_reverse() const {
    std::vector<TransactionData> list;
    list.reserve(count);
    TransactionNode* curr = tail; // Traverse backwards from tail
    while (curr) {
        list.push_back(curr->data);
        curr = curr->prev;
    }
    return list;
}

std::vector<TransactionData> BookingHistoryDoublyLinkedList::to_list_forward() const {
    std::vector<TransactionData> list;
    list.reserve(count);
    TransactionNode* curr = head; // Traverse forwards from head
    while (curr) {
        list.push_back(curr->data);
        curr = curr->next;
    }
    return list;
}

void BookingHistoryDoublyLinkedList::clear() {
    TransactionNode* curr = head;
    while (curr) {
        TransactionNode* next = curr->next;
        delete curr;
        curr = next;
    }
    head = tail = nullptr;
    count = 0;
}

} // namespace cinebook
