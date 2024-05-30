#include <Arduino.h>
#include <Vector.h>
#include <LiquidCrystal.h>
#include <Servo.h>
#include <IRremote.h>
#include <DHT.h>
#include <TimeLib.h>

#define ANGLE_ON 70
#define ANGLE_OFF 0
#define MAX_C 10
#define VELOCITY_SOUND 0.034
#define MEDIA 8
#define EMPTY 16

enum Buttons{
  Button_1 = 12,
  Button_2 = 24,
  Button_3 = 94,
  Button_4 = 8,
  Button_5 = 28,
  Button_6 = 90,
  Button_7 = 66,
  Button_8 = 82,
  Button_9 = 71
};

/*===========================
=            LCD            =
===========================*/

enum Case{
  UP = 0,DOWN = 1
};

struct LCD{
  const int rs;
  const int e;
  const int d4;
  const int d5;
  const int d6;
  const int d7;

  LiquidCrystal lcd;

  LCD(int _rs, int _e, int _d4, int _d5, int _d6, int _d7)
    : rs(_rs), e(_e), d4(_d4), d5(_d5), d6(_d6), d7(_d7), 
    lcd(_rs, _e, _d4, _d5, _d6, _d7) {}

  void setUp(){
    lcd.begin(16, 2);
  }

  void showMessage(int _case, String _message){
    lcd.setCursor(0, _case);
    lcd.print(_message);
  }

  void clearRow(int _row) {
    lcd.setCursor(0, _row);
    for (int i = 0; i < 16; ++i) {
      lcd.print(' ');  // Espacios en blanco para borrar la fila
    }
  }

  void cleanUp(){
    lcd.clear();
  }
};

/*----------  Init LCD  ----------*/
LCD lcdData{2, 3, 4, 5, 6, 7};
/*=====  End of LCD  ======*/

/*=============================
=            Servo            =
=============================*/

struct MOTOR{
  const int pin;
  int vibrador;
  Servo myServo;

  MOTOR(int _pin,int _v)
    : pin(_pin),vibrador(_v) {}

  void setUp(){
    myServo.attach(pin);
    pinMode(vibrador,OUTPUT);
  } 

  void active() {
    Serial.print("Motor Activo");
    moveServo(ANGLE_ON);
    delay(500);//dependera de la cantidad
    moveServo(ANGLE_OFF);
    delay(400);
    digitalWrite(vibrador,HIGH);
    delay(1100);
    digitalWrite(vibrador,LOW);
  }

  void moveServo(int angle) {
    myServo.write(angle);
    delay(500); // Añade un pequeño retraso para 
    //permitir que el servo alcance la posición deseada
  }

};
/*----------  Init Motor  ----------*/
MOTOR motor{A3,8};
/*=====  End of Servo  ======*/

/*============================
=            CONG            =
============================*/

struct SCHEDULES{
  int hou;
  bool flag;
  String meridiano;

  SCHEDULES(){}
  SCHEDULES(int _h,bool _f,String _m):hou(_h),flag(_f),
          meridiano(_m){}

  void setFlag(bool _b){
    flag = _b;
  }

  String getData() const {
    return (String(hou)+":00 "+meridiano);
  }

};

typedef Vector<SCHEDULES> LIST;

class CONF{
private:
  //int h,m,s,d,mo,y;
  SCHEDULES storage_array[MAX_C];
  LIST stack;
  LCD* lcd;
  MOTOR* motor;

public:
  CONF(int _h,int _m,int _s,int _d,int _mo,int _y){ 
    stack.setStorage(storage_array);
    setTime(_h,_m,_s,_d,_mo,_y); 
  }
  ~CONF(){}

  void setModules(LCD* _lcd,MOTOR* _motor){
    lcd = _lcd;
    motor = _motor;
  } 

  void addHour(SCHEDULES _target){
    Serial.println(String(_target.hou)+" : "+String(_target.flag));
    stack.push_back(_target);
    Serial.println(String(stack.size()));
  }

  void showSchedules(){
    for (const auto& target : stack) {
      // Borra la pantalla
      (*lcd).clearRow(DOWN);
      Serial.println(target.getData());
      (*lcd).showMessage(DOWN,target.getData());
      delay(2000);
    }
  }

  void hungryHourSignal(time_t _time){
    for (const auto& target: stack){
      int aux_t = hour(_time);
      if((target.hou == aux_t) && (!target.flag) ){
        (*motor).active();
        target.setFlag(true);
        break;
      }
    }
  }

};

/*----------  Init Configure  ----------*/
CONF conf(8, 59, 50, 30, 11, 2023);

/*=====  End of CONG  ======*/

/*=========================================
=            Microchip 74HC595            =
=========================================*/
struct SHIFT_R{
  const int datapin;
  const int lachtpin;
  const int clockpin;
  byte myByte = 0b00000000;

  /*----------  Intervalo de tiempo  ----------*/
  unsigned int previousMillis = 0;
  long interval = 2000;
  bool isFunctionRunning = false;
  int cont = 0;
  /*----------  -------------------  ----------*/

  SHIFT_R(int _datapin, int _lachtpin, int _clockpin)
    : datapin(_datapin), lachtpin(_lachtpin), 
      clockpin(_clockpin) {}

  void setUp(){
    pinMode(lachtpin, OUTPUT);
    pinMode(clockpin, OUTPUT);
    pinMode(datapin, OUTPUT);
  }

  /*----------  Notes Active and Block  ----------*/
  void update(int _current){//BITS
    
    myByte = 0b00000100;

    if(_current < MEDIA){
      previousMillis = 0;
      interval = 2000;
      isFunctionRunning = false;
      cont = 0;//reiniciar la alarma
    }
    
    if(_current > MEDIA){
      myByte = 0b00000010;
    }
    if(_current > EMPTY){

      myByte = 0b00000001;

      if(cont <= 5){  
        unsigned long currentMillis = millis();

        if(currentMillis - previousMillis > interval && !isFunctionRunning){
          previousMillis = currentMillis;
          isFunctionRunning = true;
          
          if(isFunctionRunning) { myByte = 0b10000001; }
        }

        if(currentMillis - previousMillis > interval * 2 && isFunctionRunning){
          isFunctionRunning = false;
          cont++;
        }//pausas
      }//intervalo
    }
  }

  void active(){
    digitalWrite(lachtpin, LOW);
    shiftOut(datapin, clockpin, LSBFIRST, myByte);
    digitalWrite(lachtpin, HIGH);
  }
};
/*----------  Init 74HC595  ----------*/
SHIFT_R shift{A0, A1, A2};
/*=====  End of Microchip 74HC595  ======*/

/*==========================================
=            Sensor Ultrasonico            =
==========================================*/

struct S_ULTRA_SONICO{
    const int pinEcho;
    const int pinTrig;
    float distance;

    S_ULTRA_SONICO(int _pinEcho, int _pinTrig)
      : pinEcho(_pinEcho), pinTrig(_pinTrig) {}

    void setUp(){
      pinMode(pinEcho, INPUT);
      pinMode(pinTrig, OUTPUT);
    }

    void active(){
      digitalWrite(pinTrig, HIGH);
      delayMicroseconds(10);
      digitalWrite(pinTrig, LOW);

      int pingTravelTime = pulseIn(pinEcho, HIGH);
      distance = pingTravelTime * VELOCITY_SOUND / 2;
    }

    int getDistance() const {
      return int(distance);
    }

    String getStatus() const {

      if(distance >= MEDIA && distance < EMPTY){
        return "  >MEDIO LLENO  ";
      }

      if(distance >= EMPTY ){
        return "     VACIO!     ";
      }

      return "     >LLENO     ";

    }
};
/*----------  Init Sensor  ----------*/
S_ULTRA_SONICO sensor_u{9, 10};
/*=====  End of Sensor Ultrasonico  ======*/

/*==================================================
=            DHT/ sensor de Temperatura            =
==================================================*/
struct DHT_S{
  const int pinT;
  DHT dht;

  DHT_S(int _pinT)
    : pinT(_pinT), dht(_pinT, DHT11) {}

  void setUp(){
    dht.begin();
  }

  String getCelsius() const {
    float Celsius = dht.readTemperature();
    String result;

    if(isnan(Celsius)){
      result = "Error: C";
    }else{
      if(int(Celsius,1) < 26){
        result = "     FRESCO     ";
      }else if(int(Celsius,1) > 26){
        result = "   >CALIENTE!   ";
      }
    }

    return result;
  }

  String getHumidity() const {
    float Humidity = dht.readHumidity();
    String result;

    if(isnan(Humidity)){
      result = "Error: H";
    }else{
      if(int(Humidity,1) <= 50){
        result = "    > SECO <    ";
      }else if(int(Humidity,1) > 50 && int(Humidity,1) < 70){
        result = "   > HUMEDO <   ";  
      }else if(int(Humidity,1) > 70){
        result = "    > MOHO <    ";
      }
    }

    return result;
  }
};
/*----------  Init DHT  ----------*/
DHT_S weather{11};
/*=====  End of DHT/ sensor de Temperatura  ======*/

/*================================
=            IRremote            =
================================*/

struct REMOTE{
  const int pin;

  REMOTE(int _pin)
    : pin(_pin) {}

  void setUp(){
    IrReceiver.begin(pin);
  }
};
/*----------  Init Remote  ----------*/
REMOTE remote{12};
/*=====  End of IRremote  ======*/

/*=============================
=            SetUp            =
=============================*/

void setup(){
  Serial.begin(115200);
  lcdData.setUp();
  motor.setUp();

  conf.setModules(&lcdData,&motor);

  shift.setUp();
  sensor_u.setUp();
  weather.setUp();
  remote.setUp();
}

/*=====  End of SetUp  ======*/

/*============================
=            Main            =
============================*/

int command = 0,x=0;
unsigned long previousMillis = 0;
const long interval = 1000;  // Intervalo de 1 segundo
bool ACTIVE;

void loop() {

  if(x==0){
     /*============================
  =            Test            =
  ============================*/
  SCHEDULES test1{9,false,"pm"};// 2:00 PM
  SCHEDULES test2{18,false,"pm"};// 6:00 PM
  conf.addHour(test1);
  conf.addHour(test2);
  /*=====  End of Test  ======*/
  }
  x=1;

  unsigned long currentMillis = millis();

  // Realizar tareas según el intervalo definido
  if (currentMillis - previousMillis >= interval) {
    // Actualizar el temporizador
    previousMillis = currentMillis;

    time_t t = now();

    sensor_u.active();
    conf.hungryHourSignal(t);
    lcdData.cleanUp();
    shift.active();
    shift.update(sensor_u.getDistance());

    switch (command) {
      case Button_1:
        lcdData.showMessage(UP, " Estado Alimen. ");
        lcdData.showMessage(DOWN, sensor_u.getStatus());
        break;
      case Button_2:
        lcdData.showMessage(UP, "    Humedad!    ");
        lcdData.showMessage(DOWN, weather.getHumidity());
        break;
      case Button_3:
        lcdData.showMessage(UP, "  Temperatura!  ");
        lcdData.showMessage(DOWN, weather.getCelsius());
        break;
      case Button_4:
        lcdData.showMessage(UP, "    Horarios    ");
        conf.showSchedules();
        break;
      case Button_5:
        lcdData.showMessage(UP, "  Hora Actual!  ");
        if(hour(t) > 0 && hour(t) < 11){ // 0:11 AM
          lcdData.showMessage(DOWN, " > "+String(hour(t)) + ":" + String(minute(t))+" AM");
        }else{ // 12:23 pm
          lcdData.showMessage(DOWN, " > "+String(hour(t)) + ":" + String(minute(t))+" PM");
        }
        break;
      case Button_6:
        lcdData.showMessage(UP, ">Hora de comer!<");
        lcdData.showMessage(DOWN, "     GOJO      ");
        motor.active();
        command = 0;
        break;
      default:
        lcdData.showMessage(UP, "Comida para Gato");
        lcdData.showMessage(DOWN, "  Gato Barato!  ");
        break;
    }
  }

  // Manejar la recepción de señales IR
  if (IrReceiver.decode() != 0) {
    command = IrReceiver.decodedIRData.command;
    IrReceiver.resume();
  }
}


/*=====  End of Main  ======*/
