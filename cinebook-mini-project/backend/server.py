"""
CineBook - Python HTTP & REST API Server (Zero External Dependencies)
Provides:
- Static Web Hosting for Frontend
- REST Endpoints for Seats, 5-Min Locks, Bookings, Cancellations
- College Data Structures Instrumentation (Matrix, HashMap, Priority Queue, FIFO Queue, Doubly Linked List)
"""

import http.server
import socketserver
import json
import os
import sys
import time
import random
import urllib.parse
from datetime import datetime

# Import local database and data structures
from database import init_database, get_connection, clean_expired_locks, DB_PATH
from data_structures import (
    SeatMatrix,
    SeatHashMap,
    LockExpiryPriorityQueue,
    BookingFIFOQueue,
    BookingHistoryDoublyLinkedList
)

PORT = 8000
FRONTEND_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "frontend"))

# Initialize in-memory Data Structures for demonstration
seat_matrix = SeatMatrix()
seat_hash_map = SeatHashMap()
lock_priority_queue = LockExpiryPriorityQueue()
booking_fifo_queue = BookingFIFOQueue()
audit_linked_list = BookingHistoryDoublyLinkedList()

def sync_data_structures_from_db():
    """Syncs the in-memory Data Structures with SQLite database."""
    clean_expired_locks()
    conn = get_connection()
    cursor = conn.cursor()
    cursor.execute("SELECT * FROM seats ORDER BY row_label, col_number")
    rows = cursor.fetchall()
    
    for row in rows:
        seat_data = {
            "code": row["seat_code"],
            "row": row["row_label"],
            "col": row["col_number"],
            "price": row["price"],
            "status": row["status"],
            "locked_until": row["locked_until"],
            "locked_by": row["locked_by"],
            "booking_id": row["booking_id"]
        }
        # Update 2D Matrix
        seat_matrix.update_status(
            row["seat_code"], 
            row["status"], 
            row["locked_until"], 
            row["locked_by"]
        )
        # Update Hash Map
        seat_hash_map.put(row["seat_code"], seat_data)

        # Check for active locks and add to Priority Queue
        if row["status"] == "LOCKED" and row["locked_until"]:
            if row["locked_until"] > time.time():
                lock_priority_queue.add_lock(row["seat_code"], row["locked_until"], row["locked_by"])

    conn.close()

class CineBookHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=FRONTEND_DIR, **kwargs)

    def _send_json(self, data, status=200):
        body = json.dumps(data).encode('utf-8')
        self.send_response(status)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Content-Length', str(len(body)))
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, POST, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type')
        self.end_headers()
        self.wfile.write(body)

    def do_OPTIONS(self):
        self.send_response(200)
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, POST, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type')
        self.end_headers()

    def do_GET(self):
        url = urllib.parse.urlparse(self.path)
        path = url.path

        if path == '/api/movie':
            self.handle_get_movie()
        elif path == '/api/seats':
            self.handle_get_seats()
        elif path == '/api/ds-metrics':
            self.handle_get_ds_metrics()
        elif path.startswith('/api/bookings/'):
            booking_id = path.replace('/api/bookings/', '')
            self.handle_get_booking(booking_id)
        else:
            # Fallback to serving static files from FRONTEND_DIR
            super().do_GET()

    def do_POST(self):
        url = urllib.parse.urlparse(self.path)
        path = url.path
        
        content_length = int(self.headers.get('Content-Length', 0))
        post_data = self.rfile.read(content_length) if content_length > 0 else b'{}'
        
        try:
            payload = json.loads(post_data.decode('utf-8'))
        except Exception:
            payload = {}

        if path == '/api/seats/lock':
            self.handle_lock_seat(payload)
        elif path == '/api/seats/unlock':
            self.handle_unlock_seat(payload)
        elif path == '/api/bookings/confirm':
            self.handle_confirm_booking(payload)
        elif path == '/api/bookings/cancel':
            self.handle_cancel_booking(payload)
        elif path == '/api/reset':
            self.handle_reset_database()
        else:
            self._send_json({"error": "Endpoint not found"}, status=404)

    def handle_get_movie(self):
        conn = get_connection()
        cursor = conn.cursor()
        cursor.execute("SELECT * FROM movie WHERE id = 1")
        movie = dict(cursor.fetchone())
        conn.close()

        # Available dates & showtimes
        movie["dates"] = [
            {"day": "TODAY", "date": "25 Oct", "full": "Today, Oct 25", "active": True},
            {"day": "TOMORROW", "date": "26 Oct", "full": "Tomorrow, Oct 26", "active": False},
            {"day": "TUE", "date": "27 Oct", "full": "Tuesday, Oct 27", "active": False},
            {"day": "WED", "date": "28 Oct", "full": "Wednesday, Oct 28", "active": False}
        ]
        movie["showtimes"] = [
            {"time": "10:30 AM", "type": "2D IMAX", "screen": "Screen 1", "available": True},
            {"time": "02:15 PM", "type": "4DX Atmos", "screen": "Screen 1", "available": True},
            {"time": "06:00 PM", "type": "3D Dolby", "screen": "Screen 1", "available": True, "selected": True},
            {"time": "09:45 PM", "type": "2D IMAX", "screen": "Screen 1", "available": True}
        ]
        self._send_json(movie)

    def handle_get_seats(self):
        # 1. Clean expired locks in SQLite and in-memory heap
        clean_expired_locks()
        sync_data_structures_from_db()

        conn = get_connection()
        cursor = conn.cursor()
        cursor.execute("SELECT * FROM seats ORDER BY row_label, col_number")
        seats = [dict(r) for r in cursor.fetchall()]

        # Compute counts
        total_seats = len(seats)
        booked_seats = sum(1 for s in seats if s["status"] == "BOOKED")
        locked_seats = sum(1 for s in seats if s["status"] == "LOCKED")
        available_seats = sum(1 for s in seats if s["status"] == "AVAILABLE")
        conn.close()

        self._send_json({
            "total_seats": total_seats,
            "available_count": available_seats,
            "booked_count": booked_seats,
            "locked_count": locked_seats,
            "server_time": time.time(),
            "seats": seats
        })

    def handle_lock_seat(self, payload):
        seat_code = payload.get("seat_code")
        session_id = payload.get("session_id", "guest_user")
        
        if not seat_code:
            self._send_json({"success": False, "message": "Seat code is required"}, status=400)
            return

        clean_expired_locks()
        conn = get_connection()
        cursor = conn.cursor()
        cursor.execute("SELECT * FROM seats WHERE seat_code = ?", (seat_code,))
        seat = cursor.fetchone()

        if not seat:
            conn.close()
            self._send_json({"success": False, "message": f"Seat {seat_code} does not exist"}, status=404)
            return

        if seat["status"] == "BOOKED":
            conn.close()
            self._send_json({"success": False, "message": f"Seat {seat_code} is already booked by another customer!"}, status=409)
            return

        now = time.time()
        # If seat is locked by someone else and lock has not expired
        if seat["status"] == "LOCKED" and seat["locked_until"] and seat["locked_until"] > now and seat["locked_by"] != session_id:
            conn.close()
            remaining = int(seat["locked_until"] - now)
            self._send_json({
                "success": False, 
                "message": f"Seat {seat_code} is temporarily locked by another user! Try again in {remaining}s."
            }, status=409)
            return

        # 5-minute temporary lock = 300 seconds
        LOCK_DURATION = 300
        locked_until = now + LOCK_DURATION

        cursor.execute("""
        UPDATE seats
        SET status = 'LOCKED', locked_until = ?, locked_by = ?
        WHERE seat_code = ?
        """, (locked_until, session_id, seat_code))
        conn.commit()
        conn.close()

        # Update in-memory DS
        seat_matrix.update_status(seat_code, "LOCKED", locked_until, session_id)
        lock_priority_queue.add_lock(seat_code, locked_until, session_id)

        audit_linked_list.append({
            "event": "SEAT_TEMPORARILY_LOCKED",
            "seat_code": seat_code,
            "session_id": session_id,
            "timestamp": datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
            "duration": "5 minutes"
        })

        self._send_json({
            "success": True,
            "seat_code": seat_code,
            "status": "LOCKED",
            "locked_until": locked_until,
            "remaining_seconds": LOCK_DURATION,
            "message": f"Seat {seat_code} locked for 5 minutes. Complete booking before timer expires!"
        })

    def handle_unlock_seat(self, payload):
        seat_code = payload.get("seat_code")
        session_id = payload.get("session_id")
        
        conn = get_connection()
        cursor = conn.cursor()
        
        if session_id:
            cursor.execute("""
            UPDATE seats
            SET status = 'AVAILABLE', locked_until = NULL, locked_by = NULL
            WHERE seat_code = ? AND locked_by = ? AND status = 'LOCKED'
            """, (seat_code, session_id))
        else:
            cursor.execute("""
            UPDATE seats
            SET status = 'AVAILABLE', locked_until = NULL, locked_by = NULL
            WHERE seat_code = ? AND status = 'LOCKED'
            """, (seat_code,))
            
        conn.commit()
        conn.close()

        seat_matrix.update_status(seat_code, "AVAILABLE", None, None)
        self._send_json({"success": True, "message": f"Seat {seat_code} released back to Available"})

    def handle_confirm_booking(self, payload):
        seat_code = payload.get("seat_code")
        session_id = payload.get("session_id", "guest_user")
        customer_name = payload.get("customer_name", "College Demo User")
        show_date = payload.get("show_date", "Today, 25 Oct")
        showtime = payload.get("showtime", "06:00 PM")

        if not seat_code:
            self._send_json({"success": False, "message": "Seat code is required"}, status=400)
            return

        clean_expired_locks()
        conn = get_connection()
        cursor = conn.cursor()
        cursor.execute("SELECT * FROM seats WHERE seat_code = ?", (seat_code,))
        seat = cursor.fetchone()

        if not seat:
            conn.close()
            self._send_json({"success": False, "message": "Seat not found"}, status=404)
            return

        if seat["status"] == "BOOKED":
            conn.close()
            self._send_json({"success": False, "message": f"Seat {seat_code} is already booked!"}, status=409)
            return

        # Check if lock expired
        now = time.time()
        if seat["status"] == "LOCKED" and seat["locked_until"] and seat["locked_until"] < now:
            conn.close()
            self._send_json({"success": False, "message": "5-Minute seat lock expired! Please select seat again."}, status=408)
            return

        # Generate unique Booking ID: e.g. CB-20261025-C4
        booking_id = f"CB-{random.randint(10000, 99999)}-{seat_code}"
        ticket_price = seat["price"]
        tax_amount = round(ticket_price * 0.18, 2)  # 18% GST / Cinema Tax
        total_price = round(ticket_price + tax_amount, 2)
        created_at = datetime.now().strftime("%Y-%m-%d %I:%M %p")

        # Enqueue to FIFO request queue
        booking_request = {
            "booking_id": booking_id,
            "seat_code": seat_code,
            "session_id": session_id,
            "timestamp": now
        }
        booking_fifo_queue.enqueue(booking_request)

        # Dequeue and process transaction
        processed_req = booking_fifo_queue.dequeue()

        # Update seat to BOOKED
        cursor.execute("""
        UPDATE seats
        SET status = 'BOOKED', locked_until = NULL, locked_by = NULL, booking_id = ?
        WHERE seat_code = ?
        """, (booking_id, seat_code))

        # Insert booking record
        cursor.execute("""
        INSERT INTO bookings (
            booking_id, movie_title, seat_code, customer_name, 
            show_date, showtime, ticket_price, tax_amount, total_price, 
            status, created_at
        ) VALUES (?, 'Avengers: Endgame', ?, ?, ?, ?, ?, ?, ?, 'CONFIRMED', ?)
        """, (booking_id, seat_code, customer_name, show_date, showtime, ticket_price, tax_amount, total_price, created_at))

        conn.commit()
        conn.close()

        # Update in-memory DS
        seat_matrix.update_status(seat_code, "BOOKED")
        audit_linked_list.append({
            "event": "BOOKING_CONFIRMED",
            "booking_id": booking_id,
            "seat_code": seat_code,
            "amount": total_price,
            "timestamp": created_at
        })

        self._send_json({
            "success": True,
            "booking": {
                "booking_id": booking_id,
                "movie_title": "Avengers: Endgame",
                "seat_code": seat_code,
                "row": seat_code[0],
                "seat_num": seat_code[1:],
                "customer_name": customer_name,
                "show_date": show_date,
                "showtime": showtime,
                "ticket_price": ticket_price,
                "tax_amount": tax_amount,
                "total_price": total_price,
                "status": "CONFIRMED",
                "created_at": created_at,
                "cinema_hall": "Audi 1 (IMAX Laser)",
                "screen": "Screen 1"
            }
        })

    def handle_cancel_booking(self, payload):
        booking_id = payload.get("booking_id")
        seat_code = payload.get("seat_code")

        conn = get_connection()
        cursor = conn.cursor()

        if booking_id:
            cursor.execute("SELECT * FROM bookings WHERE booking_id = ?", (booking_id,))
        elif seat_code:
            cursor.execute("SELECT * FROM bookings WHERE seat_code = ? AND status = 'CONFIRMED' ORDER BY created_at DESC LIMIT 1", (seat_code,))
        else:
            conn.close()
            self._send_json({"success": False, "message": "Booking ID or Seat Code required"}, status=400)
            return

        booking = cursor.fetchone()
        if not booking:
            conn.close()
            self._send_json({"success": False, "message": "Booking not found"}, status=404)
            return

        b_id = booking["booking_id"]
        s_code = booking["seat_code"]
        cancelled_at = datetime.now().strftime("%Y-%m-%d %I:%M %p")

        # Mark booking as CANCELLED in database
        cursor.execute("UPDATE bookings SET status = 'CANCELLED', cancelled_at = ? WHERE booking_id = ?", (cancelled_at, b_id))

        # Revert seat back to AVAILABLE
        cursor.execute("""
        UPDATE seats 
        SET status = 'AVAILABLE', locked_until = NULL, locked_by = NULL, booking_id = NULL 
        WHERE seat_code = ?
        """, (s_code,))

        conn.commit()

        # Recalculate available seat count
        cursor.execute("SELECT COUNT(*) FROM seats WHERE status = 'AVAILABLE'")
        new_avail_count = cursor.fetchone()[0]
        conn.close()

        # Update in-memory DS & audit trail
        seat_matrix.update_status(s_code, "AVAILABLE")
        audit_linked_list.append({
            "event": "BOOKING_CANCELLED_ROLLBACK",
            "booking_id": b_id,
            "seat_code": s_code,
            "timestamp": cancelled_at,
            "reverted_to": "AVAILABLE"
        })

        self._send_json({
            "success": True,
            "message": f"Booking {b_id} successfully cancelled. Seat {s_code} has been returned to the pool.",
            "cancelled_booking": {
                "booking_id": b_id,
                "seat_code": s_code,
                "row": s_code[0],
                "seat_num": s_code[1:],
                "cancelled_at": cancelled_at,
                "refund_amount": booking["total_price"]
            },
            "new_available_count": new_avail_count
        })

    def handle_get_booking(self, booking_id):
        conn = get_connection()
        cursor = conn.cursor()
        cursor.execute("SELECT * FROM bookings WHERE booking_id = ?", (booking_id,))
        booking = cursor.fetchone()
        conn.close()
        if booking:
            self._send_json({"success": True, "booking": dict(booking)})
        else:
            self._send_json({"success": False, "message": "Booking not found"}, status=404)

    def handle_get_ds_metrics(self):
        """Returns deep Data Structures state for viva presentation."""
        clean_expired_locks()
        sync_data_structures_from_db()
        
        all_seats = seat_matrix.get_all_seats()
        matrix_repr = []
        for r in seat_matrix.ROWS:
            row_items = [seat_matrix.get_cell(r, c) for c in range(1, seat_matrix.COLS + 1)]
            matrix_repr.append({"row": r, "seats": row_items})

        self._send_json({
            "data_structures": {
                "seat_matrix_2d": {
                    "name": "2D Matrix / Array Grid [5][6]",
                    "dimensions": "5 Rows (A-E) x 6 Columns (1-6) = 30 Seats",
                    "time_complexity": "O(1) index-based access: seats[row_idx][col_idx]",
                    "grid": matrix_repr
                },
                "seat_hash_map": {
                    "name": "Hash Map (Key-Value Dictionary)",
                    "purpose": "O(1) Instant Seat Lookup by Code (e.g. 'C4')",
                    "keys_count": len(seat_hash_map.map)
                },
                "lock_priority_queue": {
                    "name": "Min-Heap Priority Queue (Lock Expiration)",
                    "purpose": "Sorts active locks by earliest expiration time (5 min lock)",
                    "time_complexity": "O(log N) insertion & extraction",
                    "active_locks": lock_priority_queue.size(),
                    "next_expiry": lock_priority_queue.peek_next_expiry()
                },
                "fifo_booking_queue": {
                    "name": "FIFO Queue (Booking Transactions)",
                    "purpose": "Sequential First-Come-First-Served booking execution",
                    "time_complexity": "O(1) enqueue / dequeue",
                    "pending_requests": booking_fifo_queue.size()
                },
                "audit_linked_list": {
                    "name": "Doubly Linked List (Audit & Rollback Log)",
                    "purpose": "Maintains chronological order of locks, bookings, and cancellations",
                    "total_events": audit_linked_list.count,
                    "recent_logs": audit_linked_list.to_list()[:10]
                }
            }
        })

    def handle_reset_database(self):
        """Resets the database back to clean sample demo state."""
        conn = get_connection()
        cursor = conn.cursor()
        cursor.execute("DROP TABLE IF EXISTS seats")
        cursor.execute("DROP TABLE IF EXISTS bookings")
        cursor.execute("DROP TABLE IF EXISTS movie")
        conn.commit()
        conn.close()
        
        init_database()
        sync_data_structures_from_db()
        self._send_json({"success": True, "message": "Database and Data Structures reset to demo state!"})

def run_server():
    init_database()
    sync_data_structures_from_db()
    
    server_address = ('', PORT)
    # Allow port reuse
    socketserver.TCPServer.allow_reuse_address = True
    
    print("=" * 60)
    print(" CineBook - College Data Structures Mini Project (C++ Engine)")
    print("=" * 60)
    print(f" Database:   {DB_PATH}")
    print(f" Web UI:     http://localhost:{PORT}")
    print(f" REST API:   http://localhost:{PORT}/api/seats")
    print(f" C++ Demo:   run_cpp_demo.bat (Double-click for Viva)")
    print(" Press Ctrl+C to stop the server")
    print("=" * 60)
    
    with socketserver.TCPServer(server_address, CineBookHandler) as httpd:
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\nShutting down CineBook server...")
            httpd.server_close()

if __name__ == '__main__':
    run_server()
