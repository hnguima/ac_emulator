#ifndef __RING_BUFFER__
#define __RING_BUFFER__

/*
 *Includes - BLibliotecas do sistema e do usuario.
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/dirent.h>
#include <math.h>

// FreeRTOS
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_system.h"

typedef struct
{
  int16_t head;
  int16_t tail;

  size_t item_size;
  int16_t max_count;
  int16_t count;

  bool full;

  uint8_t *data;
} ring_buffer_t;

ring_buffer_t *ring_buffer_init(size_t item_size, uint16_t item_count);
bool ring_buffer_insert(ring_buffer_t *rb, void *item);
uint8_t *ring_buffer_remove(ring_buffer_t *rb);
uint8_t *ring_buffer_peek(ring_buffer_t *rb);

#endif // !__RING_BUFFER__