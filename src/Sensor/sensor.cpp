#include "common.h"
#include <Adafruit_INA219.h>

struct data {
  float *dataPoints;
  int len = SEN_LEN;
  int type = NONE;
  int curr = 0;
};

//SENSOR
class vTaskSensor {
  public:
  Adafruit_INA219 ina219;
  int T_sample = DEFAULT_T_SAMPLE; //ms
  QueueHandle_t bufManager;
  TickType_t previous = 0;
  TaskHandle_t xSensorRead;

  struct data current;
  struct data voltage;

  vTaskSensor(Adafruit_INA219 _ina219, int _T_sample, QueueHandle_t _bufManager) {
    ina219 = _ina219;
    T_sample = _T_sample;
    bufManager = _bufManager;

    xQueueReceive(bufManager, &current, portMAX_DELAY);
    current.type = CURRENT;
    xQueueReceive(bufManager, &voltage, portMAX_DELAY);
    voltage.type = VOLTAGE;

  }

};


void vTaskSensorRead(void *parameters) {

   vTaskSensor *sensor = (vTaskSensor *)parameters;
   data current = sensor->current;
   data voltage = sensor->voltage;
   QueueHandle_t bufManager = sensor->bufManager;
   TickType_t *previous = &sensor->previous;
   int T_sample = sensor->T_sample;

   Serial.print("Time: ");
   Serial.println(millis());

   float shuntvoltage = 0;
   float busvoltage = 0;
   float current_mA = 0;
   float loadvoltage = 0;
   float power_mW = 0;

   shuntvoltage = ina219.getShuntVoltage_mV();
   busvoltage = ina219.getBusVoltage_V();
   current_mA = ina219.getCurrent_mA();
   power_mW = ina219.getPower_mW();
   loadvoltage = busvoltage + (shuntvoltage / 1000);

   
   current.dataPoints[current.curr] = current_mA;
   current.curr++;

   if (current.curr >= current.len) {
   xQueueSend(tx_queue, &current, portMAX_DELAY);
   xQueueReceive(bufManager, &current, portMAX_DELAY);
   }

   
   voltage.dataPoints[voltage.curr] = loadvoltage;
   voltage.curr++;

   if (voltage.curr >= voltage.len) {
   xQueueSend(tx_queue, &voltage, portMAX_DELAY);
   xQueueReceive(bufManager, &voltage, portMAX_DELAY);
   }


   xTaskDelayUntil(previous, pdMS_TO_TICKS(T_sample));

}