#include <Adafruit_INA219.h>


struct data {
  float *dataPoints;
  int len = SEN_LEN;
  int type = NONE;
  int curr = 0;
};


struct UDPpacket {
  struct data* data[3] = {nullptr, nullptr, nullptr};
  int id = 0;
  int sz = 3;
  int curr = 0;
  uint8_t read_flag = 0b111;
  int T_sample = DEFAULT_T_SAMPLE; //ms
};


class vTaskSensor {
  public:
  //config
  Adafruit_INA219 ina219;
  int id = 0;

  //i/o
  QueueHandle_t out_buffer;
  QueueHandle_t in_buffer;
  QueueHandle_t config_queue;


  TaskHandle_t xSensorRead;


  //data handling
  struct data* read_to[3] = {nullptr, nullptr, nullptr}; 
  struct UDPpacket* packet_curr;
  struct UDPpacket* packet_next;
  TickType_t read_last = 0;

  vTaskSensor(Adafruit_INA219 ina219, int id, QueueHandle_t in, QueueHandle_t out);
};


void vTaskSensorRead(void *parameters);
void printPacket(UDPpacket& packet);
void printData(data* d, int index);