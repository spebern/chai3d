from dotenv import load_dotenv
from common import connect_to_db

user = "haptic2"
dbs = ["haptic2", "test_haptic2"]

schema = """
CREATE SCHEMA haptic;

CREATE EXTENSION IF NOT EXISTS timescaledb CASCADE;

CREATE TYPE "Device" AS ENUM (
    'Master',
    'Slave'
);

CREATE TYPE "OptimisationParameter" AS ENUM (
    'PacketRate',
    'Delay'
);

CREATE TYPE "ControlAlgorithm" AS ENUM (
    'None',
    'WAVE',
    'ISS',
    'PC',
    'MMT'
);

CREATE TYPE "Gender" AS ENUM (
    'Male',
    'Female'
);

CREATE TYPE "Handedness" AS ENUM (
    'Right',
    'Left'
);

CREATE TABLE haptic.subject (
    id         SERIAL       PRIMARY KEY,
    nickname   TEXT         NOT NULL UNIQUE,
    age        INTEGER      NOT NULL,
    gender     "Gender"     NOT NULL,
    handedness "Handedness" NOT NULL,
    created     TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE TABLE haptic.session (
    id          SERIAL      PRIMARY KEY,
    created     TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    subject_id  INTEGER REFERENCES haptic.subject(id) ON DELETE CASCADE
);

CREATE TABLE haptic.rating (
    id                     SERIAL PRIMARY KEY,
    delay                  INTEGER NOT NULL,
    packet_rate            INTEGER NOT NULL,
    rating                 INTEGER NOT NULL,
    control_algorithm      "ControlAlgorithm" NOT NULL,
    optimisation_parameter "OptimisationParameter"  NOT NULL,
    created                TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    session_id             INTEGER REFERENCES haptic.session(id) ON DELETE CASCADE
);

CREATE TABLE haptic.state (
    time            TIMESTAMPTZ         NOT NULL,
    pos             DOUBLE PRECISION[3] NOT NULL,
    vel             DOUBLE PRECISION[3] NOT NULL,
    force           DOUBLE PRECISION[3] NOT NULL,
    is_reference    BOOLEAN             NOT NULL,
    device          "Device"            NOT NULL,
    master_update   BOOLEAN             NOT NULL,
    slave_update    BOOLEAN             NOT NULL
);
SELECT create_hypertable('haptic.state', 'time');

CREATE TABLE haptic.target_position (
    time            TIMESTAMPTZ         NOT NULL,
    pos             DOUBLE PRECISION[3] NOT NULL
);
SELECT create_hypertable('haptic.target_position', 'time');
"""

priviledges = """
GRANT USAGE ON SCHEMA haptic TO haptic2;
GRANT SELECT, UPDATE, INSERT, DELETE ON ALL TABLES IN SCHEMA haptic TO haptic2;
GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA haptic TO  haptic2;
GRANT SELECT, UPDATE, USAGE ON ALL SEQUENCES IN SCHEMA haptic to haptic2;
"""


def init_dbs():
    _, cur = connect_to_db(db="postgres")

    for db in dbs:
        cur.execute("DROP DATABASE IF EXISTS {}".format(db))

    cur.execute("DROP USER IF EXISTS {}".format(user))
    cur.execute("CREATE USER {} WITH ENCRYPTED PASSWORD 'abc123'".format(user))

    for db in dbs:
        _, cur = connect_to_db(db="postgres")
        cur.execute("CREATE DATABASE {}".format(db))

        conn, cur = connect_to_db(db=db)
        cur.execute(schema)
        cur.execute(priviledges)


if __name__ == "__main__":
    load_dotenv()
    init_dbs()



