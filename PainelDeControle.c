/*
 *  Por: Wilton Lacerda Silva
 *  Data: 16-05-2025
 *
 *  Exemplo de uso de semaforo de contagem com FreeRTOS
 *
 *  Descrição: Simulação de um semáforo de contagem com um botão.
 * Cada vez que o botão é pressionado, um evento é gerado e tratado
 * por uma tarefa. A tarefa atualiza um display OLED com a contagem anotada.
 */

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "lib/ssd1306.h"
#include "lib/font.h"
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>
#include "hardware/pio.h"
#include "ws2812.pio.h"
#include "hardware/pwm.h"
#include "semphr.h"
#include "pico/bootrom.h"

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define ENDERECO 0x3C

#define BOTAO_A 5         // Aumenta o número
#define BOTAO_B 6         // Reduz o número
#define BOTAO_JOYSTICK 22 // Zera contagem
#define buzzer 10         // Pino do buzzer A
#define led_RED 13        // Red=13, Blue=12, Green=11
#define led_BLUE 12       // Red=13, Blue=12, Green=11
#define led_GREEN 11      // Red=13, Blue=12, Green=11

#define MAX_USERS 8

ssd1306_t ssd;
SemaphoreHandle_t xContadorSemA;
SemaphoreHandle_t xContadorSemB;
SemaphoreHandle_t xBinarySemaphoreReset;
SemaphoreHandle_t xOLEDMutex;
uint16_t eventosProcessados = 0;
uint8_t usuarios = 0;

void update_oled_display(const char *message1, const char *message2, const char *count_message)
{
    if (xSemaphoreTake(xOLEDMutex, portMAX_DELAY) == pdTRUE)
    {
        ssd1306_fill(&ssd, 0);
        if (message1)
            ssd1306_draw_string(&ssd, (char *)message1, 5, 10);
        if (message2)
            ssd1306_draw_string(&ssd, (char *)message2, 5, 45);
        if (count_message)
            ssd1306_draw_string(&ssd, (char *)count_message, 5, 25);
        ssd1306_send_data(&ssd);
        xSemaphoreGive(xOLEDMutex);
    }
}

// interrupcoes para os botoes
void gpio_irq_handler(uint gpio, uint32_t events)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (gpio == BOTAO_A)
    {
        gpio_set_irq_enabled(BOTAO_A, GPIO_IRQ_EDGE_FALL, false);
        xSemaphoreGiveFromISR(xContadorSemA, &xHigherPriorityTaskWoken);
    }
    else if (gpio == BOTAO_B)
    {
        gpio_set_irq_enabled(BOTAO_B, GPIO_IRQ_EDGE_FALL, false);
        xSemaphoreGiveFromISR(xContadorSemB, &xHigherPriorityTaskWoken);
    }
    else if (gpio == BOTAO_JOYSTICK)
    {
        gpio_set_irq_enabled(BOTAO_JOYSTICK, GPIO_IRQ_EDGE_FALL, false);
        xSemaphoreGiveFromISR(xBinarySemaphoreReset, &xHigherPriorityTaskWoken);
    }
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

// Tarefa para entrada
void vTaskEntrada(void *pvParameters)
{
    (void)pvParameters;
    while (true)
    {
        // Espera o contador ser solto pelo botao A
        if (xSemaphoreTake(xContadorSemA, portMAX_DELAY) == pdTRUE)
        {
            if (usuarios < MAX_USERS)
            {
                usuarios++;
                char buffer[32];
                sprintf(buffer, "%d/%d", usuarios, MAX_USERS);
                update_oled_display("Usuarios:", "Usuario entrou!", buffer);
            }
            else
            {
                char buffer[32];
                sprintf(buffer, "%d/%d", usuarios, MAX_USERS);
                update_oled_display("Usuarios:", "Sem capacidade", buffer);
                pwm_set_gpio_level(buzzer, 2048);
                vTaskDelay(pdMS_TO_TICKS(200));
                pwm_set_gpio_level(buzzer, 0);
            }
            if (usuarios == 0)
            {
                gpio_put(led_BLUE, true);
                gpio_put(led_RED, false);
                gpio_put(led_GREEN, false);
            }
            else if (usuarios >= 0 && usuarios <= (MAX_USERS - 2))
            {
                gpio_put(led_BLUE, false);
                gpio_put(led_RED, false);
                gpio_put(led_GREEN, true);
            }
            else if (usuarios == (MAX_USERS - 1))
            {
                gpio_put(led_BLUE, false);
                gpio_put(led_RED, true);
                gpio_put(led_GREEN, true);
            }
            else if (usuarios == MAX_USERS)
            {
                gpio_put(led_BLUE, false);
                gpio_put(led_RED, true);
                gpio_put(led_GREEN, false);
            }
            vTaskDelay(pdMS_TO_TICKS(200));
            gpio_set_irq_enabled(BOTAO_A, GPIO_IRQ_EDGE_FALL, true);
        }
    }
}

// Task for user exit (Button B) [cite: 8]
void vTaskSaida(void *pvParameters)
{
    (void)pvParameters;
    while (true)
    {
        // Espera o contador ser solto pelo botao B
        if (xSemaphoreTake(xContadorSemB, portMAX_DELAY) == pdTRUE)
        { // Verifica se foi precionado o B
            if (usuarios > 0)
            {
                usuarios--;
                char buffer[32];
                sprintf(buffer, "%d/%d", usuarios, MAX_USERS);
                update_oled_display("Usuarios:", "Usuario saiu!", buffer);
            }
            else
            {
                char buffer[32];
                sprintf(buffer, "%d/%d", usuarios, MAX_USERS);
                update_oled_display("Usuarios:", "Sem usuarios!", buffer);
            }
            if (usuarios == 0)
            {
                gpio_put(led_BLUE, true);
                gpio_put(led_RED, false);
                gpio_put(led_GREEN, false);
            }
            else if (usuarios >= 0 && usuarios <= (MAX_USERS - 2))
            {
                gpio_put(led_BLUE, false);
                gpio_put(led_RED, false);
                gpio_put(led_GREEN, true);
            }
            else if (usuarios == (MAX_USERS - 1))
            {
                gpio_put(led_BLUE, false);
                gpio_put(led_RED, true);
                gpio_put(led_GREEN, true);
            }
            else if (usuarios == MAX_USERS)
            {
                gpio_put(led_BLUE, false);
                gpio_put(led_RED, true);
                gpio_put(led_GREEN, false);
            }
            vTaskDelay(pdMS_TO_TICKS(200));
            gpio_set_irq_enabled(BOTAO_B, GPIO_IRQ_EDGE_FALL, true);
        }
    }
}

// Funcao para reiniciar sistema
void vTaskReset(void *pvParameters)
{
    (void)pvParameters;
    while (true)
    {
        // Espera pelo semaforo binario
        if (xSemaphoreTake(xBinarySemaphoreReset, portMAX_DELAY) == pdTRUE)
        {
            usuarios = 0;
            char buffer[32];
            sprintf(buffer, "Usuarios: %d/%d", usuarios, MAX_USERS);
            update_oled_display("Usuarios:", "Sistema reiniciou!", buffer);
            gpio_put(led_BLUE, true);
            gpio_put(led_RED, false);
            gpio_put(led_GREEN, false);
            pwm_set_gpio_level(buzzer, 2048);
            vTaskDelay(pdMS_TO_TICKS(200));
            pwm_set_gpio_level(buzzer, 0);
            vTaskDelay(pdMS_TO_TICKS(200));
            pwm_set_gpio_level(buzzer, 2048);
            vTaskDelay(pdMS_TO_TICKS(200));
            pwm_set_gpio_level(buzzer, 0);
            vTaskDelay(pdMS_TO_TICKS(200));
            gpio_set_irq_enabled(BOTAO_JOYSTICK, GPIO_IRQ_EDGE_FALL, true);
        }
    }
}

int main()
{
    stdio_init_all();

    // Inicialização do display
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, ENDERECO, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_send_data(&ssd);

    gpio_init(led_RED);
    gpio_set_dir(led_RED, GPIO_OUT);
    gpio_init(led_BLUE);
    gpio_set_dir(led_BLUE, GPIO_OUT);
    gpio_init(led_GREEN);
    gpio_set_dir(led_GREEN, GPIO_OUT);
    gpio_put(led_BLUE, true); // inicia com Azul

    // iniciacao buzzer
    gpio_set_function(buzzer, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(buzzer);
    pwm_set_wrap(slice_num, 4096);
    // Define o clock divider como 440 (nota lá para o buzzer)
    pwm_set_clkdiv(slice_num, 440.0f);
    pwm_set_enabled(slice_num, true);

    // Configura os botões
    gpio_init(BOTAO_A);
    gpio_set_dir(BOTAO_A, GPIO_IN);
    gpio_pull_up(BOTAO_A);

    gpio_init(BOTAO_B);
    gpio_set_dir(BOTAO_B, GPIO_IN);
    gpio_pull_up(BOTAO_B);

    gpio_init(BOTAO_JOYSTICK);
    gpio_set_dir(BOTAO_JOYSTICK, GPIO_IN);
    gpio_pull_up(BOTAO_JOYSTICK);

    gpio_set_irq_enabled_with_callback(BOTAO_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled(BOTAO_B, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(BOTAO_JOYSTICK, GPIO_IRQ_EDGE_FALL, true);

    xContadorSemA = xSemaphoreCreateCounting(1, 0); // Cria semáforo de contagem (máximo 8, inicial 0)
    xContadorSemB = xSemaphoreCreateCounting(1, 0); // Cria semáforo de contagem (máximo 8, inicial 0)
    xBinarySemaphoreReset = xSemaphoreCreateBinary();      // Semaforo binario para reset
    xOLEDMutex = xSemaphoreCreateMutex();                  // Mutex para protecao do OLED

    // Cria tarefa
    xTaskCreate(vTaskEntrada, "EntradaTask", configMINIMAL_STACK_SIZE + 128, NULL, 1, NULL);
    xTaskCreate(vTaskSaida, "SaidaTask", configMINIMAL_STACK_SIZE + 128, NULL, 1, NULL);
    xTaskCreate(vTaskReset, "ResetTask", configMINIMAL_STACK_SIZE + 128, NULL, 3, NULL);

    vTaskStartScheduler();
    panic_unsupported();
}
