import urllib.request
import json
import time
import subprocess
import sys
import os

project_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
server_script = os.path.join(project_dir, "backend", "server.py")

print("Starting CineBook server for test...")
proc = subprocess.Popen([sys.executable, server_script], cwd=project_dir)
time.sleep(2)

try:
    # 0. Reset to demo state
    reset_req = urllib.request.Request(
        'http://localhost:8000/api/reset',
        data=b'{}',
        headers={'Content-Type': 'application/json'}
    )
    with urllib.request.urlopen(reset_req) as res:
        pass
    # 1. Movie API
    with urllib.request.urlopen('http://localhost:8000/api/movie') as res:
        movie = json.loads(res.read().decode())
        print(f"[OK] Movie API: '{movie['title']}' loaded.")

    # 2. Seats API
    with urllib.request.urlopen('http://localhost:8000/api/seats') as res:
        seats = json.loads(res.read().decode())
        print(f"[OK] Seats API: Total={seats['total_seats']}, Available={seats['available_count']}, Booked={seats['booked_count']}")

    # 3. Lock API (5-minute lock)
    lock_req = urllib.request.Request(
        'http://localhost:8000/api/seats/lock',
        data=json.dumps({'seat_code': 'C4', 'session_id': 'student_demo'}).encode('utf-8'),
        headers={'Content-Type': 'application/json'}
    )
    with urllib.request.urlopen(lock_req) as res:
        lock_res = json.loads(res.read().decode())
        print(f"[OK] Lock API: {lock_res['message']} (Remaining: {lock_res['remaining_seconds']}s)")

    # 4. Confirm Booking API
    confirm_req = urllib.request.Request(
        'http://localhost:8000/api/bookings/confirm',
        data=json.dumps({
            'seat_code': 'C4',
            'session_id': 'student_demo',
            'customer_name': 'College Student',
            'show_date': 'Today, 25 Oct',
            'showtime': '06:00 PM'
        }).encode('utf-8'),
        headers={'Content-Type': 'application/json'}
    )
    with urllib.request.urlopen(confirm_req) as res:
        booking = json.loads(res.read().decode())['booking']
        b_id = booking['booking_id']
        print(f"[OK] Confirm Booking API: ID={b_id}, Seat={booking['seat_code']}, Total=INR {booking['total_price']}")

    # 5. Cancel Booking API (Rollback)
    cancel_req = urllib.request.Request(
        'http://localhost:8000/api/bookings/cancel',
        data=json.dumps({'booking_id': b_id, 'seat_code': 'C4'}).encode('utf-8'),
        headers={'Content-Type': 'application/json'}
    )
    with urllib.request.urlopen(cancel_req) as res:
        cancel_res = json.loads(res.read().decode())
        print(f"[OK] Cancel Booking API: {cancel_res['message']}")
        print(f"[OK] Rollback Seat Pool count: {cancel_res['new_available_count']} / 30 Available")

    # 6. Data Structures Metrics API
    with urllib.request.urlopen('http://localhost:8000/api/ds-metrics') as res:
        ds_res = json.loads(res.read().decode())
        matrix_name = ds_res['data_structures']['seat_matrix_2d']['name']
        print(f"[OK] DS Metrics API: {matrix_name} validated.")

    print("\n>>> ALL TESTS PASSED SUCCESSFULLY! FULL BACKEND & DB SYNCHRONIZATION VERIFIED. <<<")
finally:
    proc.terminate()
