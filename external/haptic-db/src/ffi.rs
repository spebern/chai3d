use crate::db::{ControlAlgorithm, Gender, Handedness, State, Trial, DB};

use libc::c_char;
use libc::size_t;
use std::ffi::CStr;

#[no_mangle]
pub extern "C" fn db_new() -> *mut DB {
    let db = DB::new(None);
    Box::into_raw(Box::new(db))
}

#[no_mangle]
pub unsafe extern "C" fn db_new_session(
    db: *mut DB,
    nickname: *const c_char,
    age: i32,
    gender: Gender,
    handedness: Handedness,
    packet_rates_data: *const i32,
    packet_rates_size: size_t,
    delays_data: *const i32,
    delays_size: size_t,
    control_algos_data: *const ControlAlgorithm,
    control_algos_size: size_t,
) {
    assert!(!db.is_null());
    let db = &mut *db;

    let nickname: &CStr = CStr::from_ptr(nickname);
    let nickname: &str = nickname.to_str().expect("failed to convert c string");

    let packet_rates = std::slice::from_raw_parts(packet_rates_data, packet_rates_size).to_vec();
    let delays = std::slice::from_raw_parts(delays_data, delays_size).to_vec();
    let control_algos = std::slice::from_raw_parts(control_algos_data, control_algos_size).to_vec();

    db.new_session(
        nickname,
        age,
        gender,
        handedness,
        packet_rates,
        delays,
        control_algos,
    );
}

#[no_mangle]
pub unsafe extern "C" fn db_rate_trial(db: *mut DB, rating: i32) {
    assert!(!db.is_null());
    let db = &mut *db;

    db.rate_trial(rating);
}

#[no_mangle]
pub unsafe extern "C" fn db_save_jnd(
    db: *mut DB,
    control_algo: ControlAlgorithm,
    packet_rate: i32,
) {
    assert!(!db.is_null());
    let db = &mut *db;

    db.save_jnd(control_algo, packet_rate);
}

#[no_mangle]
pub unsafe extern "C" fn db_current_trial(db: *mut DB) -> Trial {
    assert!(!db.is_null());
    let db = &mut *db;

    db.current_trial()
}

#[no_mangle]
pub unsafe extern "C" fn db_next_trial(db: *mut DB) -> bool {
    assert!(!db.is_null());
    let db = &mut *db;

    db.next_trial()
}

#[no_mangle]
pub unsafe extern "C" fn db_free(db: *mut DB) {
    if !db.is_null() {
        Box::from_raw(db);
    }
}

#[no_mangle]
pub unsafe extern "C" fn db_insert_haptic_state(
    db: *mut DB,
    now: u64,
    is_reference: bool,
    state: State,
) {
    assert!(!db.is_null());
    let db = &*db;

    db.insert_haptic_state(now, is_reference, state);
}
