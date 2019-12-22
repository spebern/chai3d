use crate::db::{ControlAlgorithm, Gender, Handedness, State, DB, OptimisationParameter};

use libc::c_char;
use std::ffi::CStr;

#[no_mangle]
pub extern "C" fn db_new(num_state_writer_threads: u8) -> *mut DB {
    let db = DB::new(None, num_state_writer_threads);
    Box::into_raw(Box::new(db))
}

#[no_mangle]
pub unsafe extern "C" fn db_new_session(
    db: *mut DB,
    nickname: *const c_char,
    age: i32,
    gender: Gender,
    handedness: Handedness,
) {
    assert!(!db.is_null());
    let db = &mut *db;

    let nickname: &CStr = CStr::from_ptr(nickname);
    let nickname: &str = nickname.to_str().expect("failed to convert c string");

    db.new_session(
        nickname,
        age,
        gender,
        handedness,
    );
}

#[no_mangle]
pub unsafe extern "C" fn db_insert_rating(
    db: *mut DB,
    control_algorithm: ControlAlgorithm,
    packet_rate: i32,
    delay: i32,
    optimisation_parameter: OptimisationParameter,
    rating: i32,
) {
    assert!(!db.is_null());
    let db = &mut *db;

    db.insert_rating(control_algorithm, packet_rate, delay, optimisation_parameter, rating);
}

#[no_mangle]
pub unsafe extern "C" fn db_free(db: *mut DB) {
    if !db.is_null() {
        Box::from_raw(db);
    }
}

#[no_mangle]
pub unsafe extern "C" fn db_insert_haptic_state(db: *mut DB, is_reference: bool, state: State) {
    assert!(!db.is_null());
    let db = &*db;

    db.insert_haptic_state(is_reference, state);
}

