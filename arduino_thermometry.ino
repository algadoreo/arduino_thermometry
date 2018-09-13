#include <Wire.h>

// constants
const int dt = 2000; // sampling rate [microseconds]
const int Dt = 200;  // readout rate [milliseconds]
const int N = 250;   // buffer length
const int L = 200;   // boxcar length
const byte Nch = 12;  // number of channels per device
const byte Ndev = 10; // number of devices in chain

unsigned long sum[Nch] = {0};    // sums from current board
unsigned long rcvsum[Ndev][Nch] = {0}; // sums received from another board

bool isDev0 = true; // is (not) device connected via serial

void setup() {
  unsigned short add_ptr = 0;
  unsigned short sub_ptr = add_ptr + N - L;
  unsigned long old_micros = 0, new_micros = 0;
  unsigned long old_millis = 0, new_millis = 0;
  unsigned short buffdata[N][Nch] = {0};
  unsigned short old_voltage = 0, new_voltage = 0;
  
  Serial.begin(250000);
  analogReadResolution(12);
  analogWriteResolution(12);
  for (byte i=54; i<66; i++) {
    pinMode(i, INPUT);
  }

  // Start the first I2C bus as master
  Wire.begin();
  Wire.setClock(400000);
  
  // begin I2C bus on address 9
  Wire1.begin(9);
  Wire1.setClock(400000);
  Wire1.onReceive(receiveEvent);

  while (true) {
//  while (millis() < 5000) {
    // take samples every dt microseconds
    new_micros = micros();
    if ((new_micros - old_micros) >= dt) {
      for (byte j=0; j<Nch; j++) {
        new_voltage = analogRead(54+j);
        old_voltage = buffdata[sub_ptr][j];
        buffdata[add_ptr][j] = new_voltage;
        sum[j] += new_voltage - old_voltage;
      }

      old_micros = new_micros;
      (add_ptr==N-1) ? add_ptr = 0 : add_ptr++;
      (sub_ptr==N-1) ? sub_ptr = 0 : sub_ptr++;
    }

    // if device 0, send out sums every Dt MILLIseconds
    // TO DO: message chain initialized by serial request; once
    // once that is set up, this shouldn't be time-based
    if (isDev0) {
      new_millis = millis();
      if ((new_millis - old_millis) >= Dt) {
        Wire.beginTransmission(9);
        Wire.write(1);
        Wire.write(1);
        for (byte i=0; i<Nch; i++) {
          for (byte j=0; j<3; j++) {
            Wire.write(sum[i] >> (8*j));
          }
        }
        Wire.endTransmission();
        old_millis = new_millis;
      }
    }
  }
}

void receiveEvent(int Nbytes) {
  unsigned short packetsTotal = 0;
  unsigned short packetNumber = 0;
  unsigned int x = 0;
  
  while (Wire1.available()) {
    // first two bytes are meta-data
    packetsTotal = Wire1.read();
    packetNumber = Wire1.read();

    // fill the buffer with the received packets
    for (byte i=0; i<Nch; i++) {
      x = 0;
      for (byte j=0; j<3; j++) {
        x = (Wire1.read() << (8*j)) | x;
      }
      rcvsum[packetNumber-1][i] = x;
    }
  }

  // if device 0, output to serial once all packets received;
  // else pass along message on `Wire`
  if (isDev0) {
    if (packetNumber == packetsTotal) {
      for (byte k=0; k<packetsTotal; k++) {
        Serial.print(k+1);
        // TO DO: fix serial output; currently limited by buffer size
//        for (byte i=0; i<Nch; i++) {
        for (byte i=0; i<8; i++) {
          Serial.print(" ");
          Serial.print(rcvsum[k][i]);
        }
//        Serial.println();
        Serial.print(" ");
      }
      Serial.println();
    }
  } else {
    // send the values received from other devices
    // have to break into one packet per device due to 32-byte limit
    for (byte k=0; k<packetsTotal; k++) {
      Wire.beginTransmission(9);
      Wire.write(packetsTotal+1);
      Wire.write(k+1);
      for (byte i=0; i<Nch; i++) {
        for (byte j=0; j<3; j++) {
          Wire.write(rcvsum[k][i] >> (8*j));
        }
      }
      Wire.endTransmission(); 
    }

    // send the values on current device
    Wire.beginTransmission(9);
    Wire.write(packetsTotal+1);
    Wire.write(packetsTotal+1);
    for (byte i=0; i<Nch; i++) {
      for (byte j=0; j<3; j++) {
        Wire.write(sum[i] >> (8*j));
      }
    }
    Wire.endTransmission();
  }
}

void loop() {
  // empty because the loop is in the setup()
}

