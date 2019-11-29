#ifndef haptic_db_ffi_h
#define haptic_db_ffi_h

#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <new>

enum class ControlAlgorithm : uint8_t {
  None,
  WAVE,
  ISS,
  PC,
  MMT,
};

enum class Device : uint8_t {
  Master,
  Slave,
};

enum class Gender : uint8_t {
  Male,
  Female,
};

enum class Handedness : uint8_t {
  Right,
  Left,
};

struct DB;

struct Trial {
  int32_t id;
  int32_t packetRate;
  int32_t delay;
  int32_t sessionId;
  ControlAlgorithm controlAlgo;
  int32_t rating;
};

struct State {
  double vel[3];
  double pos[3];
  double force[3];
  Device device;
};

extern "C" {

Trial db_current_trial(DB *db);

void db_free(DB *db);

void db_insert_haptic_state(DB *db, uint64_t now, bool isReference, State state);

DB *db_new();

void db_new_session(DB *db,
                    const char *nickname,
                    int32_t age,
                    Gender gender,
                    Handedness handedness,
                    const int32_t *packetRatesData,
                    size_t packetRatesSize,
                    const int32_t *delaysData,
                    size_t delaysSize,
                    const ControlAlgorithm *controlAlgosData,
                    size_t controlAlgosSize);

bool db_next_trial(DB *db);

void db_rate_trial(DB *db, int32_t rating);

void db_save_jnd(DB *db, ControlAlgorithm controlAlgo, int32_t packetRate);

} // extern "C"

#endif // haptic_db_ffi_h
