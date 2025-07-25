#include <freertos/queue.h>

#define STACK_SIZE 1024
#define RX_BUFFER_LEN 256
#define TX_BUFFER_LEN 8012
#define SEN_LEN 2
#define BUFFER_LEN 5
#define SEN_BUFFER_LEN (BUFFER_LEN * SEN_LEN)

#define NONE -1
#define CURRENT 0
#define VOLTAGE 1
#define POWER 2


#define DATA_LEN 3


#define DEFAULT_T_SAMPLE 500 //ms
//FREERTOS
QueueHandle_t tx_queue;
QueueHandle_t buffer_queue;


//ms


//buffer 
float current_buf[SEN_BUFFER_LEN];
float voltage_buf[SEN_BUFFER_LEN];

