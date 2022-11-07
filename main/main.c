#include "main.h"

static const char *TAG = "main";

/**
 * @brief TEMPERATURE params
 */
#define TEMP_UP_RATE 0.3
#define TEMP_DOWN_RATE 1
#define TEMP_ALARM_MAX 27.0
#define TEMP_ALARM_MIN 24.0
#define TEMP_MAX 40.0
#define TEMP_MIN 18.0
#define TEMP_MID (((TEMP_MAX - TEMP_MIN) / 2) + TEMP_MIN)
#define TEMP_S_RATE (12 / (TEMP_MAX - TEMP_MIN))

/**
 * @brief GPIO INPUT pins
 */
#define GPIO_INPUT_AC_1 13
#define GPIO_INPUT_AC_2 14

#define GPIO_INPUT_PIN_SEL ((1ULL << GPIO_INPUT_AC_1) | \
														(1ULL << GPIO_INPUT_AC_2))

/**
 * @brief GPIO OUTPUT pins
 */
#define GPIO_OUTPUT_AC_1_ACTIVE 27
#define GPIO_OUTPUT_AC_2_ACTIVE 26
#define GPIO_OUTPUT_TEMP_MAX 25
#define GPIO_OUTPUT_TEMP_MIN 33

#define GPIO_OUTPUT_PIN_SEL ((1ULL << GPIO_OUTPUT_AC_1_ACTIVE) | \
														 (1ULL << GPIO_OUTPUT_AC_2_ACTIVE) | \
														 (1ULL << GPIO_OUTPUT_TEMP_MAX) |    \
														 (1ULL << GPIO_OUTPUT_TEMP_MIN))

#define AIR_THERMAL_CAPACITANCE 700.0 // J/kg*K
#define AIR_DENSITY 1.225							// Kg/m3
#define ROOM_VOLUME 30								// m3
#define AMBIENT_TEMPERATURE 298.15		// K
#define ZERO_KELVIN 273.15						// K

#define EQUIPMENT_HEAT 700 // J
#define AC_HEAT 700				 // J

#define TOTAL_AIR_MASS (AIR_DENSITY * ROOM_VOLUME)
#define ROOM_THERMAL_CAP (TOTAL_AIR_MASS * AIR_THERMAL_CAPACITANCE)
#define ROOM_THERMAL_RES 0.01

float buffer[1024];

ring_buffer_t *ring_buffer_init(size_t item_size, uint16_t item_count)
{
	ring_buffer_t *rb = malloc(sizeof(ring_buffer_t));
	rb->max_count = item_count;
	rb->item_size = item_size;
	rb->head = 0;
	rb->tail = 0;
	rb->count = 0;
	rb->full = false;

	rb->data = calloc(item_count, item_size);

	return rb;
}

bool ring_buffer_insert(ring_buffer_t *rb, void *item)
{
	memcpy(rb->data + rb->head * rb->item_size, item, rb->item_size);
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

void *ring_buffer_remove(ring_buffer_t *rb)
{
	if (rb->count == 0)
	{
		return NULL;
	}

	void *out = malloc(rb->item_size);
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

bool ring_buffer_peek(ring_buffer_t *rb)
{
	void **out = calloc(rb->count, rb->item_size);

	for (uint16_t i = 0; i < rb->count; i++)
	{
		memcpy(out[i], rb->data + ((rb->tail + i) % rb->max_count) * rb->item_size, rb->item_size);
	}

	return out;
}

temp_simulator_t temp_sim = {
		.amb_temp = AMBIENT_TEMPERATURE,
		.curr_temp = 20 + ZERO_KELVIN,
		.ac_heat = 600,
		.eq_heat = 600,
		.ac1_on = false,
		.ac2_on = false,
};

xQueueHandle intr_queue;

static void gpio_interrupt_handler(void *args)
{
	int pin = (int)args;
	xQueueSendFromISR(intr_queue, &pin, NULL);
}

void gpio_controll_task(void *params)
{
	int pin;
	while (true)
	{
		if (xQueueReceive(intr_queue, &pin, portMAX_DELAY))
		{
			uint8_t level = gpio_get_level(pin);
			// printf("GPIO %d changed state. The level is %d\n", pin, level);

			if (pin == GPIO_INPUT_AC_1)
			{
				gpio_set_level(GPIO_OUTPUT_AC_1_ACTIVE, level);
				temp_sim.ac1_on = level;
			}
			else if (pin == GPIO_INPUT_AC_2)
			{
				gpio_set_level(GPIO_OUTPUT_AC_2_ACTIVE, level);
				temp_sim.ac2_on = level;
			}
		}

		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
}

void temp_simulation_task(void *params)
{
	float last_temp = temp_sim.curr_temp;

	float d_time = 1;
	float C = ROOM_THERMAL_CAP;
	float R = ROOM_THERMAL_RES;

	float heat = 100;

	while (true)
	{

		heat = temp_sim.eq_heat;

		if (gpio_get_level(GPIO_INPUT_AC_1))
		{
			heat = heat - temp_sim.ac_heat;
		}

		if (gpio_get_level(GPIO_INPUT_AC_2))
		{
			heat = heat - temp_sim.ac_heat;
		}

		temp_sim.curr_temp = (heat + ((C * last_temp) / d_time) + (temp_sim.amb_temp / R)) / ((1 / R) + (C / d_time));
		last_temp = temp_sim.curr_temp;

		ring_buffer_insert(temp_sim.buffer, &temp_sim.curr_temp);

		ESP_LOGI(TAG, "\rsimulated temperature: %.2f", temp_sim.curr_temp - ZERO_KELVIN);

		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}

void app_main(void)
{
	esp_err_t err;

	//     **Inicializando portas GPIO**
	gpio_config_t io_conf;
	io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
	io_conf.mode = GPIO_MODE_OUTPUT;
	io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
	io_conf.pull_up_en = 1;
	gpio_config(&io_conf);

	io_conf.intr_type = GPIO_PIN_INTR_ANYEDGE;
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
	io_conf.pull_down_en = 1;
	gpio_config(&io_conf);

	gpio_set_level(GPIO_OUTPUT_AC_1_ACTIVE, gpio_get_level(GPIO_INPUT_AC_1));
	gpio_set_level(GPIO_OUTPUT_AC_2_ACTIVE, gpio_get_level(GPIO_INPUT_AC_2));

	intr_queue = xQueueCreate(10, sizeof(int));
	xTaskCreate(gpio_controll_task, "gpio_control_task", 2048, NULL, 1, NULL);

	gpio_install_isr_service(0);
	gpio_isr_handler_add(GPIO_INPUT_AC_1, gpio_interrupt_handler, (void *)GPIO_INPUT_AC_1);
	gpio_isr_handler_add(GPIO_INPUT_AC_2, gpio_interrupt_handler, (void *)GPIO_INPUT_AC_2);

	temp_sim.buffer = ring_buffer_init(sizeof(float), 1024);

	xTaskCreate(temp_simulation_task, "temp_simulation_task", 2048, NULL, 1, NULL);

	while (1)
	{
		vTaskDelay(5000 / portTICK_PERIOD_MS);

		ring_buffer_peek(temp_sim.buffer);
	}
}