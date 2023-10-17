#include "BluetoothSerial.h"
#include "ESP32Servo.h"

String device_name = "My Robot Arm";  //Give name to Bluetooth Device

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#if !defined(CONFIG_BT_SPP_ENABLED)
#error Serial Bluetooth not available or not enabled. It is only available for the ESP32 chip.
#endif

BluetoothSerial BTE;

float targets[5][200][6];
float inc[6];
float angles[6];

int rec=0;  // 0=no_record  1=record
int once=0; // 0=loop_off   1=loop_on 
int stop=1; // 0=break      1=continue 

int byte1; // first byte from bluetooth
int byte2; // second byte from bluetooth

int res = 150;    // Subdivisions between targets for sweep
int speed = 10;   // 1=min_speed  20=max_speed
int espera = 5; // [ms] , wait time before go to next target

int file = 0;
int y = 0;
int max_y[5]; // qty of targets recorded

Servo Joint1; // Base
Servo Joint2; // Shoulder
Servo Joint3; // Elbow
Servo Joint4; // Wrist roll
Servo Joint5; // Wrist pitch/yaw
Servo Joint6; // End efector

void AddTarget();  // Save actual position in targets[]
void PlaySmooth();  // Play in loop fron targets[]
void WriteAngles();  // Write angles to servomotors
void AplyIncrement();  // Apply increment 
void OledPrint();  // Update Slider Posittion On Oled Display

void setup() {
  Serial.begin(115200);
  BTE.begin(device_name);

  Joint1.attach(32);
  Joint2.attach(33);
  Joint3.attach(25);
  Joint4.attach(26);
  Joint5.attach(27);
  Joint6.attach(14);
}

void loop() {
  if (BTE.available() >= 2) { // #if are 2 bytes in buffer; do
    
    byte1 = BTE.read(); //Get byte2
    byte2 = BTE.read(); //Get byte2

    if (byte1 < 7) { // Move joints manually
      switch (byte1) {
        case 1: // Move Joint 1
          angles[0] = byte2;
          Joint1.write(angles[0]);
          break;
        case 2: // Move Joint 2
          angles[1] = byte2;
          Joint2.write(angles[1]); 
          break;
        case 3: // Move Joint 2
          angles[2] = byte2;
          Joint3.write(angles[2]); 
          break;
        case 4: // Move Joint 4
          angles[3] = byte2;
          Joint4.write(angles[3]); 
          break;
        case 5: // Move Joint 5
          angles[4] = byte2;
          Joint5.write(angles[4]); 
          break;
        case 6: // Move Joint 6  
          angles[5] = byte2;
          Joint6.write(angles[5]); 
          break;
        default:
          break;  
      }
    }

    if (byte1 > 10) { // Actions
      switch (byte1) {
        case 11: // Disable Servos
          digitalWrite(32, LOW);
          digitalWrite(33, LOW);
          digitalWrite(25, LOW);
          digitalWrite(26, LOW);
          digitalWrite(27, LOW);
          digitalWrite(14, LOW);
          break;
        case 12: // Recording mode
          rec = 1;
          y = 0;
          file = byte2;
          break;
        case 13: // Add target
          AddTarget();
          y++;
          break;
        case 14: // Delete last target
          y--;
          break;
        case 15: // Exit recording mode & save
          max_y[byte2] = y;
          rec = 0;
          break;
        case 16: // Play in loop
          once = 2;
          stop = 1;
          file = byte2;
          PlaySmooth();
          break;
        case 17: // Play once
          once = 0;
          stop = 1;
          file = byte2;
          PlaySmooth();
          break;
        case 18: // Cancel record
          rec = 0; 
        default:
          break;
      }
    }

  }
}

void PlaySmooth() {
  while (true) { // Play in loop
    if (max_y[byte2] == 0) {
      while (true) {
        if (BTE.available() >= 1) {
          stop = BTE.read();
        }
        if (stop == 0) {
          break;
        }
      }
    }
    if (stop == 0) {
      break;
    }
    for (int j=0; j<=5; j++) { // Calculate increments to go from current position to start target
      inc[j] = (targets[file][0][j]-angles[j])/res;
    }
    for (int j=0; j<res; j++) { // Go
      AplyIncrement();
      WriteAngles();
      delay(20-speed);
    }

    if (once == 1) {
      break;
    } 

    if (once <1) {
      once += 1;
    }

    for (int i=0; i<(max_y[byte2]-1); i++) { // Follow targets

      for (int j=0; j<=5; j++) { // Calculate increments to go from i target to i+1 target
        inc[j] = (targets[file][i+1][j]-targets[file][i][j])/res;
      }
      for (int j=0; j<res; j++) { // Go
        AplyIncrement();
        WriteAngles();
        delay(20-speed);
        if (BTE.available() >= 1) {
          stop = BTE.read();
        }
        if (stop == 0) {
          break;
        } 
      }

      if (stop == 0) {
        break;
      }

      for (int j=0; j<=5; j++) { // Update angles ​​to avoid any gap caused by the rounding of the increments
        angles[j] = targets[file][i+1][j];
      }
      delay(espera);       
    }
  }
}

void AddTarget() {
  for (int k=0; k<=5; k++) {
    targets[file][y][k] = angles[k];
  }
}
void WriteAngles() {
  Joint1.write(angles[0]);
  Joint2.write(angles[1]);
  Joint3.write(angles[2]);
  Joint4.write(angles[3]);  
  Joint5.write(angles[4]);
  Joint6.write(angles[5]);
}
void OledPrint() {
  //slider position on 128xx64 i2c oled
}
void AplyIncrement() {
  for (int k=0; k<=5; k++) {
    angles[k] += inc[k];
  }
}