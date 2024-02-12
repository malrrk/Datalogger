#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>
//#include <ArduinoSTL.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <SPI.h>
#include <SD.h>
#include <TimeLib.h>
#include "Adafruit_TCS34725.h"







using namespace std;

// Constants (valid for the whole programm)
const float No_Val = -404;
const int chipSelect = BUILTIN_SDCARD;
const int LED_PIN = 13;

void blink_short_2(){
  digitalWrite(LED_PIN, HIGH);
  delay(500);
  digitalWrite(LED_PIN, LOW);
  delay(500);
  digitalWrite(LED_PIN, HIGH);
  delay(500);
  digitalWrite(LED_PIN, LOW);
}

void blink_long_1(){
  digitalWrite(LED_PIN, HIGH);
  delay(1000);
  digitalWrite(LED_PIN, LOW);
}

class LED{
  private:
    int PIN;
    bool state = false;

  public:
    LED(int PIN){
      this->PIN = PIN;
    }

    void blink(){
      if(state){
        digitalWrite(13, LOW);
      }else{
          digitalWrite(13, HIGH);
      }
      state = !state;
    }
};

class buzzer{
  private:
    int PIN;

  public:
    buzzer(int PIN){
      this->PIN = PIN;
    }

    void buzz(){
      beep(23, 2000, 200, 350);

      beep(23, 1000, 200, 250);
    }

    void beep(int pin, int f, int d1, int d2){
      tone(pin, f);
      delay(d1);
      noTone(pin);
      delay(d2);
    }
};

class button{
  private:
    int PIN;

  public:
    button(int PIN){
      this->PIN = PIN;
    }
};

class RTC{
  private:
    
  public:
    RTC() {

    }

    String get_time() {
      return String(day()) + ":" + String(hour()) + ":" + String(minute()) + ":" + String(second());
    }
};

class data_logger{
  private:
    RTC clock;
    
  public:
    data_logger(RTC &clock_address) {
      clock = clock_address;

    }

    bool initialise(){
      if(!SD.begin(chipSelect)){
        return false;
      }
      return true;
    }

    void log_data(String data) {
      File dataFile = SD.open("datalog.txt", FILE_WRITE);
      if (dataFile) {
        data = clock.get_time() + ":" + data;
        dataFile.println(data);
        dataFile.close();
      }else{
        blink_short_2();
        /*data = "Problem mit SD-Karte";
        dataFile.println(data);
        dataFile.close();*/
      }
      Serial.println(data);
    }


    void log_raw_data(double data) {
      File dataFile = SD.open("datalog.txt", FILE_WRITE);
      if (dataFile) {
        dataFile.println(data);
        dataFile.close();
      }else{
        //blink_short_2();
        /*dataFile.println("Problem mit SD-Karte");
        dataFile.close();*/
      }
      Serial.println(data);
    }

};

class sensor{
  protected:
    String position; //positon of the sensor on the probe
    String type; //Identification of the sensor
    String measurement; //the kind of measurement taken by the sensor e.g. temperatue, humidity...

    double data;

  public:
    sensor(String position, String type, String measurement) {
      this->position = position;
      this->type = type;
      this->measurement = measurement;

      data = 0;
    }

    String get_information() {
      return position + ":" + type + ":" + measurement;
    }
};

class sensor_operator{
  protected:

    data_logger logger;

    int AMOUNT;

    String sensor_information;

  public:
    sensor_operator(int AMOUNT, data_logger &logger_address) : logger(logger_address){
      this->AMOUNT = AMOUNT;
    }
};

class Hall_SS490 : public sensor{
  private:
    int PIN;

  public:
    Hall_SS490(String position, String type, String measurement, int PIN) : sensor(position, type, measurement) {
      this->PIN = PIN;
      pinMode(PIN, INPUT);
    }

    long measure_Tesla() {
      return analogRead(PIN);
    }
};

class Hall_SS490_operator : public sensor_operator{
  private:
    Hall_SS490* sensor_list;

    double Tesla_data;
    String Tesla_data_string;

  public:
    Hall_SS490_operator(int AMOUNT, Hall_SS490* sensor_list, data_logger &logger_address) : sensor_operator(AMOUNT, logger_address){  
      this->sensor_list = sensor_list;     
    }

    void take_Measurements() {
      for(int i  = 0; i < AMOUNT; i++){
          Tesla_data = sensor_list[i].measure_Tesla();
          sensor_information = sensor_list[i].get_information();

          Tesla_data_string = String(Tesla_data, 3);

          logger.log_data(sensor_information + ":" + Tesla_data_string);
        }
      }
};

class color_TCS34725 : public sensor{
  private:
    Adafruit_TCS34725 color_sensor;
    byte gammatable[256];
    uint16_t r, g, b, c, colorTemp, lux;


  public:
    color_TCS34725(String position, String type, String measurement) : sensor(position, type, measurement){

      for (int i = 0; i < 256; i++){
        float x = i;
        x /= 255;
        x = pow(x, 2.5);
        x *= 255;
        gammatable[i] = x;
      } 

    }

    bool initialise(){
      color_sensor = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_1X);
      if (color_sensor.begin()) {
      // Alles OK
        Serial.println("TCS34725 gefunden");
        return true;
      } else {
        // Kein Sensor gefunden. Programm an dieser Stelle einfrieren
        Serial.println("TCS34725 nicht gefunden");
        return false;
      }
      return true;
    }

    float measure_color_red(){
      color_sensor.getRawData(&r, &g, &b, &c);
      return gammatable[(int)r];
    }

    float measure_color_blue(){
      color_sensor.getRawData(&r, &g, &b, &c);
      return gammatable[(int)b];
    }

    float measure_color_green(){
      color_sensor.getRawData(&r, &g, &b, &c);
      return gammatable[(int)g];
    }

    float measure_color_temp(){
      color_sensor.getRawData(&r, &g, &b, &c);
      return color_sensor.calculateColorTemperature_dn40(r, g, b, c);
    }

    float measure_color_lux(){
      color_sensor.getRawData(&r, &g, &b, &c);
      return color_sensor.calculateLux(r, g, b);
    }
};

class color_TCS34725_operator : public sensor_operator{
  private:
    color_TCS34725* sensor_list;
  public:
    color_TCS34725_operator(int AMOUNT, color_TCS34725* sensor_list, data_logger &logger_address) : sensor_operator(AMOUNT, logger_address){
      this->AMOUNT = AMOUNT;
      this->sensor_list = sensor_list;
    }

    bool initialise(){
      for(int i = 0; i < AMOUNT; i++){
        if(!sensor_list[i].initialise()){
          return false;
        }
        return true;
      }
    }
    

    void take_Measurements() {
      for(int i = 0; i < AMOUNT; i++){
          sensor_information = sensor_list[i].get_information();

          logger.log_data(sensor_information + ":" + String(sensor_list[i].measure_color_red(), 3) + ":" + String(sensor_list[i].measure_color_green(), 3) + ":" + String(sensor_list[i].measure_color_blue(), 3) + ":" + String(sensor_list[i].measure_color_temp(), 3) + ":" + String(sensor_list[i].measure_color_lux(), 3));
      }
    }

};

class temp_hum_DHT22 : public sensor{
  private:
    DHT dht;
    int PIN;

  public:
    temp_hum_DHT22(String position, String type, String measurement, int PIN):dht(PIN, DHT22), sensor(position, type, measurement) {
      this->PIN = PIN;

      dht.begin();

      data = 0;

    }

    float measure_temp() {
      return dht.readTemperature();
    }

    float measure_hum() {
      return dht.readHumidity(); 
    }
};

class temp_hum_DHT22_operator : public sensor_operator{
  private:
    temp_hum_DHT22* sensor_list;

    double temp_data;
    double hum_data;
    String temp_data_string;
    String hum_data_string;

  public:
    temp_hum_DHT22_operator(int temp_hum_DHT22_AMOUNT, temp_hum_DHT22* temp_hum_DHT22_list, data_logger &logger_address) : sensor_operator(temp_hum_DHT22_AMOUNT, logger_address){  
      this->sensor_list = temp_hum_DHT22_list;
    }

    void take_Measurements() {
      for(int i = 0; i < AMOUNT; i++){
          temp_data = sensor_list[i].measure_temp();
          hum_data = sensor_list[i].measure_hum();
          sensor_information = sensor_list[i].get_information();

          temp_data_string = String(temp_data, 3);
          hum_data_string = String(hum_data, 3);

          logger.log_data(sensor_information + ":" + temp_data_string + ":" + hum_data_string);
        }
      }
};

class pres_BMP280{
  private:
    String position; //positon of the sensor on the probe
    String type; //Identification of the sensor
    String measurement; //the kind of measurement taken by the sensor e.g. temperatue, humidity...

    Adafruit_BMP280 bmp;

    double data;
    

  public:
    pres_BMP280(String position, String type, String measurement) {
      this->position = position;
      this->type = type;
      this->measurement = measurement;

    }

    bool initialise(){
      if(!bmp.begin(0x76)){
        return false;
      }
      return true;
    }

    float measure_pres(){
        return bmp.readPressure();
    }

    float measure_temp(){
        return bmp.readTemperature();
    }

    String get_information() {
      return position + ":" + type + ":" + measurement;
    }


};

class pres_BMP280_operator{
  private:
    pres_BMP280* pres_BMP280_list;
    data_logger logger;

    int pres_BMP280_AMOUNT;

    double pres_data;
    double temp_data;
    String pres_data_string;
    String temp_data_string;
    String sensor_information;

  public:
    pres_BMP280_operator(int pres_BMP280_AMOUNT, pres_BMP280* pres_BMP280_list, data_logger &logger_address):logger(logger_address) {  
      this->pres_BMP280_list = pres_BMP280_list;
      logger = logger_address;  

      this->pres_BMP280_AMOUNT = pres_BMP280_AMOUNT;

    
      
    }

    bool initialise(){
      for(int i  = 0; i < pres_BMP280_AMOUNT; i++){
        if(!pres_BMP280_list[i].initialise()){
          return false;
        }
      }
      return true;
    }

    void take_Measurements() {
      for(int i  = 0; i < pres_BMP280_AMOUNT; i++){
          pres_data = pres_BMP280_list[i].measure_pres();
          temp_data = pres_BMP280_list[i].measure_temp();
          sensor_information = pres_BMP280_list[i].get_information();
          pres_data_string = String(pres_data, 3);
          temp_data_string = String(temp_data, 3);

          logger.log_data(sensor_information + ":" + pres_data_string + ":" + temp_data_string);
        }
      }
};

class temp_DS18B20{
  private:
    String position; //positon of the sensor on the probe
    String type; //Identification of the sensor
    String measurement; //the kind of measurement taken by the sensor e.g. temperatue, humidity...

    double data;
    

  public:
    temp_DS18B20(String position, String type, String measurement) {
      this->position = position;
      this->type = type;
      this->measurement = measurement;

      data = 0;

    }

    double measure_temp(int id, DallasTemperature sensors) {
       data = sensors.getTempCByIndex(id);
       if(data == DEVICE_DISCONNECTED_C){
         data = No_Val;
       }
      return data;
    }

    String get_information() {
      return position + ":" + type + ":" + measurement;
    }


};

class temp_DS18B20_operator{
  private:
    int ONE_WIRE_BUS;
    int temp_DS18B20_AMOUNT;
    int DS18B20_Resolution;

    temp_DS18B20* temp_DS18B20_list;
    data_logger logger;

    double data;
    String data_string;
    String sensor_information;
    int sensor_id = 0;
 
    DeviceAddress DS18B20_Addresses;

  public:
      //DallasTemperature sensors;

    temp_DS18B20_operator(int ONE_WIRE_BUS, int DS18B20_Resolution, int temp_DS18B20_AMOUNT, temp_DS18B20* temp_DS18B20_list, data_logger &logger_address):logger(logger_address) {  
      this->ONE_WIRE_BUS = ONE_WIRE_BUS;
      this->DS18B20_Resolution = DS18B20_Resolution;
      this->temp_DS18B20_list = temp_DS18B20_list;
      //logger = logger_address;
      
      this->temp_DS18B20_AMOUNT = temp_DS18B20_AMOUNT;

      OneWire oneWire(ONE_WIRE_BUS); 
      DallasTemperature sensors(&oneWire);
      //this->sensors = sensors;

      //float Temperature[DS18B20_AMOUNT]; not needed anymore, i think
      //Serial.print("Anzahl aktivierter Sensoren: ");

      if (temp_DS18B20_AMOUNT > 0) {
        sensors.begin();
        Serial.print("Anzahl angeschlossener Sensoren: ");
        Serial.println(sensors.getDeviceCount(), DEC);
        Serial.println();
 
        for(byte i=0 ;i < sensors.getDeviceCount(); i++) {
          if(sensors.getAddress(DS18B20_Addresses, i)) {
              sensors.setResolution(DS18B20_Addresses, DS18B20_Resolution);
          }
        }
      }
    }

    void take_Measurements() {
      if (temp_DS18B20_AMOUNT > 0) {
        OneWire oneWire(ONE_WIRE_BUS); 
      DallasTemperature sensors(&oneWire);
        
        sensors.requestTemperatures();

        for(int i  = 0; i < temp_DS18B20_AMOUNT; i++){
          //sensor = temp_DS18B20_list[i];

          data = temp_DS18B20_list[i].measure_temp(i, sensors);
          sensor_information = temp_DS18B20_list[i].get_information();

          data_string = String(data, 3);

          logger.log_data(sensor_information + ":" + data_string);
        }
      }

    }
};

class UV_GUVA{
  private:
    String position; //positon of the sensor on the probe
    String type; //Identification of the sensor
    String measurement; //the kind of measurement taken by the sensor e.g. temperatue, humidity...

    int PIN;

    double data;
    

  public:
    UV_GUVA(String position, String type, String measurement, int PIN) {
      this->position = position;
      this->type = type;
      this->measurement = measurement;
      this->PIN = PIN;

      data = 0;

    }

    long measure_UV() {
      return analogRead(PIN);
    }

    String get_information() {
      return position + ":" + type + ":" + measurement;
    }


};

class UV_GUVA_operator{
  private:
    UV_GUVA* UV_GUVA_list;
    data_logger logger;

    int UV_GUVA_AMOUNT;

    double UV_data;
    String UV_data_string;
    String sensor_information;

  public:
    UV_GUVA_operator(int UV_GUVA_AMOUNT, UV_GUVA* UV_GUVA_list, data_logger &logger_address):logger(logger_address) {  
      this->UV_GUVA_list = UV_GUVA_list;
      logger = logger_address;  

      this->UV_GUVA_AMOUNT = UV_GUVA_AMOUNT;
    
      
    }

    void take_Measurements() {
      for(int i  = 0; i < UV_GUVA_AMOUNT; i++){
          UV_data = UV_GUVA_list[i].measure_UV();
          sensor_information = UV_GUVA_list[i].get_information();

          UV_data_string = String(UV_data, 3);

          logger.log_data(sensor_information + ":" + UV_data_string);
        }
      }
};

class sound{
  private:
    String position; //positon of the sensor on the probe
    String type; //Identification of the sensor
    String measurement; //the kind of measurement taken by the sensor e.g. temperatue, humidity...

    int PIN;

    double data;
    

  public:
    sound(String position, String type, String measurement, int PIN) {
      this->position = position;
      this->type = type;
      this->measurement = measurement;
      this->PIN = PIN;

      data = 0;

    }

    long measure_sound() {
      return analogRead(PIN);
    }

    String get_information() {
      return position + ":" + type + ":" + measurement;
    }


};

class sound_operator{
  private:
    sound* sound_list;
    data_logger logger;

    int sound_AMOUNT;

    double sound_data;
    String sound_data_string;
    String sensor_information;

  public:
    sound_operator(int sound_AMOUNT, sound* sound_list, data_logger &logger_address):logger(logger_address) {  
      this->sound_list = sound_list;
      logger = logger_address;  

      this->sound_AMOUNT = sound_AMOUNT;
    
      
    }

    void take_Measurements() {
      for(int i  = 0; i < sound_AMOUNT; i++){
          sound_data = sound_list[i].measure_sound();
          sensor_information = sound_list[i].get_information();

          sound_data_string = String(sound_data, 3);

          logger.log_data(sensor_information + ":" + sound_data_string);
        }
      }
};

class acc_MPU6050{
  private:
    String position; //positon of the sensor on the probe
    String type; //Identification of the sensor
    String measurement; //the kind of measurement taken by the sensor e.g. temperatue, humidity...

    const int MPU = 0x68;
    float AccX, AccY, AccZ;
    float GyroX, GyroY, GyroZ;




    String data;
    

  public:
    acc_MPU6050(String position, String type, String measurement) {
      this->position = position;
      this->type = type;
      this->measurement = measurement;

      data = "";

    }

    String measure_acc() {
      Wire.beginTransmission(MPU);
      Wire.write(0x3B); // Start with register 0x3B (ACCEL_XOUT_H)
      Wire.endTransmission(false);
      Wire.requestFrom(MPU, 6, true);

      AccX = (Wire.read() << 8 | Wire.read()) / 16384.0; // X-axis value
      AccY = (Wire.read() << 8 | Wire.read()) / 16384.0; // Y-axis value
      AccZ = (Wire.read() << 8 | Wire.read()) / 16384.0; // Z-axis value
      
      Wire.beginTransmission(MPU);
      Wire.write(0x43); // Gyro data first register address 0x43
      Wire.endTransmission(false);
      Wire.requestFrom(MPU, 6, true); // Read 4 registers total, each axis value is stored in 2 registers
      GyroX = (Wire.read() << 8 | Wire.read()) / 131.0; 
      GyroY = (Wire.read() << 8 | Wire.read()) / 131.0;
      GyroZ = (Wire.read() << 8 | Wire.read()) / 131.0;

      data = String(AccX, 3) + ":" + String(AccY, 3) +  ":" + String(AccZ, 3) +  ":" + String(GyroX, 3) +  ":" + String(GyroY, 3) +  ":" + String(GyroZ, 3);

      return data;
    }

    String get_information() {
      return position + ":" + type + ":" + measurement;
    }


};

class acc_MPU6050_operator{
  private:
    acc_MPU6050* acc_MPU6050_list;
    data_logger logger;

    int acc_MPU6050_AMOUNT;

    double acc_data;
    String acc_data_string;
    String sensor_information;

  public:
    acc_MPU6050_operator(int acc_MPU6050_AMOUNT, acc_MPU6050* acc_MPU6050_list, data_logger &logger_address):logger(logger_address) {  
      this->acc_MPU6050_list = acc_MPU6050_list;
      this->acc_MPU6050_AMOUNT = acc_MPU6050_AMOUNT;
      logger = logger_address;    
      
    }

    void initialise(){
      Wire.beginTransmission(0x68);       // Start communication with MPU6050 // MPU=0x68
      Wire.write(0x6B);                  // Talk to the register 6B
      Wire.write(0x00);                  // Make reset - place a 0 into the 6B register
      Wire.endTransmission(true);
    }

    void take_Measurements() {
      for(int i  = 0; i < acc_MPU6050_AMOUNT; i++){
          acc_data_string = acc_MPU6050_list[i].measure_acc();
          sensor_information = acc_MPU6050_list[i].get_information();

          logger.log_data(sensor_information + ":" + acc_data_string);
        }
      }
};


RTC timer;

data_logger logger(timer);

LED led(LED_PIN);

buzzer buzzer(23);

 



pres_BMP280 pres_1("1", "BMP280", "pres");
//pres_BMP280 pres_2("2", "BMP280", "pres");

pres_BMP280 pres_list[] = {pres_1};//, pres_2};

pres_BMP280_operator pres_operator(1, pres_list, logger);


    
UV_GUVA UV_1("1", "UV_GUVA", "UV", A7);
UV_GUVA UV_2("2", "UV_GUVA", "UV", A8);

UV_GUVA UV_list[] = {UV_1, UV_2};
    
UV_GUVA_operator UV_operator(2, UV_list, logger);




Hall_SS490 Hall_1("1", "Hall_SS490", "Hall", 38);
Hall_SS490 Hall_2("2", "Hall_SS490", "Hall", 39);


Hall_SS490 Hall_list[] = {Hall_1, Hall_2};
    
Hall_SS490_operator Hall_operator(2, Hall_list, logger);



/*sound sound_1("1", "Big_sound", "sound", 41);
sound sound_2("2", "Small_sound", "sound", 40);

sound sound_list[] = {sound_1, sound_2};
    
sound_operator sound_operator(2, sound_list, logger);*/



temp_hum_DHT22 temp_hum_1("1", "DHT22", "temp_hum", 3);
temp_hum_DHT22 temp_hum_2("2", "DHT22", "temp_hum", 2);

temp_hum_DHT22 temp_hum_list[] = {temp_hum_1, temp_hum_2};
    
temp_hum_DHT22_operator temp_hum_operator(2, temp_hum_list, logger);



temp_DS18B20 temp_1("1", "DS18b20", "Temp");
temp_DS18B20 temp_2("2", "DS18b20", "Temp");
temp_DS18B20 temp_3("3", "DS18b20", "Temp");
temp_DS18B20 temp_4("4", "DS18b20", "Temp");
temp_DS18B20 temp_5("5", "DS18b20", "Temp");
temp_DS18B20 temp_6("6", "DS18b20", "Temp");
temp_DS18B20 temp_7("7", "DS18b20", "Temp");

temp_DS18B20 temp_list[] = {temp_1, temp_2, temp_3, temp_4, temp_5, temp_6, temp_7};
    
temp_DS18B20_operator temp_operator(33, 12, 7, temp_list, logger);




acc_MPU6050 acc_1("1", "MPU6050", "Acc/Gyro");

acc_MPU6050 acc_list[] = {acc_1};
    
acc_MPU6050_operator acc_operator(1, acc_list, logger);


color_TCS34725 color_1("1", "TCS34725", "Color");
color_TCS34725 color_list[] = {color_1};
color_TCS34725_operator color_operator(1, color_list, logger);

bool error = false;




void setup() {
  // put your setup code here, to run once:
    Serial.begin(9600);

    Wire.begin();

    pinMode(13, OUTPUT);
      
    acc_operator.initialise();

    if(!color_operator.initialise()){
      Serial.println("Problem with TCS");
      error = true;
      blink_long_1();
    }else{
      blink_short_2();
    }
    delay(250);

    if(!pres_operator.initialise()){
      Serial.println("Problem with Pres");
      error = true;
      blink_long_1();
    }else{
      blink_short_2();
    }
    delay(250);

    if(!logger.initialise()){
      Serial.println("Problem with SD-Card");
      error = true;
      blink_long_1();
    }else{
      blink_short_2();
    }
    delay(250);

    if(error){
      Serial.println("Problem");
      digitalWrite(13, HIGH);

      //while(1);
    }

    delay(1000);

    Serial.println("Start");

    digitalWrite(13, HIGH);
    delay(500);
    digitalWrite(13, LOW);
    delay(500);
    digitalWrite(13, HIGH);
    delay(500);
    digitalWrite(13, LOW);
    delay(500);

}

void loop() {
  // put your main code here, to run repeatedly:
  led.blink();
  delay(500);
  Serial.println("Messen");
  temp_operator.take_Measurements();
  temp_hum_operator.take_Measurements();
  UV_operator.take_Measurements();
  //sound_operator.take_Measurements();
  Hall_operator.take_Measurements();
  pres_operator.take_Measurements();
  acc_operator.take_Measurements();
  color_operator.take_Measurements();
  led.blink();
  delay(500);

  /*if(hour() >= 5){
    buzzer.buzz();
  }*/

}