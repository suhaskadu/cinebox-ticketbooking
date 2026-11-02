"""
CineBook - Data Structures Module for College Mini Project
Native C++ Integration Layer

This module provides high-performance data structures backed by a native C++ engine:
1. 2D Array / Matrix (Cinema Seating Grid: 5 Rows x 6 Columns)
2. Hash Map with Bucket Chaining (O(1) Instant Seat Lookup)
3. Min-Heap Priority Queue (5-Minute Temporary Seat Lock Expiration)
4. FIFO Queue (Sequential Booking Transaction Processing)
5. Doubly Linked List (Chronological Audit Trail & Cancellation Rollback)

The underlying core logic is implemented in C++ (backend/cpp/data_structures.cpp),
providing maximum performance, spatial efficiency, and authentic college DSA demonstration.
"""

import os
import sys
import time
import json
import subprocess
import threading
import heapq
from collections import deque

CPP_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), "cpp"))
CPP_EXE = os.path.join(CPP_DIR, "cinebook_ds.exe" if sys.platform == "win32" else "cinebook_ds")

class CppEngineBridge:
    """
    Subprocess JSON-IPC bridge communicating with the native C++ Data Structures Engine.
    """
    _instance = None
    _lock = threading.Lock()

    def __init__(self):
        self.proc = None
        self.active = False
        self._ensure_binary()
        self._start_bridge()

    @classmethod
    def get_instance(cls):
        with cls._lock:
            if cls._instance is None:
                cls._instance = cls()
            return cls._instance

    def _ensure_binary(self):
        """Compiles C++ binary if not yet compiled."""
        if os.path.exists(CPP_EXE):
            return
        cpp_src = os.path.join(CPP_DIR, "data_structures.cpp")
        main_src = os.path.join(CPP_DIR, "main.cpp")
        if os.path.exists(cpp_src) and os.path.exists(main_src):
            try:
                print(f"[C++ Engine] Compiling native C++ data structures engine...")
                cmd = ["g++", "-std=c++14", "-O2", cpp_src, main_src, "-o", CPP_EXE]
                subprocess.run(cmd, check=True, cwd=CPP_DIR)
                print(f"[C++ Engine] Native C++ engine successfully compiled at: {CPP_EXE}")
            except Exception as e:
                print(f"[C++ Engine Warning] Could not auto-compile C++ binary: {e}")

    def _start_bridge(self):
        if not os.path.exists(CPP_EXE):
            self.active = False
            return
        try:
            self.proc = subprocess.Popen(
                [CPP_EXE, "--bridge"],
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                bufsize=1
            )
            # Send ping
            self.proc.stdin.write(json.dumps({"cmd": "ping"}) + "\n")
            self.proc.stdin.flush()
            line = self.proc.stdout.readline()
            res = json.loads(line) if line else {}
            if res.get("status") == "ok":
                self.active = True
                print(f"[C++ Engine] Native C++ Data Structures Engine active (PID: {self.proc.pid})")
            else:
                self.active = False
        except Exception as e:
            self.active = False
            print(f"[C++ Engine Warning] Failed to start native C++ bridge: {e}")

    def send_cmd(self, cmd_dict):
        if not self.active or not self.proc:
            return None
        with self._lock:
            try:
                payload = json.dumps(cmd_dict) + "\n"
                self.proc.stdin.write(payload)
                self.proc.stdin.flush()
                resp_line = self.proc.stdout.readline()
                if not resp_line:
                    return None
                return json.loads(resp_line)
            except Exception:
                self.active = False
                return None


# Global singleton bridge instance
bridge = CppEngineBridge.get_instance()


class SeatMatrix:
    """
    2D Matrix representation of the cinema theater (5 Rows x 6 Columns = 30 Seats)
    Row Indices: 0 -> A, 1 -> B, 2 -> C, 3 -> D, 4 -> E
    Col Indices: 0 -> 1, 1 -> 2, ..., 5 -> 6
    Direct index access: seats[row_idx][col_idx] in O(1) time complexity.
    """
    ROWS = ['A', 'B', 'C', 'D', 'E']
    COLS = 6

    def __init__(self):
        self.grid = [[None for _ in range(self.COLS)] for _ in range(len(self.ROWS))]
        self._initialize_grid()
        if bridge.active:
            bridge.send_cmd({"cmd": "init_grid"})

    def _initialize_grid(self):
        for r_idx, row in enumerate(self.ROWS):
            for c_idx in range(self.COLS):
                code = f"{row}{c_idx + 1}"
                price = 350.0 if row == 'E' else (250.0 if row in ['C', 'D'] else 180.0)
                self.grid[r_idx][c_idx] = {
                    "code": code,
                    "row": row,
                    "col": c_idx + 1,
                    "status": "AVAILABLE",
                    "locked_until": None,
                    "locked_by": None,
                    "price": price,
                    "booking_id": None
                }

    def get_cell(self, row_char, col_num):
        if row_char not in self.ROWS or not (1 <= col_num <= self.COLS):
            return None
        r_idx = self.ROWS.index(row_char)
        c_idx = col_num - 1

        if bridge.active:
            res = bridge.send_cmd({"cmd": "get_cell", "row": row_char, "col": col_num})
            if res and res.get("found"):
                # keep local grid synchronized
                self.grid[r_idx][c_idx] = res["seat"]
                return res["seat"]

        return self.grid[r_idx][c_idx]

    def update_status(self, seat_code, status, locked_until=None, locked_by=None, booking_id=None):
        if len(seat_code) < 2:
            return False
        row_char = seat_code[0]
        try:
            col_num = int(seat_code[1:])
        except ValueError:
            return False

        if row_char not in self.ROWS or not (1 <= col_num <= self.COLS):
            return False

        r_idx = self.ROWS.index(row_char)
        c_idx = col_num - 1
        cell = self.grid[r_idx][c_idx]

        if cell:
            cell["status"] = status
            cell["locked_until"] = locked_until
            cell["locked_by"] = locked_by
            if booking_id is not None or status == "AVAILABLE":
                cell["booking_id"] = booking_id

            if bridge.active:
                bridge.send_cmd({
                    "cmd": "update_matrix",
                    "seat_code": seat_code,
                    "status": status,
                    "locked_until": locked_until if locked_until else 0.0,
                    "locked_by": locked_by if locked_by else "",
                    "booking_id": booking_id if booking_id else ""
                })
            return True
        return False

    def get_all_seats(self):
        if bridge.active:
            res = bridge.send_cmd({"cmd": "get_all_seats"})
            if res and "seats" in res:
                return res["seats"]

        seats = []
        for row in self.grid:
            for seat in row:
                seats.append(seat.copy())
        return seats

    def get_available_count(self):
        now = time.time()
        if bridge.active:
            res = bridge.send_cmd({"cmd": "get_available_count", "now": now})
            if res and "available_count" in res:
                return res["available_count"]

        count = 0
        for row in self.grid:
            for seat in row:
                if seat["status"] == "LOCKED" and seat["locked_until"] and seat["locked_until"] < now:
                    seat["status"] = "AVAILABLE"
                    seat["locked_until"] = None
                    seat["locked_by"] = None
                if seat["status"] == "AVAILABLE":
                    count += 1
        return count


class SeatHashMap:
    """
    Hash Map for O(1) Instant Seat Lookup by Seat Code (e.g., 'C4' -> Seat Object).
    Implemented in C++ via Bucket Chaining Hash Table.
    """
    def __init__(self):
        self.map = {}

    def put(self, seat_code, seat_data):
        self.map[seat_code] = seat_data
        if bridge.active:
            bridge.send_cmd({"cmd": "hash_put", "seat_code": seat_code})

    def get(self, seat_code):
        if bridge.active:
            res = bridge.send_cmd({"cmd": "hash_get", "seat_code": seat_code})
            if res and res.get("found"):
                return res["seat"]
        return self.map.get(seat_code)

    def contains(self, seat_code):
        if bridge.active:
            res = bridge.send_cmd({"cmd": "hash_contains", "seat_code": seat_code})
            if res and "contains" in res:
                return res["contains"]
        return seat_code in self.map

    def remove(self, seat_code):
        if seat_code in self.map:
            del self.map[seat_code]
        if bridge.active:
            bridge.send_cmd({"cmd": "hash_remove", "seat_code": seat_code})


class LockExpiryPriorityQueue:
    """
    Min-Heap Priority Queue for managing 5-minute temporary seat lock expirations.
    Stores tuples of (expiration_timestamp, seat_code, session_id).
    O(log N) insertion and O(1) peek for earliest seat lock to expire.
    """
    def __init__(self):
        self.heap = []

    def add_lock(self, seat_code, expiry_timestamp, session_id):
        heapq.heappush(self.heap, (expiry_timestamp, seat_code, session_id))
        if bridge.active:
            bridge.send_cmd({
                "cmd": "pq_add_lock",
                "seat_code": seat_code,
                "expiry_time": expiry_timestamp,
                "session_id": session_id
            })

    def pop_expired(self, current_time=None):
        if current_time is None:
            current_time = time.time()

        if bridge.active:
            res = bridge.send_cmd({"cmd": "pq_pop_expired", "now": current_time})
            if res and "expired" in res:
                # Synchronize local heap
                while self.heap and self.heap[0][0] <= current_time:
                    heapq.heappop(self.heap)
                return [(item[0], item[1]) for item in res["expired"]]

        expired_seats = []
        while self.heap and self.heap[0][0] <= current_time:
            expiry, code, session_id = heapq.heappop(self.heap)
            expired_seats.append((code, session_id))
        return expired_seats

    def peek_next_expiry(self):
        if bridge.active:
            res = bridge.send_cmd({"cmd": "pq_peek"})
            if res and res.get("has_entry") and res.get("entry"):
                e = res["entry"]
                return (e["expiry_time"], e["seat_code"], e["session_id"])

        if self.heap:
            return self.heap[0]
        return None

    def size(self):
        if bridge.active:
            res = bridge.send_cmd({"cmd": "pq_size"})
            if res and "size" in res:
                return res["size"]
        return len(self.heap)


class BookingFIFOQueue:
    """
    FIFO Queue (First In, First Out) for sequential booking requests.
    Prevents race conditions when two users click 'Confirm' simultaneously.
    """
    def __init__(self):
        self.queue = deque()

    def enqueue(self, booking_request):
        self.queue.append(booking_request)
        if bridge.active:
            bridge.send_cmd({
                "cmd": "fifo_enqueue",
                "booking_id": booking_request.get("booking_id", ""),
                "seat_code": booking_request.get("seat_code", ""),
                "session_id": booking_request.get("session_id", ""),
                "timestamp": booking_request.get("timestamp", 0.0)
            })

    def dequeue(self):
        if bridge.active:
            res = bridge.send_cmd({"cmd": "fifo_dequeue"})
            if res and res.get("has_entry"):
                if self.queue:
                    self.queue.popleft()
                return res["request"]

        if self.queue:
            return self.queue.popleft()
        return None

    def is_empty(self):
        return self.size() == 0

    def size(self):
        if bridge.active:
            res = bridge.send_cmd({"cmd": "fifo_size"})
            if res and "size" in res:
                return res["size"]
        return len(self.queue)


class TransactionNode:
    """Node for Doubly Linked List tracking Booking Audit Trail."""
    def __init__(self, data):
        self.data = data
        self.prev = None
        self.next = None


class BookingHistoryDoublyLinkedList:
    """
    Doubly Linked List for audit logs and rollback/undo operations (Cancellation).
    Supports bidirectional traversal with prev and next pointers.
    """
    def __init__(self):
        self.head = None
        self.tail = None
        self.count = 0

    def append(self, transaction_data):
        node = TransactionNode(transaction_data)
        if not self.head:
            self.head = node
            self.tail = node
        else:
            self.tail.next = node
            node.prev = self.tail
            self.tail = node
        self.count += 1

        if bridge.active:
            bridge.send_cmd({
                "cmd": "dll_append",
                "event": transaction_data.get("event", ""),
                "booking_id": transaction_data.get("booking_id", ""),
                "seat_code": transaction_data.get("seat_code", ""),
                "session_id": transaction_data.get("session_id", ""),
                "timestamp": transaction_data.get("timestamp", ""),
                "amount": float(transaction_data.get("amount", 0.0)),
                "duration": transaction_data.get("duration", ""),
                "reverted_to": transaction_data.get("reverted_to", "")
            })
        return node

    def to_list(self):
        if bridge.active:
            res = bridge.send_cmd({"cmd": "dll_to_list"})
            if res and "logs" in res:
                return res["logs"]

        result = []
        curr = self.tail  # Most recent first
        while curr:
            result.append(curr.data)
            curr = curr.prev
        return result
