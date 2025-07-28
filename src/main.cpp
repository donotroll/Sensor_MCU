#include <AsyncUdp.h>
#include "common.h"
#include "Sensor/sensor.h"


const char* ssid = "WIFI_NAME";
const char* pass = "WIFI_PASSWORD";
const int relay = 23;
AsyncUDP udp;

Adafruit_INA219 ina219(0x44);
Adafruit_INA219 ina219_b(0x40);
vTaskSensor* taskSensor1;
vTaskSensor* taskSensor2;
//UDP
char *rx_buffer[RX_BUFFER_LEN];
char *tx_buffer[TX_BUFFER_LEN];


int T_send = 1000;
//buffer 
float data_buffer[SEN_BUFFER_LEN] = {0};
QueueHandle_t tx_queue;
QueueHandle_t buffer_queue;

void setup(void) 
{

  //begin serial
  Serial.begin(115200);
  while (!Serial) {
      // will pause Zero, Leonardo, etc until serial console opens
      delay(1);
  }
  Serial.println("Hello!");

  //set up buffer queue, partition bufs
  buffer_queue = xQueueCreate(BUFFER_LEN * 3, sizeof (data *));
  for (int i = 0; i < BUFFER_LEN * 2 ; ++i) {
    data *d = new data;
    d->dataPoints = &data_buffer[i * SEN_LEN];
    d->len = SEN_LEN;

    //printData(d, -1);
    //Serial.println((unsigned int) d);
    //queue requires pointer to queuing item don't forget ur queuing pointers!!
    if (xQueueSend(buffer_queue, (void *)&d, portMAX_DELAY) == pdFALSE) {
      Serial.println("Failed to send data to buffer queue");
    }
  }

 
  //set up UDP queue
  tx_queue = xQueueCreate(BUFFER_LEN, sizeof (UDPpacket));


  taskSensor1 = new vTaskSensor(ina219, 0, buffer_queue, tx_queue);
  taskSensor2 = new vTaskSensor(ina219_b, 1, buffer_queue, tx_queue);

  BaseType_t xReturned;
  if (xReturned = xTaskCreate(
    vTaskSensorRead,
    "sensor1",
    STACK_SIZE,
    (void *) taskSensor1,
    tskIDLE_PRIORITY,
    &taskSensor1->xSensorRead
  ) == pdFAIL) {
    Serial.println("could not create sensor task 1");
  }

  if (xReturned = xTaskCreate(
    vTaskSensorRead,
    "sensor2",
    STACK_SIZE,
    (void *) taskSensor2,
    tskIDLE_PRIORITY,
    &taskSensor2->xSensorRead
  ) == pdFAIL) {
    Serial.println("could not create sensor task 2");
  }

}

void loop(void) {

 for (;;) {
  UDPpacket *packet = new UDPpacket;
  if ( xQueueReceive(tx_queue, packet, portMAX_DELAY) == pdFALSE) {
    Serial.print("tx buffer remains empty? time: ");
    Serial.println(millis());
    continue;
  }
  Serial.println("===== Received packet: ===== \n time:");
  Serial.println(millis());
  printPacket(*packet);

  for (int i = 0; i < packet->sz; ++i) {
    xQueueSend(buffer_queue, (void *)&(packet->data[i]), portMAX_DELAY);
  }

  delete packet;
 }
}