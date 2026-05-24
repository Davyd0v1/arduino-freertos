#include <Arduino_FreeRTOS.h>

void TaskBlink1(void *pvParameters);
void TaskBlink2(void *pvParameters);
void Taskprint(void *pvParameters);
void TaskSerialCmd(void *pvParameters);

void clearSerialMonitor()
{
  // ANSI-очистка (если монитор поддерживает)
  Serial.write(27);
  Serial.print("[2J");
  Serial.write(27);
  Serial.print("[H");

 
  for (int i = 0; i < 30; i++)
  {
    Serial.println();
  }
}

void setup()
{
  Serial.begin(9600);

  pinMode(8, OUTPUT);
  pinMode(7, OUTPUT);

  xTaskCreate(TaskBlink1, "task1", 128, NULL, 1, NULL);
  xTaskCreate(TaskBlink2, "task2", 128, NULL, 1, NULL);
  xTaskCreate(Taskprint, "task3", 128, NULL, 1, NULL);
  xTaskCreate(TaskSerialCmd, "serialcmd", 192, NULL, 1, NULL);

  vTaskStartScheduler();
}

void loop()
{
}

void TaskBlink1(void *pvParameters)
{
  while (1)
  {
    Serial.println("Task1");
    digitalWrite(8, HIGH);
    vTaskDelay(200 / portTICK_PERIOD_MS);
    digitalWrite(8, LOW);
    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}

void TaskBlink2(void *pvParameters)
{
  while (1)
  {
    Serial.println("Task2");
    digitalWrite(7, HIGH);
    vTaskDelay(300 / portTICK_PERIOD_MS);
    digitalWrite(7, LOW);
    vTaskDelay(300 / portTICK_PERIOD_MS);
  }
}

void Taskprint(void *pvParameters)
{
  int counter = 0;
  while (1)
  {
    counter++;
    Serial.println(counter);
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

void TaskSerialCmd(void *pvParameters)
{
  char buf[20];
  byte idx = 0;

  while (1)
  {
    while (Serial.available() > 0)
    {
      char c = Serial.read();

      if (c == '\r' || c == '\n')
      {
        buf[idx] = '\0';

        if (strcmp(buf, "clear") == 0)
        {
          clearSerialMonitor();
        }

        idx = 0;
      }
      else
      {
        if (idx < sizeof(buf) - 1)
        {
          buf[idx++] = c;
        }
      }
    }

    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}
