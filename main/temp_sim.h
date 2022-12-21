#ifndef __TEMP_SIM__
#define __TEMP_SIM__

/*
 *Includes - BLibliotecas do sistema e do usuario.
 */
#include <stdio.h>

// FreeRTOS
#include "freertos/FreeRTOS.h"

#include "ring_buffer.h"

#define AIR_THERMAL_CAPACITANCE 700.0 // J/kg*K
#define AIR_DENSITY 1.225             // Kg/m3
#define ROOM_VOLUME 30                // m3
#define AMBIENT_TEMPERATURE 298.15    // K
#define ZERO_KELVIN 273.15            // K

#define EQUIPMENT_HEAT 700 // J
#define AC_HEAT 700        // J

#define TOTAL_AIR_MASS (AIR_DENSITY * ROOM_VOLUME)
#define ROOM_THERMAL_CAP (TOTAL_AIR_MASS * AIR_THERMAL_CAPACITANCE)
#define ROOM_THERMAL_RES 0.01

typedef struct
{

  float curr_temp;
  float ac_heat;
  float eq_heat;
  float amb_temp;

  bool ac1_on;
  bool ac2_on;
  bool ac3_on;
  bool ac4_on;

  ring_buffer_t *buffer;

} temp_simulator_t;

#endif // __TEMP_SIM__
