//Thermometer with thermistor

/*thermistor parameters:
 * RT0: 10 000 Ω
 * B: 3977 K +- 0.75%
 * T0:  25 C
 * +- 5%
 */

//These values are in the datasheet
#define RT0 10000   // Ω
#define B 3952      // K
//--------------------------------------


#define VCC 3.3    //Supply voltage
#define R 50000  //R=50KΩ

//Variables
float RT, VR, ln, TX, T0, VRT;
float arrRT[5], arrVR[5], arrln[5], arrTX[5], arrVRT[5];

void setup() {
  Serial.begin(9600);
  T0 = 25 + 273.15;                 //Temperature T0 from datasheet, conversion from Celsius to kelvin
}

void loop() {
  VRT = analogRead(A16);              //Acquisition analog value of VRT
  VRT = (3.30 / 1023.00) * VRT;      //Conversion to voltage
  VR = VCC - VRT;
  RT = VRT / (VR / R);               //Resistance of RT

  ln = log(RT / RT0);
  TX = (1 / ((ln / B) + (1 / T0))); //Temperature from thermistor

  TX = TX - 273.15;                 //Conversion to Celsius

  Serial.print("Temperature:");
  Serial.print("\t");
  Serial.print(TX);
  Serial.print("C\t\t");
  Serial.print(TX + 273.15);        //Conversion to Kelvin
  Serial.print("K\t\t");
  Serial.print((TX * 1.8) + 32);    //Conversion to Fahrenheit
  Serial.println("F");
  delay(500);


//  arrVRT[0] = analogRead(A16);
//  arrVRT[1] = analogRead(A17);
//  arrVRT[2] = analogRead(A18);
//  arrVRT[3] = analogRead(A19);
//  for (int i = 0; i < 4; i++){
//    arrVRT[i] = (5.00 / 1023.00) * arrVRT[i];
//    arrVR[i] = VCC - arrVRT[i];
//    arrRT[i] = arrVRT[i] / (arrVR[i] / R);               
//  
//    arrln[i] = log(arrRT[i] / RT0);
//    arrTX[i] = (1 / ((arrln[0] / B) + (1 / T0)));
//    arrTX[i] = arrTX[i] - 273.15;
//
//    Serial.print("Temperature:");
//    Serial.print("\t");
//    Serial.print(TX);
//    Serial.print("C\t\t");
//    delay(500);
//  }

}