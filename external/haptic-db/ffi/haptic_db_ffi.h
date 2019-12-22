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

enum class OptimisationParameter : uint8_t {
  PacketRate,
  Delay,
};

struct DB;

struct State {
  double vel[3];
  double pos[3];
  double force[3];
  Device device;
  bool masterUpdate;
  bool slaveUpdate;
};

extern "C" {

void db_free(DB *db);

void db_insert_haptic_state(DB *db, bool isReference, State state);

void db_insert_rating(DB *db,
                      ControlAlgorithm controlAlgorithm,
                      int32_t packetRate,
                      int32_t delay,
                      OptimisationParameter optimisationParameter,
                      int32_t rating);

DB *db_new(uint8_t numStateWriterThreads);

void db_new_session(DB *db,
                    const char *nickname,
                    int32_t age,
                    Gender gender,
                    Handedness handedness);

} // extern "C"

#endif // haptic_db_ffi_h
