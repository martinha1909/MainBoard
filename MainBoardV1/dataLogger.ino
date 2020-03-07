/*
 * Simple data logger.
 */
#include "SdFat.h"
#include <Wire.h>
#include <SPI.h> // SPI library included for SparkFunLSM9DS1
#include <SparkFunLSM9DS1.h> // SparkFun LSM9DS1 library
#include <Adafruit_MPL3115A2.h>
// SD chip select pin.  Be sure to disable any other SPI devices such as Enet.
//const uint8_t chipSelect = BUILTIN_SDCARD;

// Interval between data records in milliseconds.
// The interval must be greater than the maximum SD write latency plus the
// time to acquire and write data to the SD to avoid overrun errors.
// Run the bench example to check the quality of your SD card.
const uint32_t SAMPLE_INTERVAL_MS = 1000;

// Log file base name.  Must be six characters or less.
//------------------------------------------------------------------------------
// File system object.
Adafruit_MPL3115A2 baro = Adafruit_MPL3115A2();
LSM9DS1 imu;

#define DECLINATION 14.02 // Declination (degrees) in Boulder, CO.

//Thermometer with thermistor
/*thermistor parameters:
 * RT0: 10 000 Ω
 * B: 3977 K +- 0.75%
 * T0:  25 C
 * +- 5%
 */

//These values are in the datasheet
#define RT0 50000   // Ω
#define B 3952      // K
//--------------------------------------


#define VCC 3.3    //Supply voltage
#define R 10000  //R=50KΩ

#define THERMISTORSCOUNT 4
#define PHOTORESISTORCOUNT 4

//Variables
#define FILE_BASE_NAME  "sensors"
#define error(msg) sd.errorHalt(F(msg))
void readIMU();
void printAccel();
void printMag();
void readThermistors();
void readBarometer();
void readPhotoresistors();
void recordValues();
void printAttitude(float ax, float ay, float az, float mx, float my, float mz);
//Variables
SdFatSdio sd;
SdFile myfile;
int photoValueArray[PHOTORESISTORCOUNT]; 
int thermoValueArray[THERMISTORSCOUNT];
int imuGyrox, imuGyroy, imuGyroz, imuAccelx, imuAccely, imuAccelz, imuMagx, imuMagy, imuMagz; 
int baroPressure, baroAltitude, baroTemperature;
int attitudePitch, attitudeRoll;
int globalTime;
float RT, VR, ln, TX, T0, VRT;
float arrRT[5], arrVR[5], arrln[5], arrTX[5], arrVRT[5];
int thermistorPins[] = {A16, A17, A18, A19, A20};

int photoresistorPins[] = {A12, A13, A14, A15};

int pinBaroSDA = 4; 
int pinBaroSCL = 3;

// Time in micros for next data record.
uint32_t logTime;

//==============================================================================
// User functions.  Edit writeHeader() and logData() for your requirements.

//------------------------------------------------------------------------------
// Write data header.
//void writeHeader() {
//  file.print(F("micros"));
//  for (uint8_t i = 0; i < ANALOG_COUNT; i++) {
//    file.print(F(",adc"));
//    file.print(i, DEC);
//  }
//  file.println();
//}
//------------------------------------------------------------------------------
// Log a data record.
void recordValues() {
  myfile.print(globalTime); myfile.print(",");
    for(int i=0; i<THERMISTORSCOUNT; i++)
    {
      myfile.print(thermoValueArray[i]);
      myfile.print(",");
    }
    for(int i=0; i<PHOTORESISTORCOUNT; i++)
    {
      myfile.print(photoValueArray[i]);
      myfile.print(",");
    }
    myfile.print(baroPressure); myfile.print(",");
    myfile.print(baroAltitude); myfile.print(",");
    myfile.print(baroTemperature);  myfile.print(",");
    myfile.print(imuGyrox); myfile.print(",");
    myfile.print(imuGyroy); myfile.print(",");
    myfile.print(imuGyroz); myfile.print(",");
    myfile.print(imuAccelx);  myfile.print(",");
    myfile.print(imuAccely);  myfile.print(",");
    myfile.print(imuAccelz);  myfile.print(",");
    myfile.print(imuMagx);   myfile.print(",");
    myfile.print(imuMagy);  myfile.print(",");
    myfile.print(imuMagz);  myfile.print(",");
    myfile.print(attitudePitch); myfile.print(",");
    myfile.print(attitudeRoll); myfile.print(",");
    myfile.println();
}
//==============================================================================
// Error messages stored in flash.
#define error(msg) sd.errorHalt(F(msg))
//------------------------------------------------------------------------------
void setup() {
  const uint8_t BASE_NAME_SIZE = sizeof(FILE_BASE_NAME) - 1;
  char fileName[13] = FILE_BASE_NAME "00.csv";

  Serial.begin(9600);
  
  // Wait for USB Serial 
  while (!Serial) {
    SysCall::yield();
  }
  delay(1000);

  Serial.println(F("Type any character to start"));
//  while (!Serial.available()) {
//    SysCall::yield();
//  }
  
  // Initialize at the highest speed supported by the board that is
  // not over 50 MHz. Try a lower speed if SPI errors occur.
//  /chipSelect, SD_SCK_MHZ(50))
  if (!sd.begin()) {
    sd.initErrorHalt();
  }

  // Find an unused file name.
  if (BASE_NAME_SIZE > 6) {
    error("FILE_BASE_NAME too long");
  }
  while (sd.exists(fileName)) {
    if (fileName[BASE_NAME_SIZE + 1] != '9') {
      fileName[BASE_NAME_SIZE + 1]++;
    } else if (fileName[BASE_NAME_SIZE] != '9') {
      fileName[BASE_NAME_SIZE + 1] = '0';
      fileName[BASE_NAME_SIZE]++;
    } else {
      error("Can't create file name");
    }
  }
  if (!myfile.open(fileName, O_WRONLY | O_CREAT | O_EXCL)) {
    error("file.open");
  }
  // Read any Serial data.
  do {
    delay(10);
  } while (Serial.available() && Serial.read() >= 0);

  Serial.print(F("Logging to: "));
  Serial.println(fileName);
  Serial.println(F("Type any character to stop"));

  // Write data header.
  Serial.println("initialization done.");
    myfile.print("Time:,");
    myfile.print("Temperature 0:,");
    myfile.print("Temperature 1:,");
    myfile.print("Temperature 2:,");
    myfile.print("Temperature 3:,");
    myfile.print("Photoresistor 0:,");
    myfile.print("Photoresistor 1:,");
    myfile.print("Photoresistor 2:,");
    myfile.print("Photoresistor 3:,");
    myfile.print("Pressure (Hg):,");
    myfile.print("Altitude(meters):,");
    myfile.print("Temperature(C):,");
    myfile.print("IMU Gyro(x,y,z):,");
    myfile.print("IMU Accel(x,y,z):,");
    myfile.print("IMU Mag(x,y,z):,");
    myfile.print("Pitch:,");
    myfile.print("Roll:,");
    myfile.println();

  // Start on a multiple of the sample interval.
  logTime = micros()/(1000UL*SAMPLE_INTERVAL_MS) + 1;
  logTime *= 1000UL*SAMPLE_INTERVAL_MS;
}
//------------------------------------------------------------------------------
void loop() {
  // Time for next record.
  logTime += 1000UL*SAMPLE_INTERVAL_MS;

  // Wait for log time.
  int32_t diff;
  do {
    diff = micros() - logTime;
  } while (diff < 0);

  // Check for data rate too high.
  if (diff > 10) {
    error("Missed data record");
  }

  readThermistors();
  readPhotoresistors();
  readBarometer();
  readIMU();
  recordValues();

  // Force data to SD and update the directory entry to avoid data loss.
  if (!myfile.sync() || myfile.getWriteError()) {
    error("write error");
  }

  if (Serial.available()) {
    // Close file and stop.
    myfile.close();
    Serial.println(F("Done"));
    SysCall::halt();
  }
}
void readThermistors(){
  for (int i = 0; i < THERMISTORSCOUNT; i++){
    arrVRT[i] = analogRead(thermistorPins[i]);
    arrVRT[i] = (3.30 / 1023.00) * arrVRT[i];
    arrVR[i] = VCC - arrVRT[i];
    arrRT[i] = arrVRT[i] / (arrVR[i] / R);               
  
    arrln[i] = log(arrRT[i] / RT0);
    arrTX[i] = (1 / ((arrln[i] / B) + (1 / T0)));
    arrTX[i] = arrTX[i] - 273.15;
    thermoValueArray[i] = arrTX[i];
    Serial.printf("Temperature %d: ", i);
    Serial.print(arrTX[i]);
    Serial.print("C\t");
  }
  Serial.println();
}

void readPhotoresistors(){
  for (int i = 0; i < PHOTORESISTORCOUNT; i++){
    Serial.printf("Photoresistor %d: %d\t", i, analogRead(photoresistorPins[i]));

    photoValueArray[i] = analogRead(photoresistorPins[i]);
  }
  Serial.println();
}

void readBarometer(){
  if (! baro.begin(&Wire2)) {
    Serial.println("Failed to communicate with Barometer.");
    return;
  }
  
  float pascals = baro.getPressure();
  baroPressure = pascals/3377;
  // Our weather page presents pressure in Inches (Hg)
  // Use http://www.onlineconversion.com/pressure.htm for other units
  float altm = baro.getAltitude();
  baroAltitude = altm;
  float tempC = baro.getTemperature();
  baroTemperature = tempC;
  Serial.printf("%.2f Inches (Hg)\t%.2f meters\t%.2f C\n", pascals/3377, altm, tempC);
}

void readIMU(){
  if (!imu.begin())
  {
    Serial.println("Failed to communicate with IMU.");
    return;
  }
  if ( imu.gyroAvailable() )
  {
    imu.readGyro();
  }
  if ( imu.accelAvailable() )
  {
    imu.readAccel();
  }
  if ( imu.magAvailable() )
  {
    imu.readMag();
  }

  printGyro();  // Print "G: gx, gy, gz"
  printAccel(); // Print "A: ax, ay, az"
  printMag();   // Print "M: mx, my, mz"
  // Print the heading and orientation for fun!
  // Call print attitude. The LSM9DS1's mag x and y
  // axes are opposite to the accelerometer, so my, mx are
  // substituted for each other.
  printAttitude(imu.ax, imu.ay, imu.az,
                -imu.my, -imu.mx, imu.mz);
  Serial.println();
}

void printGyro()
{
  // Now we can use the gx, gy, and gz variables as we please calculated in DPS.
  Serial.print("G: ");
  Serial.print(imu.calcGyro(imu.gx), 2);
  imuGyrox = imu.calcGyro(imu.gx);
  Serial.print(", ");
  Serial.print(imu.calcGyro(imu.gy), 2);
  imuGyroy = imu.calcGyro(imu.gy);
  Serial.print(", ");
  Serial.print(imu.calcGyro(imu.gz), 2);
  imuGyroz = imu.calcGyro(imu.gz);
  Serial.println(" deg/s");
}

void printAccel()
{
  // Now we can use the ax, ay, and az variables as we please calculated in g's.
  Serial.print("A: ");
  Serial.print(imu.calcAccel(imu.ax), 2);
  imuAccelx = imu.calcAccel(imu.ax);
  Serial.print(", ");
  Serial.print(imu.calcAccel(imu.ay), 2);
  imuAccely = imu.calcAccel(imu.ay);
  Serial.print(", ");
  Serial.print(imu.calcAccel(imu.az), 2);
  imuAccelz = imu.calcAccel(imu.az);
  Serial.println(" g");
}

void printMag()
{
  // Now we can use the mx, my, and mz variables as we please calculated in Gauss.
  Serial.print("M: ");
  Serial.print(imu.calcMag(imu.mx), 2);
  imuMagx = imu.calcMag(imu.mx);
  Serial.print(", ");
  Serial.print(imu.calcMag(imu.my), 2);
  imuMagy = imu.calcMag(imu.my);
  Serial.print(", ");
  Serial.print(imu.calcMag(imu.mz), 2);
  imuMagz = imu.calcMag(imu.mz);
  Serial.println(" gauss");
}

// Calculate pitch, roll, and heading.
// Pitch/roll calculations take from this app note:
// http://cache.freescale.com/files/sensors/doc/app_note/AN3461.pdf?fpsp=1
// Heading calculations taken from this app note:
// http://www51.honeywell.com/aero/common/documents/myaerospacecatalog-documents/Defense_Brochures-documents/Magnetic__Literature_Application_notes-documents/AN203_Compass_Heading_Using_Magnetometers.pdf
void printAttitude(float ax, float ay, float az, float mx, float my, float mz)
{
  float roll = atan2(ay, az);
  float pitch = atan2(-ax, sqrt(ay * ay + az * az));

  float heading;
  if (my == 0)
    heading = (mx < 0) ? PI : 0;
  else
    heading = atan2(mx, my);

  heading -= DECLINATION * PI / 180;

  if (heading > PI) heading -= (2 * PI);
  else if (heading < -PI) heading += (2 * PI);

  // Convert everything from radians to degrees:
  heading *= 180.0 / PI;
  pitch *= 180.0 / PI;
  roll  *= 180.0 / PI;
  attitudePitch = pitch;
  attitudeRoll = roll;
  Serial.print("Pitch, Roll: ");
  Serial.print(pitch, 2);
  Serial.print(", ");
  Serial.println(roll, 2);
  Serial.print("Heading: "); Serial.println(heading, 2);
}