#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <queue.h>

// Очереди сообщений для каждого светодиода.
QueueHandle_t qLED0, qLED1, qLED2, qLED3;

// DIP1..DIP8 подключены к пинам D2..D9
const byte dipPins[8] = {2, 3, 4, 5, 6, 7, 8, 9};

// Светодиоды подключены к D10, D11, D12 и A0
const byte ledPins[4] = {10, 11, 12, A0};

// Перечисление состояний конечного автомата.
// Каждое состояние соответствует одному шагу программы.
typedef enum {
  ST_STEP1 = 0,
  ST_STEP2,
  ST_STEP3,
  ST_STEP4,
  ST_STEP5,
  ST_STEP6,
  ST_STEP7,
  ST_STEP8
} state_t;

// Функция записи команды в очередь.
// Используется для передачи значения 0 или 1 в задачу управления светодиодом.
static inline void sendCmdOverwrite(QueueHandle_t q, uint8_t cmd)
{
  xQueueOverwrite(q, &cmd);
}


static uint8_t readDipSample()
{
  uint8_t sample = 0;

  for (byte i = 0; i < 8; i++)
  {
    if (!digitalRead(dipPins[i]))
    {
      sample |= (1 << (7 - i));
    }
  }

  return sample;
}

static void allLedsOff()
{
  sendCmdOverwrite(qLED0, 0);
  sendCmdOverwrite(qLED1, 0);
  sendCmdOverwrite(qLED2, 0);
  sendCmdOverwrite(qLED3, 0);
}


static void TaskSampleFSM(void *pvParameters)
{
  (void)pvParameters;

  // Текущее состояние автомата
  state_t state = ST_STEP1;

  
  uint8_t lastSample = 0xFF;

  Serial.println("READY");

  for (;;)
  {
    // Считываем 8-битный код с DIP-переключателей
    uint8_t sample = readDipSample();

   
    if (sample != lastSample)
    {
      Serial.print("DIP = 0x");
      if (sample < 16) Serial.print("0");
      Serial.println(sample, HEX);
      lastSample = sample;
    }


    switch (state)
    {
      case ST_STEP1:
        if (sample == 0x55) {          // DIP1,3,5,7
          sendCmdOverwrite(qLED1, 1);  // включить LED1
          state = ST_STEP2;
          Serial.println("STEP 1 OK");
        }
        break;

      case ST_STEP2:
        if (sample == 0x59) {          // DIP1,4,5,7
          sendCmdOverwrite(qLED0, 1);  // включить LED0
          state = ST_STEP3;
          Serial.println("STEP 2 OK");
        }
        break;

      case ST_STEP3:
        if (sample == 0x5A) {          // DIP2,4,5,7
          sendCmdOverwrite(qLED2, 1);  // включить LED2
          state = ST_STEP4;
          Serial.println("STEP 3 OK");
        }
        break;

      case ST_STEP4:
        if (sample == 0x6A) {          // DIP2,4,6,7
          sendCmdOverwrite(qLED3, 1);  // включить LED3
          state = ST_STEP5;
          Serial.println("STEP 4 OK");
        }
        break;

      case ST_STEP5:
        if (sample == 0xAA) {          // DIP2,4,6,8
          sendCmdOverwrite(qLED3, 0);  // выключить LED3
          state = ST_STEP6;
          Serial.println("STEP 5 OK");
        }
        break;

      case ST_STEP6:
        if (sample == 0x6A) {          // DIP2,4,6,7
          sendCmdOverwrite(qLED2, 0);  // выключить LED2
          state = ST_STEP7;
          Serial.println("STEP 6 OK");
        }
        break;

      case ST_STEP7:
        if (sample == 0x5A) {          // DIP2,4,5,7
          sendCmdOverwrite(qLED0, 0);  // выключить LED0
          state = ST_STEP8;
          Serial.println("STEP 7 OK");
        }
        break;

      case ST_STEP8:
        if (sample == 0x59) {          // DIP1,4,5,7
          allLedsOff();                // выключить все светодиоды
          state = ST_STEP1;            // возврат к началу цикла
          Serial.println("STEP 8 OK / RESET");
        }
        break;
    }


    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// Задача управления светодиодом LED0.
static void TaskLED0(void *pvParameters)
{
  (void)pvParameters;
  uint8_t cmd;

  for (;;)
  {
    if (xQueueReceive(qLED0, &cmd, portMAX_DELAY) == pdPASS)
      digitalWrite(ledPins[0], cmd ? HIGH : LOW);
  }
}

// Задача управления светодиодом LED1.
static void TaskLED1(void *pvParameters)
{
  (void)pvParameters;
  uint8_t cmd;

  for (;;)
  {
    if (xQueueReceive(qLED1, &cmd, portMAX_DELAY) == pdPASS)
      digitalWrite(ledPins[1], cmd ? HIGH : LOW);
  }
}

// Задача управления светодиодом LED2.
static void TaskLED2(void *pvParameters)
{
  (void)pvParameters;
  uint8_t cmd;

  for (;;)
  {
    if (xQueueReceive(qLED2, &cmd, portMAX_DELAY) == pdPASS)
      digitalWrite(ledPins[2], cmd ? HIGH : LOW);
  }
}

// Задача управления светодиодом LED3.
static void TaskLED3(void *pvParameters)
{
  (void)pvParameters;
  uint8_t cmd;

  for (;;)
  {
    if (xQueueReceive(qLED3, &cmd, portMAX_DELAY) == pdPASS)
      digitalWrite(ledPins[3], cmd ? HIGH : LOW);
  }
}

// Функция начальной настройки.
// Здесь задаются режимы пинов, создаются очереди и запускаются задачи RTOS.
void setup()
{
  Serial.begin(9600);

  // Настройка входов DIP с внутренней подтяжкой к +5 В
  for (byte i = 0; i < 8; i++)
    pinMode(dipPins[i], INPUT_PULLUP);

  // Настройка светодиодов как выходов
  for (byte i = 0; i < 4; i++)
  {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW);
  }

  // Создание очередей длиной 1 элемент
  qLED0 = xQueueCreate(1, sizeof(uint8_t));
  qLED1 = xQueueCreate(1, sizeof(uint8_t));
  qLED2 = xQueueCreate(1, sizeof(uint8_t));
  qLED3 = xQueueCreate(1, sizeof(uint8_t));

  // Если хотя бы одна очередь не создалась, программа останавливается
  if (!qLED0 || !qLED1 || !qLED2 || !qLED3)
  {
    while (1) {}
  }

  // Инициализация: все выходы выключены
  allLedsOff();

  // Создание задач FreeRTOS
  xTaskCreate(TaskSampleFSM, "FSM", 96, NULL, 1, NULL);
  xTaskCreate(TaskLED0,      "LED0", 64, NULL, 2, NULL);
  xTaskCreate(TaskLED1,      "LED1", 64, NULL, 2, NULL);
  xTaskCreate(TaskLED2,      "LED2", 64, NULL, 2, NULL);
  xTaskCreate(TaskLED3,      "LED3", 64, NULL, 2, NULL);

  // Запуск планировщика задач
  vTaskStartScheduler();
}


void loop() {}
