//important libraries
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <SPI.h>
#include <MFRC522.h>

//pin allocation for MFRC522 interface
//reset pin for MFRC522
#define RST_PIN         22 
//slave select pin for MFRC522         
#define SS_PIN          5

//initialise MFRC522 module at given pins  
MFRC522 mfrc522(SS_PIN, RST_PIN);  

//initialise an 16by2 I2C LCD at 0x3F
LiquidCrystal_I2C lcd(0x3F,16,2);

//MIFARE KEY to make card readable/writable
MFRC522::MIFARE_Key key;

//Name of the item to be added in inventory
byte nameByte[18];
byte countByte[18];

//byte variable for read/write operation 
//stores the name of the component 
byte readbackblock1[18];
//stores the number of units
byte readbackblock2[18];

int r1 = 6;
int r2 = 7;
int r3 = 8;
int r4 = 9;
int c1 = 10;
int c2 = 11;
int c3 = 14;

void setup(){
  
  //start the serial port  
  Serial.begin(115200);

  //LCD interfacing    
  //set up SDA and SCL pins for I2C LCD     
  Wire.setSDA(12);
  Wire.setSCL(13);
  //start I2C port
  Wire.begin();
  //intialise the LCD  
  lcd.init();
  //clear the display
  lcd.clear(); 
  //turn on backlight        
  lcd.backlight();
  
  //Greeting text
  lcd.setCursor(0,0);
  lcd.print("Greetings!");  
  
//interfacing the MFRC522 module
  //set up SPI pins for MFRC522 interfacing  
  SPI.setRX(4);
  SPI.setTX(3);
  SPI.setSCK(2);
  SPI.setCS(5);
  //start SPI communication  
  SPI.begin();
  pinMode(r1, OUTPUT);
  pinMode(r2, OUTPUT);
  pinMode(r3, OUTPUT);
  pinMode(r4, OUTPUT);
  pinMode(c1, INPUT_PULLUP);
  pinMode(c2, INPUT_PULLUP);
  pinMode(c3, INPUT_PULLUP);
  //initialise MFRC522 module
  mfrc522.PCD_Init();
  
  //allow some time for initialisation      
  delay(2000);
  
  lcd.clear();
                                              
  mfrc522.PCD_DumpVersionToSerial();         

  //set key = FF FF FF FF FF FF for read/write operation
  for (byte i = 0; i < 6; i++){
    key.keyByte[i] = 0xFF;  // Prepare the security key for the read and write operations.
  }
  
}

void loop() {

  lcd.setCursor(0,0);
  lcd.print("1:AddI 3:AddC  M");
  lcd.setCursor(0,1);  
  lcd.print("2:SubI 4:Read >");
  
  String mode = getString(15,1,"",0);
    
  if(mode == "1"){ 
         
  increment(); 
  }
  else if(mode == "2"){ 
      
  decrement(); 
  }
  else if(mode == "3"){
  
  addCard();
  } 
  else if(mode == "4"){
  
  readCard();    
  }

  else{
    lcd.setCursor(15,1);
    lcd.print(" ");    
  }
  }

//function to write data on card
int writeBlock(int blockNumber, byte arrayAddress[])
{
  //check if the block number corresponds to data block or triler block, rtuen with error if it's trailer block.
  int largestModulo4Number = blockNumber / 4 * 4;
  int trailerBlock = largestModulo4Number + 3; //determine trailer block for the sector
  if (blockNumber > 2 && (blockNumber + 1) % 4 == 0) {
    Serial.print(blockNumber);
    Serial.println(" is a trailer block: Error");
    return 2;
  }
  //authentication
  byte status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Authentication failed: ");
    Serial.println(mfrc522.GetStatusCodeName((MFRC522::StatusCode)status));
    return 3;//return "3" as error message
  }
  //writing data to the block
  status = mfrc522.MIFARE_Write(blockNumber, arrayAddress, 16);
  //status = mfrc522.MIFARE_Write(9, value1Block, 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Data write failed: ");
    Serial.println(mfrc522.GetStatusCodeName((MFRC522::StatusCode)status));
    return 4;//return "4" as error message
  }
  Serial.print("Data written to block ");
  Serial.println(blockNumber);
  return 0;
}

//function to read data on card
int readBlock(int blockNumber, byte arrayAddress[])
{
  int largestModulo4Number = blockNumber / 4 * 4;
  int trailerBlock = largestModulo4Number + 3; 
  
  byte status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Authentication failed : ");
    Serial.println(mfrc522.GetStatusCodeName((MFRC522::StatusCode)status));
    return 3;
  }
  
  byte buffersize = 18;
  status = mfrc522.MIFARE_Read(blockNumber, arrayAddress, &buffersize);//&buffersize is a pointer to the buffersize variable; MIFARE_Read requires a pointer instead of just a number
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Data read failed: ");
    Serial.println(mfrc522.GetStatusCodeName((MFRC522::StatusCode)status));
    return 4;
  }
  Serial.print("Data read from block ");
  Serial.print(blockNumber);
  Serial.println(" successfully");
  return 0;  
}

//fucntion to increment inventory count
void increment(){
  mfrc522.PCD_Init();  
  
  lcd.clear();
  lcd.print("Scan Your Card");
  delay(5000);  
  
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
              return;
  }

  if ( ! mfrc522.PICC_ReadCardSerial()) {
              return;
  }
  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Reading...");
  lcd.setCursor(0,1);
  lcd.print("Please Wait...");
  
  readBlock(2, readbackblock2);
  delay(3000);
  
  lcd.clear();
  lcd.setCursor(2,0);
  lcd.print("Adding item to");
  lcd.setCursor(1,1);
  lcd.print("the inventory");  
  
  int place = 0;
  for(int i = 0; i < 17; i++){
    if((char)readbackblock2[i] != 10){
    place = place + 1;
    }
    else{
      break;
    }   
  }
  
  int number = 0;
  for(int i = 0; i<place; i++){
    int place_value = readbackblock2[i]-48;
     number = number + place_value*pow(10, place-i-1);     
  }
  number = number + 1;
  String count = String(number);
  count += "\n";
  count.getBytes(countByte,18);
  writeBlock(2, countByte);
  delay(2000);
  
  lcd.clear();
  lcd.print("Item added");
  lcd.setCursor(2,1);
  lcd.print("Successfully");
  
  readBlock(1, readbackblock1);
  delay(3000);  
  readBlock(2, readbackblock2);
  delay(3000);
  
  lcd.clear();
  lcd.setCursor(0,0);
  for(int i = 0; i < 17; i++){
    lcd.print((char)readbackblock1[i]);
    lcd.leftToRight();    
  }
  lcd.setCursor(0,1);
  lcd.print("Count :");
  lcd.setCursor(8,1);

  for(int i = 0; i < 17; i++){
    lcd.print((char)readbackblock2[i]);    
    lcd.leftToRight();    
  }  
  delay(5000);
  
  lcd.clear();            
}

//Function to decrement inventory count
void decrement(){
  mfrc522.PCD_Init();  
  
  lcd.clear();
  lcd.print("Scan Your Card");
  delay(5000);
  
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
              return;
  }

  if ( ! mfrc522.PICC_ReadCardSerial()) {
              return;
  }
  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Reading...");
  lcd.setCursor(0,1);
  lcd.print("Please Wait...");
  
  readBlock(2, readbackblock2);
  delay(3000);
  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Removing item");
  lcd.setCursor(1,1);
  lcd.print("from inventory");
  
  int place = 0;
  for(int i = 0; i < 17; i++){
    if((char)readbackblock2[i] != 10){
    place = place + 1;
    }
    else{
      break;
    }   
  }
  
  int number = 0;
  for(int i = 0; i<place; i++){
    int place_value = readbackblock2[i]-48;
     number = number + place_value*pow(10, place-i-1);     
  }
  number = number - 1;
  String count = String(number);
  count += "\n";
  count.getBytes(countByte,18);
  writeBlock(2, countByte);
  delay(2000);
  
  lcd.clear();
  lcd.setCursor(2,0);
  lcd.print("Item removed");
  lcd.setCursor(2,1);
  lcd.print("Successfully");

  readBlock(1, readbackblock1);
  delay(3000);  
  readBlock(2, readbackblock2);
  delay(3000);

  lcd.clear();
  lcd.setCursor(0,0);
  for(int i = 0; i < 17; i++){
    lcd.print((char)readbackblock1[i]);
    lcd.leftToRight();    
  }
  lcd.setCursor(0,1);
  lcd.print("Count :");
  lcd.setCursor(8,1);

  for(int i = 0; i < 17; i++){
    lcd.print((char)readbackblock2[i]);    
    lcd.leftToRight();    
  }
  delay(5000);
  
  lcd.clear();          
}

//function to read card
void readCard(){
  mfrc522.PCD_Init();  
  
  lcd.clear();
  lcd.print("Scan Your Card");
  delay(5000);
  
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
              return;
  }

  if ( ! mfrc522.PICC_ReadCardSerial()) {
              return;
  }
  
  //User Interface msg: Card Detected...
  //User Interface msg: Writing...
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Card detected...");
  lcd.setCursor(0,1);
  lcd.print("Reading...");
 
  readBlock(1, readbackblock1);
  delay(3000);  
  readBlock(2, readbackblock2);
  delay(3000);
  lcd.clear();
  
  lcd.setCursor(0,0);
  for(int i = 0; i < 17; i++){
    lcd.print((char)readbackblock1[i]);
    lcd.leftToRight();    
  }
  lcd.setCursor(0,1);
  lcd.print("Count :");
  lcd.setCursor(8,1);

  for(int i = 0; i < 17; i++){
    lcd.print((char)readbackblock2[i]);    
    lcd.leftToRight();    
  }  
  delay(7000);
  
  lcd.clear();
}

//function to add card
void addCard(){
  mfrc522.PCD_Init();  
  
  lcd.clear();
  lcd.print("Scan Your Card");
  delay(5000);
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
              return;
  }

  if ( ! mfrc522.PICC_ReadCardSerial()) {
              return;
  }
  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Enter Item Name");
  lcd.setCursor(0, 1);
  lcd.print(">");  
  lcd.setCursor(1,1);
  
  String name = getString(1,1,"",15);
  String name_text = name;
  
  lcd.setCursor(0,0);
  lcd.print("Name :"); 
  for (int i = name_text.length()-1; i < 15; i++){
    name_text[i]  = ' '; 
  }   
  lcd.print(name_text);
  
  name.getBytes(nameByte,18);
  writeBlock(1, nameByte);
  delay(2000);
  
  lcd.clear(); 
  
  //Prompt the user for the count of item
  lcd.setCursor(0,0);
  lcd.print("Enter Item Count");
  lcd.setCursor(0, 1);
  lcd.print(">");
  lcd.setCursor(1,1);
  
  String count = getString(1,1,"",15);
  String count_text = count;

  lcd.setCursor(0,0);
  lcd.print("Count :");
  for (int i = count_text.length()-1; i < 15; i++){
    count_text[i]  = ' '; 
  } 
  lcd.print(count_text);

  count.getBytes(countByte,18);
  writeBlock(2, countByte);
  delay(2000);
  
  lcd.clear();
   
  readBlock(1, readbackblock1);
  delay(3000);  
  readBlock(2, readbackblock2);
  delay(3000);
  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Check-In Success");
  delay(1000);
  
  lcd.setCursor(0,0);
  for(int i = 0; i < 17; i++){
    lcd.print((char)readbackblock1[i]);
    lcd.leftToRight();    
  }
  lcd.setCursor(0,1);
  lcd.print("Count :");
  lcd.setCursor(8,1);
      
  for(int i = 0; i < 17; i++){
    lcd.print((char)readbackblock2[i]);    
    lcd.leftToRight();    
  }
  delay(7000);
  
  lcd.clear();  
}

//function for keypad interfacing
String getKey(int cursorRow, int cursorCol){
  while(1){
    String key;
    digitalWrite(r1, LOW);digitalWrite(r2, HIGH);digitalWrite(r3, HIGH);digitalWrite(r4, HIGH);
      if(digitalRead(c1) == LOW){
         
        key = "1";
        lcd.setCursor(cursorCol,cursorRow);
        lcd.print(key);
        delay(500);
        if(digitalRead(c1) == LOW){
           
          key = "a";
          lcd.setCursor(cursorCol,cursorRow);
          lcd.print(key);
          delay(500);
          
          if(digitalRead(c1) == LOW){
             
            key = "b";
            lcd.setCursor(cursorCol,cursorRow);
            lcd.print(key);
            delay(500);
            if(digitalRead(c1) == LOW){
               
              key = "c";
              lcd.setCursor(cursorCol,cursorRow);
              lcd.print(key);
              delay(500);
            }
          }
        }                
        return key;        
      }
      
      else if(digitalRead(c2) == LOW){
         
        key = "2";
        lcd.setCursor(cursorCol,cursorRow);
        lcd.print(key);
        delay(500);
        if(digitalRead(c2) == LOW){
           
          key = "d";
          lcd.setCursor(cursorCol,cursorRow);
          lcd.print(key);
          delay(500);
          
          if(digitalRead(c2) == LOW){
             
            key = "e";
            lcd.setCursor(cursorCol,cursorRow);
            lcd.print(key);
            delay(500);
            if(digitalRead(c2) == LOW){
               
              key = "f";
              lcd.setCursor(cursorCol,cursorRow);
              lcd.print(key);
              delay(500);
            }
          }
        }
        return key;        
      }
      
      else if(digitalRead(c3) == LOW){
         
        key = "3";
        lcd.setCursor(cursorCol,cursorRow);
        lcd.print(key);
        delay(500);
        if(digitalRead(c3) == LOW){
           
          key = "g";
          lcd.setCursor(cursorCol,cursorRow);
          lcd.print(key);
          delay(500);
          
          if(digitalRead(c3) == LOW){
             
            key = "h";
            lcd.setCursor(cursorCol,cursorRow);
            lcd.print(key);
            delay(500);
            if(digitalRead(c3) == LOW){
               
              key = "i";
              lcd.setCursor(cursorCol,cursorRow);
              lcd.print(key);
              delay(500);
            }
          }
        }
        return key;        
      }
    
    digitalWrite(r1, HIGH);digitalWrite(r2, LOW);digitalWrite(r3, HIGH);digitalWrite(r4, HIGH);
      if(digitalRead(c1) == LOW){
         
        key = "4";
        lcd.setCursor(cursorCol,cursorRow);
        lcd.print(key);
        delay(500);
        if(digitalRead(c1) == LOW){
           
          key = "j";
          lcd.setCursor(cursorCol,cursorRow);
          lcd.print(key);
          delay(500);
          
          if(digitalRead(c1) == LOW){
             
            key = "k";
            lcd.setCursor(cursorCol,cursorRow);
            lcd.print(key);
            delay(500);
            if(digitalRead(c1) == LOW){
               
              key = "l";
              lcd.setCursor(cursorCol,cursorRow);
              lcd.print(key);
              delay(500);
            }
          }
        }
        return key;        
      }
      
      else if(digitalRead(c2) == LOW){
         
        key = "5";
        lcd.setCursor(cursorCol,cursorRow);
        lcd.print(key);
        delay(500);
        if(digitalRead(c2) == LOW){
           
          key = "m";
          lcd.setCursor(cursorCol,cursorRow);
          lcd.print(key);
          delay(500);
          
          if(digitalRead(c2) == LOW){
             
            key = "n";
            lcd.setCursor(cursorCol,cursorRow);
            lcd.print(key);
            delay(500);
            if(digitalRead(c2) == LOW){
               
              key = "o";
              lcd.setCursor(cursorCol,cursorRow);
              lcd.print(key);
              delay(500);
            }
          }
        }
        return key;        
      }
      
      else if(digitalRead(c3) == LOW){
         
        key = "6";
        lcd.setCursor(cursorCol,cursorRow);
        lcd.print(key);
        delay(500);
        if(digitalRead(c3) == LOW){
           
          key = "p";
          lcd.setCursor(cursorCol,cursorRow);
          lcd.print(key);
          delay(500);
          
          if(digitalRead(c3) == LOW){
             
            key = "q";
            lcd.setCursor(cursorCol,cursorRow);
            lcd.print(key);
            delay(500);
            if(digitalRead(c3) == LOW){
               
              key = "r";
              lcd.setCursor(cursorCol,cursorRow);
              lcd.print(key);
              delay(500);
            }
          }
        }
        return key;        
      }
    
    digitalWrite(r1, HIGH);digitalWrite(r2, HIGH);digitalWrite(r3, LOW);digitalWrite(r4, HIGH);
      if(digitalRead(c1) == LOW){
         
        key = "7";
        lcd.setCursor(cursorCol,cursorRow);
        lcd.print(key);
        delay(500);
        if(digitalRead(c1) == LOW){
           
          key = "s";
          lcd.setCursor(cursorCol,cursorRow);
          lcd.print(key);
          delay(500);
          
          if(digitalRead(c1) == LOW){
             
            key = "t";
            lcd.setCursor(cursorCol,cursorRow);
            lcd.print(key);
            delay(500);
            if(digitalRead(c1) == LOW){
               
              key = "u";
              lcd.setCursor(cursorCol,cursorRow);
              lcd.print(key);
              delay(500);
            }
          }
        }
        return key;        
      }
      
      else if(digitalRead(c2) == LOW){
         
        key = "8";
        lcd.setCursor(cursorCol,cursorRow);
        lcd.print(key);
        delay(500);
        if(digitalRead(c2) == LOW){
           
          key = "v";
          lcd.setCursor(cursorCol,cursorRow);
          lcd.print(key);
          delay(500);
          
          if(digitalRead(c2) == LOW){
             
            key = "w";
            lcd.setCursor(cursorCol,cursorRow);
            lcd.print(key);
            delay(500);
            if(digitalRead(c2) == LOW){
               
              key = "x";
              lcd.setCursor(cursorCol,cursorRow);
              lcd.print(key);
              delay(500);
            }
          }
        }
        return key;        
      }
      
      else if(digitalRead(c3) == LOW){
         
        key = "9";
        lcd.setCursor(cursorCol,cursorRow);
        lcd.print(key);
        delay(500);
        if(digitalRead(c3) == LOW){
           
          key = "y";
          lcd.setCursor(cursorCol,cursorRow);
          lcd.print(key);
          delay(500);
          
          if(digitalRead(c3) == LOW){
             
            key = "z";
            lcd.setCursor(cursorCol,cursorRow);
            lcd.print(key);
            delay(500);
          }
        }
        return key;        
      }
    
    digitalWrite(r1, HIGH);digitalWrite(r2, HIGH);digitalWrite(r3, HIGH);digitalWrite(r4, LOW);
      if(digitalRead(c1) == LOW){delay(400);return("*");}
      else if(digitalRead(c2) == LOW){
         
        key = "0";
        lcd.setCursor(cursorCol,cursorRow);
        lcd.print(key);
        delay(500);
        if(digitalRead(c2) == LOW){
           
          key = "_";
          lcd.setCursor(cursorCol,cursorRow);
          lcd.print(key);
          delay(500);
          
          if(digitalRead(c2) == LOW){
             
            key = "!";
            lcd.setCursor(cursorCol,cursorRow);
            lcd.print(key);
            delay(500);
          }
        }
        return key;        
      }
      else if(digitalRead(c3) == LOW){delay(400); return("\n");}    
  }
}

//function for keypad interfacing
String getString(int startCursorCol, int startCursorRow, String text, int stringMaxLength){
    
  int cursorRow  = startCursorRow; int cursorCol = startCursorCol;
  // lcd.clear();
  lcd.setCursor(cursorCol, cursorRow);
  String key = "0";
  int count= 0;

  while(key != "\n"){

    if(count > stringMaxLength){
      return text;
    }

    key = getKey(cursorRow, cursorCol);       
        
    if(key == "*" && cursorCol > startCursorCol){
      cursorCol = cursorCol - 1;
      count = count - 1;
      text[count] = ' ';
      text.trim();      
      lcd.setCursor(cursorCol, cursorRow);
      lcd.print(" ");
    }

    else{
      if(key != "*"){
        
        lcd.setCursor(cursorCol, cursorRow);
        lcd.print(key);
        text = text + key;   
        count = count + 1;            
        cursorCol = cursorCol + 1;
        }
    }   
  }
  lcd.clear();
  return text;
}
