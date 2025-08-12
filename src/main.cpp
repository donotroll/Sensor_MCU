#include "common.h"
#include "Sensor/sensor.h"
#include "WiFi.h"
#include <HTTPClient.h>


const char* ssid = "yuhome";
const char* pass = "MyHome@5WyvernSt";
const char* server = "172.20.10.5";
const int port = 8888;

const int relay = 23;

Adafruit_INA219 ina219(0x40);
Adafruit_INA219 ina219_b(0x4A);
vTaskSensor* taskSensor1;
vTaskSensor* taskSensor2;
//UDP
uint8_t rx_buffer[RX_BUFFER_LEN];


//buffer 
float data_buffer[SEN_BUFFER_LEN] = {0};
QueueHandle_t tx_queue;
QueueHandle_t buffer_queue;


//Wifi
WiFiClient client;

void wifi_init() {
  Serial.println();
  Serial.print("[WiFi] Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, pass);
  // Auto reconnect is set true as default
  // To set auto connect off, use the following function
  //    WiFi.setAutoReconnect(false);

  // Will try for about 10 seconds (20x 500ms)
  int tryDelay = 500;
  int numberOfTries = 1000;

  // Wait for the WiFi event
  while (true) {

    switch (WiFi.status()) {
      case WL_NO_SSID_AVAIL: Serial.println("[WiFi] SSID not found"); break;
      case WL_CONNECT_FAILED:
        Serial.print("[WiFi] Failed - WiFi not connected! Reason: ");
        return;
        break;
      case WL_CONNECTION_LOST: Serial.println("[WiFi] Connection was lost"); break;
      case WL_SCAN_COMPLETED:  Serial.println("[WiFi] Scan is completed"); break;
      case WL_DISCONNECTED:    Serial.println("[WiFi] WiFi is disconnected"); break;
      case WL_CONNECTED:
        Serial.println("[WiFi] WiFi is connected!");
        Serial.print("[WiFi] IP address: ");
        Serial.println(WiFi.localIP());
        return;
        break;
      default:
        Serial.print("[WiFi] WiFi Status: ");
        Serial.println(WiFi.status());
        break;
    }
    delay(tryDelay);

    if (numberOfTries <= 0) {
      Serial.print("[WiFi] Failed to connect to WiFi!");
      // Use disconnect function to force stop trying to connect
      WiFi.disconnect();
      return;
    } else {
      numberOfTries--;
    }
  }
}



void setup(void) 
{

  //begin serial
  Serial.begin(115200);
  while (!Serial) {
      // will pause Zero, Leonardo, etc until serial console opens
      delay(1);
  }
  delay(5000);
  Serial.println("Hello!");


  //init wifi
  wifi_init();

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


byte get_flags(UDPpacket* packet) {
  byte flags = 0;
  if (packet->id == 1) 
    flags = flags | 0x4;

  flags = flags | packet->read_flag;

  return flags;
}

void loop(void) {

 for (;;) {
  UDPpacket *packet = new UDPpacket;
  if ( xQueueReceive(tx_queue, packet, portMAX_DELAY) == pdFALSE) {
    //Serial.print("tx buffer remains empty? time: ");
    //Serial.println(millis());
    continue;
  }
  Serial.println("===== Received packet: ===== \n time:");
  Serial.println(millis());
  printPacket(*packet);
  
  if (client.connect(server, port)) {
    //Serial.println("printing!");
    printPacket(*packet);

    client.write(packet->id);  
    client.write("\r\n\n");
    client.write(packet->read_flag);
    client.write("\r\n\n");
    client.write((uint8_t *)&(packet->T_sample), sizeof(packet->T_sample));
    client.write("\r\n\n");
    client.write((uint8_t *)(packet->data[0]->dataPoints), packet->data[0]->len * sizeof(float));
    client.write("\r\n\n");
    client.write((uint8_t *)(packet->data[1]->dataPoints), packet->data[1]->len * sizeof(float));
    client.write("\r\n\n");
    client.write((uint8_t *)(packet->data[2]->dataPoints), packet->data[2]->len * sizeof(float));

    client.flush();

  }


  for (int i = 0; i < packet->sz; ++i) {
    xQueueSend(buffer_queue, (void *)&(packet->data[i]), portMAX_DELAY);
  }

  delete packet;
 }
}