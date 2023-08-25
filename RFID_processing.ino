#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <IRremote.h>
#define irSignal A3

//SETUP CLASS QUEUE
class Queue{
  public:
  int data;
  int size;
  Queue *next;
  Queue(){
    size = 0;
    next = nullptr;
  }
  void push(int data);
  int front();
  void pop();
  bool empty();
  void displayQueue();
};

void Queue::displayQueue(){
  Queue *temp = next;
  for (int i = 0; i < size; i++){
    Serial.print((String)i + ": " + (String)temp->data + " ");
    temp = temp->next;
  }
  Serial.println();
}

void Queue::push(int data){
  Queue *newIdx = new Queue;
  newIdx->next = next;
  newIdx->data = data;
  next = newIdx;
  size++;
}

int Queue::front(){
  return next->data;
}

void Queue::pop(){
  if (!size)
    return;
  Queue *delIdx = next;
  next = delIdx->next;
  delete delIdx;
  size--;
}

bool Queue::empty(){
  if (!size)
    return true;
  return false;
}
//END OF CLASS QUEUE

const byte SS_PIN2 = 8;
const byte RST_PIN = 9;     
const byte SS_PIN1 = 10;
MFRC522 mfrc522_1(SS_PIN1,RST_PIN);
MFRC522 mfrc522_2(SS_PIN2,RST_PIN); 

Servo barrierIn;

LiquidCrystal_I2C lcd_In(0x27,16,2);

String nowParking[8];
const int buzzerIn = A0, buzzerOut = A1, signalBarrierOut = A2, signalError = 7, sensorIn = 2, nextLevitate[2] = {5,6}, levitateDone = 4, capacity = 8;
int volume = 900;
Queue waitingList;

void setup() {
  pinMode(sensorIn,INPUT);
  pinMode(levitateDone,INPUT);
  pinMode(signalBarrierOut,OUTPUT);
  pinMode(signalError,OUTPUT);
  Serial.begin(9600);

  IrReceiver.begin(irSignal, ENABLE_LED_FEEDBACK);

  //Set up for RFID
  SPI.begin();
  mfrc522_1.PCD_Init();
  mfrc522_2.PCD_Init();

  //Set up for servo
  barrierIn.attach(3);
  barrierIn.write(175);  

  //Set up for lcd
  lcd_In.init();
  lcd_In.backlight();

  Serial.println("Welcome");
  for (int i = 0; i < capacity; i++){
    nowParking[i] = "Empty";
    Serial.println("Slot " + (String)i + ": " + nowParking[i]);
  }
  Serial.println("------------------------------------");

  digitalWrite(signalBarrierOut,0);
  digitalWrite(signalError,0);
  display("Welcome",0);
}

void loop() {
  String cardData_In = "";
  String cardData_Out = "";
  if (IrReceiver.decode()){
    String data = String(IrReceiver.decodedIRData.decodedRawData,HEX);
    Serial.println(data);
    IrReceiver.resume();
    if (data == "7609ff00"){
      volume += 100;
      soundCorrect(buzzerIn);
    }
    if (data == "6a15ff00"){
      volume -= 100;
      soundCorrect(buzzerIn);
    }
    if (data == "bf40ff00"){
      display("ADMIN MODE",1);
      openBarrier();
      display("Welcome",0);
    }
  }
  if (!waitingList.empty() && digitalRead(levitateDone)){
    bool fst, snd, thd;
    switch (waitingList.front()){
      case 1:
        fst = 0;
        snd = 0;
      break;
      case 2:
        fst = 0;
        snd = 1;
      break;
      case 3:
        fst = 1;
        snd = 1;
      break;
      case 4:
      default:
        fst = 0;
        snd = 0;
      break;
    }
    Serial.println("fst: " + (String)fst + " snd: " + (String)snd);
    digitalWrite(nextLevitate[0],fst);
    digitalWrite(nextLevitate[1],snd);
    waitingList.pop();
  }
  else{
    digitalWrite(nextLevitate[0],0);
    digitalWrite(nextLevitate[1],0);
  }
  if (Serial.available() > 0){//Chi thuc hien khi co tin hieu duoc gui 
    byte signal = Serial.read();
    Serial.println("Mode: " + (String)signal);
    if (signal == 1){
      display("ADMIN MODE",1);
      openBarrier();
      display("Welcome",0);
    }
  }
  if (mfrc522_1.PICC_IsNewCardPresent() && mfrc522_1.PICC_ReadCardSerial()){
    for (uint8_t i = 0; i < mfrc522_1.uid.size; i++){
      cardData_In.concat(String(mfrc522_1.uid.uidByte[i] < 0x10 ? "0" : " "));
      cardData_In.concat(String(mfrc522_1.uid.uidByte[i], HEX));
    }
    mfrc522_1.PICC_HaltA();
    Serial.println("UID In: " + cardData_In);
    int floor = 0;
    if (cardData_In != ""){
      if(insertUID(cardData_In,&floor)){
        waitingList.push(floor);
        soundCorrect(buzzerIn);
        display("OPENING BARRIER",1);
        openBarrier();
      }
      else{
        soundError(buzzerIn);
        if (isFull()){
          display("IS FULL",1);
          delay(1000);
        }
        else if (isUsed(cardData_In)){
          display("USED CARD",1);
          delay(1000);
        }
        else{
          display("UNAVAILABLE",1);
          delay(1000);
        }
      }
    } 
    for (int i = 0; i < capacity; i++)
      Serial.println("Slot " + (String)i + ": " + nowParking[i]);
    Serial.println("------------------------------------");
    display("Welcome",0);
  }
  if (mfrc522_2.PICC_IsNewCardPresent() && mfrc522_2.PICC_ReadCardSerial()){
    for (uint8_t i = 0; i < mfrc522_2.uid.size; i++){
      cardData_Out.concat(String(mfrc522_2.uid.uidByte[i] < 0x10 ? "0" : " "));
      cardData_Out.concat(String(mfrc522_2.uid.uidByte[i], HEX));
    }
    mfrc522_2.PICC_HaltA();
    Serial.println("UID Out: " + cardData_Out);
    if (cardData_Out != ""){
      if(removeUID(cardData_Out)){
        soundCorrect(buzzerOut);
        digitalWrite(signalBarrierOut,1);
        delay(20);
        digitalWrite(signalBarrierOut,0);
      }
      else
      {
        soundError(buzzerOut);
        digitalWrite(signalError,1);
        delay(20);
        digitalWrite(signalError,0);
      }
    }
    for (int i = 0; i < capacity; i++)
      Serial.println("Slot " + (String)i + ": " + nowParking[i]);
    Serial.println("------------------------------------");
    display("Welcome",0);
  }
}

bool insertUID(String UID, int *slot){
  for (int i = 0; i < capacity; i++)
    if (nowParking[i] == UID)
      return false;
  for (int i = 0; i < capacity; i++){
    if (nowParking[i] == "Empty"){
      *slot = i/2 + 1;
      nowParking[i] = UID;
      return true;
    }
  }
  return false;
}

bool removeUID(String UID){
  for (int i = 0; i < capacity; i++){
    if(nowParking[i] == UID)
      {
        nowParking[i] = "Empty";
        return true;
      }
  }
  return false;
}

void soundError(int buzzerName){
  tone(buzzerName,volume);
  delay(100);
  noTone(buzzerName);
  delay(100);
  tone(buzzerName,volume);
  delay(100);
  noTone(buzzerName);
}

void soundCorrect(int buzzerName){
  tone(buzzerName,volume);
  delay(200);
  noTone(buzzerName);
}

void openBarrier(){
  for (int i = 175; i >= 75; i--){
    barrierIn.write(i);
    delay(20);
  }
  while(digitalRead(sensorIn))
    delay(20);
  while (!digitalRead(sensorIn))
    delay(20);
  delay(2000);
  for (int i = 75; i <= 175; i++){
    barrierIn.write(i);
    delay(20);
  }
}

void display(String text, bool mode){
  if (!mode){
    int position = 0;
    lcd_In.clear();
    lcd_In.setCursor(0,0);
    lcd_In.print("WELCOME");
    lcd_In.setCursor(8,0);
    lcd_In.print("AVAI:");
    lcd_In.setCursor(13,0);
    lcd_In.print(countAvailable());
    lcd_In.setCursor(14,0);
    lcd_In.print("/" + (String)capacity);
    int B[4];
    B[0] = ((nowParking[0] == "Empty")?1:0) + ((nowParking[1] == "Empty")?1:0);
    B[1] = ((nowParking[2] == "Empty")?1:0) + ((nowParking[3] == "Empty")?1:0);
    B[2] = ((nowParking[4] == "Empty")?1:0) + ((nowParking[5] == "Empty")?1:0);
    B[3] = ((nowParking[6] == "Empty")?1:0) + ((nowParking[7] == "Empty")?1:0);
    for (int i = 0; i < capacity/2; i++){
        lcd_In.setCursor(position,1);
        lcd_In.print("B" + (String)(i+1) + ":");
        position += 3;
        lcd_In.setCursor(position,1);
        lcd_In.print(B[i]);
        position++;
    }
  }
  else{
    lcd_In.clear();
    if (text.length() < 8)
      lcd_In.setCursor(4,0);
    else if (text.length() < 11)
      lcd_In.setCursor(3,0);
    else if (text.length() < 13)
      lcd_In.setCursor(2,0);
    else
      lcd_In.setCursor(0,0);
    lcd_In.print(text);
  }
}

int countAvailable(){
  int Count = 0;
  for (int i = 0; i < capacity; i++){
    if(nowParking[i] != "Empty")
      Count++;
  }
  return capacity - Count;
}

bool isFull(){
  for (int i = 0; i < capacity; i++){
    if (nowParking[i] == "Empty")
      return false;
  }
  return true;
}

bool isUsed(String UID){
  for (int i = 0; i < capacity; i++){
    if (nowParking[i] == UID)
      return true;
  }
  return false;
}
