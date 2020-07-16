/** RF24Mesh_Example.ino by TMRh20

   This example sketch shows how to manually configure a node via RF24Mesh, and send data to the
   master node.
   The nodes will refresh their network address as soon as a single write fails. This allows the
   nodes to change position in relation to each other and the master node.
*/


#include "RF24.h"
#include "RF24Network.h"
#include "RF24Mesh.h"
#include <SPI.h>
#include <DS18B20.h>
#include <BH1750.h>
#include <EduIntro.h>
#include <Adafruit_BMP085.h>



BH1750 lightMeter;
Adafruit_BMP085 bmp;

DHT11 dht11(D8); 



//PIN  NRF24L01  Arduino UNO
//1      GND         GND
//2      VCC         3.3V
//3      CE         digIO 9
//4      CSN        digIO 10
//5      SCK        digIO 13
//6      MOSI       digIO 11
//7      MISO       digIO 12

/**** Configure the nrf24l01 CE and CS pins ****/
RF24 radio(9, 10);
RF24Network network(radio);
RF24Mesh mesh(radio, network);

/**
   User Configuration: nodeID - A unique identifier for each radio. Allows addressing
   to change dynamically with physical changes to the mesh.

   In this example, configuration takes place below, prior to uploading the sketch to the device
   A unique value from 1-255 must be configured for each node.
   This will be stored in EEPROM on AVR devices, so remains persistent between further uploads, loss of power, etc.

 **/
#define nodeID 4


//数据包结构体
struct sensor_package {
  float temperature;
  unsigned long humidity;
  unsigned long light;
  unsigned long atm;
};

unsigned long random_timeout;
unsigned long last_sent;


void setup() {

  Serial.begin(115200);


  randomSeed(analogRead(0));
  random_timeout = random(2000, 4000);

  // printf_begin();
  // Set the nodeID manually
  mesh.setNodeID(nodeID);
  // Connect to the mesh
  Serial.println(F("Connecting to the mesh..."));
  mesh.begin();


  
  if (!bmp.begin()) {
    Serial.println("Could not find a valid BMP085 sensor, check wiring!");
  while (1) {}
  }

   Wire.begin();
   lightMeter.begin();
}



void loop() {

  mesh.update();

  //获取环境信息
  //GY-30
  float lux = lightMeter.readLightLevel();
  //DHT11
  dht11.update();
  int humidity = dht11.readHumidity();     // Reading the humidity index
  //GY-68
  float temp = bmp.readTemperature();
  long atm = bmp.readPressure();


  Serial.print("Light: ");
  Serial.print(lux);
  Serial.println(" lx\t");

  Serial.print("Temperature: ");
  Serial.print(temp);
  Serial.println(" *C\t");
    
  Serial.print("Pressure: ");
  Serial.print(atm);
  Serial.println(" Pa\t");
  
  Serial.print("humidity: ");
  Serial.print(humidity);
  Serial.println(" %");

  // Send to the master node every second
  unsigned long now = millis();  //获取运行时间
  if (now - last_sent >= random_timeout) {

    last_sent = now;
    random_timeout = random(3000, 6000);

    sensor_package payload = {0};
    payload.temperature = temp;

    // Send an 'M' type message containing the current millis()
    if (!mesh.write(&payload, 'M', sizeof(sensor_package))) {

      // If a write fails, check connectivity to the mesh network
      if ( ! mesh.checkConnection() ) {
        //refresh the network address
        Serial.println("Renewing Address");
        if (!mesh.renewAddress()) {
          //If address renewal fails, reconfigure the radio and restart the mesh
          //This allows recovery from most if not all radio errors
          mesh.begin();
        }
      } else {
        Serial.println("Send fail, Test OK");
      }
    } else {
      Serial.print("Send OK: ");
      Serial.println(payload.temperature);
    }
  }
  delay(200);

}
