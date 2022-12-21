#include "ring_buffer.h"

ring_buffer_t *ring_buffer_init(size_t item_size, uint16_t item_count)
{
  ring_buffer_t *rb = malloc(sizeof(ring_buffer_t));
  rb->max_count = item_count;
  rb->item_size = item_size;
  rb->head = 0;
  rb->tail = 0;
  rb->count = 0;
  rb->full = false;

  rb->data = malloc(item_count * item_size);

  return rb;
}

bool ring_buffer_insert(ring_buffer_t *rb, void *item)
{
  memcpy(rb->data + rb->head * rb->item_size, (uint8_t *)item, rb->item_size);
  rb->head = (rb->head + 1) % rb->max_count;

  rb->count++;
  if (rb->count >= rb->max_count)
  {
    rb->count = rb->max_count;
    rb->tail = rb->head;
    rb->full = true;
  }

  return true;
}

uint8_t *ring_buffer_remove(ring_buffer_t *rb)
{
  if (rb->count == 0)
  {
    return NULL;
  }

  uint8_t *out = malloc(rb->item_size * sizeof(uint8_t));
  memcpy(out, rb->data + rb->tail * rb->item_size, rb->item_size);
  rb->tail = (rb->tail + 1) % rb->max_count;

  rb->full = false;

  rb->count--;
  if (rb->count <= 0)
  {
    rb->count = 0;
  }

  return out;
}

uint8_t *ring_buffer_peek(ring_buffer_t *rb) 
{
  uint8_t *out = malloc(rb->count * rb->item_size * sizeof(uint8_t));

  for (uint16_t i = 0; i < rb->count; i++)
  {
    // ESP_LOGI(TAG, "%f, ", *(float *)(rb->data + ((rb->tail + i) % rb->max_count) * rb->item_size) - ZERO_KELVIN);
    memcpy(out + i * rb->item_size, rb->data + ((rb->tail + i) % rb->max_count) * rb->item_size, sizeof(void *));
  }

  return out;
}
