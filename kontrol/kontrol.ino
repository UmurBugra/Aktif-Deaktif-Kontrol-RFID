#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN 22
#define SS_PIN 5
#define BASLANGIC_BLOK 4

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
MFRC522::StatusCode status;

bool cardPresent = false;        // Kartın okunup okunmadığı
String currentData = "";  // Kartın içeriğini tutuyor
unsigned long lastCardCheck = 0; // Kartın en son ne zaman algılandığı
unsigned long lastReadTime = 0;  // Kartın son okunan zamanını saklıyor

const unsigned long CHECK_INTERVAL = 100;   // Kart kontrol sıklığı 
const unsigned long CARD_TIMEOUT = 1000;    // Kart kaybolma süresi 

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  
  Serial.println("RFID Test başlıyor...");
  
  SPI.begin();
  mfrc522.PCD_Init();
  delay(50);
  
  // Varsayılan anahtarı ayarla
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
}

void loop() {
  cardStatus();
  delay(10); 
}

// Kartın varlığını sürekli kontrol eder ve içeriği okur.
void cardStatus() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastReadTime >= CHECK_INTERVAL) {
    lastReadTime = currentTime;
    
    mfrc522.PCD_Init(); // Modül kararsız kalmasın diye her döngüde tekrar başlatma
    
    if (mfrc522.PICC_IsNewCardPresent()) {
      if (mfrc522.PICC_ReadCardSerial()) {
        if (!cardPresent) {
          Serial.println();
          Serial.println("Kart Tespit Edildi!");
          Serial.print("Kart UID: ");
          for (byte i = 0; i < mfrc522.uid.size; i++) {
            Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
            Serial.print(mfrc522.uid.uidByte[i], HEX);
          }
          Serial.println();
        }
        lastCardCheck = currentTime;
        byte buffer[18];
        byte size = sizeof(buffer);
        
        status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, BASLANGIC_BLOK, &key, &(mfrc522.uid));
        if (status == MFRC522::STATUS_OK) {
   
          status = mfrc522.MIFARE_Read(BASLANGIC_BLOK, buffer, &size);
          if (status == MFRC522::STATUS_OK) {
            String data = "";
            for (byte i = 0; i < 16 && buffer[i] != 0; i++) {
              data += (char)buffer[i];
            }
            
            if (!cardPresent || data != currentData) {
              currentData = data;
              Serial.print("Kart algılandı. Okunan içerik: ");
              Serial.println(data.length() > 0 ? data : "(Boş)");
           
              if (!cardPresent) {
                Serial.println("Kart aktif edildi.");
                cardPresent = true;
              }
            }
          }
        }
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
      }
    }
    // Kart kaldırıldığında 
    if (cardPresent && (currentTime - lastCardCheck >= CARD_TIMEOUT)) {
      Serial.println("Kart algılanamıyor, deaktif ediliyor.");
      currentData = "";
      cardPresent = false;
      Serial.println("Kart deaktif edildi.");
    }
  }
}
