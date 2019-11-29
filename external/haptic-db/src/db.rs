use chrono::prelude::*;
use crossbeam::{unbounded, Receiver, Sender};
use postgres::params::{ConnectParams, Host};
use postgres::{Connection, TlsMode};
use rand::prelude::*;
use std::thread;

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
    trials: Vec<Trial>,
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
}

#[repr(C)]
#[derive(Debug, Clone)]
pub struct Trial {
    id: i32,
    packet_rate: i32,
    delay: i32,
    session_id: i32,
    control_algo: ControlAlgorithm,
    rating: i32,
}

#[repr(C)]
#[derive(Debug, Clone)]
pub struct TrialInfo {
    packet_rate: i32,
    delay: i32,
    control_algos: [ControlAlgorithm; 4],
}

fn handle_haptic_states(rx: Receiver<(i32, bool, DateTime<Utc>, State)>, conn: Connection) {
    thread::spawn(move || {
        let stmt = conn
            .prepare(
                r#"
            INSERT INTO haptic.state
            (device, pos, vel, force, is_reference, trial_id)
            VALUES ($1, $2, $3, $4, $5, $6)
        "#,
            )
            .unwrap();
        for (trial_id, is_reference, now, state) in rx {
            let now = now.with_timezone(&Local);
            stmt.execute(&[
                &state.device,
                &&state.pos[..],
                &&state.vel[..],
                &&state.force[..],
                &is_reference,
                //&now,
                &trial_id,
            ])
            .expect("failed to insert haptic state");
        }
    });
}

pub struct DB {
    tx_haptic_slave_states: Sender<(i32, bool, DateTime<Utc>, State)>,
    tx_haptic_master_states: Sender<(i32, bool, DateTime<Utc>, State)>,
    conn: Connection,
    session: Session,
    current_trial_idx: usize,
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
        packet_rates: Vec<i32>,
        delays: Vec<i32>,
        control_algos: Vec<ControlAlgorithm>,
    ) {
        let subject = self.new_subject(nickname, age, gender, handedness);
        let stmt = self
            .conn
            .prepare("INSERT INTO haptic.session (subject_id) VALUES ($1) RETURNING id")
            .expect("failed to create session");
        let rows = stmt
            .query(&[&subject.id])
            .expect("failed to create session");
        let session_id = rows.get(0).get(0);

        let mut combinations =
            Vec::with_capacity(control_algos.len() * packet_rates.len() * delays.len());
        for control_algo in &control_algos {
            for packet_rate in &packet_rates {
                for delay in &delays {
                    combinations.push((control_algo.clone(), *packet_rate, *delay));
                }
            }
        }
        combinations.shuffle(&mut thread_rng());

        let trials = combinations
            .into_iter()
            .map(|(control_algo, packet_rate, delay)| {
                self.new_trial(session_id, packet_rate, delay, control_algo)
            })
            .collect();

        self.session = Session {
            id: session_id,
            subject,
            trials,
        };
    }

    fn new_trial(
        &self,
        session_id: i32,
        packet_rate: i32,
        delay: i32,
        control_algo: ControlAlgorithm,
    ) -> Trial {
        let stmt = self
            .conn
            .prepare("INSERT INTO haptic.trial (session_id, packet_rate, delay, control_algo) VALUES ($1, $2, $3, $4) RETURNING id")
            .expect("failed to create session");
        let rows = stmt
            .query(&[&session_id, &packet_rate, &delay, &control_algo])
            .expect("failed to create trial");
        let trial_id = rows.get(0).get(0);
        Trial {
            id: trial_id,
            packet_rate,
            delay,
            session_id,
            control_algo,
            rating: 0,
        }
    }

    pub fn current_trial(&self) -> Trial {
        self.session.trials[self.current_trial_idx].clone()
    }

    pub fn next_trial(&mut self) -> bool {
        if self.session.trials.len() - 1 == self.current_trial_idx {
            false
        } else {
            self.current_trial_idx += 1;
            true
        }
    }

    pub fn rate_trial(&self, rating: i32) {
        let trial = self.current_trial();
        let stmt = self
            .conn
            .prepare("UPDATE haptic.trial SET rating = $1 WHERE id = $2")
            .expect("failed to create session");
        stmt.execute(&[&rating, &trial.id])
            .expect("failed to update rating of trial");
    }

    pub fn save_jnd(&self, control_algo: ControlAlgorithm, packet_rate: i32) {
        let stmt = self
            .conn
            .prepare(
                r#"
                    INSERT INTO haptic.jnd 
                        (subject_id, control_algo, packet_rate) VALUES 
                        ($1, $2, $3) 
                    ON CONFLICT (subject_id, control_algo) 
                    DO UPDATE SET 
                        packet_rate = EXCLUDED.packet_rate"#,
            )
            .expect("failed to create jnd");
        stmt.execute(&[&self.session.subject.id, &control_algo, &packet_rate])
            .expect("failed to create session");
    }

    pub fn new(db_name: Option<&str>) -> Self {
        let (tx_haptic_master_states, rx_haptic_master_states) = unbounded();
        handle_haptic_states(rx_haptic_master_states, DB::connect(db_name));

        let (tx_haptic_slave_states, rx_haptic_slave_states) = unbounded();
        handle_haptic_states(rx_haptic_slave_states, DB::connect(db_name));

        Self {
            tx_haptic_slave_states,
            tx_haptic_master_states,
            conn: Self::connect(db_name),
            // sample session
            session: Session {
                id: 0,
                subject: Subject {
                    id: 0,
                    age: 0,
                    gender: Gender::Male,
                    handedness: Handedness::Right,
                },
                trials: vec![Trial {
                    id: 0,
                    packet_rate: 1000,
                    delay: 0,
                    session_id: 0,
                    control_algo: Default::default(),
                    rating: 0,
                }],
            },
            // sample subject
            current_trial_idx: 0,
        }
    }

    pub fn insert_haptic_state(&self, now: u64, is_reference: bool, state: State) {
        let now =
            DateTime::<Utc>::from(std::time::UNIX_EPOCH + std::time::Duration::from_micros(now));
        let trial_id = self.current_trial().id;
        match state.device {
            Device::Master => {
                self.tx_haptic_master_states
                    .send((trial_id, is_reference, now, state))
                    .expect("failed to queue haptic master state");
            }
            Device::Slave => {
                self.tx_haptic_slave_states
                    .send((trial_id, is_reference, now, state))
                    .expect("failed to queue haptic slave state");
            }
        }
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
        let mut db = DB::new(Some(db_name));

        db.new_session(
            "ben",
            25,
            Gender::Male,
            Handedness::Right,
            vec![10, 50, 100],
            vec![10, 20],
            vec![ControlAlgorithm::MMT, ControlAlgorithm::ISS],
        );

        db.new_subject("ben", 12, Gender::Male, Handedness::Right);

        let state = State {
            vel: [0.1, 0.2, 0.3],
            pos: [0.4, 0.5, 0.6],
            force: [0.7, 0.8, 0.9],
            device: Device::Master,
        };
        db.insert_haptic_state(1, true, state.clone());
        db.insert_haptic_state(2, true, state.clone());
        db.insert_haptic_state(3, true, state.clone());
        db.insert_haptic_state(4, true, state.clone());

        db.save_jnd(ControlAlgorithm::ISS, 120);
        db.save_jnd(ControlAlgorithm::ISS, 110);
        db.save_jnd(ControlAlgorithm::None, 110);

        std::thread::sleep(std::time::Duration::from_secs(2));

        // db.delete_subjects();
    }
}
