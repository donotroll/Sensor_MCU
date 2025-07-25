#include <AsyncUdp.h>
#include "common.h"
#include "sensor.cpp"




const char* ssid = "WIFI_NAME";
const char* pass = "WIFI_PASSWORD";
const int relay = 23;
AsyncUDP udp;

Adafruit_INA219 ina219(0x44);
vTaskSensor taskSensor;

//UDP
char *rx_buffer[RX_BUFFER_LEN];
char *tx_buffer[TX_BUFFER_LEN];
int T_send = 1000;


void setup(void) 
{
  Serial.begin(115200);
  while (!Serial) {
      // will pause Zero, Leonardo, etc until serial console opens
      delay(1);
  }
    
  Serial.println("Hello!");
  
  // Initialize the INA219.
  // By default the initialization will use the largest range (32V, 2A).  However
  // you can call a setCalibration function to change this range (see comments).
  if (! ina219.begin()) {
    Serial.println("Failed to find INA219 chip");
    while (1) { delay(10); }
  }
  // To use a slightly lower 32V, 1A range (higher precision on amps):
  //ina219.setCalibration_32V_1A();
  // Or to use a lower 16V, 400mA range (higher precision on volts and amps):
  //ina219.setCalibration_16V_400mA();

  Serial.println("Measuring voltage and current with INA219 ...");

  buffer_queue = xQueueCreate(BUFFER_LEN, sizeof (data));
  tx_queue = xQueueCreate(BUFFER_LEN, sizeof (data));


  taskSensor = vTaskSensor(ina219, DEFAULT_T_SAMPLE, buffer_queue);
  BaseType_t xReturned;
  
  if (xReturned = xTaskCreate(
    vTaskSensorRead,
    "sensor1",
    STACK_SIZE,
    (void *) &taskSensor,
    tskIDLE_PRIORITY,
    &taskSensor.xSensorRead
  ) == pdFAIL) {
    Serial.println("could not create sensor task");
  }

}

void loop(void) 
{
  data ptr;
  if ( xQueueReceive(tx_queue, &ptr, pdMS_TO_TICKS(T_send / 100)) == pdFALSE) {
    Serial.print("empty tx buffer! time: ");
    Serial.println(millis());
  }



  vTaskDelay(pdMS_TO_TICKS(T_send));
}