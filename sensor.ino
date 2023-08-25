#include <Servo.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <IRremote.h>
Servo barrierOut;
#define irSignal A2
#define lightSensor A3
const int sensorOut = 2, signalBarrier = 4, signalError = 5, bulbs = 6;
bool manualBulbsMode = false, signalOut = false, signalIn = false;

LiquidCrystal_I2C lcd_Out(0x27,16,2);

void setup() {
  
  pinMode(signalBarrier,INPUT);
  pinMode(signalError,INPUT);
  pinMode(lightSensor,INPUT);
  pinMode(bulbs,OUTPUT);

  barrierOut.attach(3);
  barrierOut.write(175);

  //Set up for lcd
  lcd_Out.init();
  lcd_Out.backlight();

  IrReceiver.begin(irSignal, ENABLE_LED_FEEDBACK);

  Serial.begin(9600);

  display("default");
}

void loop() {
  Serial.println("Signal Error: " + (String)digitalRead(signalError));
  if (IrReceiver.decode()){
    Serial.println("");
    String data = String(IrReceiver.decodedIRData.decodedRawData,HEX);
    Serial.println(data);
    IrReceiver.resume();
    if (data == "ba45ff00"){
      manualBulbsMode = !manualBulbsMode;
    }
    else if (data == "bc43ff00")
      signalOut = true;
    else if (data == "bf40ff00")
      signalIn = true;
  }
  if (manualBulbsMode){
    digitalWrite(bulbs,1);
  }
  else{
    if (analogRead(lightSensor) >= 500)
      digitalWrite(bulbs,1);
    else
      digitalWrite(bulbs,0);
  }
  if (signalOut){
    display("admin");
    openBarrier();
    signalOut = false;
    display("default");
  }
  if (signalIn){
    display("admin");
    byte mode = 1;//mode 1 là mở cửa vào
    Serial.write(mode);
    signalIn = false;
    display("default");
  }
  else{
    byte mode = 0;
    Serial.write(mode);
  }
  if (digitalRead(signalBarrier)){
    display("true");
    openBarrier();
    display("default");
  }
  else if (digitalRead(signalError)){
    display("wrong");
    delay(600);
    display("default");
  }
}

void openBarrier(){
  for (int i = 175; i >= 75; i--){
    barrierOut.write(i);
    delay(20);
  }
  while(digitalRead(sensorOut))
    delay(20);
  while (!digitalRead(sensorOut))
    delay(20);
  delay(2000);
  for (int i = 75; i <= 175; i++){
    barrierOut.write(i);
    delay(20);
  }
}

void display(String type){
  if (type != "default" && type != "wrong" && type != "true" && type != "admin")
    return;
  lcd_Out.clear();
  if (type == "default"){
    lcd_Out.setCursor(5,0);
    lcd_Out.print("PLEASE");
    lcd_Out.setCursor(0,1);
    lcd_Out.print("INSERT YOUR CARD");
  }
  else if (type == "wrong"){
    lcd_Out.setCursor(3,0);
    lcd_Out.print("WRONG CARD");
    lcd_Out.setCursor(0,1);
    lcd_Out.print("PLEASE TRY AGAIN");
  }
  else if (type == "admin"){
    lcd_Out.setCursor(3,0);
    lcd_Out.print("ADMIN MODE");
    lcd_Out.setCursor(0,1);
    lcd_Out.print("OPENING BARRIER");
  }
  else{
    lcd_Out.setCursor(0,0);
    lcd_Out.print("HAVE A GOOD DAY");
    lcd_Out.setCursor(0,1);
    lcd_Out.print("OPENING BARRIER");
  }
}
