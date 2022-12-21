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
#define GPIO_INPUT_AC_1 32
#define GPIO_INPUT_AC_2 33
#define GPIO_INPUT_AC_3 25
#define GPIO_INPUT_AC_4 27

#define GPIO_INPUT_PIN_SEL ((1ULL << GPIO_INPUT_AC_1) | \
														(1ULL << GPIO_INPUT_AC_2) | \
														(1ULL << GPIO_INPUT_AC_3) | \
														(1ULL << GPIO_INPUT_AC_4))

/**
 * @brief GPIO OUTPUT pins
 */
#define GPIO_OUTPUT_AC_1_ACTIVE 15
#define GPIO_OUTPUT_AC_2_ACTIVE 4
#define GPIO_OUTPUT_AC_3_ACTIVE 5
#define GPIO_OUTPUT_AC_4_ACTIVE 18

#define GPIO_OUTPUT_TEMP_MAX 19
#define GPIO_OUTPUT_TEMP_MIN 21

#define GPIO_OUTPUT_PIN_SEL ((1ULL << GPIO_OUTPUT_AC_1_ACTIVE) | \
														 (1ULL << GPIO_OUTPUT_AC_2_ACTIVE) | \
														 (1ULL << GPIO_OUTPUT_AC_3_ACTIVE) | \
														 (1ULL << GPIO_OUTPUT_AC_4_ACTIVE) | \
														 (1ULL << GPIO_OUTPUT_TEMP_MAX) |    \
														 (1ULL << GPIO_OUTPUT_TEMP_MIN))

temp_simulator_t temp_sim = {
		.amb_temp = AMBIENT_TEMPERATURE,
		.curr_temp = 20 + ZERO_KELVIN,
		.ac_heat = 600,
		.eq_heat = 600,
		.ac1_on = false,
		.ac2_on = false,
};

QueueHandle_t intr_queue;

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
				gpio_set_level(GPIO_OUTPUT_AC_1_ACTIVE, !level);
				temp_sim.ac1_on = level;
			}
			else if (pin == GPIO_INPUT_AC_2)
			{
				gpio_set_level(GPIO_OUTPUT_AC_2_ACTIVE, !level);
				temp_sim.ac2_on = level;
			}
			else if (pin == GPIO_INPUT_AC_3)
			{
				gpio_set_level(GPIO_OUTPUT_AC_3_ACTIVE, !level);
				temp_sim.ac3_on = level;
			}
			else if (pin == GPIO_INPUT_AC_4)
			{
				gpio_set_level(GPIO_OUTPUT_AC_4_ACTIVE, !level);
				temp_sim.ac4_on = level;
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
		int ac_active = gpio_get_level(GPIO_INPUT_AC_1) + gpio_get_level(GPIO_INPUT_AC_2) + gpio_get_level(GPIO_INPUT_AC_3) + gpio_get_level(GPIO_INPUT_AC_4);

		heat = temp_sim.eq_heat - (ac_active * temp_sim.ac_heat);

		temp_sim.curr_temp = (heat + ((C * last_temp) / d_time) + (temp_sim.amb_temp / R)) / ((1 / R) + (C / d_time));
		last_temp = temp_sim.curr_temp;

		if (temp_sim.curr_temp >= 27 + ZERO_KELVIN)
		{
			gpio_set_level(GPIO_OUTPUT_TEMP_MAX, 0);
		}
		else if (temp_sim.curr_temp <= 24 + ZERO_KELVIN)
		{
			gpio_set_level(GPIO_OUTPUT_TEMP_MIN, 0);
		}
		else
		{
			gpio_set_level(GPIO_OUTPUT_TEMP_MAX, 1);
			gpio_set_level(GPIO_OUTPUT_TEMP_MIN, 1);
		}

		ring_buffer_insert(temp_sim.buffer, &temp_sim.curr_temp);

		printf("temperature: %.2f\n", temp_sim.curr_temp - ZERO_KELVIN);

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
	io_conf.pull_up_en = 0;
	io_conf.pull_down_en = 1;
	gpio_config(&io_conf);

	io_conf.intr_type = GPIO_PIN_INTR_ANYEDGE;
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
	io_conf.pull_down_en = 1;
	gpio_config(&io_conf);

	gpio_set_level(GPIO_OUTPUT_AC_1_ACTIVE, !gpio_get_level(GPIO_INPUT_AC_1));
	gpio_set_level(GPIO_OUTPUT_AC_2_ACTIVE, !gpio_get_level(GPIO_INPUT_AC_2));
	gpio_set_level(GPIO_OUTPUT_AC_3_ACTIVE, !gpio_get_level(GPIO_INPUT_AC_3));
	gpio_set_level(GPIO_OUTPUT_AC_4_ACTIVE, !gpio_get_level(GPIO_INPUT_AC_4));
	gpio_set_level(GPIO_OUTPUT_TEMP_MAX, 1);
	gpio_set_level(GPIO_OUTPUT_TEMP_MIN, 1);

	intr_queue = xQueueCreate(10, sizeof(int));
	xTaskCreate(gpio_controll_task, "gpio_control_task", 2048, NULL, 1, NULL);

	gpio_install_isr_service(0);
	gpio_isr_handler_add(GPIO_INPUT_AC_1, gpio_interrupt_handler, (void *)GPIO_INPUT_AC_1);
	gpio_isr_handler_add(GPIO_INPUT_AC_2, gpio_interrupt_handler, (void *)GPIO_INPUT_AC_2);
	gpio_isr_handler_add(GPIO_INPUT_AC_3, gpio_interrupt_handler, (void *)GPIO_INPUT_AC_3);
	gpio_isr_handler_add(GPIO_INPUT_AC_4, gpio_interrupt_handler, (void *)GPIO_INPUT_AC_4);

	temp_sim.buffer = ring_buffer_init(sizeof(float), 256);

	xTaskCreate(temp_simulation_task, "temp_simulation_task", 2048, NULL, 1, NULL);

	fs_mount();

	wifi_data_t wifi_config = {.enabled = 1, .password = "ati12345", .ssid = "TEMP_SIM_ATI"};
	wifi_driver_init(&wifi_config);

	http_server_init();

	while (1)
	{
		// printf("\33[1Bmem : %d", heap_caps_get_free_size(MALLOC_CAP_DEFAULT));
		vTaskDelay(5000 / portTICK_PERIOD_MS);
	}
}