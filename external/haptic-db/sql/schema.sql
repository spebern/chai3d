DROP DATABASE haptic;
DROP DATABASE test_haptic;
DROP USER haptic;

CREATE DATABASE haptic;

\c haptic;

CREATE SCHEMA haptic;

CREATE EXTENSION IF NOT EXISTS timescaledb CASCADE;

CREATE TYPE "Device" AS ENUM (
    'Master',
    'Slave'
);

CREATE TABLE haptic.session (
    id          SERIAL PRIMARY KEY,
    created     DATE   NOT NULL DEFAULT NOW(),
    description TEXT   NOT NULL
);

CREATE TABLE haptic.msg_m2s (
    session_id      INTEGER REFERENCES haptic.session(id) ON DELETE CASCADE,
    sequence_number INTEGER NOT NULL,
    time_sent       TIMESTAMP           NOT NULL,
	time_recv       TIMESTAMP,
    device          "Device"            NOT NULL,
    vel             DOUBLE PRECISION[3] NOT NULL,
    pos             DOUBLE PRECISION[3] NOT NULL,
);

CREATE TABLE haptic.msg_s2m (
    session_id      INTEGER REFERENCES haptic.session(id) ON DELETE CASCADE ,
    sequence_number INTEGER NOT NULL,
    time_recv       TIMESTAMP,
    time_sent       TIMESTAMP           NOT NULL,
    device         "Device"            NOT NULL,
    force           DOUBLE PRECISION[3] NOT NULL,
);

CREATE USER haptic WITH ENCRYPTED PASSWORD 'abc123';

GRANT USAGE ON SCHEMA haptic TO haptic;
GRANT SELECT, UPDATE, INSERT, DELETE ON ALL TABLES IN SCHEMA haptic TO haptic;
GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA haptic TO  haptic;
GRANT SELECT, UPDATE, USAGE ON ALL SEQUENCES IN SCHEMA haptic to haptic;
