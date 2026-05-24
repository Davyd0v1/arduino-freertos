#include <Arduino_FreeRTOS.h>
#include <queue.h>

QueueHandle_t queue_1;

void TaskDisplay(void * pvParameters);
void TaskLDR(void * pvParameters);

void setup()
{
  Serial.begin(9600);

  queue_1 = xQueueCreate(5, sizeof(int));
  
  if (queue_1 == NULL)
  {
    Serial.println("Queue can not be created");
  }

  xTaskCreate(TaskDisplay, "Display_task", 128, NULL, 1, NULL);
  xTaskCreate(TaskLDR, "LDR_task", 128, NULL, 1, NULL);

  vTaskStartScheduler();
}

void loop()
{
}

// Принимаем из очереди и выводим в Serial
void TaskDisplay(void * pvParameters)
{
  int intensity = 0;

  while(1)
  {
    if (xQueueReceive(queue_1, &intensity, portMAX_DELAY) == pdPASS)
    {
      Serial.print("Intensity: ");
      Serial.println(intensity);
    }
  }
}

// Читаем с A0 и отправляем в очередь
void TaskLDR(void * pvParameters)
{
  int current_intensity;

  while(1)
  {
    current_intensity = analogRead(A0);

    Serial.print("Read: ");
    Serial.println(current_intensity);

    xQueueSend(queue_1, &current_intensity, portMAX_DELAY);

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
