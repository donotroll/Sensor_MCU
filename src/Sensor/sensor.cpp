#include <Adafruit_INA219.h>
#include "common.h"
#include "sensor.h"


#define  READ_CURR  0b001
#define  READ_VOLT  0b010
#define  READ_POW  0b100
#define DEFAULT_TIMEOUT pdMS_TO_TICKS( DEFAULT_T_SAMPLE * 5)


void printData(data* d, int index) {
  if (!d) {
    Serial.print("  data["); Serial.print(index); Serial.println("] = nullptr");
    return;
  }

  Serial.print("  data["); Serial.print(index); Serial.println("]:");
  Serial.print("    type: "); Serial.println(d->type);
  Serial.print("    len: "); Serial.print(d->len);
  Serial.print(", curr: "); Serial.println(d->curr);

  Serial.print("    dataPoints: [");
  for (int i = 0; i < d->curr && i < d->len; ++i) {
    Serial.print(d->dataPoints[i]); 
    if (i < d->curr - 1) Serial.print(", ");
  }
  Serial.println("]");
}

void printPacket(UDPpacket& packet) {
  Serial.println("=== UDPpacket ===");
  Serial.print("id: "); Serial.println(packet.id);
  Serial.print("sz: "); Serial.print(packet.sz);
  Serial.print(", curr: "); Serial.println(packet.curr);

  Serial.print("read_flag: 0b");
  for (int i = 2; i >= 0; --i) {
    Serial.print((packet.read_flag >> i) & 1);
  }
  Serial.println();

  Serial.print("T_sample: "); Serial.print(packet.T_sample); Serial.println(" ms");

  for (int i = 0; i < packet.sz; ++i) {
    printData(packet.data[i], i);
  }
}


//SENSOR

vTaskSensor::vTaskSensor(Adafruit_INA219 ina219, int id, QueueHandle_t in, QueueHandle_t out) {
  this->ina219 = ina219;
  this->in_buffer = in;
  this->out_buffer = out;
  this->id = id;

  for (int i = 0; i < 3; ++i) {
    if ( xQueueReceive(in_buffer, (void *)&read_to[i], DEFAULT_TIMEOUT) == pdFALSE) {
      Serial.println("uninitialised buffer!");
    }
    //Serial.println((unsigned int) read_to[i]);
    printData(read_to[i], i);
  }


  //default initialise
  this->packet_curr= new UDPpacket;
  this->packet_next= new UDPpacket;
  this->packet_curr->id = id;
  printPacket(*this->packet_curr);


  // Initialize the INA219.
  // By default the initialization will use the largest range (32V, 2A).  However
  // you can call a setCalibration function to change this range (see comments).
  if (! this->ina219.begin()) {
    Serial.println("Failed to find INA219 chip");
    //while (1) { delay(10); }
  }
  // To use a slightly lower 32V, 1A range (higher precision on amps):
  //ina219.setCalibration_32V_1A();
  // Or to use a lower 16V, 400mA range (higher precision on volts and amps):
  //ina219.setCalibration_16V_400mA();

  Serial.print("Measuring voltage and current with INA219, id: ");
  Serial.println(id);

}

//maybe add inside class?
float ReadFrom(Adafruit_INA219 ina219, int type) {
  return 1;
  switch (type)
  {
    case CURRENT:
      return ina219.getCurrent_mA();
      break;
    case VOLTAGE:
      return ina219.getBusVoltage_V();
    case POWER:
      return ina219.getPower_mW();
    default:
      Serial.println("not configured");
      return 0;
  }
}



void data_reset(data *d, int type) {
  d->curr = 0;
  d->type = type;
}

void vTaskSensorRead(void *parameters) {
  //copy params
  vTaskSensor *sensor = static_cast<vTaskSensor *>(parameters);

  for(;;) {
  UDPpacket* packet = sensor->packet_curr;
  //printPacket(*packet);

  //modify next packet here?
    Serial.print("Sensor ID: ");
   Serial.println(sensor->id);
   Serial.print("Time: ");
   Serial.println(millis());
   
   //ordered by current, voltage, power
   for (int i = 0; i < 3; ++i) {
    //old
    data *d_old = sensor->read_to[i];
    //retrieve new buffer, config? (blocking)
    if (d_old->curr == d_old->len) {
      data *d_new = new data;
      if ( ((packet->read_flag >> i) & 0x1) == 0x1) {
        if ( xQueueReceive(sensor->in_buffer,d_new, DEFAULT_TIMEOUT ) == pdFALSE) {
          Serial.println("Something is blocking buffer queue!");
        }
        data_reset(d_new, i);
        sensor->read_to[i] = d_new;
      }
    }

    //point again, then read
    data *d = sensor->read_to[i];
    d->dataPoints[d->curr] =1;
    d->curr++;

    //add to packet
    if (d->curr == d->len) {
      packet->data[packet->curr] = d;
      packet->curr++;
    }

   }

   //send packet to UDP queue, set new packet config
   if (packet->sz == packet->curr) {
   if ( xQueueSend(sensor->out_buffer, packet, DEFAULT_TIMEOUT) == pdFALSE) { //WARNING DO NOT ACCESS OLD PACKET!!
    Serial.println("something is blocking output buffer queue");
   }
    sensor->packet_curr = sensor->packet_next;

    //TODO: CONFIG

    //copy param
    sensor->packet_next = new UDPpacket;
    sensor->packet_next->id = sensor->packet_curr->id;
    sensor->packet_next->read_flag = sensor->packet_curr->read_flag;
    sensor->packet_next->sz = sensor->packet_curr->sz;
   }

/*
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
*/

  if (xTaskDelayUntil(&(sensor->read_last), pdMS_TO_TICKS(packet->T_sample)) == pdTRUE) {
  }
  else {
    Serial.println("Failed to delay task");
  }
}
}
