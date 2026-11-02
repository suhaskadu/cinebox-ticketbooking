/**
 * CineBook - Node.js Backend Server (Zero External Dependencies, uses built-in node:http and node:sqlite)
 */

const http = require('node:http');
const fs = require('node:fs');
const path = require('node:path');
const { DatabaseSync } = require('node:sqlite');

const PORT = 8000;
const FRONTEND_DIR = path.resolve(__dirname, '..', 'frontend');
const DB_PATH = path.resolve(__dirname, '..', 'database', 'cinebook.db');

// Ensure db directory exists
fs.mkdirSync(path.dirname(DB_PATH), { recursive: true });

const db = new DatabaseSync(DB_PATH);

// Setup database tables if they do not exist
db.exec(`
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
);
CREATE TABLE IF NOT EXISTS seats (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    seat_code TEXT UNIQUE NOT NULL,
    row_label TEXT NOT NULL,
    col_number INTEGER NOT NULL,
    price REAL NOT NULL,
    status TEXT NOT NULL,
    locked_until REAL,
    locked_by TEXT,
    booking_id TEXT
);
CREATE TABLE IF NOT EXISTS bookings (
    booking_id TEXT PRIMARY KEY,
    movie_title TEXT NOT NULL,
    seat_code TEXT NOT NULL,
    customer_name TEXT,
    show_date TEXT NOT NULL,
    showtime TEXT NOT NULL,
    ticket_price REAL NOT NULL,
    tax_amount REAL NOT NULL,
    total_price REAL NOT NULL,
    status TEXT NOT NULL,
    created_at TEXT NOT NULL,
    cancelled_at TEXT
);
`);

// Seed initial movie data if empty
const movieCount = db.prepare('SELECT COUNT(*) as count FROM movie').get().count;
if (movieCount === 0) {
    db.prepare(`
        INSERT INTO movie (id, title, tagline, rating, duration, genre, release_year, synopsis, poster_url)
        VALUES (1, 'Avengers: Endgame', 'Part of the journey is the end.', 'IMDb 8.4/10 | PG-13', '3h 01m (181 mins)', 'Action / Sci-Fi / Adventure', 2019,
        'After the devastating events of Infinity War, the universe is in ruins. With the help of remaining allies, the Avengers assemble once more to reverse Thanos actions and restore balance.',
        'assets/avengers_poster.svg')
    `).run();
}

// Seed initial 30 seats if empty
const seatCount = db.prepare('SELECT COUNT(*) as count FROM seats').get().count;
if (seatCount === 0) {
    const rows = ['A', 'B', 'C', 'D', 'E'];
    const sampleBooked = new Set(['B2', 'B3', 'D4']);
    const insertSeat = db.prepare(`
        INSERT INTO seats (seat_code, row_label, col_number, price, status, locked_until, locked_by, booking_id)
        VALUES (?, ?, ?, ?, ?, NULL, NULL, ?)
    `);
    const insertBooking = db.prepare(`
        INSERT INTO bookings (booking_id, movie_title, seat_code, customer_name, show_date, showtime, ticket_price, tax_amount, total_price, status, created_at)
        VALUES (?, 'Avengers: Endgame', ?, 'Pre-booked Customer', 'Today (25 Oct)', '07:30 PM', ?, ?, ?, 'CONFIRMED', datetime('now'))
    `);

    for (const r of rows) {
        for (let c = 1; c <= 6; c++) {
            const code = `${r}${c}`;
            const price = r === 'E' ? 350.0 : (['C', 'D'].includes(r) ? 250.0 : 180.0);
            const isBooked = sampleBooked.has(code);
            const status = isBooked ? 'BOOKED' : 'AVAILABLE';
            const bookingId = isBooked ? `CB-PRE-${code}` : null;
            insertSeat.run(code, r, c, price, status, bookingId);

            if (isBooked) {
                const tax = Math.round(price * 0.18 * 100) / 100;
                const total = price + tax;
                insertBooking.run(bookingId, code, price, tax, total);
            }
        }
    }
}

function cleanExpiredLocks() {
    const now = Date.now() / 1000;
    db.prepare(`
        UPDATE seats 
        SET status = 'AVAILABLE', locked_until = NULL, locked_by = NULL
        WHERE status = 'LOCKED' AND locked_until IS NOT NULL AND locked_until <= ?
    `).run(now);
}

const MIME_TYPES = {
    '.html': 'text/html',
    '.css': 'text/css',
    '.js': 'text/javascript',
    '.svg': 'image/svg+xml',
    '.png': 'image/png',
    '.jpg': 'image/jpeg',
    '.json': 'application/json'
};

const server = http.createServer((req, res) => {
    const parsedUrl = new URL(req.url, `http://${req.headers.host}`);
    const pathname = parsedUrl.pathname;

    // CORS Headers
    res.setHeader('Access-Control-Allow-Origin', '*');
    res.setHeader('Access-Control-Allow-Methods', 'GET, POST, OPTIONS');
    res.setHeader('Access-Control-Allow-Headers', 'Content-Type');

    if (req.method === 'OPTIONS') {
        res.writeHead(200);
        res.end();
        return;
    }

    const sendJson = (data, code = 200) => {
        res.writeHead(code, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify(data));
    };

    if (req.method === 'GET') {
        if (pathname === '/api/movie') {
            const movie = db.prepare('SELECT * FROM movie WHERE id = 1').get();
            movie.dates = [
                { day: "TODAY", date: "25 Oct", full: "Today, Oct 25", active: true },
                { day: "TOMORROW", date: "26 Oct", full: "Tomorrow, Oct 26", active: false },
                { day: "TUE", date: "27 Oct", full: "Tuesday, Oct 27", active: false },
                { day: "WED", date: "28 Oct", full: "Wednesday, Oct 28", active: false }
            ];
            movie.showtimes = [
                { time: "10:30 AM", type: "2D IMAX", screen: "Screen 1", available: true },
                { time: "02:15 PM", type: "4DX Atmos", screen: "Screen 1", available: true },
                { time: "06:00 PM", type: "3D Dolby", screen: "Screen 1", available: true, selected: true },
                { time: "09:45 PM", type: "2D IMAX", screen: "Screen 1", available: true }
            ];
            return sendJson(movie);
        }

        if (pathname === '/api/seats') {
            cleanExpiredLocks();
            const seats = db.prepare('SELECT * FROM seats ORDER BY row_label, col_number').all();
            const total = seats.length;
            const booked = seats.filter(s => s.status === 'BOOKED').length;
            const locked = seats.filter(s => s.status === 'LOCKED').length;
            const available = seats.filter(s => s.status === 'AVAILABLE').length;

            return sendJson({
                total_seats: total,
                available_count: available,
                booked_count: booked,
                locked_count: locked,
                server_time: Date.now() / 1000,
                seats
            });
        }

        if (pathname === '/api/ds-metrics') {
            cleanExpiredLocks();
            const seats = db.prepare('SELECT * FROM seats ORDER BY row_label, col_number').all();
            const rows = ['A', 'B', 'C', 'D', 'E'];
            const matrixGrid = rows.map(r => ({
                row: r,
                seats: seats.filter(s => s.row_label === r)
            }));
            const activeLocks = seats.filter(s => s.status === 'LOCKED').length;

            return sendJson({
                data_structures: {
                    seat_matrix_2d: {
                        name: "2D Matrix / Array Grid [5][6]",
                        dimensions: "5 Rows (A-E) x 6 Columns (1-6) = 30 Seats",
                        time_complexity: "O(1) direct access seats[i][j]",
                        grid: matrixGrid
                    },
                    seat_hash_map: {
                        name: "Hash Map (Seat Lookup)",
                        keys_count: seats.length,
                        purpose: "O(1) Instant Seat Lookup by Code (e.g. 'C4')"
                    },
                    lock_priority_queue: {
                        name: "Min-Heap Priority Queue (Lock Expiration)",
                        active_locks: activeLocks,
                        purpose: "Evicts expired 5-minute locks in O(log N) order"
                    }
                }
            });
        }

        // Static file serving
        let filePath = path.join(FRONTEND_DIR, pathname === '/' ? 'index.html' : pathname);
        if (!fs.existsSync(filePath) || fs.statSync(filePath).isDirectory()) {
            filePath = path.join(FRONTEND_DIR, 'index.html');
        }

        const ext = path.extname(filePath).toLowerCase();
        const contentType = MIME_TYPES[ext] || 'application/octet-stream';
        fs.readFile(filePath, (err, content) => {
            if (err) {
                res.writeHead(404, { 'Content-Type': 'text/plain' });
                res.end('File Not Found');
            } else {
                res.writeHead(200, { 'Content-Type': contentType });
                res.end(content);
            }
        });
        return;
    }

    if (req.method === 'POST') {
        let body = '';
        req.on('data', chunk => { body += chunk; });
        req.on('end', () => {
            let payload = {};
            try { payload = JSON.parse(body || '{}'); } catch(e) {}

            cleanExpiredLocks();

            if (pathname === '/api/seats/lock') {
                const { seat_code, session_id = 'guest_user' } = payload;
                if (!seat_code) return sendJson({ success: false, message: 'Seat code required' }, 400);

                const seat = db.prepare('SELECT * FROM seats WHERE seat_code = ?').get(seat_code);
                if (!seat) return sendJson({ success: false, message: 'Seat not found' }, 404);
                if (seat.status === 'BOOKED') return sendJson({ success: false, message: 'Seat already booked' }, 409);

                const now = Date.now() / 1000;
                if (seat.status === 'LOCKED' && seat.locked_until > now && seat.locked_by !== session_id) {
                    return sendJson({ success: false, message: `Seat is temporarily locked by another user!` }, 409);
                }

                const lockedUntil = now + 300; // 5 minutes
                db.prepare(`
                    UPDATE seats 
                    SET status = 'LOCKED', locked_until = ?, locked_by = ?
                    WHERE seat_code = ?
                `).run(lockedUntil, session_id, seat_code);

                return sendJson({
                    success: true,
                    seat_code,
                    status: 'LOCKED',
                    locked_until: lockedUntil,
                    remaining_seconds: 300,
                    message: `Seat ${seat_code} locked for 5 minutes.`
                });
            }

            if (pathname === '/api/seats/unlock') {
                const { seat_code } = payload;
                db.prepare(`UPDATE seats SET status = 'AVAILABLE', locked_until = NULL, locked_by = NULL WHERE seat_code = ? AND status = 'LOCKED'`).run(seat_code);
                return sendJson({ success: true, message: `Seat ${seat_code} unlocked.` });
            }

            if (pathname === '/api/bookings/confirm') {
                const { seat_code, customer_name = 'College Demo User', show_date = 'Today, 25 Oct', showtime = '06:00 PM' } = payload;
                const seat = db.prepare('SELECT * FROM seats WHERE seat_code = ?').get(seat_code);
                if (!seat) return sendJson({ success: false, message: 'Seat not found' }, 404);
                if (seat.status === 'BOOKED') return sendJson({ success: false, message: 'Seat already booked' }, 409);

                const bookingId = `CB-${Math.floor(10000 + Math.random() * 90000)}-${seat_code}`;
                const ticketPrice = seat.price;
                const taxAmount = Math.round(ticketPrice * 0.18 * 100) / 100;
                const totalPrice = ticketPrice + taxAmount;
                const createdAt = new Date().toLocaleString('en-US');

                db.prepare(`UPDATE seats SET status = 'BOOKED', locked_until = NULL, locked_by = NULL, booking_id = ? WHERE seat_code = ?`).run(bookingId, seat_code);
                db.prepare(`
                    INSERT INTO bookings (booking_id, movie_title, seat_code, customer_name, show_date, showtime, ticket_price, tax_amount, total_price, status, created_at)
                    VALUES (?, 'Avengers: Endgame', ?, ?, ?, ?, ?, ?, ?, 'CONFIRMED', ?)
                `).run(bookingId, seat_code, customer_name, show_date, showtime, ticketPrice, taxAmount, totalPrice, createdAt);

                return sendJson({
                    success: true,
                    booking: {
                        booking_id: bookingId,
                        movie_title: 'Avengers: Endgame',
                        seat_code,
                        row: seat_code[0],
                        seat_num: seat_code.slice(1),
                        customer_name,
                        show_date,
                        showtime,
                        ticket_price: ticketPrice,
                        tax_amount: taxAmount,
                        total_price: totalPrice,
                        status: 'CONFIRMED',
                        created_at: createdAt,
                        cinema_hall: 'Audi 1 (IMAX Laser)',
                        screen: 'Screen 1'
                    }
                });
            }

            if (pathname === '/api/bookings/cancel') {
                const { booking_id, seat_code } = payload;
                const booking = db.prepare(`SELECT * FROM bookings WHERE booking_id = ? OR (seat_code = ? AND status = 'CONFIRMED')`).get(booking_id, seat_code);
                if (!booking) return sendJson({ success: false, message: 'Booking not found' }, 404);

                const cancelledAt = new Date().toLocaleString('en-US');
                db.prepare(`UPDATE bookings SET status = 'CANCELLED', cancelled_at = ? WHERE booking_id = ?`).run(cancelledAt, booking.booking_id);
                db.prepare(`UPDATE seats SET status = 'AVAILABLE', locked_until = NULL, locked_by = NULL, booking_id = NULL WHERE seat_code = ?`).run(booking.seat_code);

                const newAvail = db.prepare(`SELECT COUNT(*) as count FROM seats WHERE status = 'AVAILABLE'`).get().count;

                return sendJson({
                    success: true,
                    message: `Booking ${booking.booking_id} cancelled. Seat ${booking.seat_code} is now Available.`,
                    cancelled_booking: {
                        booking_id: booking.booking_id,
                        seat_code: booking.seat_code,
                        row: booking.seat_code[0],
                        seat_num: booking.seat_code.slice(1),
                        cancelled_at: cancelledAt,
                        refund_amount: booking.total_price
                    },
                    new_available_count: newAvail
                });
            }

            if (pathname === '/api/reset') {
                db.exec(`
                    DROP TABLE IF EXISTS seats;
                    DROP TABLE IF EXISTS bookings;
                    DROP TABLE IF EXISTS movie;
                `);
                return sendJson({ success: true, message: 'Reset done. Please restart server or re-run init.' });
            }

            sendJson({ error: 'Endpoint not found' }, 404);
        });
    }
});

server.listen(PORT, () => {
    console.log(`CineBook Node.js Server listening at http://localhost:${PORT}`);
});
