"""
Database access and initialization for CineBook Mini Project.
SQLite handles persistent storage, synced with in-memory Data Structures.
"""

import sqlite3
import os
import time

DB_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "database"))
DB_PATH = os.path.join(DB_DIR, "cinebook.db")

def get_connection():
    os.makedirs(DB_DIR, exist_ok=True)
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    return conn

def init_database():
    conn = get_connection()
    cursor = conn.cursor()

    # Create Movie Info Table
    cursor.execute("""
    CREATE TABLE IF NOT EXISTS movie (
        id INTEGER PRIMARY KEY,
        title TEXT NOT NULL,
        tagline TEXT,
        rating TEXT,
        duration TEXT,
        genre TEXT,
        release_year INTEGER,
        synopsis TEXT,
        poster_url TEXT
    )
    """)

    # Create Seats Table
    cursor.execute("""
    CREATE TABLE IF NOT EXISTS seats (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        seat_code TEXT UNIQUE NOT NULL,
        row_label TEXT NOT NULL,
        col_number INTEGER NOT NULL,
        price REAL NOT NULL,
        status TEXT NOT NULL CHECK(status IN ('AVAILABLE', 'LOCKED', 'BOOKED')),
        locked_until REAL,
        locked_by TEXT,
        booking_id TEXT
    )
    """)

    # Create Bookings Table
    cursor.execute("""
    CREATE TABLE IF NOT EXISTS bookings (
        booking_id TEXT PRIMARY KEY,
        movie_title TEXT NOT NULL,
        seat_code TEXT NOT NULL,
        customer_name TEXT DEFAULT 'Student / Guest',
        show_date TEXT NOT NULL,
        showtime TEXT NOT NULL,
        ticket_price REAL NOT NULL,
        tax_amount REAL NOT NULL,
        total_price REAL NOT NULL,
        status TEXT NOT NULL CHECK(status IN ('CONFIRMED', 'CANCELLED')),
        created_at TEXT NOT NULL,
        cancelled_at TEXT
    )
    """)

    # Seed Movie Details
    cursor.execute("SELECT COUNT(*) FROM movie")
    if cursor.fetchone()[0] == 0:
        cursor.execute("""
        INSERT INTO movie (id, title, tagline, rating, duration, genre, release_year, synopsis, poster_url)
        VALUES (
            1,
            'Avengers: Endgame',
            'Part of the journey is the end.',
            'IMDb 8.4/10 | PG-13',
            '3h 01m (181 mins)',
            'Action / Sci-Fi / Adventure',
            2019,
            'After the devastating events of Infinity War, the universe is in ruins. With the help of remaining allies, the Avengers assemble once more to reverse Thanos'' actions and restore balance.',
            'assets/avengers_poster.svg'
        )
        """)

    # Seed Seats (30 seats: Rows A-E, Cols 1-6)
    cursor.execute("SELECT COUNT(*) FROM seats")
    if cursor.fetchone()[0] == 0:
        rows = ['A', 'B', 'C', 'D', 'E']
        cols = 6
        
        # Pre-seed realistic booked seats: B2, B3, D4
        sample_booked = {'B2', 'B3', 'D4'}
        
        seats_to_insert = []
        for r in rows:
            for c in range(1, cols + 1):
                code = f"{r}{c}"
                price = 350.0 if r == 'E' else (250.0 if r in ['C', 'D'] else 180.0)
                status = 'BOOKED' if code in sample_booked else 'AVAILABLE'
                booking_id = f"CB-PRE-{code}" if code in sample_booked else None
                seats_to_insert.append((code, r, c, price, status, None, None, booking_id))
        
        cursor.executemany("""
        INSERT INTO seats (seat_code, row_label, col_number, price, status, locked_until, locked_by, booking_id)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?)
        """, seats_to_insert)

        # Insert corresponding pre-booked records
        for code in sample_booked:
            price = 250.0 if code.startswith('D') else 180.0
            tax = round(price * 0.18, 2)
            total = price + tax
            cursor.execute("""
            INSERT OR IGNORE INTO bookings (booking_id, movie_title, seat_code, customer_name, show_date, showtime, ticket_price, tax_amount, total_price, status, created_at)
            VALUES (?, 'Avengers: Endgame', ?, 'Pre-booked Customer', 'Today (25 Oct)', '07:30 PM', ?, ?, ?, 'CONFIRMED', datetime('now'))
            """, (f"CB-PRE-{code}", code, price, tax, total))

    conn.commit()
    conn.close()

def clean_expired_locks():
    """Unlocks any seats whose 5-minute lock has expired."""
    now = time.time()
    conn = get_connection()
    cursor = conn.cursor()
    cursor.execute("""
    UPDATE seats
    SET status = 'AVAILABLE', locked_until = NULL, locked_by = NULL
    WHERE status = 'LOCKED' AND locked_until IS NOT NULL AND locked_until <= ?
    """, (now,))
    unlocked_count = cursor.rowcount
    conn.commit()
    conn.close()
    return unlocked_count

if __name__ == '__main__':
    init_database()
    print("Database initialized successfully at:", DB_PATH)
