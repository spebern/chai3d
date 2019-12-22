use crossbeam::{unbounded, Receiver, Sender};
use postgres::params::{ConnectParams, Host};
use postgres::{Connection, TlsMode};
use std::thread;
use time::{self, PreciseTime, Timespec};

#[repr(u8)]
#[derive(Debug, ToSql, FromSql, Clone)]
pub enum Device {
    Master,
    Slave,
}

#[repr(u8)]
#[derive(Debug, ToSql, FromSql, Clone)]
pub enum ControlAlgorithm {
    None,
    WAVE,
    ISS,
    PC,
    MMT,
}

#[repr(u8)]
#[derive(Debug, ToSql, FromSql, Clone)]
pub enum OptimisationParameter {
    PacketRate,
    Delay,
}

impl Default for ControlAlgorithm {
    fn default() -> Self {
        ControlAlgorithm::None
    }
}

#[repr(u8)]
#[derive(Debug, ToSql, FromSql)]
pub enum Gender {
    Male,
    Female,
}

#[repr(u8)]
#[derive(Debug, ToSql, FromSql)]
pub enum Handedness {
    Right,
    Left,
}

#[repr(C)]
#[derive(Debug)]
pub struct Session {
    id: i32,
    subject: Subject,
}

#[derive(Debug)]
pub struct Subject {
    id: i32,
    age: i32,
    gender: Gender,
    handedness: Handedness,
}

#[repr(C)]
#[derive(Clone, Debug)]
pub struct State {
    vel: [f64; 3],
    pos: [f64; 3],
    force: [f64; 3],
    device: Device,
    master_update: bool,
    slave_update: bool,
}

fn handle_haptic_states(rx: Receiver<(bool, Timespec, State)>, conn: Connection) {
    thread::spawn(move || {
        let stmt = conn
            .prepare(
                r#"
            INSERT INTO haptic.state
            (device, pos, vel, force, is_reference, time, master_update, slave_update)
            VALUES ($1, $2, $3, $4, $5, $6, $7, $8)
        "#,
            )
            .unwrap();
        for (is_reference, now, state) in rx {
            stmt.execute(&[
                &state.device,
                &&state.pos[..],
                &&state.vel[..],
                &&state.force[..],
                &is_reference,
                &now,
                &state.master_update,
                &state.slave_update,
            ]).expect("failed to insert haptic state");
        }
    });
}

fn handle_target_positions(rx: Receiver<(Timespec, [f64; 3])>, conn: Connection) {
    thread::spawn(move || {
        let stmt = conn
            .prepare(
                r#"
            INSERT INTO haptic.target_position
            (time, pos)
            VALUES ($1, $2)
        "#,
            )
            .unwrap();
        for (now,  pos) in rx {
            stmt.execute(&[&now, &&pos[..]]).expect("failed to insert haptic position");
        }
    });
}

#[repr(C)]
pub struct DB {
    tx_haptic_states: Sender<(bool, Timespec, State)>,
    tx_target_positions: Sender<(Timespec, [f64; 3])>,
    conn: Connection,
    session: Session,
    start: Timespec,
    precise_start: PreciseTime,
}

impl DB {
    fn connect(db_name: Option<&str>) -> Connection {
        let params = ConnectParams::builder()
            .user("haptic2", Some("abc123"))
            .database(db_name.unwrap_or("haptic2"))
            .build(Host::Tcp("localhost".to_string()));
        Connection::connect(params, TlsMode::None).expect("failed to connect to database")
    }

    fn new_subject(
        &mut self,
        nickname: &str,
        age: i32,
        gender: Gender,
        handedness: Handedness,
    ) -> Subject {
        let stmt = self
            .conn
            .prepare(
                r#"
                    INSERT INTO haptic.subject 
                        (nickname, age, gender, handedness) VALUES 
                        ($1, $2, $3, $4) 
                    ON CONFLICT (nickname) 
                    DO UPDATE SET 
                        age        = EXCLUDED.age, 
                        gender     = EXCLUDED.gender, 
                        handedness = EXCLUDED.handedness
                    RETURNING id"#,
            )
            .expect("failed to create subject");
        let rows = stmt
            .query(&[&nickname, &age, &gender, &handedness])
            .expect("failed to create session");
        Subject {
            id: rows.get(0).get(0),
            age,
            gender,
            handedness,
        }
    }

    pub fn new_session(
        &mut self,
        nickname: &str,
        age: i32,
        gender: Gender,
        handedness: Handedness,
    ) {
        let subject = self.new_subject(nickname, age, gender, handedness);
        let stmt = self
            .conn
            .prepare("INSERT INTO haptic.session (subject_id) VALUES ($1) RETURNING id")
            .expect("failed to create session");
        let rows = stmt
            .query(&[&subject.id])
            .expect("failed to create session");
        let id = rows.get(0).get(0);
        self.session = Session {
            id,
            subject,
        };
    }

    pub fn new(db_name: Option<&str>, num_state_writer_threads: u8) -> Self {
        let (tx_haptic_states, rx_haptic_states) = unbounded();
        for _ in 0..num_state_writer_threads {
            handle_haptic_states(rx_haptic_states.clone(), DB::connect(db_name));
        }
        let (tx_target_positions, rx_target_positions) = unbounded();
        handle_target_positions(rx_target_positions.clone(), DB::connect(db_name));

        Self {
            tx_haptic_states,
            tx_target_positions,
            conn: Self::connect(db_name),
            start: time::now_utc().to_timespec(),
            precise_start: time::PreciseTime::now(),
            session: Session {
                id: 0,
                subject: Subject {
                    id: 0,
                    age: 0,
                    gender: Gender::Male,
                    handedness: Handedness::Right,
                },
            },
        }
    }

    fn timestamp(&self) -> time::Timespec {
        let passed = self.precise_start.to(time::PreciseTime::now());
        self.start + passed
    }

    pub fn insert_haptic_state(&self, is_reference: bool, state: State) {
        let now = self.timestamp();
        self.tx_haptic_states
            .send((is_reference, now, state))
            .expect("failed to queue haptic master state");
    }

    pub fn insert_rating(&mut self, control_algorithm: ControlAlgorithm, packet_rate: i32, delay: i32, optimisation_parameter: OptimisationParameter, rating: i32) {
        let stmt = self
            .conn
            .prepare(
                r#"
                    INSERT INTO haptic.rating
                        (session_id, control_algorithm, packet_rate, delay, optimisation_parameter, rating) VALUES
                        ($1, $2, $3, $4, $5, $6)
                    RETURNING id"#,
            )
            .expect("failed to create insert rating stmt");

        let session_id = self.session.id;
        stmt
            .execute(&[&session_id, &control_algorithm, &packet_rate, &delay, &optimisation_parameter, &rating])
            .expect("failed to insert rating");
    }

    #[cfg(test)]
    fn delete_subjects(&self) {
        self.conn
            .execute("DELETE FROM haptic.subject", &[])
            .expect("failed to delete subjects");
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn basic() {
        let db_name = "test_haptic2";
        let mut db = DB::new(Some(db_name), 1);

        db.new_session(
            "ben",
            25,
            Gender::Male,
            Handedness::Right,
        );

        db.new_subject("ben", 12, Gender::Male, Handedness::Right);

        let state = State {
            vel: [0.1, 0.2, 0.3],
            pos: [0.4, 0.5, 0.6],
            force: [0.7, 0.8, 0.9],
            device: Device::Master,
            master_update: true,
            slave_update: true,
        };
        db.insert_haptic_state(true, state.clone());
        db.insert_haptic_state(true, state.clone());
        db.insert_haptic_state(true, state.clone());
        db.insert_haptic_state(true, state.clone());

        db.insert_rating(ControlAlgorithm::ISS, 10, 10, OptimisationParameter::Delay, 5);
        db.insert_rating(ControlAlgorithm::None, 10, 10, OptimisationParameter::PacketRate, 5);

        std::thread::sleep(std::time::Duration::from_secs(2));

        // db.delete_subjects();
    }
}
