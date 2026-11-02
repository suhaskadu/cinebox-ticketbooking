/**
 * CineBook - Movie Ticket Booking System
 * Frontend Controller & Data Structures Demonstration
 */

// Application State
const state = {
  sessionId: 'user_' + Math.random().toString(36).substring(2, 9),
  selectedDate: 'Today, Oct 25',
  selectedTime: '06:00 PM',
  selectedSeat: null,
  lockExpiryTime: null,
  lockInterval: null,
  activeBooking: null,
  lastCancelledBooking: null,
  backendConnected: false,
  // Local fallback mock seats in case frontend is previewed directly via file://
  seatsData: []
};

// API Base URL
const API_BASE = window.location.origin.startsWith('http') ? window.location.origin : 'http://localhost:8000';

// ============================================================================
// INITIALIZATION
// ============================================================================
document.addEventListener('DOMContentLoaded', () => {
  initApp();
});

async function initApp() {
  checkBackendStatus();
  await loadMovieData();
  await loadSeats();

  // Poll seats every 5 seconds to sync lock expirations and changes
  setInterval(() => {
    if (state.backendConnected) {
      loadSeats(false);
    }
  }, 5000);
}

// ============================================================================
// SCREEN NAVIGATION (Allows easy preview of all 4 screens for presentation)
// ============================================================================
function navigateToScreen(screenId) {
  // Update nav tabs
  document.querySelectorAll('.tab-btn').forEach(btn => {
    btn.classList.remove('active');
  });
  const activeTab = document.getElementById(`tab-${screenId}`);
  if (activeTab) activeTab.classList.add('active');

  // Switch screens
  document.querySelectorAll('.screen-view').forEach(view => {
    view.classList.remove('active-screen');
  });

  const targetScreen = document.getElementById(screenId);
  if (targetScreen) {
    targetScreen.classList.add('active-screen');
    window.scrollTo({ top: 0, behavior: 'smooth' });
  }

  // If navigating directly to Screen 3 without a booking, generate sample demo data
  if (screenId === 'screen-confirm' && !state.activeBooking) {
    populateSampleConfirmation();
  }

  // If navigating directly to Screen 4 without a cancellation, generate sample demo data
  if (screenId === 'screen-cancelled' && !state.lastCancelledBooking) {
    populateSampleCancellation();
  }
}

// ============================================================================
// SCREEN 1: HOME PAGE INTERACTIONS
// ============================================================================
async function loadMovieData() {
  try {
    const res = await fetch(`${API_BASE}/api/movie`);
    if (res.ok) {
      const data = await res.json();
      document.getElementById('home-movie-title').textContent = data.title;
      document.getElementById('home-movie-synopsis').textContent = data.synopsis;
      state.backendConnected = true;
    }
  } catch (e) {
    console.log('Running in local standalone mode.');
  }
}

function selectDate(btn, dateStr) {
  document.querySelectorAll('.date-card-btn').forEach(b => b.classList.remove('selected'));
  btn.classList.add('selected');
  state.selectedDate = dateStr;
  updateSummaryDateTime();
}

function selectTime(btn, timeStr) {
  document.querySelectorAll('.time-card-btn').forEach(b => b.classList.remove('selected'));
  btn.classList.add('selected');
  state.selectedTime = timeStr;
  updateSummaryDateTime();
}

function updateSummaryDateTime() {
  const dtElem = document.getElementById('summary-datetime');
  if (dtElem) {
    dtElem.textContent = `${state.selectedDate} • ${state.selectedTime}`;
  }
}

function proceedToSeatSelection() {
  navigateToScreen('screen-seats');
  showToast('🍿 Please select your preferred seat in Audi 1', '🎬');
}

// ============================================================================
// SCREEN 2: SEAT SELECTION & 5-MINUTE LOCK MANAGEMENT
// ============================================================================
async function loadSeats(renderUI = true) {
  try {
    const res = await fetch(`${API_BASE}/api/seats`);
    if (res.ok) {
      const data = await res.json();
      state.backendConnected = true;
      updateBackendBadge(true);
      state.seatsData = data.seats;
      document.getElementById('avail-seat-badge').textContent = `Available: ${data.available_count} / ${data.total_seats}`;
      
      if (renderUI) {
        renderSeatsGrid(data.seats);
      }
      return;
    }
  } catch (err) {
    updateBackendBadge(false);
  }

  // Local fallback if backend is offline
  if (state.seatsData.length === 0) {
    generateLocalSeats();
  }
  if (renderUI) {
    renderSeatsGrid(state.seatsData);
  }
}

function generateLocalSeats() {
  const rows = ['A', 'B', 'C', 'D', 'E'];
  const sampleBooked = new Set(['B2', 'B3', 'D4']);
  const seats = [];

  for (const r of rows) {
    for (let c = 1; c <= 6; c++) {
      const code = `${r}${c}`;
      const price = r === 'E' ? 350.0 : (['C', 'D'].includes(r) ? 250.0 : 180.0);
      const isBooked = sampleBooked.has(code);
      seats.push({
        seat_code: code,
        row_label: r,
        col_number: c,
        price: price,
        status: isBooked ? 'BOOKED' : 'AVAILABLE',
        locked_until: null,
        locked_by: null
      });
    }
  }
  state.seatsData = seats;
  const availCount = seats.filter(s => s.status === 'AVAILABLE').length;
  document.getElementById('avail-seat-badge').textContent = `Available: ${availCount} / 30`;
}

function renderSeatsGrid(seats) {
  const gridContainer = document.getElementById('cinema-seats-grid');
  gridContainer.innerHTML = '';

  const rows = ['A', 'B', 'C', 'D', 'E'];

  rows.forEach(rowLetter => {
    const rowEl = document.createElement('div');
    rowEl.className = 'seat-row';

    // Row Label Pill
    const labelPill = document.createElement('div');
    labelPill.className = 'row-label-pill';
    labelPill.textContent = rowLetter;
    rowEl.appendChild(labelPill);

    // Group for row seats
    const groupEl = document.createElement('div');
    groupEl.className = 'row-seats-group';

    // Find all 6 seats in this row
    for (let c = 1; c <= 6; c++) {
      const code = `${rowLetter}${c}`;
      const seat = seats.find(s => s.seat_code === code) || {
        seat_code: code,
        row_label: rowLetter,
        col_number: c,
        price: 250,
        status: 'AVAILABLE'
      };

      const seatBtn = document.createElement('button');
      seatBtn.className = 'seat-item';
      seatBtn.dataset.code = code;
      seatBtn.dataset.price = seat.price;
      seatBtn.dataset.row = rowLetter;
      seatBtn.dataset.col = c;

      // Check current state
      let statusClass = seat.status.toLowerCase();

      // If locally selected
      if (state.selectedSeat && state.selectedSeat.code === code) {
        statusClass = 'selected';
      } else if (seat.status === 'LOCKED') {
        statusClass = 'locked';
      }

      seatBtn.classList.add(statusClass);
      seatBtn.innerHTML = `<span class="seat-num-text">${code}</span>`;

      // Aisle gap after 3rd seat
      if (c === 3) {
        seatBtn.classList.add('seat-gap');
      }

      // Click handler
      seatBtn.onclick = () => handleSeatClick(seat);

      groupEl.appendChild(seatBtn);
    }

    rowEl.appendChild(groupEl);
    gridContainer.appendChild(rowEl);
  });
}

async function handleSeatClick(seat) {
  const code = seat.seat_code;

  // Cannot click booked seats
  if (seat.status === 'BOOKED') {
    showToast(`Seat ${code} is already booked! Please select an available green seat.`, '❌');
    return;
  }

  // Cannot click seats locked by others
  if (seat.status === 'LOCKED' && (!state.selectedSeat || state.selectedSeat.code !== code)) {
    showToast(`Seat ${code} is temporarily locked by another user! Try another seat.`, '⏳');
    return;
  }

  // If clicking currently selected seat, deselect and unlock it
  if (state.selectedSeat && state.selectedSeat.code === code) {
    cancelCurrentSeatSelection();
    return;
  }

  // If user already had a different seat locked, unlock that one first
  if (state.selectedSeat && state.selectedSeat.code !== code) {
    await unlockPreviousSeat(state.selectedSeat.code);
  }

  // Lock this new seat for 5 minutes
  await lockSeat(seat);
}

async function lockSeat(seat) {
  const code = seat.seat_code;

  if (state.backendConnected) {
    try {
      const res = await fetch(`${API_BASE}/api/seats/lock`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ seat_code: code, session_id: state.sessionId })
      });
      const data = await res.json();
      if (!data.success) {
        showToast(data.message, '⚠️');
        return;
      }
    } catch (e) {
      console.log('Using local lock mechanism');
    }
  }

  // Set seat as locally selected
  state.selectedSeat = {
    code: code,
    row: seat.row_label,
    col: seat.col_number,
    price: seat.price
  };

  // Start 5-Minute (300 seconds) Countdown
  const lockDurationSeconds = 300;
  state.lockExpiryTime = Date.now() + (lockDurationSeconds * 1000);
  startLockTimer(lockDurationSeconds);

  // Update Summary Sidebar
  updateCheckoutSummary(seat);

  // Update UI grid
  document.querySelectorAll('.seat-item').forEach(btn => {
    if (btn.dataset.code === code) {
      btn.className = 'seat-item selected' + (btn.classList.contains('seat-gap') ? ' seat-gap' : '');
    } else if (btn.classList.contains('selected')) {
      btn.className = 'seat-item available' + (btn.classList.contains('seat-gap') ? ' seat-gap' : '');
    }
  });

  // Enable Confirm button
  document.getElementById('btn-confirm-booking').disabled = false;

  showToast(`Seat ${code} is temporarily locked for 5 minutes! Complete booking.`, '⏱️');
}

function startLockTimer(seconds) {
  clearInterval(state.lockInterval);
  const timerBox = document.getElementById('lock-timer-box');
  const timerDisplay = document.getElementById('lock-countdown-display');
  
  timerBox.style.display = 'flex';

  function update() {
    const diff = Math.max(0, Math.floor((state.lockExpiryTime - Date.now()) / 1000));
    const mins = Math.floor(diff / 60);
    const secs = diff % 60;
    timerDisplay.textContent = `${String(mins).padStart(2, '0')}:${String(secs).padStart(2, '0')}`;

    if (diff <= 0) {
      clearInterval(state.lockInterval);
      handleLockExpired();
    }
  }

  update();
  state.lockInterval = setInterval(update, 1000);
}

function handleLockExpired() {
  showToast('⚠️ 5-minute reservation time expired! Seat has been released.', '⏰');
  cancelCurrentSeatSelection(false);
}

async function unlockPreviousSeat(code) {
  if (state.backendConnected) {
    try {
      await fetch(`${API_BASE}/api/seats/unlock`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ seat_code: code, session_id: state.sessionId })
      });
    } catch (e) {}
  }
}

async function cancelCurrentSeatSelection(notify = true) {
  if (state.selectedSeat) {
    await unlockPreviousSeat(state.selectedSeat.code);
  }

  clearInterval(state.lockInterval);
  state.selectedSeat = null;
  state.lockExpiryTime = null;

  document.getElementById('lock-timer-box').style.display = 'none';
  document.getElementById('summary-seat-label').textContent = 'None (Click a green seat)';
  document.getElementById('summary-seat-label').style.color = '#facc15';
  document.getElementById('summary-tier').textContent = 'Standard Class';
  document.getElementById('summary-base-price').textContent = '₹0.00';
  document.getElementById('summary-tax').textContent = '₹0.00';
  document.getElementById('summary-total-price').textContent = '₹0.00';
  document.getElementById('btn-confirm-booking').disabled = true;

  loadSeats();
  if (notify) {
    showToast('Seat selection cancelled. Seat is available.', 'ℹ️');
  }
}

function updateCheckoutSummary(seat) {
  const code = seat.seat_code;
  const row = seat.row_label;
  const basePrice = seat.price;
  const tax = Math.round(basePrice * 0.18 * 100) / 100;
  const total = basePrice + tax;

  const tier = row === 'E' ? 'IMAX Club / Recliner' : (['C', 'D'].includes(row) ? 'Standard Prime' : 'Economy');

  document.getElementById('summary-seat-label').textContent = `Row ${row} • Seat ${seat.col_number} (${code})`;
  document.getElementById('summary-seat-label').style.color = '#34d399';
  document.getElementById('summary-tier').textContent = tier;
  document.getElementById('summary-base-price').textContent = `₹${basePrice.toFixed(2)}`;
  document.getElementById('summary-tax').textContent = `₹${tax.toFixed(2)}`;
  document.getElementById('summary-total-price').textContent = `₹${total.toFixed(2)}`;
}

// ============================================================================
// SCREEN 3: CONFIRM BOOKING & DISPLAY TICKET
// ============================================================================
async function confirmSelectedSeatBooking() {
  if (!state.selectedSeat) return;

  const seatCode = state.selectedSeat.code;
  const btn = document.getElementById('btn-confirm-booking');
  btn.disabled = true;
  btn.textContent = 'Processing Transaction...';

  let bookingData = null;

  if (state.backendConnected) {
    try {
      const res = await fetch(`${API_BASE}/api/bookings/confirm`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          seat_code: seatCode,
          session_id: state.sessionId,
          customer_name: 'Student / College Demo',
          show_date: state.selectedDate,
          showtime: state.selectedTime
        })
      });
      const data = await res.json();
      if (data.success) {
        bookingData = data.booking;
      } else {
        showToast(data.message, '❌');
        btn.disabled = false;
        btn.textContent = 'Confirm Booking';
        return;
      }
    } catch (e) {
      console.log('Falling back to local confirmation');
    }
  }

  // Local mock booking fallback
  if (!bookingData) {
    const base = state.selectedSeat.price;
    const tax = Math.round(base * 0.18 * 100) / 100;
    bookingData = {
      booking_id: `CB-${Math.floor(10000 + Math.random() * 90000)}-${seatCode}`,
      movie_title: 'Avengers: Endgame',
      seat_code: seatCode,
      row: state.selectedSeat.row,
      seat_num: state.selectedSeat.col,
      customer_name: 'Student / College Demo',
      show_date: state.selectedDate,
      showtime: state.selectedTime,
      ticket_price: base,
      tax_amount: tax,
      total_price: base + tax,
      status: 'CONFIRMED',
      created_at: new Date().toLocaleTimeString('en-US', { hour: '2-digit', minute: '2-digit' })
    };
  }

  clearInterval(state.lockInterval);
  document.getElementById('lock-timer-box').style.display = 'none';

  state.activeBooking = bookingData;
  state.selectedSeat = null;

  // Populate Screen 3 Ticket details
  document.getElementById('ticket-booking-id').textContent = bookingData.booking_id;
  document.getElementById('ticket-movie-title').textContent = bookingData.movie_title;
  document.getElementById('ticket-show-date').textContent = bookingData.show_date;
  document.getElementById('ticket-show-time').textContent = bookingData.showtime;
  document.getElementById('ticket-seat-code').textContent = `Row ${bookingData.row} • Seat ${bookingData.seat_num} (${bookingData.seat_code})`;
  document.getElementById('ticket-created-at').textContent = bookingData.created_at;
  document.getElementById('ticket-total-price').textContent = `₹${bookingData.total_price.toFixed(2)}`;
  document.getElementById('ticket-base-price').textContent = `₹${bookingData.ticket_price.toFixed(2)}`;
  document.getElementById('ticket-tax-price').textContent = `₹${bookingData.tax_amount.toFixed(2)}`;

  btn.disabled = false;
  btn.textContent = 'Confirm Booking';

  // Navigate to Screen 3
  navigateToScreen('screen-confirm');
  showToast(`🎉 Booking Successful! Booking ID: ${bookingData.booking_id}`, '🎟️');

  // Reload seats to reflect the new red booked seat
  loadSeats(true);
}

function populateSampleConfirmation() {
  document.getElementById('ticket-booking-id').textContent = 'CB-84920-C4';
  document.getElementById('ticket-movie-title').textContent = 'Avengers: Endgame';
  document.getElementById('ticket-show-date').textContent = state.selectedDate;
  document.getElementById('ticket-show-time').textContent = state.selectedTime;
  document.getElementById('ticket-seat-code').textContent = 'Row C • Seat 4 (C4)';
  document.getElementById('ticket-created-at').textContent = 'Today, 12:08 PM';
  document.getElementById('ticket-total-price').textContent = '₹295.00';
  document.getElementById('ticket-base-price').textContent = '₹250.00';
  document.getElementById('ticket-tax-price').textContent = '₹45.00';
}

// ============================================================================
// SCREEN 4: CANCEL BOOKING & DEMONSTRATE ROLLBACK
// ============================================================================
async function cancelActiveBookingFromConfirmation() {
  const booking = state.activeBooking || {
    booking_id: 'CB-84920-C4',
    seat_code: 'C4',
    total_price: 295.00
  };

  if (state.backendConnected) {
    try {
      const res = await fetch(`${API_BASE}/api/bookings/cancel`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ booking_id: booking.booking_id, seat_code: booking.seat_code })
      });
      const data = await res.json();
      if (data.success) {
        state.lastCancelledBooking = data.cancelled_booking;
        document.getElementById('cancelled-avail-count').textContent = `${data.new_available_count} / 30 Available`;
      }
    } catch (e) {
      console.log('Falling back to local cancel');
    }
  }

  // Populate Screen 4 UI
  document.getElementById('cancelled-seat-code-prev').textContent = booking.seat_code;
  document.getElementById('cancelled-seat-code-now').textContent = booking.seat_code;

  state.lastCancelledBooking = booking;
  state.activeBooking = null;

  navigateToScreen('screen-cancelled');
  showToast(`Booking ${booking.booking_id} cancelled. Seat ${booking.seat_code} is available again!`, '🔄');

  // Refresh seats to verify seat has turned green available
  loadSeats(true);
}

function populateSampleCancellation() {
  document.getElementById('cancelled-seat-code-prev').textContent = 'C4';
  document.getElementById('cancelled-seat-code-now').textContent = 'C4';
  document.getElementById('cancelled-avail-count').textContent = '27 / 30 Available';
}

// ============================================================================
// DATA STRUCTURES MODAL CONTROLS
// ============================================================================
function openDSModal() {
  document.getElementById('ds-modal-overlay').classList.add('open');
}

function closeDSModal() {
  document.getElementById('ds-modal-overlay').classList.remove('open');
}

// Close modal when clicking outside
window.onclick = function(event) {
  const modal = document.getElementById('ds-modal-overlay');
  if (event.target === modal) {
    closeDSModal();
  }
};

// ============================================================================
// UTILITIES
// ============================================================================
function showToast(message, icon = 'ℹ️') {
  const toast = document.getElementById('toast-notif');
  const msgEl = document.getElementById('toast-message');
  const iconEl = document.getElementById('toast-icon');

  if (!toast) return;

  msgEl.textContent = message;
  iconEl.textContent = icon;
  toast.style.display = 'flex';

  clearTimeout(toast._timeout);
  toast._timeout = setTimeout(() => {
    toast.style.display = 'none';
  }, 4000);
}

function updateBackendBadge(connected) {
  const pill = document.getElementById('backend-status-pill');
  const text = document.getElementById('backend-status-text');
  const dot = pill.querySelector('.status-dot');

  if (connected) {
    pill.style.borderColor = 'rgba(16, 185, 129, 0.4)';
    text.textContent = 'SQLite Connected';
    dot.style.background = '#10b981';
    dot.style.boxShadow = '0 0 10px #10b981';
  } else {
    pill.style.borderColor = 'rgba(245, 158, 11, 0.4)';
    text.textContent = 'Demo Mode (Local)';
    dot.style.background = '#f59e0b';
    dot.style.boxShadow = '0 0 10px #f59e0b';
  }
}

async function checkBackendStatus() {
  try {
    const res = await fetch(`${API_BASE}/api/seats`);
    if (res.ok) {
      state.backendConnected = true;
      updateBackendBadge(true);
    }
  } catch (e) {
    state.backendConnected = false;
    updateBackendBadge(false);
  }
}
