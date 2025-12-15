// #include <Arduino.h>
// #include <SPI.h>
// #include <MFRC522.h>
// #include <EEPROM.h>
// #include <ArduinoJson.h>

// // ==================== CONFIG ====================
// #define RC522_SS_PIN 53
// #define RC522_RST_PIN 49

// #define REGISTERED_BOOKS_START 0
// #define MAX_REGISTERED_BOOKS 50
// #define UID_LENGTH 8

// #define PENDING_START (REGISTERED_BOOKS_START + MAX_REGISTERED_BOOKS * (UID_LENGTH + 1))
// #define PENDING_ENTRY_SIZE 256
// #define PENDING_MAX 8

// #define DEBOUNCE_TIME 3000UL
// #define REQUEST_TIMEOUT 8000UL
// #define PING_INTERVAL 30000UL

// #define NUM_SENSORS 20
// const byte sensorPins[NUM_SENSORS] = {22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41};

// // ==================== GLOBALS ====================
// MFRC522 mfrc522(RC522_SS_PIN, RC522_RST_PIN);
// bool wifiConnected = false;
// String lastCardUID = "";
// unsigned long lastCardTime = 0;
// String registeredUIDs[MAX_REGISTERED_BOOKS];

// // ==================== CLEAN EEPROM INIT ====================
// void initializeEEPROM() {
//   // Clear corrupted ASCII from EEPROM
//   Serial.println("Cleaning EEPROM...");
  
//   // Clear registered books area
//   for (int i = 0; i < MAX_REGISTERED_BOOKS * (UID_LENGTH + 1); i++) {
//     byte value = EEPROM.read(REGISTERED_BOOKS_START + i);
//     if (value > 127 || value < 32) { // Non-ASCII or control characters
//       EEPROM.write(REGISTERED_BOOKS_START + i, 0xFF);
//     }
//   }
  
//   // Clear pending queue area
//   for (int i = 0; i < PENDING_MAX * PENDING_ENTRY_SIZE; i++) {
//     EEPROM.write(PENDING_START + i, 0xFF);
//   }
  
//   Serial.println("EEPROM cleaned");
// }

// // ==================== EEPROM HELPERS ====================
// String eepromReadString(int addr, int maxLen) {
//   char buffer[maxLen + 1];
//   for (int i = 0; i < maxLen; i++) {
//     byte c = EEPROM.read(addr + i);
//     if (c == 0xFF || c == 0x00) {
//       buffer[i] = '\0';
//       break;
//     }
//     if (c >= 32 && c <= 126) { // Only ASCII printable characters
//       buffer[i] = c;
//     } else {
//       buffer[i] = '\0';
//       break;
//     }
//   }
//   buffer[maxLen] = '\0';
//   return String(buffer);
// }

// void eepromWriteString(int addr, const String &str, int maxLen) {
//   unsigned int strLen = str.length();
//   unsigned int len = strLen < (unsigned int)(maxLen - 1) ? strLen : (maxLen - 1);
  
//   for (unsigned int i = 0; i < len; i++) {
//     char c = str.charAt(i);
//     if (c >= 32 && c <= 126) { // Only write ASCII printable characters
//       EEPROM.write(addr + i, c);
//     } else {
//       EEPROM.write(addr + i, '?'); // Replace with safe character
//     }
//   }
//   EEPROM.write(addr + len, 0xFF); // Mark end with 0xFF
// }

// bool isBookRegistered(const String &uid) {
//   if (uid.length() != UID_LENGTH) return false;
  
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     if (registeredUIDs[i] == uid) {
//       return true;
//     }
//   }
//   return false;
// }

// void saveRegisteredBook(const String &uid) {
//   if (uid.length() != UID_LENGTH || isBookRegistered(uid)) return;
  
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     if (registeredUIDs[i].length() == 0) {
//       int addr = REGISTERED_BOOKS_START + i * (UID_LENGTH + 1);
//       eepromWriteString(addr, uid, UID_LENGTH);
//       registeredUIDs[i] = uid;
//       Serial.print("SAVED UID: ");
//       Serial.println(uid);
//       return;
//     }
//   }
// }

// // ==================== PENDING QUEUE ====================
// int pendingIndexAddr(int index) { 
//   return PENDING_START + index * PENDING_ENTRY_SIZE; 
// }

// bool addPendingEntry(const String &payload) {
//   for (int i = 0; i < PENDING_MAX; i++) {
//     int addr = pendingIndexAddr(i);
//     if (EEPROM.read(addr) == 0xFF) {
//       eepromWriteString(addr, payload, PENDING_ENTRY_SIZE - 1);
//       Serial.print("QUEUED: ");
//       Serial.println(payload.substring(0, 30));
//       return true;
//     }
//   }
//   return false;
// }

// bool popPendingEntry(String &outPayload) {
//   for (int i = 0; i < PENDING_MAX; i++) {
//     int addr = pendingIndexAddr(i);
//     if (EEPROM.read(addr) != 0xFF) {
//       outPayload = eepromReadString(addr, PENDING_ENTRY_SIZE - 1);
      
//       // Clear the slot
//       for (int j = 0; j < PENDING_ENTRY_SIZE; j++) {
//         EEPROM.write(addr + j, 0xFF);
//       }
//       return true;
//     }
//   }
//   return false;
// }

// // ==================== ESP COMMUNICATION ====================
// void sendToESP(const String &msg) {
//   // Clean command - no emojis, only ASCII
//   Serial1.println(msg); // Simple println, ESP will handle line endings
//   Serial.print("TO_ESP: ");
//   Serial.println(msg);
// }

// bool waitForResponse(String &response, unsigned long timeout) {
//   unsigned long start = millis();
//   String buffer = "";
  
//   while (millis() - start < timeout) {
//     while (Serial1.available()) {
//       char c = Serial1.read();
      
//       if (c == '\n') {
//         response = buffer;
//         buffer = "";
//         response.trim();
        
//         if (response.length() > 0) {
//           // Filter out debug prints from ESP
//           if (!response.startsWith("Sending enhanced JSON:") && 
//               !response.startsWith("PATCH") && 
//               !response.startsWith("GET") &&
//               response != "PONG") {
//             Serial.print("FROM_ESP: ");
//             Serial.println(response);
//           }
//           return true;
//         }
//       } else if (c != '\r' && buffer.length() < 512) {
//         buffer += c;
//       }
//     }
//     delay(1);
//   }
//   return false;
// }

// void flushPendingQueue() {
//   if (!wifiConnected) return;
  
//   String payload;
//   while (popPendingEntry(payload)) {
//     if (payload.length() > 0) {
//       sendToESP(payload);
//       delay(50); // Small delay between sends
//     }
//   }
// }

// // ==================== RFID ====================
// String readRFID() {
//   if (!mfrc522.PICC_IsNewCardPresent()) return "";
//   if (!mfrc522.PICC_ReadCardSerial()) return "";
  
//   String uid = "";
//   for (byte i = 0; i < mfrc522.uid.size; i++) {
//     if (mfrc522.uid.uidByte[i] < 0x10) uid += "0";
//     uid += String(mfrc522.uid.uidByte[i], HEX);
//   }
//   uid.toUpperCase();
//   mfrc522.PICC_HaltA();
//   return uid;
// }

// // ==================== SENSOR ====================
// int getSensorIndexForUID(const String &uid) {
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     if (registeredUIDs[i] == uid) {
//       return i < NUM_SENSORS ? i : -1;
//     }
//   }
//   return -1;
// }

// String getBookStatus(int sensorIndex) {
//   if (sensorIndex < 0 || sensorIndex >= NUM_SENSORS) return "AVL";
//   int state = digitalRead(sensorPins[sensorIndex]);
//   return (state == LOW) ? "AVL" : "BRW";
// }

// // ==================== PROCESS CARD ====================
// void processCard(const String &cardUID) {
//   // Debounce
//   if (cardUID == lastCardUID && millis() - lastCardTime < DEBOUNCE_TIME) {
//     return;
//   }
  
//   lastCardUID = cardUID;
//   lastCardTime = millis();
  
//   // Get sensor status
//   int sensorIndex = getSensorIndexForUID(cardUID);
//   String sensorStatus = getBookStatus(sensorIndex);
  
//   Serial.println("=================================");
//   Serial.print("CARD: ");
//   Serial.println(cardUID);
//   Serial.print("SENSOR: ");
//   Serial.println(sensorStatus);
//   Serial.println("=================================");
  
//   // Step 1: Get existing data from Firebase
//   bool serverIsRegistered = false;
//   String serverTitle = "New Book";
//   String serverAuthor = "Unknown";
//   String serverLocation = "Unassigned";
//   String serverEspStatus = "AVL";
  
//   if (wifiConnected) {
//     sendToESP("GET:" + cardUID);
//     String response;
//     if (waitForResponse(response, REQUEST_TIMEOUT)) {
//       if (response.startsWith("BOOKDATA:")) {
//         int p = response.indexOf('|');
//         if (p != -1) {
//           String serverData = response.substring(p + 1);
          
//           if (serverData != "null") {
//             DynamicJsonDocument doc(1024);
//             DeserializationError err = deserializeJson(doc, serverData);
//             if (!err) {
//               serverIsRegistered = doc["isRegistered"] | false;
//               serverTitle = doc["title"] | "New Book";
//               serverAuthor = doc["author"] | "Unknown";
//               serverLocation = doc["location"] | "Unassigned";
              
//               if (doc.containsKey("espStatus")) {
//                 serverEspStatus = doc["espStatus"] | "AVL";
//               } else if (doc.containsKey("status")) {
//                 String status = doc["status"] | "Available";
//                 if (status == "Borrowed" || status == "BRW") serverEspStatus = "BRW";
//                 else if (status == "Overdue" || status == "OVD") serverEspStatus = "OVD";
//                 else if (status == "Misplaced" || status == "MIS") serverEspStatus = "MIS";
//                 else serverEspStatus = "AVL";
//               }
              
//               Serial.print("FIREBASE: Registered=");
//               Serial.print(serverIsRegistered ? "YES" : "NO");
//               Serial.print(", Status=");
//               Serial.println(serverEspStatus);
              
//               if (serverIsRegistered && !isBookRegistered(cardUID)) {
//                 saveRegisteredBook(cardUID);
//                 Serial.println("EEPROM updated from Firebase");
//               }
//             }
//           }
//         }
//       }
//     }
//   }
  
//   // Step 2: Prepare JSON
//   DynamicJsonDocument doc(1024);
  
//   doc["title"] = serverTitle;
//   doc["author"] = serverAuthor;
//   doc["location"] = serverLocation;
//   doc["isRegistered"] = serverIsRegistered;
//   doc["lastSeen"] = String(millis() / 1000);
//   doc["sensor"] = sensorIndex >= 0 ? sensorIndex : 0;
  
//   // Determine final ESP status
//   String finalEspStatus;
//   if (serverIsRegistered && serverEspStatus != "AVL" && serverEspStatus != "BRW") {
//     finalEspStatus = serverEspStatus; // Keep special status
//   } else {
//     finalEspStatus = sensorStatus; // Use sensor status
//   }
  
//   doc["espStatus"] = finalEspStatus;
  
//   // Add human-readable status
//   if (finalEspStatus == "AVL") doc["status"] = "Available";
//   else if (finalEspStatus == "BRW") doc["status"] = "Borrowed";
//   else if (finalEspStatus == "OVD") doc["status"] = "Overdue";
//   else if (finalEspStatus == "MIS") doc["status"] = "Misplaced";
//   else doc["status"] = "Available";
  
//   doc["lastUpdated"] = String(millis() / 1000);
  
//   String outJson;
//   serializeJson(doc, outJson);
  
//   // Step 3: Send to Firebase
//   String sendCmd = "SEND:/books/" + cardUID + "|" + outJson;
  
//   if (wifiConnected) {
//     sendToESP(sendCmd);
    
//     String response;
//     if (waitForResponse(response, REQUEST_TIMEOUT)) {
//       if (response.startsWith("SUCCESS:")) {
//         if (serverIsRegistered && !isBookRegistered(cardUID)) {
//           saveRegisteredBook(cardUID);
//         }
//         Serial.println("Firebase update OK");
//       } else if (response.startsWith("ERROR:")) {
//         Serial.print("Firebase error: ");
//         Serial.println(response);
//         addPendingEntry(sendCmd);
//       } else {
//         addPendingEntry(sendCmd);
//       }
//     } else {
//       Serial.println("No ESP response");
//       addPendingEntry(sendCmd);
//     }
//   } else {
//     Serial.println("WiFi offline - queued");
//     addPendingEntry(sendCmd);
//   }
// }

// // ==================== SETUP ====================
// void setup() {
//   Serial.begin(115200);
//   Serial1.begin(115200);
//   delay(1000);
  
//   Serial.println();
//   Serial.println("LIBRARY SYSTEM - MEGA 2560");
//   Serial.println("==========================");
  
//   // Initialize EEPROM
//   initializeEEPROM();
  
//   // Initialize RFID
//   pinMode(RC522_RST_PIN, OUTPUT);
//   digitalWrite(RC522_RST_PIN, HIGH);
//   SPI.begin();
//   mfrc522.PCD_Init();
//   delay(200);
  
//   // Initialize sensors
//   for (int i = 0; i < NUM_SENSORS; i++) {
//     pinMode(sensorPins[i], INPUT_PULLUP);
//   }
  
//   // Load registered UIDs
//   Serial.println("Loading registered books...");
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     int addr = REGISTERED_BOOKS_START + i * (UID_LENGTH + 1);
//     String uid = eepromReadString(addr, UID_LENGTH);
    
//     if (uid.length() == UID_LENGTH) {
//       registeredUIDs[i] = uid;
//       Serial.print("- ");
//       Serial.println(uid);
//     } else {
//       registeredUIDs[i] = "";
//     }
//   }
  
//   // Clear pending queue
//   for (int i = 0; i < PENDING_MAX; i++) {
//     int addr = pendingIndexAddr(i);
//     for (int j = 0; j < PENDING_ENTRY_SIZE; j++) {
//       EEPROM.write(addr + j, 0xFF);
//     }
//   }
  
//   // Test ESP connection
//   Serial.println("Testing ESP connection...");
//   sendToESP("PING");
//   String response;
//   if (waitForResponse(response, 3000)) {
//     if (response == "PONG") {
//       wifiConnected = true;
//       Serial.println("ESP connected");
//       Serial.println("WiFi OK");
//       flushPendingQueue();
//     }
//   } else {
//     wifiConnected = false;
//     Serial.println("ESP not responding");
//   }
  
//   Serial.println("System Ready");
// }

// // ==================== LOOP ====================
// void loop() {
//   // 1. Read RFID
//   String uid = readRFID();
//   if (uid.length() == 8) {
//     processCard(uid);
//   }
  
//   // 2. Handle background ESP messages
//   static String espBuffer = "";
//   while (Serial1.available()) {
//     char c = Serial1.read();
    
//     if (c == '\n') {
//       espBuffer.trim();
//       if (espBuffer.length() > 0) {
//         // Filter out debug messages
//         if (!espBuffer.startsWith("Sending enhanced JSON:") && 
//             !espBuffer.startsWith("PATCH") && 
//             !espBuffer.startsWith("GET") &&
//             espBuffer != "PONG") {
//           Serial.print("ESP_MSG: ");
//           Serial.println(espBuffer);
//         }
        
//         // Handle command responses
//         if (espBuffer == "WIFI:OK") {
//           wifiConnected = true;
//           Serial1.println("ACK_WIFI");
//           flushPendingQueue();
//         } else if (espBuffer == "WIFI:OFFLINE") {
//           wifiConnected = false;
//         }
//       }
//       espBuffer = "";
//     } else if (c != '\r' && espBuffer.length() < 256) {
//       espBuffer += c;
//     }
//   }
  
//   // 3. Periodic ping
//   static unsigned long lastPing = 0;
//   if (millis() - lastPing > PING_INTERVAL) {
//     lastPing = millis();
    
//     sendToESP("PING");
//     String response;
//     if (waitForResponse(response, 3000)) {
//       if (response == "PONG") {
//         if (!wifiConnected) {
//           wifiConnected = true;
//           Serial.println("WiFi restored");
//           flushPendingQueue();
//         }
//       }
//     } else {
//       wifiConnected = false;
//     }
//   }
  
//   delay(10);
// }
















// aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
// #include <Arduino.h>
// #include <SPI.h>
// #include <MFRC522.h>
// #include <EEPROM.h>
// #include <ArduinoJson.h>

// // ==================== CONFIG ====================
// #define RC522_SS_PIN 53
// #define RC522_RST_PIN 49

// #define REGISTERED_BOOKS_START 0
// #define MAX_REGISTERED_BOOKS 50
// #define UID_LENGTH 8

// #define PENDING_START (REGISTERED_BOOKS_START + MAX_REGISTERED_BOOKS * (UID_LENGTH + 1))
// #define PENDING_ENTRY_SIZE 512
// #define PENDING_MAX 8

// #define DEBOUNCE_TIME 3000UL
// #define REQUEST_TIMEOUT 8000UL
// #define PING_INTERVAL 30000UL

// #define NUM_SENSORS 20
// const byte sensorPins[NUM_SENSORS] = {22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41};

// // ==================== GLOBALS ====================
// MFRC522 mfrc522(RC522_SS_PIN, RC522_RST_PIN);
// bool wifiConnected = false;
// String lastCardUID = "";
// unsigned long lastCardTime = 0;
// String registeredUIDs[MAX_REGISTERED_BOOKS];

// // ==================== FIXED EEPROM - BINARY SAFE ====================
// String eepromReadString(int addr, int maxLen) {
//   char buffer[maxLen + 1];
//   int i;
//   for (i = 0; i < maxLen; i++) {
//     byte c = EEPROM.read(addr + i);
//     if (c == 0x00) { // Null terminator
//       break;
//     }
//     buffer[i] = c;
//   }
//   buffer[i] = '\0';
//   return String(buffer);
// }

// void eepromWriteString(int addr, const String &str, int maxLen) {
//   unsigned int strLen = str.length();
//   unsigned int len = min(strLen, (unsigned int)(maxLen - 1));
  
//   // Write string bytes
//   for (unsigned int i = 0; i < len; i++) {
//     EEPROM.write(addr + i, str.charAt(i));
//   }
//   // Write null terminator
//   EEPROM.write(addr + len, 0x00);
//   // Fill rest with 0xFF to mark as used but with terminator
//   for (unsigned int i = len + 1; i < maxLen; i++) {
//     EEPROM.write(addr + i, 0xFF);
//   }
// }

// bool isBookRegistered(const String &uid) {
//   if (uid.length() != UID_LENGTH) return false;
  
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     if (registeredUIDs[i] == uid) {
//       return true;
//     }
//   }
//   return false;
// }

// void saveRegisteredBook(const String &uid) {
//   if (uid.length() != UID_LENGTH || isBookRegistered(uid)) return;
  
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     if (registeredUIDs[i].length() == 0) {
//       int addr = REGISTERED_BOOKS_START + i * (UID_LENGTH + 1);
//       eepromWriteString(addr, uid, UID_LENGTH);
//       registeredUIDs[i] = uid;
//       Serial.print("SAVED UID: ");
//       Serial.println(uid);
//       return;
//     }
//   }
// }

// // ==================== PENDING QUEUE ====================
// int pendingIndexAddr(int index) { 
//   return PENDING_START + index * PENDING_ENTRY_SIZE; 
// }

// bool addPendingEntry(const String &payload) {
//   for (int i = 0; i < PENDING_MAX; i++) {
//     int addr = pendingIndexAddr(i);
//     byte firstByte = EEPROM.read(addr);
    
//     if (firstByte == 0xFF) { // Empty slot
//       eepromWriteString(addr, payload, PENDING_ENTRY_SIZE - 1);
//       Serial.print("QUEUED FULL: ");
//       Serial.println(payload);
//       return true;
//     }
//   }
//   Serial.println("QUEUE FULL!");
//   return false;
// }

// bool popPendingEntry(String &outPayload) {
//   for (int i = 0; i < PENDING_MAX; i++) {
//     int addr = pendingIndexAddr(i);
//     byte firstByte = EEPROM.read(addr);
    
//     if (firstByte != 0xFF && firstByte != 0x00) {
//       outPayload = eepromReadString(addr, PENDING_ENTRY_SIZE - 1);
      
//       // Clear the slot
//       for (int j = 0; j < PENDING_ENTRY_SIZE; j++) {
//         EEPROM.write(addr + j, 0xFF);
//       }
      
//       if (outPayload.length() > 10) {
//         return true;
//       }
//     }
//   }
//   return false;
// }

// // ==================== FIXED SERIAL COMMUNICATION ====================
// void sendToESP(const String &msg) {
//   // Send in chunks to avoid buffer overflow
//   String cleanMsg = msg;
//   cleanMsg.replace("\r", "");
//   cleanMsg.replace("\n", "");
  
//   Serial1.println(cleanMsg);
//   Serial.print("TO_ESP: ");
//   Serial.println(cleanMsg);
// }

// bool waitForResponse(String &response, unsigned long timeout) {
//   unsigned long start = millis();
//   response = "";
  
//   while (millis() - start < timeout) {
//     while (Serial1.available()) {
//       char c = Serial1.read();
      
//       if (c == '\n') {
//         response.trim();
//         if (response.length() > 0) {
//           if (response.startsWith("DEBUG:")) {
//             Serial.print("ESP_DEBUG: ");
//             Serial.println(response.substring(6));
//             response = "";
//             continue;
//           }
//           Serial.print("FROM_ESP: ");
//           Serial.println(response);
//           return true;
//         }
//       } else if (c != '\r' && response.length() < 2048) {
//         response += c;
//       }
//     }
//     delay(1);
//   }
  
//   if (response.length() > 0) {
//     response.trim();
//     if (response.length() > 0) {
//       Serial.print("FROM_ESP (incomplete): ");
//       Serial.println(response);
//       return true;
//     }
//   }
  
//   return false;
// }

// void flushPendingQueue() {
//   if (!wifiConnected) return;
  
//   String payload;
//   while (popPendingEntry(payload)) {
//     if (payload.length() > 10) {
//       Serial.print("FLUSHING: ");
//       Serial.println(payload);
      
//       sendToESP(payload);
//       String response;
      
//       if (waitForResponse(response, REQUEST_TIMEOUT)) {
//         if (response.startsWith("SUCCESS:")) {
//           Serial.println("FLUSH SUCCESS");
//           delay(100);
//         } else {
//           // Re-add to queue
//           addPendingEntry(payload);
//           Serial.println("FLUSH FAILED - RE-QUEUED");
//           break;
//         }
//       } else {
//         addPendingEntry(payload);
//         Serial.println("NO RESPONSE - RE-QUEUED");
//         break;
//       }
//     }
//   }
// }

// // ==================== RFID ====================
// String readRFID() {
//   if (!mfrc522.PICC_IsNewCardPresent()) return "";
//   if (!mfrc522.PICC_ReadCardSerial()) return "";
  
//   String uid = "";
//   for (byte i = 0; i < mfrc522.uid.size; i++) {
//     if (mfrc522.uid.uidByte[i] < 0x10) uid += "0";
//     uid += String(mfrc522.uid.uidByte[i], HEX);
//   }
//   uid.toUpperCase();
//   mfrc522.PICC_HaltA();
//   return uid;
// }

// // ==================== IMPROVED SENSOR READING ====================
// String getBookStatus(int sensorIndex) {
//   if (sensorIndex < 0 || sensorIndex >= NUM_SENSORS) return "AVL";
  
//   // Read sensor multiple times for stability
//   int readings = 0;
//   for (int i = 0; i < 5; i++) {
//     if (digitalRead(sensorPins[sensorIndex]) == LOW) {
//       readings++;
//     }
//     delay(2);
//   }
  
//   // If 3 or more readings are LOW, book is present (AVL)
//   return (readings >= 3) ? "AVL" : "BRW";
// }

// int getSensorIndexForUID(const String &uid) {
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     if (registeredUIDs[i] == uid) {
//       return i < NUM_SENSORS ? i : -1;
//     }
//   }
//   return -1;
// }

// // ==================== PROCESS CARD - COMPLETE FIX ====================
// void processCard(const String &cardUID) {
//   // Debounce
//   if (cardUID == lastCardUID && millis() - lastCardTime < DEBOUNCE_TIME) {
//     return;
//   }
  
//   lastCardUID = cardUID;
//   lastCardTime = millis();
  
//   // Get sensor status with better debouncing
//   int sensorIndex = getSensorIndexForUID(cardUID);
//   String sensorStatus = getBookStatus(sensorIndex);
  
//   Serial.println("=================================");
//   Serial.print("CARD: ");
//   Serial.println(cardUID);
//   Serial.print("SENSOR INDEX: ");
//   Serial.println(sensorIndex);
//   Serial.print("SENSOR STATUS: ");
//   Serial.println(sensorStatus);
//   Serial.println("=================================");
  
//   // Step 1: Get existing data from Firebase
//   bool serverIsRegistered = false;
//   String serverTitle = "New Book";
//   String serverAuthor = "Unknown";
//   String serverLocation = "Unassigned";
//   String serverEspStatus = "AVL";
  
//   if (wifiConnected) {
//     sendToESP("GET:" + cardUID);
//     String response;
//     if (waitForResponse(response, REQUEST_TIMEOUT)) {
//       if (response.startsWith("BOOKDATA:")) {
//         int p = response.indexOf('|');
//         if (p != -1) {
//           String serverData = response.substring(p + 1);
          
//           if (serverData != "null") {
//             DynamicJsonDocument doc(1024);
//             DeserializationError err = deserializeJson(doc, serverData);
//             if (!err) {
//               serverIsRegistered = doc["isRegistered"] | false;
//               serverTitle = String(doc["title"] | "New Book");
//               serverAuthor = String(doc["author"] | "Unknown");
//               serverLocation = String(doc["location"] | "Unassigned");
              
//               if (doc.containsKey("espStatus")) {
//                 serverEspStatus = String(doc["espStatus"] | "AVL");
//               } else if (doc.containsKey("status")) {
//                 String status = doc["status"] | "Available";
//                 if (status == "Borrowed" || status == "BRW") serverEspStatus = "BRW";
//                 else if (status == "Overdue" || status == "OVD") serverEspStatus = "OVD";
//                 else if (status == "Misplaced" || status == "MIS") serverEspStatus = "MIS";
//                 else serverEspStatus = "AVL";
//               }
              
//               Serial.print("FIREBASE: Registered=");
//               Serial.print(serverIsRegistered ? "YES" : "NO");
//               Serial.print(", Status=");
//               Serial.println(serverEspStatus);
              
//               if (serverIsRegistered && !isBookRegistered(cardUID)) {
//                 saveRegisteredBook(cardUID);
//                 Serial.println("EEPROM updated from Firebase");
//               }
//             }
//           } else {
//             Serial.println("Book not found in Firebase");
//           }
//         }
//       }
//     } else {
//       Serial.println("GET request timeout");
//     }
//   }
  
//   // Step 2: Prepare COMPLETE JSON
//   DynamicJsonDocument doc(1536); // Increased buffer
  
//   // Clean strings
//   String cleanTitle = serverTitle;
//   cleanTitle.trim();
  
//   String cleanAuthor = serverAuthor;
//   cleanAuthor.trim();
  
//   String cleanLocation = serverLocation;
//   cleanLocation.trim();
  
//   doc["title"] = cleanTitle;
//   doc["author"] = cleanAuthor;
//   doc["location"] = cleanLocation;
//   doc["isRegistered"] = serverIsRegistered;
//   doc["lastSeen"] = String(millis() / 1000);
//   doc["sensor"] = sensorIndex >= 0 ? sensorIndex : 0;
  
//   // Determine final ESP status
//   String finalEspStatus;
//   if (serverIsRegistered && (serverEspStatus == "OVD" || serverEspStatus == "MIS")) {
//     finalEspStatus = serverEspStatus; // Keep special status
//   } else {
//     finalEspStatus = sensorStatus; // Use sensor status
//   }
  
//   doc["espStatus"] = finalEspStatus;
  
//   // Add human-readable status
//   if (finalEspStatus == "AVL") doc["status"] = "Available";
//   else if (finalEspStatus == "BRW") doc["status"] = "Borrowed";
//   else if (finalEspStatus == "OVD") doc["status"] = "Overdue";
//   else if (finalEspStatus == "MIS") doc["status"] = "Misplaced";
//   else doc["status"] = "Available";
  
//   doc["lastUpdated"] = String(millis() / 1000);
  
//   String outJson;
//   serializeJson(doc, outJson);
  
//   Serial.print("FULL JSON SIZE: ");
//   Serial.println(outJson.length());
//   Serial.print("JSON: ");
//   Serial.println(outJson);
  
//   // Step 3: Send to Firebase
//   String sendCmd = "SEND:/books/" + cardUID + "|" + outJson;
  
//   if (wifiConnected) {
//     sendToESP(sendCmd);
    
//     String response;
//     if (waitForResponse(response, REQUEST_TIMEOUT)) {
//       if (response.startsWith("SUCCESS:")) {
//         if (serverIsRegistered && !isBookRegistered(cardUID)) {
//           saveRegisteredBook(cardUID);
//         }
//         Serial.println("Firebase update OK");
//       } else if (response.startsWith("ERROR:")) {
//         Serial.print("Firebase error: ");
//         Serial.println(response);
//         addPendingEntry(sendCmd);
//       } else {
//         Serial.print("Unexpected response: ");
//         Serial.println(response);
//         addPendingEntry(sendCmd);
//       }
//     } else {
//       Serial.println("No ESP response");
//       addPendingEntry(sendCmd);
//     }
//   } else {
//     Serial.println("WiFi offline - queued");
//     addPendingEntry(sendCmd);
//   }
// }

// // ==================== SETUP ====================
// void setup() {
//   Serial.begin(115200);
//   Serial1.begin(115200);
  
//   // Clear serial buffers
//   while (Serial.available()) Serial.read();
//   while (Serial1.available()) Serial1.read();
  
//   delay(2000);
  
//   Serial.println();
//   Serial.println("LIBRARY SYSTEM - MEGA 2560");
//   Serial.println("==========================");
  
//   // Check if EEPROM needs initialization
//   byte initialized = EEPROM.read(4095); // Use last byte
//   if (initialized != 0xAA) {
//     Serial.println("Initializing EEPROM...");
//     // Clear only if not initialized
//     for (int i = 0; i < 4096; i++) {
//       EEPROM.write(i, 0xFF);
//     }
//     EEPROM.write(4095, 0xAA);
//     Serial.println("EEPROM initialized");
//   } else {
//     Serial.println("EEPROM already initialized");
//   }
  
//   // Initialize RFID
//   SPI.begin();
//   mfrc522.PCD_Init();
//   delay(200);
  
//   // Initialize sensors
//   for (int i = 0; i < NUM_SENSORS; i++) {
//     pinMode(sensorPins[i], INPUT_PULLUP);
//   }
  
//   // Load registered UIDs
//   Serial.println("Loading registered books...");
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     registeredUIDs[i] = "";
//   }
  
//   // Test ESP connection
//   Serial.println("Testing ESP connection...");
//   wifiConnected = false;
  
//   for (int i = 0; i < 3; i++) {
//     sendToESP("PING");
//     String response;
//     if (waitForResponse(response, 2000)) {
//       if (response == "PONG") {
//         wifiConnected = true;
//         Serial.println("ESP connected");
//         break;
//       }
//     }
//     delay(1000);
//   }
  
//   if (!wifiConnected) {
//     Serial.println("ESP not responding");
//   } else {
//     // Try to flush queue
//     flushPendingQueue();
//   }
  
//   Serial.println("System Ready");
// }

// // ==================== LOOP ====================
// void loop() {
//   // 1. Read RFID
//   String uid = readRFID();
//   if (uid.length() == 8) {
//     processCard(uid);
//   }
  
//   // 2. Handle ESP messages
//   static String espBuffer = "";
//   while (Serial1.available()) {
//     char c = Serial1.read();
    
//     if (c == '\n') {
//       espBuffer.trim();
//       if (espBuffer.length() > 0) {
//         if (espBuffer.startsWith("WIFI:")) {
//           if (espBuffer == "WIFI:OK") {
//             wifiConnected = true;
//             Serial.println("WiFi connected");
//             flushPendingQueue();
//           } else if (espBuffer == "WIFI:OFFLINE") {
//             wifiConnected = false;
//             Serial.println("WiFi offline");
//           }
//         } else if (espBuffer == "PONG") {
//           wifiConnected = true;
//         }
//       }
//       espBuffer = "";
//     } else if (c != '\r' && espBuffer.length() < 1024) {
//       espBuffer += c;
//     }
//   }
  
//   // 3. Periodic ping
//   static unsigned long lastPing = 0;
//   if (millis() - lastPing > PING_INTERVAL) {
//     lastPing = millis();
    
//     sendToESP("PING");
//     String response;
//     if (waitForResponse(response, 3000)) {
//       if (response == "PONG") {
//         if (!wifiConnected) {
//           wifiConnected = true;
//           Serial.println("WiFi restored");
//           flushPendingQueue();
//         }
//       }
//     } else {
//       if (wifiConnected) {
//         wifiConnected = false;
//         Serial.println("WiFi lost");
//       }
//     }
//   }
  
//   delay(10);
// }






// //bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbOOFFLINE ISUE, ---TURN TO NEW BOOK
// #include <Arduino.h>
// #include <SPI.h>
// #include <MFRC522.h>
// #include <EEPROM.h>
// #include <ArduinoJson.h>

// // ==================== CONFIG ====================
// #define RC522_SS_PIN 53
// #define RC522_RST_PIN 49

// #define REGISTERED_BOOKS_START 0
// #define MAX_REGISTERED_BOOKS 50
// #define UID_LENGTH 8

// #define PENDING_START (REGISTERED_BOOKS_START + MAX_REGISTERED_BOOKS * (UID_LENGTH + 1))
// #define PENDING_ENTRY_SIZE 512
// #define PENDING_MAX 8

// #define DEBOUNCE_TIME 3000UL
// #define REQUEST_TIMEOUT 8000UL
// #define PING_INTERVAL 30000UL

// #define NUM_SENSORS 20
// const byte sensorPins[NUM_SENSORS] = {22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41};

// // ==================== GLOBALS ====================
// MFRC522 mfrc522(RC522_SS_PIN, RC522_RST_PIN);
// bool wifiConnected = false;
// String lastCardUID = "";
// unsigned long lastCardTime = 0;
// String registeredUIDs[MAX_REGISTERED_BOOKS];
// byte sensorAssignments[MAX_REGISTERED_BOOKS]; // Track which sensor goes with which UID

// // ==================== EEPROM FUNCTIONS ====================
// String eepromReadString(int addr, int maxLen) {
//   char buffer[maxLen + 1];
//   int i;
//   for (i = 0; i < maxLen; i++) {
//     byte c = EEPROM.read(addr + i);
//     if (c == 0x00 || c == 0xFF) break;
//     buffer[i] = c;
//   }
//   buffer[i] = '\0';
//   return String(buffer);
// }

// void eepromWriteString(int addr, const String &str, int maxLen) {
//   unsigned int len = min(str.length(), (unsigned int)(maxLen - 1));
//   for (unsigned int i = 0; i < len; i++) {
//     EEPROM.write(addr + i, str.charAt(i));
//   }
//   EEPROM.write(addr + len, 0x00);
// }

// // ==================== SENSOR FUNCTIONS ====================
// String readSensor(int pin) {
//   // Read multiple times with debouncing
//   int lowCount = 0;
//   for (int i = 0; i < 10; i++) {
//     if (digitalRead(pin) == LOW) lowCount++;
//     delay(1);
//   }
//   return (lowCount >= 7) ? "AVL" : "BRW"; // Book present if 70% of readings are LOW
// }

// int findSensorForBook(const String &uid) {
//   // First, check if this UID has an assigned sensor
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     if (registeredUIDs[i] == uid) {
//       return sensorAssignments[i]; // Return assigned sensor
//     }
//   }
  
//   // If not registered or no sensor assigned, find first available sensor
//   // that's not assigned to any book
//   for (int s = 0; s < NUM_SENSORS; s++) {
//     bool sensorUsed = false;
//     for (int j = 0; j < MAX_REGISTERED_BOOKS; j++) {
//       if (sensorAssignments[j] == s) {
//         sensorUsed = true;
//         break;
//       }
//     }
//     if (!sensorUsed) {
//       // Assign this sensor to the book
//       for (int j = 0; j < MAX_REGISTERED_BOOKS; j++) {
//         if (registeredUIDs[j].length() == 0) {
//           registeredUIDs[j] = uid;
//           sensorAssignments[j] = s;
//           return s;
//         }
//       }
//     }
//   }
  
//   return 0; // Default to sensor 0 if all are used
// }

// String getBookStatus(const String &uid) {
//   int sensorIndex = findSensorForBook(uid);
//   if (sensorIndex >= 0 && sensorIndex < NUM_SENSORS) {
//     return readSensor(sensorPins[sensorIndex]);
//   }
//   return "AVL"; // Default if no sensor
// }

// // ==================== BOOK REGISTRATION ====================
// bool isBookRegistered(const String &uid) {
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     if (registeredUIDs[i] == uid) return true;
//   }
//   return false;
// }

// void saveRegisteredBook(const String &uid, int sensorIndex = -1) {
//   if (isBookRegistered(uid)) return;
  
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     if (registeredUIDs[i].length() == 0) {
//       // Save to EEPROM
//       int addr = REGISTERED_BOOKS_START + i * (UID_LENGTH + 1);
//       eepromWriteString(addr, uid, UID_LENGTH);
      
//       // Save in memory
//       registeredUIDs[i] = uid;
      
//       // Assign sensor if provided, otherwise find one
//       if (sensorIndex >= 0 && sensorIndex < NUM_SENSORS) {
//         sensorAssignments[i] = sensorIndex;
//       } else {
//         // Find first available sensor
//         for (int s = 0; s < NUM_SENSORS; s++) {
//           bool sensorUsed = false;
//           for (int j = 0; j < MAX_REGISTERED_BOOKS; j++) {
//             if (sensorAssignments[j] == s) {
//               sensorUsed = true;
//               break;
//             }
//           }
//           if (!sensorUsed) {
//             sensorAssignments[i] = s;
//             break;
//           }
//         }
//       }
      
//       Serial.print("SAVED UID: ");
//       Serial.print(uid);
//       Serial.print(" with sensor ");
//       Serial.println(sensorAssignments[i]);
//       return;
//     }
//   }
// }

// // ==================== SERIAL COMMUNICATION ====================
// void clearSerial1Buffer() {
//   delay(100);
//   while (Serial1.available()) {
//     Serial1.read();
//     delay(1);
//   }
// }

// void sendToESP(const String &msg) {
//   clearSerial1Buffer();
//   Serial1.println(msg);
//   Serial.print("TO_ESP: ");
//   Serial.println(msg);
//   delay(10);
// }

// bool waitForResponse(String &response, unsigned long timeout) {
//   unsigned long start = millis();
//   response = "";
  
//   while (millis() - start < timeout) {
//     while (Serial1.available()) {
//       char c = Serial1.read();
//       if (c == '\n') {
//         response.trim();
//         if (response.length() > 0) {
//           if (!response.startsWith("DEBUG:") && !response.startsWith("WIFI:")) {
//             Serial.print("FROM_ESP: ");
//             Serial.println(response);
//           }
//           return true;
//         }
//       } else if (c != '\r' && response.length() < 2048) {
//         response += c;
//       }
//     }
//     delay(1);
//   }
//   return false;
// }

// // ==================== PENDING QUEUE ====================
// int pendingIndexAddr(int index) { 
//   return PENDING_START + index * PENDING_ENTRY_SIZE; 
// }

// bool addPendingEntry(const String &payload) {
//   for (int i = 0; i < PENDING_MAX; i++) {
//     int addr = pendingIndexAddr(i);
//     if (EEPROM.read(addr) == 0xFF) {
//       eepromWriteString(addr, payload, PENDING_ENTRY_SIZE - 1);
//       Serial.print("QUEUED: ");
//       Serial.println(payload);
//       return true;
//     }
//   }
//   return false;
// }

// bool popPendingEntry(String &outPayload) {
//   for (int i = 0; i < PENDING_MAX; i++) {
//     int addr = pendingIndexAddr(i);
//     if (EEPROM.read(addr) != 0xFF && EEPROM.read(addr) != 0x00) {
//       outPayload = eepromReadString(addr, PENDING_ENTRY_SIZE - 1);
//       // Clear slot
//       for (int j = 0; j < PENDING_ENTRY_SIZE; j++) {
//         EEPROM.write(addr + j, 0xFF);
//       }
//       return outPayload.length() > 10;
//     }
//   }
//   return false;
// }

// void flushPendingQueue() {
//   if (!wifiConnected) return;
  
//   String payload;
//   while (popPendingEntry(payload)) {
//     if (payload.length() > 10) {
//       Serial.print("FLUSHING: ");
//       Serial.println(payload);
//       sendToESP(payload);
      
//       String response;
//       if (waitForResponse(response, REQUEST_TIMEOUT)) {
//         if (response.startsWith("SUCCESS:")) {
//           Serial.println("FLUSH SUCCESS");
//           delay(100);
//         } else {
//           addPendingEntry(payload);
//           break;
//         }
//       } else {
//         addPendingEntry(payload);
//         break;
//       }
//     }
//   }
// }

// // ==================== RFID ====================
// String readRFID() {
//   if (!mfrc522.PICC_IsNewCardPresent()) return "";
//   if (!mfrc522.PICC_ReadCardSerial()) return "";
  
//   String uid = "";
//   for (byte i = 0; i < mfrc522.uid.size; i++) {
//     if (mfrc522.uid.uidByte[i] < 0x10) uid += "0";
//     uid += String(mfrc522.uid.uidByte[i], HEX);
//   }
//   uid.toUpperCase();
//   mfrc522.PICC_HaltA();
//   return uid;
// }

// // ==================== MAIN PROCESSING ====================
// void processCard(const String &cardUID) {
//   // Debounce
//   if (cardUID == lastCardUID && millis() - lastCardTime < DEBOUNCE_TIME) {
//     return;
//   }
  
//   lastCardUID = cardUID;
//   lastCardTime = millis();
  
//   // Get REAL sensor status
//   String sensorStatus = getBookStatus(cardUID);
//   int sensorIndex = findSensorForBook(cardUID);
  
//   Serial.println("=================================");
//   Serial.print("CARD: ");
//   Serial.println(cardUID);
//   Serial.print("SENSOR INDEX: ");
//   Serial.println(sensorIndex);
//   Serial.print("SENSOR STATUS: ");
//   Serial.println(sensorStatus);
//   Serial.println("=================================");
  
//   // Get Firebase data
//   bool serverIsRegistered = false;
//   String serverTitle = "New Book";
//   String serverAuthor = "Unknown";
//   String serverLocation = "Unassigned";
//   String serverEspStatus = "AVL";
//   int serverSensor = 0;
  
//   if (wifiConnected) {
//     sendToESP("GET:" + cardUID);
//     String response;
//     if (waitForResponse(response, REQUEST_TIMEOUT)) {
//       if (response.startsWith("BOOKDATA:")) {
//         int p = response.indexOf('|');
//         if (p != -1) {
//           String serverData = response.substring(p + 1);
          
//           if (serverData != "null") {
//             DynamicJsonDocument doc(1024);
//             DeserializationError err = deserializeJson(doc, serverData);
//             if (!err) {
//               serverIsRegistered = doc["isRegistered"] | false;
//               serverTitle = String(doc["title"] | "New Book");
//               serverAuthor = String(doc["author"] | "Unknown");
//               serverLocation = String(doc["location"] | "Unassigned");
//               serverSensor = doc["sensor"] | 0;
              
//               if (doc.containsKey("espStatus")) {
//                 serverEspStatus = String(doc["espStatus"] | "AVL");
//               }
              
//               Serial.print("FIREBASE: Registered=");
//               Serial.print(serverIsRegistered ? "YES" : "NO");
//               Serial.print(", Status=");
//               Serial.println(serverEspStatus);
              
//               // If Firebase says registered but we don't have it locally, save it
//               if (serverIsRegistered && !isBookRegistered(cardUID)) {
//                 saveRegisteredBook(cardUID, serverSensor);
//               }
//             }
//           }
//         }
//       }
//     }
//   }
  
//   // Create JSON - USE REAL SENSOR STATUS
//   DynamicJsonDocument doc(1024);
  
//   doc["title"] = serverTitle;
//   doc["author"] = serverAuthor;
//   doc["location"] = serverLocation;
//   doc["isRegistered"] = serverIsRegistered;
//   doc["lastSeen"] = String(millis() / 1000);
//   doc["sensor"] = sensorIndex; // Use actual sensor index
  
//   // Use SENSOR STATUS for espStatus, not Firebase status
//   doc["espStatus"] = sensorStatus;
  
//   // Human readable status
//   if (sensorStatus == "AVL") doc["status"] = "Available";
//   else if (sensorStatus == "BRW") doc["status"] = "Borrowed";
//   else if (sensorStatus == "OVD") doc["status"] = "Overdue";
//   else if (sensorStatus == "MIS") doc["status"] = "Misplaced";
//   else doc["status"] = "Available";
  
//   doc["lastUpdated"] = String(millis() / 1000);
  
//   String outJson;
//   serializeJson(doc, outJson);
  
//   Serial.print("JSON SIZE: ");
//   Serial.println(outJson.length());
//   Serial.print("JSON: ");
//   Serial.println(outJson);
  
//   // Send to Firebase
//   String sendCmd = "SEND:/books/" + cardUID + "|" + outJson;
  
//   if (wifiConnected) {
//     sendToESP(sendCmd);
    
//     String response;
//     if (waitForResponse(response, REQUEST_TIMEOUT)) {
//       if (response.startsWith("SUCCESS:")) {
//         // If successful and registered, save locally
//         if (serverIsRegistered && !isBookRegistered(cardUID)) {
//           saveRegisteredBook(cardUID, sensorIndex);
//         }
//         Serial.println("Firebase update OK");
//       } else {
//         addPendingEntry(sendCmd);
//       }
//     } else {
//       addPendingEntry(sendCmd);
//     }
//   } else {
//     addPendingEntry(sendCmd);
//   }
// }

// // ==================== SETUP ====================
// void setup() {
//   Serial.begin(115200);
//   Serial1.begin(115200);
  
//   // Clear initial garbage
//   delay(2000);
//   clearSerial1Buffer();
  
//   Serial.println();
//   Serial.println("LIBRARY SYSTEM - MEGA 2560");
//   Serial.println("==========================");
  
//   // Initialize sensor pins
//   for (int i = 0; i < NUM_SENSORS; i++) {
//     pinMode(sensorPins[i], INPUT_PULLUP);
//   }
  
//   // Initialize arrays
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     registeredUIDs[i] = "";
//     sensorAssignments[i] = 255; // 255 means no sensor assigned
//   }
  
//   // Load registered books from EEPROM
//   Serial.println("Loading registered books...");
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     int addr = REGISTERED_BOOKS_START + i * (UID_LENGTH + 1);
//     String uid = eepromReadString(addr, UID_LENGTH);
//     if (uid.length() == UID_LENGTH) {
//       registeredUIDs[i] = uid;
//       // Assign next available sensor
//       for (int s = 0; s < NUM_SENSORS; s++) {
//         bool sensorUsed = false;
//         for (int j = 0; j < i; j++) {
//           if (sensorAssignments[j] == s) {
//             sensorUsed = true;
//             break;
//           }
//         }
//         if (!sensorUsed) {
//           sensorAssignments[i] = s;
//           break;
//         }
//       }
//       Serial.print("- ");
//       Serial.print(uid);
//       Serial.print(" (sensor ");
//       Serial.print(sensorAssignments[i]);
//       Serial.println(")");
//     }
//   }
  
//   // Initialize RFID
//   SPI.begin();
//   mfrc522.PCD_Init();
//   delay(200);
  
//   // Test ESP connection
//   Serial.println("Testing ESP connection...");
//   wifiConnected = false;
  
//   for (int i = 0; i < 5; i++) {
//     sendToESP("PING");
//     String response;
//     if (waitForResponse(response, 2000)) {
//       if (response == "PONG") {
//         wifiConnected = true;
//         Serial.println("ESP connected");
//         break;
//       }
//     }
//     delay(1000);
//   }
  
//   if (!wifiConnected) {
//     Serial.println("ESP not responding");
//   } else {
//     flushPendingQueue();
//   }
  
//   Serial.println("System Ready");
// }

// // ==================== LOOP ====================
// void loop() {
//   // Read RFID
//   String uid = readRFID();
//   if (uid.length() == 8) {
//     processCard(uid);
//   }
  
//   // Handle ESP messages
//   static String espBuffer = "";
//   while (Serial1.available()) {
//     char c = Serial1.read();
//     if (c == '\n') {
//       espBuffer.trim();
//       if (espBuffer.length() > 0) {
//         if (espBuffer.startsWith("WIFI:")) {
//           if (espBuffer == "WIFI:OK") {
//             wifiConnected = true;
//             Serial.println("WiFi connected");
//             flushPendingQueue();
//           } else if (espBuffer == "WIFI:OFFLINE") {
//             wifiConnected = false;
//             Serial.println("WiFi offline");
//           }
//         } else if (espBuffer == "PONG") {
//           wifiConnected = true;
//         }
//       }
//       espBuffer = "";
//     } else if (c != '\r') {
//       espBuffer += c;
//     }
//   }
  
//   // Periodic ping
//   static unsigned long lastPing = 0;
//   if (millis() - lastPing > PING_INTERVAL) {
//     lastPing = millis();
//     sendToESP("PING");
//     String response;
//     if (waitForResponse(response, 3000)) {
//       if (response == "PONG" && !wifiConnected) {
//         wifiConnected = true;
//         Serial.println("WiFi restored");
//         flushPendingQueue();
//       }
//     } else if (wifiConnected) {
//       wifiConnected = false;
//       Serial.println("WiFi lost");
//     }
//   }
  
//   delay(10);
// }












// // CRITICAL PROBLEM IDENTIFIED! You're absolutely right!

// // The issue is: When offline, the system creates "New Book" entries that overwrite registered books when WiFi comes back!

// // This is a SERIOUS DATA LOSS problem. When offline:

// // GET request fails → returns empty data

// // System creates "New Book" entry with isRegistered: false

// // When online, this overwrites the REAL registered book data!

// // SOLUTION: Offline Cache System
// // We need to:

// // Cache Firebase data locally when online

// // Use cached data when offline instead of creating "New Book"

// // Preserve registration status during offline mode


// #include <Arduino.h>
// #include <SPI.h>
// #include <MFRC522.h>
// #include <EEPROM.h>
// #include <ArduinoJson.h>

// // ==================== CONFIG ====================
// #define RC522_SS_PIN 53
// #define RC522_RST_PIN 49

// #define REGISTERED_BOOKS_START 0
// #define MAX_REGISTERED_BOOKS 50
// #define UID_LENGTH 8

// #define BOOK_CACHE_START (REGISTERED_BOOKS_START + MAX_REGISTERED_BOOKS * (UID_LENGTH + 1))
// #define BOOK_CACHE_SIZE 512  // Enough for book data
// #define MAX_CACHED_BOOKS 20

// #define PENDING_START (BOOK_CACHE_START + MAX_CACHED_BOOKS * BOOK_CACHE_SIZE)
// #define PENDING_ENTRY_SIZE 768  // Increased from 512 for large JSON
// // #define PENDING_ENTRY_SIZE 512
// #define PENDING_MAX 8

// #define DEBOUNCE_TIME 3000UL
// #define REQUEST_TIMEOUT 8000UL
// #define PING_INTERVAL 30000UL

// // // CHANGE THIS:
// // #define NUM_SENSORS 20
// // const byte sensorPins[NUM_SENSORS] = {22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41};

// // TO THIS (using only 15 connected sensors):
// #define NUM_SENSORS 15
// const byte sensorPins[NUM_SENSORS] = {22,23,24,25,26,27,28,29,30,31,32,33,34,35,36}; // Only pins 22-36

// // ==================== GLOBALS ====================
// MFRC522 mfrc522(RC522_SS_PIN, RC522_RST_PIN);
// bool wifiConnected = false;
// String lastCardUID = "";
// unsigned long lastCardTime = 0;
// String registeredUIDs[MAX_REGISTERED_BOOKS];
// byte sensorAssignments[MAX_REGISTERED_BOOKS];


// // ==================== EEPROM HELPERS ====================

// String eepromReadString(int addr, int maxLen) {
//   char buffer[maxLen + 1];
//   int i;
//   for (i = 0; i < maxLen; i++) {
//     byte c = EEPROM.read(addr + i);
//     if (c == 0x00) { // Null terminator
//       break;
//     }
//     if (c == 0xFF) { // Our empty marker (shouldn't be in middle of string)
//       // If we hit 0xFF before null terminator, string was corrupted
//       break;
//     }
//     buffer[i] = c;
//   }
//   buffer[i] = '\0';
//   return String(buffer);
// }
// // String eepromReadString(int addr, int maxLen) {
// //   char buffer[maxLen + 1];
// //   int i;
// //   for (i = 0; i < maxLen; i++) {
// //     byte c = EEPROM.read(addr + i);
// //     if (c == 0x00 || c == 0xFF) break; // Stop at null terminator or empty marker
// //     if (c == 0xFE) {
// //       buffer[i] = (char)0xFF; // Restore original 0xFF
// //     } else {
// //       buffer[i] = c;
// //     }
// //   }
// //   buffer[i] = '\0';
// //   return String(buffer);
// // }
// // ==================== EEPROM HELPERS ====================
// // String eepromReadString(int addr, int maxLen) {
// //   char buffer[maxLen + 1];
// //   int i;
// //   for (i = 0; i < maxLen; i++) {
// //     byte c = EEPROM.read(addr + i);
// //     if (c == 0x00 || c == 0xFF) break;
// //     buffer[i] = c;
// //   }
// //   buffer[i] = '\0';
// //   return String(buffer);
// // }




// void eepromWriteString(int addr, const String &str, int maxLen) {
//   unsigned int len = min(str.length(), (unsigned int)(maxLen - 1));
  
//   // Write ALL characters exactly as they are
//   for (unsigned int i = 0; i < len; i++) {
//     char c = str.charAt(i);
//     EEPROM.write(addr + i, c); // Write ANY byte value
//   }
  
//   // Mark end with 0x00 (null terminator)
//   EEPROM.write(addr + len, 0x00);
  
//   // Fill rest with 0xFF (our empty marker) starting AFTER null terminator
//   for (unsigned int i = len + 1; i < maxLen; i++) {
//     EEPROM.write(addr + i, 0xFF);
//   }
  
//   // Debug: verify important bytes
//   Serial.print("EEPROM write: ");
//   Serial.print(len);
//   Serial.print(" chars, last 5: ");
//   for (int j = max(0, (int)len - 5); j < len; j++) {
//     char c = str.charAt(j);
//     if (c == '\n') Serial.print("\\n");
//     else if (c == '\r') Serial.print("\\r");
//     else if (c == '\0') Serial.print("\\0");
//     else if (c == (char)0xFF) Serial.print("[0xFF]");
//     else Serial.print(c);
//   }
//   Serial.println();
// }
// // void eepromWriteString(int addr, const String &str, int maxLen) {
// //   unsigned int len = min(str.length(), (unsigned int)(maxLen - 1));
  
// //   // Write ALL characters, including special JSON characters
// //   for (unsigned int i = 0; i < len; i++) {
// //     char c = str.charAt(i);
// //     // Write ANY byte value (0-255) except we need to avoid 0xFF (our empty marker)
// //     if (c == (char)0xFF) {
// //       EEPROM.write(addr + i, 0xFE); // Replace 0xFF with 0xFE
// //     } else {
// //       EEPROM.write(addr + i, c);
// //     }
// //   }
  
// //   // Mark end with 0x00 (null terminator)
// //   if (len < (unsigned int)(maxLen - 1)) {
// //     EEPROM.write(addr + len, 0x00);
// //   } else {
// //     EEPROM.write(addr + maxLen - 1, 0x00);
// //   }
// // }
// // void eepromWriteString(int addr, const String &str, int maxLen) {
// //   unsigned int len = min(str.length(), (unsigned int)(maxLen - 1));
// //   for (unsigned int i = 0; i < len; i++) {
// //     EEPROM.write(addr + i, str.charAt(i));
// //   }
// //   EEPROM.write(addr + len, 0x00);
// // }

// // ==================== BOOK CACHE SYSTEM ====================
// int getBookCacheAddr(const String &uid) {
//   // Simple hash function to find cache slot
//   unsigned int hash = 0;
//   for (unsigned int i = 0; i < uid.length(); i++) {
//     hash = (hash * 31) + uid.charAt(i);
//   }
//   int slot = hash % MAX_CACHED_BOOKS;
//   return BOOK_CACHE_START + slot * BOOK_CACHE_SIZE;
// }

// void cacheBookData(const String &uid, const String &jsonData) {
//   if (uid.length() != 8 || jsonData.length() == 0) return;
  
//   int addr = getBookCacheAddr(uid);
//   eepromWriteString(addr, jsonData, BOOK_CACHE_SIZE - 1);
  
//   Serial.print("CACHED BOOK: ");
//   Serial.println(uid);
// }

// String getCachedBookData(const String &uid) {
//   if (uid.length() != 8) return "";
  
//   int addr = getBookCacheAddr(uid);
//   String cached = eepromReadString(addr, BOOK_CACHE_SIZE - 1);
  
//   if (cached.length() > 10 && cached.startsWith("{")) {
//     Serial.print("USING CACHED DATA FOR: ");
//     Serial.println(uid);
//     return cached;
//   }
  
//   return "";
// }

// void clearBookCache(const String &uid) {
//   int addr = getBookCacheAddr(uid);
//   for (int i = 0; i < BOOK_CACHE_SIZE; i++) {
//     EEPROM.write(addr + i, 0xFF);
//   }
// }


// String readSensor(int pin) {
//   // REMOVED: pinMode(pin, INPUT_PULLUP); // Already set in setup()
  
//   int lowCount = 0;
//   for (int i = 0; i < 10; i++) {
//     if (digitalRead(pin) == LOW) lowCount++;
//     delay(1);
//   }
  
//   // If sensor reading is unstable (neither clearly HIGH nor LOW), assume not connected
//   if (lowCount > 3 && lowCount < 7) {
//     return "AVL"; // Assume book is present if sensor is disconnected/floating
//   }
  
//   return (lowCount >= 7) ? "AVL" : "BRW";
// }
// // // ==================== SENSOR FUNCTIONS ====================
// // String readSensor(int pin) {
// //   int lowCount = 0;
// //   for (int i = 0; i < 10; i++) {
// //     if (digitalRead(pin) == LOW) lowCount++;
// //     delay(1);
// //   }
// //   return (lowCount >= 7) ? "AVL" : "BRW";
// // }

// int findSensorForBook(const String &uid) {
//   // FIRST: Check if this book already has a sensor assigned
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     if (registeredUIDs[i] == uid) {
//       // If sensor is already assigned (0-14), return it
//       if (sensorAssignments[i] >= 0 && sensorAssignments[i] < NUM_SENSORS) {
//         return sensorAssignments[i];
//       }
//       // If sensor is 255 (not assigned), break and assign new one
//       break;
//     }
//   }
  
//   // SECOND: Book needs a sensor - find first UNUSED sensor
//   for (int sensorNum = 0; sensorNum < NUM_SENSORS; sensorNum++) {
//     bool sensorInUse = false;
    
//     // Check if any book is already using this sensor
//     for (int j = 0; j < MAX_REGISTERED_BOOKS; j++) {
//       if (sensorAssignments[j] == sensorNum) {
//         sensorInUse = true;
//         break;
//       }
//     }
    
//     // Found an available sensor!
//     if (!sensorInUse) {
//       // Find empty slot to store this book
//       for (int j = 0; j < MAX_REGISTERED_BOOKS; j++) {
//         if (registeredUIDs[j].length() == 0) {
//           registeredUIDs[j] = uid;
//           sensorAssignments[j] = sensorNum;
          
//           // Save to EEPROM
//           int addr = REGISTERED_BOOKS_START + j * (UID_LENGTH + 1);
//           eepromWriteString(addr, uid, UID_LENGTH);
          
//           Serial.print("ASSIGNED: Book ");
//           Serial.print(uid);
//           Serial.print(" → Sensor ");
//           Serial.print(sensorNum);
//           Serial.print(" (Pin ");
//           Serial.print(sensorPins[sensorNum]);
//           Serial.println(")");
//           return sensorNum;
//         }
//       }
//     }
//   }
  
//   // If we get here, all sensors are in use
//   Serial.println("WARNING: All sensors in use! Using sensor 0 as fallback");
//   return 0;
// }

// String getBookStatus(const String &uid) {
//   int sensorIndex = findSensorForBook(uid);
//   if (sensorIndex >= 0 && sensorIndex < NUM_SENSORS) {
//     return readSensor(sensorPins[sensorIndex]);
//   }
//   return "AVL";
// }

// // ==================== BOOK REGISTRATION ====================
// bool isBookRegistered(const String &uid) {
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     if (registeredUIDs[i] == uid) return true;
//   }
//   return false;
// }

// void saveRegisteredBook(const String &uid, int sensorIndex = -1) {
//   if (isBookRegistered(uid)) return;
  
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     if (registeredUIDs[i].length() == 0) {
//       registeredUIDs[i] = uid;
      
//       // Assign sensor
//       if (sensorIndex >= 0 && sensorIndex < NUM_SENSORS) {
//         sensorAssignments[i] = sensorIndex;
//       } else {
//         // Find available sensor
//         for (int s = 0; s < NUM_SENSORS; s++) {
//           bool sensorUsed = false;
//           for (int j = 0; j < MAX_REGISTERED_BOOKS; j++) {
//             if (sensorAssignments[j] == s) {
//               sensorUsed = true;
//               break;
//             }
//           }
//           if (!sensorUsed) {
//             sensorAssignments[i] = s;
//             break;
//           }
//         }
//       }
      
//       // Save to EEPROM
//       int addr = REGISTERED_BOOKS_START + i * (UID_LENGTH + 1);
//       eepromWriteString(addr, uid, UID_LENGTH);
      
//       Serial.print("SAVED UID: ");
//       Serial.print(uid);
//       Serial.print(" with sensor ");
//       Serial.println(sensorAssignments[i]);
//       return;
//     }
//   }
// }

// // ==================== SERIAL COMMUNICATION ====================

// void sendToESP(const String &msg) {
//   // Clear any leftover data in serial buffer
//   while (Serial1.available()) Serial1.read();
//   delay(10);
  
//   // Send the complete message
//   Serial1.println(msg);
  
//   // Debug output
//   Serial.print("TO_ESP: ");
  
//   if (msg.startsWith("SEND:")) {
//     // For SEND messages, show compact version
//     int pipePos = msg.indexOf('|');
//     if (pipePos != -1) {
//       Serial.print(msg.substring(0, pipePos + 1));
//       Serial.print("... [JSON length: ");
//       Serial.print(msg.length() - pipePos - 1);
//       Serial.println("]");
//     } else {
//       // No pipe - show first part
//       int showLen = min(msg.length(), 60);
//       Serial.println(msg.substring(0, showLen));
//     }
//   } else {
//     // For other messages (PING, GET, etc.)
//     Serial.println(msg);
//   }
  
//   // Extra delay for long messages
//   if (msg.length() > 200) {
//     delay(50);
//   }
// }

// // void sendToESP(const String &msg) {
// //   // Send in chunks to avoid buffer overflow
// //   const int CHUNK_SIZE = 64;
  
// //   for (int i = 0; i < msg.length(); i += CHUNK_SIZE) {
// //     String chunk = msg.substring(i, min(i + CHUNK_SIZE, msg.length()));
// //     Serial1.print(chunk);
// //     delay(10); // Small delay between chunks
// //   }
  
// //   Serial1.println(); // Send newline at the end
  
// //   Serial.print("TO_ESP: ");
// //   // Show first and last parts for debugging
// //   int showLen = min(msg.length(), 80);
// //   Serial.print(msg.substring(0, showLen));
// //   if (msg.length() > 80) {
// //     Serial.print("...");
// //     Serial.print(msg.substring(msg.length() - 20));
// //   }
// //   Serial.println();
  
// //   // Add extra delay for long messages
// //   if (msg.length() > 100) {
// //     delay(50);
// //   }
// // }
// // void sendToESP(const String &msg) {
// //   Serial1.println(msg);
// //   Serial.print("TO_ESP: ");
// //   Serial.println(msg);
// // }

// bool waitForResponse(String &response, unsigned long timeout) {
//   unsigned long start = millis();
//   response = "";
  
//   while (millis() - start < timeout) {
//     while (Serial1.available()) {
//       char c = Serial1.read();
//       if (c == '\n') {
//         response.trim();
//         if (response.length() > 0) {
//           if (!response.startsWith("DEBUG:") && !response.startsWith("WIFI:")) {
//             Serial.print("FROM_ESP: ");
//             Serial.println(response);
//           }
//           return true;
//         }
//       } else if (c != '\r' && response.length() < 2048) {
//         response += c;
//       }
//     }
//     delay(1);
//   }
//   return false;
// }

// // ==================== PENDING QUEUE ====================
// int pendingIndexAddr(int index) { 
//   return PENDING_START + index * PENDING_ENTRY_SIZE; 
// }





// // Change the addPendingEntry() logging to show full command (up to reasonable length):
// bool addPendingEntry(const String &payload) {
//   if (!payload.startsWith("SEND:")) {
//     Serial.print("ERROR: Cannot queue - invalid format: ");
//     int showLen = min(payload.length(), 50);
//     Serial.println(payload.substring(0, showLen));
//     return false;
//   }
  
//   for (int i = 0; i < PENDING_MAX; i++) {
//     int addr = pendingIndexAddr(i);
//     if (EEPROM.read(addr) == 0xFF) {
//       eepromWriteString(addr, payload, PENDING_ENTRY_SIZE - 1);
//       Serial.print("QUEUED: ");
      
//       // SAFE: Show first 60 chars OR full length if shorter
//       int showLen = min(payload.length(), 60);
//       Serial.println(payload.substring(0, showLen));
      
//       return true;
//     }
//   }
//   Serial.println("QUEUE FULL!");
//   return false;
// }
// // bool addPendingEntry(const String &payload) {
// //   // FIX: Validate that payload starts with SEND:
// //   if (!payload.startsWith("SEND:")) {
// //     Serial.print("ERROR: Cannot queue - invalid format: ");
// //     Serial.println(payload.substring(0, 50));
// //     return false;
// //   }
  
// //   for (int i = 0; i < PENDING_MAX; i++) {
// //     int addr = pendingIndexAddr(i);
// //     if (EEPROM.read(addr) == 0xFF) {
// //       eepromWriteString(addr, payload, PENDING_ENTRY_SIZE - 1);
// //       Serial.print("QUEUED: ");
// //       Serial.println(payload.substring(0, 60));
// //       return true;
// //     }
// //   }
// //   Serial.println("QUEUE FULL!");
// //   return false;
// // }
// // bool addPendingEntry(const String &payload) {
// //   for (int i = 0; i < PENDING_MAX; i++) {
// //     int addr = pendingIndexAddr(i);
// //     if (EEPROM.read(addr) == 0xFF) {
// //       eepromWriteString(addr, payload, PENDING_ENTRY_SIZE - 1);
// //       Serial.print("QUEUED: ");
// //       Serial.println(payload);
// //       return true;
// //     }
// //   }
// //   return false;
// // }

// bool popPendingEntry(String &outPayload) {
//   for (int i = 0; i < PENDING_MAX; i++) {
//     int addr = pendingIndexAddr(i);
//     if (EEPROM.read(addr) != 0xFF && EEPROM.read(addr) != 0x00) {
//       outPayload = eepromReadString(addr, PENDING_ENTRY_SIZE - 1);
      
//       // Clear slot
//       for (int j = 0; j < PENDING_ENTRY_SIZE; j++) {
//         EEPROM.write(addr + j, 0xFF);
//       }
      
//       // VALIDATE: Must start with "SEND:" and contain "|"
//       if (outPayload.length() > 10 && 
//           outPayload.startsWith("SEND:") && 
//           outPayload.indexOf('|') > 5) {
//         return true;
//       } else {
//         // Invalid entry - skip it
//         Serial.print("INVALID QUEUE ENTRY SKIPPED: ");
//         Serial.println(outPayload.substring(0, 60));
//       }
//     }
//   }
//   return false;
// }

// // bool popPendingEntry(String &outPayload) {
// //   for (int i = 0; i < PENDING_MAX; i++) {
// //     int addr = pendingIndexAddr(i);
// //     if (EEPROM.read(addr) != 0xFF && EEPROM.read(addr) != 0x00) {
// //       outPayload = eepromReadString(addr, PENDING_ENTRY_SIZE - 1);
// //       // Clear slot
// //       for (int j = 0; j < PENDING_ENTRY_SIZE; j++) {
// //         EEPROM.write(addr + j, 0xFF);
// //       }
// //       return outPayload.length() > 10;
// //     }
// //   }
// //   return false;
// // }



// void flushPendingQueue() {
//   if (!wifiConnected) return;
  
//   String payload;
//   int flushed = 0;
  
//   while (popPendingEntry(payload)) {
//     if (payload.length() > 10) {
//       // FIX: Skip malformed entries
//       if (!payload.startsWith("SEND:")) {
//         Serial.print("SKIPPING INVALID QUEUE ENTRY: ");
//         Serial.println(payload.substring(0, 60));
//         continue;
//       }
      
//       Serial.print("FLUSHING: ");
//       Serial.println(payload.substring(0, 60));
//       sendToESP(payload);
      
//       String response;
//       if (waitForResponse(response, REQUEST_TIMEOUT)) {
//         if (response.startsWith("SUCCESS:")) {
//           flushed++;
//           Serial.println("FLUSH SUCCESS");
//           delay(100);
//         } else {
//           addPendingEntry(payload);
//           Serial.println("FLUSH FAILED - RE-QUEUED");
//           break;
//         }
//       } else {
//         addPendingEntry(payload);
//         Serial.println("NO RESPONSE - RE-QUEUED");
//         break;
//       }
//     }
//   }
  
//   if (flushed > 0) {
//     Serial.print("Successfully flushed ");
//     Serial.print(flushed);
//     Serial.println(" queued entries");
//   }
// }

// // void flushPendingQueue() {
// //   if (!wifiConnected) return;
  
// //   String payload;
// //   while (popPendingEntry(payload)) {
// //     if (payload.length() > 10) {
// //       Serial.print("FLUSHING: ");
// //       Serial.println(payload);
// //       sendToESP(payload);
      
// //       String response;
// //       if (waitForResponse(response, REQUEST_TIMEOUT)) {
// //         if (response.startsWith("SUCCESS:")) {
// //           Serial.println("FLUSH SUCCESS");
// //           delay(100);
// //         } else {
// //           addPendingEntry(payload);
// //           break;
// //         }
// //       } else {
// //         addPendingEntry(payload);
// //         break;
// //       }
// //     }
// //   }
// // }

// // ==================== RFID ====================
// String readRFID() {
//   if (!mfrc522.PICC_IsNewCardPresent()) return "";
//   if (!mfrc522.PICC_ReadCardSerial()) return "";
  
//   String uid = "";
//   for (byte i = 0; i < mfrc522.uid.size; i++) {
//     if (mfrc522.uid.uidByte[i] < 0x10) uid += "0";
//     uid += String(mfrc522.uid.uidByte[i], HEX);
//   }
//   uid.toUpperCase();
//   mfrc522.PICC_HaltA();
//   return uid;
// }

// // ==================== MAIN PROCESSING - WITH OFFLINE PROTECTION ====================
// void processCard(const String &cardUID) {
//   // Debounce
//   if (cardUID == lastCardUID && millis() - lastCardTime < DEBOUNCE_TIME) {
//     return;
//   }
  
//   lastCardUID = cardUID;
//   lastCardTime = millis();
  
//   // Get sensor status
//   String sensorStatus = getBookStatus(cardUID);
//   int sensorIndex = findSensorForBook(cardUID);
  
//   Serial.println("=================================");
//   Serial.print("CARD: ");
//   Serial.println(cardUID);
//   Serial.print("SENSOR INDEX: ");
//   Serial.println(sensorIndex);
//   Serial.print("SENSOR STATUS: ");
//   Serial.println(sensorStatus);
//   Serial.println("=================================");
  
//   // Step 1: Try to get data (online or from cache)
//   bool serverIsRegistered = false;
//   String serverTitle = "";
//   String serverAuthor = "";
//   String serverLocation = "";
//   String serverEspStatus = "AVL";
//   int serverSensor = 0;
//   bool usingCachedData = false;
  
//   if (wifiConnected) {
//     // Try to get from Firebase
//     sendToESP("GET:" + cardUID);
//     String response;
//     if (waitForResponse(response, REQUEST_TIMEOUT)) {
//       if (response.startsWith("BOOKDATA:")) {
//         int p = response.indexOf('|');
//         if (p != -1) {
//           String serverData = response.substring(p + 1);
          
//           if (serverData != "null") {
//             DynamicJsonDocument doc(1024);
//             DeserializationError err = deserializeJson(doc, serverData);
//             if (!err) {
//               serverIsRegistered = doc["isRegistered"] | false;
//               serverTitle = String(doc["title"] | "");
//               serverAuthor = String(doc["author"] | "");
//               serverLocation = String(doc["location"] | "");
//               serverSensor = doc["sensor"] | 0;
              
//               if (doc.containsKey("espStatus")) {
//                 serverEspStatus = String(doc["espStatus"] | "AVL");
//               }
              
//               Serial.print("FIREBASE: Registered=");
//               Serial.print(serverIsRegistered ? "YES" : "NO");
//               Serial.print(", Status=");
//               Serial.println(serverEspStatus);
              
//               // Cache this data for offline use
//               cacheBookData(cardUID, serverData);
              
//               // Save locally if registered
//               if (serverIsRegistered && !isBookRegistered(cardUID)) {
//                 saveRegisteredBook(cardUID, serverSensor);
//               }
//             }
//           } else {
//             Serial.println("Book not found in Firebase");
//           }
//         }
//       }
//     } else {
//       Serial.println("GET request timeout, checking cache...");
//       wifiConnected = false; // Mark as offline
//     }
//   }
  
//   // If offline or GET failed, check cache
//   if (!wifiConnected || serverTitle.length() == 0) {
//     String cachedData = getCachedBookData(cardUID);
//     if (cachedData.length() > 0) {
//       DynamicJsonDocument doc(1024);
//       DeserializationError err = deserializeJson(doc, cachedData);
//       if (!err) {
//         serverIsRegistered = doc["isRegistered"] | false;
//         serverTitle = String(doc["title"] | "");
//         serverAuthor = String(doc["author"] | "");
//         serverLocation = String(doc["location"] | "");
//         serverSensor = doc["sensor"] | 0;
        
//         if (doc.containsKey("espStatus")) {
//           serverEspStatus = String(doc["espStatus"] | "AVL");
//         }
        
//         usingCachedData = true;
//         Serial.print("USING CACHED DATA: ");
//         Serial.print(serverTitle);
//         Serial.print(serverIsRegistered ? " (Registered)" : " (Not Registered)");
//         Serial.println(usingCachedData ? " [CACHE]" : "");
//       }
//     }
//   }
  
//   // If still no data (no cache), use defaults BUT preserve registration if known
//   if (serverTitle.length() == 0) {
//     if (isBookRegistered(cardUID)) {
//       // Book is registered locally but no cache - use generic but keep registered status
//       serverTitle = "Book Title Unknown";
//       serverAuthor = "Author Unknown";
//       serverLocation = "Location Unknown";
//       serverIsRegistered = true;
//       Serial.println("Using local registration (no cache)");
//     } else {
//       // Truly new book
//       serverTitle = "New Book";
//       serverAuthor = "Unknown";
//       serverLocation = "Unassigned";
//       serverIsRegistered = false;
//       Serial.println("New unregistered book");
//     }
//   }
  
//   // Step 2: Create JSON
//   DynamicJsonDocument doc(1024);
  
//   // Use cached/online data for title/author/location
//   doc["title"] = serverTitle;
//   doc["author"] = serverAuthor;
//   doc["location"] = serverLocation;
//   doc["isRegistered"] = serverIsRegistered;
//   doc["lastSeen"] = String(millis() / 1000);
//   doc["sensor"] = sensorIndex;
  
//   // CRITICAL: Use SENSOR status for espStatus, NOT cached Firebase status
//   // This ensures physical state is always correct
//   doc["espStatus"] = sensorStatus;
  
//   // Human readable status based on sensor
//   if (sensorStatus == "AVL") doc["status"] = "Available";
//   else if (sensorStatus == "BRW") doc["status"] = "Borrowed";
//   else if (sensorStatus == "OVD") doc["status"] = "Overdue";
//   else if (sensorStatus == "MIS") doc["status"] = "Misplaced";
//   else doc["status"] = "Available";
  
//   doc["lastUpdated"] = String(millis() / 1000);
  
//   // Add cache flag for debugging
//   if (usingCachedData) {
//     doc["source"] = "cached";
//   }
  
//   String outJson;
//   serializeJson(doc, outJson);
  
//   Serial.print("JSON SIZE: ");
//   Serial.println(outJson.length());
//   Serial.print("JSON: ");
//   Serial.println(outJson);



//   // ============ ADD DEBUG CODE RIGHT HERE ============
// // Add this code IMMEDIATELY AFTER serializeJson():

// Serial.print("JSON SIZE: ");
// Serial.println(outJson.length());
// Serial.print("JSON: ");
// int showLen = min(outJson.length(), 100);
// Serial.println(outJson.substring(0, showLen));
// if (outJson.length() > 100) {
//   Serial.print("... (truncated) ends with: ");
//   Serial.println(outJson.substring(outJson.length() - 20));
// }

// // CRITICAL: Validate JSON is complete
// if (!outJson.endsWith("}")) {
//   Serial.println("ERROR: JSON missing closing } - FIXING");
//   outJson += "}";
// }

// // Validate JSON structure
// DynamicJsonDocument validateDoc(1024);
// DeserializationError err = deserializeJson(validateDoc, outJson);
// if (err) {
//   Serial.print("JSON VALIDATION ERROR: ");
//   Serial.println(err.c_str());
//   Serial.print("JSON: ");
//   Serial.println(outJson);
//   return; // Don't send invalid JSON
// }

// // Validate JSON length is reasonable
// if (outJson.length() < 50 || outJson.length() > 1000) {
//   Serial.print("WARNING: JSON length suspicious: ");
//   Serial.println(outJson.length());
// }
// // ============ END OF ADDED CODE ============
  
//   // Step 3: Send to Firebase or queue
//   String sendCmd = "SEND:/books/" + cardUID + "|" + outJson;
  
//   if (wifiConnected) {
//     sendToESP(sendCmd);
    
//     String response;
//     if (waitForResponse(response, REQUEST_TIMEOUT)) {
//       if (response.startsWith("SUCCESS:")) {
//         // Update cache with new data (including sensor status)
//         cacheBookData(cardUID, outJson);
        
//         // Save registration if not already saved
//         if (serverIsRegistered && !isBookRegistered(cardUID)) {
//           saveRegisteredBook(cardUID, sensorIndex);
//         }
//         Serial.println("Firebase update OK");
//       } else {
//         addPendingEntry(sendCmd);
//       }
//     } else {
//       addPendingEntry(sendCmd);
//     }
//   } else {
//     addPendingEntry(sendCmd);
//     Serial.println("WiFi offline - queued");
//   }
// }


// void clearCorruptedEEPROM() {
//   Serial.println("Clearing corrupted EEPROM data...");
  
//   // Clear registered books area
//   for (int i = 0; i < MAX_REGISTERED_BOOKS * (UID_LENGTH + 1); i++) {
//     EEPROM.write(REGISTERED_BOOKS_START + i, 0xFF);
//   }
  
//   // Clear book cache area
//   for (int i = 0; i < MAX_CACHED_BOOKS * BOOK_CACHE_SIZE; i++) {
//     EEPROM.write(BOOK_CACHE_START + i, 0xFF);
//   }
  
//   // Clear pending queue area
//   for (int i = 0; i < PENDING_MAX * PENDING_ENTRY_SIZE; i++) {
//     EEPROM.write(PENDING_START + i, 0xFF);
//   }
  
//   Serial.println("EEPROM cleared successfully");
// }
// // ==================== SETUP ====================
// // ==================== SETUP ====================
// void setup() {
//   Serial.begin(115200);
//   Serial1.begin(115200);
  
//   // Clear initial garbage
//   delay(2000);
//   while (Serial1.available()) Serial1.read();
  
//   Serial.println();
//   Serial.println("LIBRARY SYSTEM - MEGA 2560 WITH OFFLINE CACHE");
//   Serial.println("==============================================");


//     // ==== ADD THIS ====
//     // Check if EEPROM is corrupted and clear it
//     bool eepromCorrupted = false;
//     for (int i = 0; i < 20; i++) { // Check first 20 bytes
//         byte val = EEPROM.read(REGISTERED_BOOKS_START + i);
//         if (val != 0xFF && val != 0x00 && (val < '0' || val > '9') && (val < 'A' || val > 'F')) {
//         eepromCorrupted = true;
//         break;
//         }
//     }
    
//     if (eepromCorrupted) {
//         Serial.println("EEPROM corrupted - clearing all data");
//         clearCorruptedEEPROM();
//     }
//     // ==== END ADD ====
  
//   // Initialize sensor pins
//   for (int i = 0; i < NUM_SENSORS; i++) {
//     pinMode(sensorPins[i], INPUT_PULLUP);
//   }
  
//   // Initialize arrays - IMPORTANT: Start with NO sensors assigned
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     registeredUIDs[i] = "";
//     sensorAssignments[i] = 255; // 255 means "not assigned yet"
//   }
  
//   // Load registered books from EEPROM - WITHOUT assigning sensors
//   Serial.println("Loading registered books from EEPROM...");
//   int bookCount = 0;
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     int addr = REGISTERED_BOOKS_START + i * (UID_LENGTH + 1);
//     String uid = eepromReadString(addr, UID_LENGTH);
//     if (uid.length() == UID_LENGTH) {
//       registeredUIDs[bookCount] = uid;
//       sensorAssignments[bookCount] = 255; // Sensor not assigned yet
//       Serial.print("- Book ");
//       Serial.print(bookCount);
//       Serial.print(": ");
//       Serial.print(uid);
//       Serial.println(" (sensor not assigned)");
//       bookCount++;
//     }
//   }
  
//   if (bookCount == 0) {
//     Serial.println("No registered books found in EEPROM");
//   } else {
//     Serial.print("Loaded ");
//     Serial.print(bookCount);
//     Serial.println(" registered books");
//   }
  
//   // Initialize RFID
//   SPI.begin();
//   mfrc522.PCD_Init();
//   delay(200);
  
//   // Test ESP connection
//   Serial.println("Testing ESP connection...");
//   wifiConnected = false;
  
//   for (int i = 0; i < 3; i++) {
//     sendToESP("PING");
//     String response;
//     if (waitForResponse(response, 2000)) {
//       if (response == "PONG") {
//         wifiConnected = true;
//         Serial.println("ESP connected");
//         break;
//       }
//     }
//     delay(1000);
//   }
  
//   if (!wifiConnected) {
//     Serial.println("ESP not responding - OFFLINE MODE");
//   } else {
//     flushPendingQueue();
//   }
  
//   Serial.println("System Ready - Sensors will be assigned when books are scanned");
// }


// // ==================== LOOP ====================
// void loop() {
//   // Read RFID
//   String uid = readRFID();
//   if (uid.length() == 8) {
//     processCard(uid);
//   }
  
//   // Handle ESP messages
//   static String espBuffer = "";
//   while (Serial1.available()) {
//     char c = Serial1.read();
//     if (c == '\n') {
//       espBuffer.trim();
//       if (espBuffer.length() > 0) {
//         if (espBuffer.startsWith("WIFI:")) {
//           if (espBuffer == "WIFI:OK") {
//             if (!wifiConnected) {
//               wifiConnected = true;
//               Serial.println("WiFi connected - FLUSHING QUEUE");
//               flushPendingQueue();
//             }
//           } else if (espBuffer == "WIFI:OFFLINE") {
//             wifiConnected = false;
//             Serial.println("WiFi offline - CACHE MODE");
//           }
//         } else if (espBuffer == "PONG") {
//           wifiConnected = true;
//         }
//       }
//       espBuffer = "";
//     } else if (c != '\r') {
//       espBuffer += c;
//     }
//   }
  
//   // Periodic ping
//   static unsigned long lastPing = 0;
//   if (millis() - lastPing > PING_INTERVAL) {
//     lastPing = millis();
//     sendToESP("PING");
//     String response;
//     if (waitForResponse(response, 3000)) {
//       if (response == "PONG" && !wifiConnected) {
//         wifiConnected = true;
//         Serial.println("WiFi restored - FLUSHING QUEUE");
//         flushPendingQueue();
//       }
//     } else if (wifiConnected) {
//       wifiConnected = false;
//       Serial.println("WiFi lost - CACHE MODE");
//     }
//   }
  
//   delay(10);
// }
// // PERFECT! The system is working GREAT! But I see one small bug in the queue flushing. Look at this line:

// // text
// // FLUSHING: {"title":"Book Title Unknown","author":"Author Unknown","location":"Location Unknown","isRegistered":true,"lastSeen":"246","sensor":1,"espStatus":"AVL","status":"Available","lastUpdated":"246"}
// // The queued entry is missing the SEND:/books/UID| prefix! That's why it fails when trying to flush. Let me fix this ONE LAST BUG:




//==========================================================================================================================
//==========================================================================================================================
//==========================================================================================================================
//==========================================================================================================================
//==========================================================================================================================
//==========================================================================================================================
//==========================================================================================================================
//=========================================================================================================================
//==========================================================================================================================
//==========================================================================================================================
//==========================================================================================================================
//==========================================================================================================================
//==========================================================================================================================
//==========================================================================================================================
//==========================================================================================================================
//=========================================================================================================================
//==========================================================================================================================
//==========================================================================================================================
//==========================================================================================================================
//==========================================================================================================================
//==========================================================================================================================
//==========================================================================================================================
//==========================================================================================================================
//==========================================================================================================================
// CRITICAL PROBLEM IDENTIFIED! You're absolutely right!

// The issue is: When offline, the system creates "New Book" entries that overwrite registered books when WiFi comes back!

// This is a SERIOUS DATA LOSS problem. When offline:

// GET request fails → returns empty data

// System creates "New Book" entry with isRegistered: false

// When online, this overwrites the REAL registered book data!

// SOLUTION: Offline Cache System
// We need to:

// Cache Firebase data locally when online

// Use cached data when offline instead of creating "New Book"

// Preserve registration status during offline mode
// #include <Arduino.h>
// #include <SPI.h>
// #include <MFRC522.h>
// #include <EEPROM.h>
// #include <ArduinoJson.h>

// // ==================== CONFIG ====================
// #define RC522_SS_PIN 53
// #define RC522_RST_PIN 49

// #define REGISTERED_BOOKS_START 0
// #define MAX_REGISTERED_BOOKS 50
// #define UID_LENGTH 8

// #define BOOK_CACHE_START (REGISTERED_BOOKS_START + MAX_REGISTERED_BOOKS * (UID_LENGTH + 1))
// #define BOOK_CACHE_SIZE 512  // Enough for book data
// #define MAX_CACHED_BOOKS 20

// #define PENDING_START (BOOK_CACHE_START + MAX_CACHED_BOOKS * BOOK_CACHE_SIZE)
// #define PENDING_ENTRY_SIZE 768  // Increased from 512 for large JSON
// #define PENDING_MAX 8

// #define DEBOUNCE_TIME 3000UL
// #define REQUEST_TIMEOUT 8000UL
// #define PING_INTERVAL 30000UL

// #define NUM_SENSORS 15
// const byte sensorPins[NUM_SENSORS] = {22,23,24,25,26,27,28,29,30,31,32,33,34,35,36};

// // ==================== GLOBALS ====================
// MFRC522 mfrc522(RC522_SS_PIN, RC522_RST_PIN);
// bool wifiConnected = false;
// String lastCardUID = "";
// unsigned long lastCardTime = 0;
// String registeredUIDs[MAX_REGISTERED_BOOKS];
// byte sensorAssignments[MAX_REGISTERED_BOOKS];

// // ==================== EEPROM HELPERS - FIXED ====================
// String eepromReadString(int addr, int maxLen) {
//   char buffer[maxLen + 1];
//   int i;
//   for (i = 0; i < maxLen; i++) {
//     byte c = EEPROM.read(addr + i);
    
//     // Only stop on actual null terminator (0x00)
//     if (c == 0x00) {
//       break;
//     }
//     // Convert 0x01 back to 0x00 if we're using escape codes
//     if (c == 0x01) {
//       buffer[i] = (char)0x00;
//     } else {
//       buffer[i] = (char)c;
//     }
//   }
//   buffer[i] = '\0';
//   return String(buffer);
// }

// void eepromWriteString(int addr, const String &str, int maxLen) {
//   unsigned int len = min(str.length(), (unsigned int)(maxLen - 1));
  
//   // Write the string with escape handling for 0x00
//   for (unsigned int i = 0; i < len; i++) {
//     char c = str.charAt(i);
    
//     if (c == (char)0x00) {
//       // Escape 0x00 as 0x01
//       EEPROM.write(addr + i, 0x01);
//     } else {
//       EEPROM.write(addr + i, (byte)c);
//     }
//   }
  
//   // Write null terminator
//   EEPROM.write(addr + len, 0x00);
  
//   // Fill remaining with 0xFF (but only after null terminator!)
//   for (unsigned int i = len + 1; i < maxLen; i++) {
//     EEPROM.write(addr + i, 0xFF);
//   }
  
//   Serial.print("EEPROM write: ");
//   Serial.print(len);
//   Serial.print(" chars, last 5: ");
//   for (int j = max(0, (int)len - 5); j < len; j++) {
//     char c = str.charAt(j);
//     if (c == '\n') Serial.print("\\n");
//     else if (c == '\r') Serial.print("\\r");
//     else if (c == '\0') Serial.print("\\0");
//     else if (c == (char)0xFF) Serial.print("[0xFF]");
//     else Serial.print(c);
//   }
//   Serial.println();
// }

// // ==================== JSON VALIDATION ====================
// bool isValidJSON(const String &json) {
//   // Quick validation
//   if (json.length() < 10) return false;
//   if (!json.startsWith("{")) return false;
//   if (!json.endsWith("}")) return false;
  
//   // Count braces to ensure it's balanced
//   int braceCount = 0;
//   bool inString = false;
  
//   for (unsigned int i = 0; i < json.length(); i++) {
//     char c = json.charAt(i);
    
//     // Handle escaped quotes in strings
//     if (c == '\\' && i + 1 < json.length()) {
//       i++; // Skip next character
//       continue;
//     }
    
//     // Track string boundaries
//     if (c == '"') {
//       inString = !inString;
//     }
    
//     // Count braces (but only outside strings)
//     if (!inString) {
//       if (c == '{') braceCount++;
//       else if (c == '}') braceCount--;
      
//       // If braces go negative, something's wrong
//       if (braceCount < 0) return false;
//     }
//   }
  
//   // Should end with braceCount = 0 and not in a string
//   return (braceCount == 0 && !inString);
// }

// // ==================== CORRUPTION DETECTION - PROPERLY FIXED ====================
// bool isEEPROMCorrupted() {
//   // Check if EEPROM is COMPLETELY empty (all 0xFF) - that's OK, not corrupted
//   bool allEmpty = true;
//   for (int i = 0; i < 20; i++) {
//     if (EEPROM.read(REGISTERED_BOOKS_START + i) != 0xFF) {
//       allEmpty = false;
//       break;
//     }
//   }
  
//   if (allEmpty) {
//     Serial.println("EEPROM is empty (not corrupted)");
//     return false;
//   }
  
//   // Check for actual corruption (non-hex characters in UID positions)
//   for (int i = 0; i < 5; i++) {
//     int addr = REGISTERED_BOOKS_START + i * (UID_LENGTH + 1);
    
//     // First check if this slot is empty (0xFF or 0x00)
//     bool slotEmpty = true;
//     for (int j = 0; j < UID_LENGTH; j++) {
//       byte c = EEPROM.read(addr + j);
//       if (c != 0xFF && c != 0x00) {
//         slotEmpty = false;
//         break;
//       }
//     }
    
//     if (slotEmpty) continue; // Empty slot, move to next
    
//     // Read the UID string
//     String uid = eepromReadString(addr, UID_LENGTH);
    
//     // A valid UID should be exactly 8 hex characters (0-9, A-F)
//     if (uid.length() == UID_LENGTH) {
//       bool validHex = true;
//       for (int j = 0; j < uid.length(); j++) {
//         char c = uid.charAt(j);
//         if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) {
//           validHex = false;
//           break;
//         }
//       }
      
//       if (!validHex) {
//         Serial.print("Found corrupted UID at slot ");
//         Serial.print(i);
//         Serial.print(": ");
//         Serial.println(uid);
//         return true;
//       }
//     } else if (uid.length() > 0 && uid.length() < UID_LENGTH) {
//       // Partial UID (not 8 chars) - corrupted
//       Serial.print("Found partial UID at slot ");
//       Serial.print(i);
//       Serial.print(": ");
//       Serial.println(uid);
//       return true;
//     }
//     // If uid.length() == 0, it's an empty slot (0x00 terminator), that's OK
//   }
  
//   return false;
// }

// void clearCorruptedEEPROM() {
//   Serial.println("Clearing corrupted EEPROM data...");
  
//   // Clear registered books area
//   for (int i = 0; i < MAX_REGISTERED_BOOKS * (UID_LENGTH + 1); i++) {
//     EEPROM.write(REGISTERED_BOOKS_START + i, 0xFF);
//   }
  
//   // Clear book cache area
//   for (int i = 0; i < MAX_CACHED_BOOKS * BOOK_CACHE_SIZE; i++) {
//     EEPROM.write(BOOK_CACHE_START + i, 0xFF);
//   }
  
//   // Clear pending queue area
//   for (int i = 0; i < PENDING_MAX * PENDING_ENTRY_SIZE; i++) {
//     EEPROM.write(PENDING_START + i, 0xFF);
//   }
  
//   Serial.println("EEPROM cleared successfully");
// }

// // ==================== BOOK CACHE SYSTEM - FIXED ====================
// int getBookCacheAddr(const String &uid) {
//   // Simple hash function to find cache slot
//   unsigned int hash = 0;
//   for (unsigned int i = 0; i < uid.length(); i++) {
//     hash = (hash * 31) + uid.charAt(i);
//   }
//   int slot = hash % MAX_CACHED_BOOKS;
//   return BOOK_CACHE_START + slot * BOOK_CACHE_SIZE;
// }

// void cacheBookData(const String &uid, const String &jsonData) {
//   if (uid.length() != 8 || jsonData.length() == 0) return;
  
//   // IMPORTANT: Validate JSON is complete before storing
//   if (!isValidJSON(jsonData)) {
//     Serial.println("ERROR: Invalid JSON - not caching");
//     return;
//   }
  
//   int addr = getBookCacheAddr(uid);
  
//   // Store with length prefix for reliability
//   unsigned int len = jsonData.length();
//   len = min(len, (unsigned int)(BOOK_CACHE_SIZE - 3));
  
//   // Store length (2 bytes)
//   EEPROM.write(addr, (len >> 8) & 0xFF);
//   EEPROM.write(addr + 1, len & 0xFF);
  
//   // Store the data
//   for (unsigned int i = 0; i < len; i++) {
//     char c = jsonData.charAt(i);
//     if (c == (char)0x00) {
//       EEPROM.write(addr + 2 + i, 0x01); // Escape 0x00
//     } else {
//       EEPROM.write(addr + 2 + i, (byte)c);
//     }
//   }
  
//   // Terminate with 0xFF
//   EEPROM.write(addr + 2 + len, 0xFF);
  
//   Serial.print("CACHED BOOK: ");
//   Serial.println(uid);
// }

// String getCachedBookData(const String &uid) {
//   if (uid.length() != 8) return "";
  
//   int addr = getBookCacheAddr(uid);
  
//   // Try to read with length prefix first
//   int len = (EEPROM.read(addr) << 8) | EEPROM.read(addr + 1);
  
//   // Validate length
//   if (len > 0 && len <= (BOOK_CACHE_SIZE - 3)) {
//     // Read the data using length prefix
//     String cached = "";
//     for (int i = 0; i < len; i++) {
//       byte c = EEPROM.read(addr + 2 + i);
//       if (c == 0x01) {
//         cached += (char)0x00; // Unescape
//       } else {
//         cached += (char)c;
//       }
//     }
    
//     // Validate it's JSON
//     if (isValidJSON(cached)) {
//       Serial.print("USING CACHED DATA FOR: ");
//       Serial.println(uid);
//       return cached;
//     }
//   }
  
//   // Fallback: Try old string method for backward compatibility
//   String cached = eepromReadString(addr, BOOK_CACHE_SIZE - 1);
//   if (isValidJSON(cached)) {
//     Serial.print("USING OLD CACHE FORMAT FOR: ");
//     Serial.println(uid);
//     return cached;
//   }
  
//   return "";
// }

// // ==================== REGISTERED BOOKS MANAGEMENT ====================
// bool isBookRegistered(const String &uid) {
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     if (registeredUIDs[i] == uid) return true;
//   }
//   return false;
// }

// void saveRegisteredBook(const String &uid, int sensorIndex = -1) {
//   if (isBookRegistered(uid)) return;
  
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     if (registeredUIDs[i].length() == 0) {
//       registeredUIDs[i] = uid;
      
//       // Assign sensor
//       if (sensorIndex >= 0 && sensorIndex < NUM_SENSORS) {
//         sensorAssignments[i] = sensorIndex;
//       } else {
//         // Find available sensor
//         for (int s = 0; s < NUM_SENSORS; s++) {
//           bool sensorUsed = false;
//           for (int j = 0; j < MAX_REGISTERED_BOOKS; j++) {
//             if (sensorAssignments[j] == s) {
//               sensorUsed = true;
//               break;
//             }
//           }
//           if (!sensorUsed) {
//             sensorAssignments[i] = s;
//             break;
//           }
//         }
//       }
      
//       // Save to EEPROM
//       int addr = REGISTERED_BOOKS_START + i * (UID_LENGTH + 1);
//       eepromWriteString(addr, uid, UID_LENGTH);
      
//       Serial.print("SAVED UID: ");
//       Serial.print(uid);
//       Serial.print(" with sensor ");
//       Serial.println(sensorAssignments[i]);
//       return;
//     }
//   }
// }

// void loadRegisteredBooks() {
//   Serial.println("Loading registered books from EEPROM...");
//   int bookCount = 0;
  
//   // Initialize arrays
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     registeredUIDs[i] = "";
//     sensorAssignments[i] = 255; // 255 means "not assigned yet"
//   }
  
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     int addr = REGISTERED_BOOKS_START + i * (UID_LENGTH + 1);
//     String uid = eepromReadString(addr, UID_LENGTH);
    
//     // Only accept valid 8-character hex UIDs
//     if (uid.length() == UID_LENGTH) {
//       bool validHex = true;
//       for (int j = 0; j < uid.length(); j++) {
//         char c = uid.charAt(j);
//         if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) {
//           validHex = false;
//           break;
//         }
//       }
      
//       if (validHex) {
//         registeredUIDs[bookCount] = uid;
//         sensorAssignments[bookCount] = 255; // Will be assigned when scanned
//         Serial.print("- Loaded book ");
//         Serial.print(bookCount);
//         Serial.print(": ");
//         Serial.println(uid);
//         bookCount++;
//       }
//     }
//   }
  
//   if (bookCount == 0) {
//     Serial.println("No registered books found in EEPROM");
//   } else {
//     Serial.print("Loaded ");
//     Serial.print(bookCount);
//     Serial.println(" valid registered books");
//   }
// }

// // ==================== SENSOR FUNCTIONS ====================
// String readSensor(int pin) {
//   int lowCount = 0;
//   for (int i = 0; i < 10; i++) {
//     if (digitalRead(pin) == LOW) lowCount++;
//     delay(1);
//   }
  
//   // If sensor reading is unstable (neither clearly HIGH nor LOW), assume not connected
//   if (lowCount > 3 && lowCount < 7) {
//     return "AVL"; // Assume book is present if sensor is disconnected/floating
//   }
  
//   return (lowCount >= 7) ? "AVL" : "BRW";
// }

// int findSensorForBook(const String &uid) {
//   // FIRST: Check if this book already has a sensor assigned
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     if (registeredUIDs[i] == uid) {
//       // If sensor is already assigned (0-14), return it
//       if (sensorAssignments[i] >= 0 && sensorAssignments[i] < NUM_SENSORS) {
//         return sensorAssignments[i];
//       }
//       // If sensor is 255 (not assigned), break and assign new one
//       break;
//     }
//   }
  
//   // SECOND: Book needs a sensor - find first UNUSED sensor
//   for (int sensorNum = 0; sensorNum < NUM_SENSORS; sensorNum++) {
//     bool sensorInUse = false;
    
//     // Check if any book is already using this sensor
//     for (int j = 0; j < MAX_REGISTERED_BOOKS; j++) {
//       if (sensorAssignments[j] == sensorNum) {
//         sensorInUse = true;
//         break;
//       }
//     }
    
//     // Found an available sensor!
//     if (!sensorInUse) {
//       // Find empty slot to store this book
//       for (int j = 0; j < MAX_REGISTERED_BOOKS; j++) {
//         if (registeredUIDs[j].length() == 0) {
//           registeredUIDs[j] = uid;
//           sensorAssignments[j] = sensorNum;
          
//           // Save to EEPROM
//           int addr = REGISTERED_BOOKS_START + j * (UID_LENGTH + 1);
//           eepromWriteString(addr, uid, UID_LENGTH);
          
//           Serial.print("ASSIGNED: Book ");
//           Serial.print(uid);
//           Serial.print(" → Sensor ");
//           Serial.print(sensorNum);
//           Serial.print(" (Pin ");
//           Serial.print(sensorPins[sensorNum]);
//           Serial.println(")");
//           return sensorNum;
//         }
//       }
//     }
//   }
  
//   // If we get here, all sensors are in use
//   Serial.println("WARNING: All sensors in use! Using sensor 0 as fallback");
//   return 0;
// }

// String getBookStatus(const String &uid) {
//   int sensorIndex = findSensorForBook(uid);
//   if (sensorIndex >= 0 && sensorIndex < NUM_SENSORS) {
//     return readSensor(sensorPins[sensorIndex]);
//   }
//   return "AVL";
// }

// // ==================== SERIAL COMMUNICATION ====================
// void sendToESP(const String &msg) {
//   // Clear any leftover data in serial buffer
//   while (Serial1.available()) Serial1.read();
//   delay(10);
  
//   // Send the complete message
//   Serial1.println(msg);
  
//   // Debug output
//   Serial.print("TO_ESP: ");
  
//   if (msg.startsWith("SEND:")) {
//     // For SEND messages, show compact version
//     int pipePos = msg.indexOf('|');
//     if (pipePos != -1) {
//       Serial.print(msg.substring(0, pipePos + 1));
//       Serial.print("... [JSON length: ");
//       Serial.print(msg.length() - pipePos - 1);
//       Serial.println("]");
//     } else {
//       // No pipe - show first part
//       int showLen = min(msg.length(), 60);
//       Serial.println(msg.substring(0, showLen));
//     }
//   } else {
//     // For other messages (PING, GET, etc.)
//     Serial.println(msg);
//   }
  
//   // Extra delay for long messages
//   if (msg.length() > 200) {
//     delay(50);
//   }
// }

// bool waitForResponse(String &response, unsigned long timeout) {
//   unsigned long start = millis();
//   response = "";
  
//   while (millis() - start < timeout) {
//     while (Serial1.available()) {
//       char c = Serial1.read();
//       if (c == '\n') {
//         response.trim();
//         if (response.length() > 0) {
//           if (!response.startsWith("DEBUG:") && !response.startsWith("WIFI:")) {
//             Serial.print("FROM_ESP: ");
//             Serial.println(response);
//           }
//           return true;
//         }
//       } else if (c != '\r' && response.length() < 2048) {
//         response += c;
//       }
//     }
//     delay(1);
//   }
//   return false;
// }

// // ==================== PENDING QUEUE ====================
// int pendingIndexAddr(int index) { 
//   return PENDING_START + index * PENDING_ENTRY_SIZE; 
// }

// bool addPendingEntry(const String &payload) {
//   if (!payload.startsWith("SEND:")) {
//     Serial.print("ERROR: Cannot queue - invalid format: ");
//     int showLen = min(payload.length(), 50);
//     Serial.println(payload.substring(0, showLen));
//     return false;
//   }
  
//   // Validate the JSON part
//   int pipePos = payload.indexOf('|');
//   if (pipePos != -1) {
//     String jsonPart = payload.substring(pipePos + 1);
//     if (!isValidJSON(jsonPart)) {
//       Serial.println("ERROR: Invalid JSON in payload - not queuing");
//       return false;
//     }
//   }
  
//   for (int i = 0; i < PENDING_MAX; i++) {
//     int addr = pendingIndexAddr(i);
//     if (EEPROM.read(addr) == 0xFF) {
//       eepromWriteString(addr, payload, PENDING_ENTRY_SIZE - 1);
//       Serial.print("QUEUED: ");
      
//       // Show first 60 chars
//       int showLen = min(payload.length(), 60);
//       Serial.println(payload.substring(0, showLen));
      
//       return true;
//     }
//   }
//   Serial.println("QUEUE FULL!");
//   return false;
// }

// bool popPendingEntry(String &outPayload) {
//   for (int i = 0; i < PENDING_MAX; i++) {
//     int addr = pendingIndexAddr(i);
//     if (EEPROM.read(addr) != 0xFF && EEPROM.read(addr) != 0x00) {
//       outPayload = eepromReadString(addr, PENDING_ENTRY_SIZE - 1);
      
//       // Clear slot
//       for (int j = 0; j < PENDING_ENTRY_SIZE; j++) {
//         EEPROM.write(addr + j, 0xFF);
//       }
      
//       // VALIDATE: Must start with "SEND:" and contain valid JSON
//       if (outPayload.length() > 10 && 
//           outPayload.startsWith("SEND:") && 
//           outPayload.indexOf('|') > 5) {
        
//         // Validate JSON part
//         int pipePos = outPayload.indexOf('|');
//         String jsonPart = outPayload.substring(pipePos + 1);
//         if (isValidJSON(jsonPart)) {
//           return true;
//         } else {
//           Serial.print("INVALID JSON IN QUEUE ENTRY - SKIPPED: ");
//           Serial.println(outPayload.substring(0, 60));
//         }
//       } else {
//         // Invalid entry - skip it
//         Serial.print("INVALID QUEUE ENTRY SKIPPED: ");
//         Serial.println(outPayload.substring(0, 60));
//       }
//     }
//   }
//   return false;
// }

// void flushPendingQueue() {
//   if (!wifiConnected) return;
  
//   String payload;
//   int flushed = 0;
  
//   while (popPendingEntry(payload)) {
//     if (payload.length() > 10) {
//       // Skip malformed entries
//       if (!payload.startsWith("SEND:")) {
//         Serial.print("SKIPPING INVALID QUEUE ENTRY: ");
//         Serial.println(payload.substring(0, 60));
//         continue;
//       }
      
//       Serial.print("FLUSHING: ");
//       Serial.println(payload.substring(0, 60));
//       sendToESP(payload);
      
//       String response;
//       if (waitForResponse(response, REQUEST_TIMEOUT)) {
//         if (response.startsWith("SUCCESS:")) {
//           flushed++;
//           Serial.println("FLUSH SUCCESS");
//           delay(100);
//         } else {
//           addPendingEntry(payload);
//           Serial.println("FLUSH FAILED - RE-QUEUED");
//           break;
//         }
//       } else {
//         addPendingEntry(payload);
//         Serial.println("NO RESPONSE - RE-QUEUED");
//         break;
//       }
//     }
//   }
  
//   if (flushed > 0) {
//     Serial.print("Successfully flushed ");
//     Serial.print(flushed);
//     Serial.println(" queued entries");
//   }
// }

// // ==================== RFID ====================
// String readRFID() {
//   if (!mfrc522.PICC_IsNewCardPresent()) return "";
//   if (!mfrc522.PICC_ReadCardSerial()) return "";
  
//   String uid = "";
//   for (byte i = 0; i < mfrc522.uid.size; i++) {
//     if (mfrc522.uid.uidByte[i] < 0x10) uid += "0";
//     uid += String(mfrc522.uid.uidByte[i], HEX);
//   }
//   uid.toUpperCase();
//   mfrc522.PICC_HaltA();
//   return uid;
// }

// // ==================== MAIN PROCESSING ====================
// void processCard(const String &cardUID) {
//   // Debounce
//   if (cardUID == lastCardUID && millis() - lastCardTime < DEBOUNCE_TIME) {
//     return;
//   }
  
//   lastCardUID = cardUID;
//   lastCardTime = millis();
  
//   // Get sensor status
//   String sensorStatus = getBookStatus(cardUID);
//   int sensorIndex = findSensorForBook(cardUID);
  
//   Serial.println("=================================");
//   Serial.print("CARD: ");
//   Serial.println(cardUID);
//   Serial.print("SENSOR INDEX: ");
//   Serial.println(sensorIndex);
//   Serial.print("SENSOR STATUS: ");
//   Serial.println(sensorStatus);
//   Serial.println("=================================");
  
//   // Try to get data (online or from cache)
//   bool serverIsRegistered = false;
//   String serverTitle = "";
//   String serverAuthor = "";
//   String serverLocation = "";
//   String serverEspStatus = "AVL";
//   int serverSensor = 0;
//   bool usingCachedData = false;
  
//   if (wifiConnected) {
//     // Try to get from Firebase
//     sendToESP("GET:" + cardUID);
//     String response;
//     if (waitForResponse(response, REQUEST_TIMEOUT)) {
//       if (response.startsWith("BOOKDATA:")) {
//         int p = response.indexOf('|');
//         if (p != -1) {
//           String serverData = response.substring(p + 1);
          
//           if (serverData != "null") {
//             DynamicJsonDocument doc(1024);
//             DeserializationError err = deserializeJson(doc, serverData);
//             if (!err) {
//               serverIsRegistered = doc["isRegistered"] | false;
//               serverTitle = String(doc["title"] | "");
//               serverAuthor = String(doc["author"] | "");
//               serverLocation = String(doc["location"] | "");
//               serverSensor = doc["sensor"] | 0;
              
//               if (doc.containsKey("espStatus")) {
//                 serverEspStatus = String(doc["espStatus"] | "AVL");
//               }
              
//               Serial.print("FIREBASE: Registered=");
//               Serial.print(serverIsRegistered ? "YES" : "NO");
//               Serial.print(", Status=");
//               Serial.println(serverEspStatus);
              
//               // Cache this data for offline use
//               cacheBookData(cardUID, serverData);
              
//               // Save locally if registered
//               if (serverIsRegistered && !isBookRegistered(cardUID)) {
//                 saveRegisteredBook(cardUID, serverSensor);
//               }
//             }
//           } else {
//             Serial.println("Book not found in Firebase");
//           }
//         }
//       }
//     } else {
//       Serial.println("GET request timeout, checking cache...");
//       wifiConnected = false; // Mark as offline
//     }
//   }
  
//   // If offline or GET failed, check cache
//   if (!wifiConnected || serverTitle.length() == 0) {
//     String cachedData = getCachedBookData(cardUID);
//     if (cachedData.length() > 0) {
//       DynamicJsonDocument doc(1024);
//       DeserializationError err = deserializeJson(doc, cachedData);
//       if (!err) {
//         serverIsRegistered = doc["isRegistered"] | false;
//         serverTitle = String(doc["title"] | "");
//         serverAuthor = String(doc["author"] | "");
//         serverLocation = String(doc["location"] | "");
//         serverSensor = doc["sensor"] | 0;
        
//         if (doc.containsKey("espStatus")) {
//           serverEspStatus = String(doc["espStatus"] | "AVL");
//         }
        
//         usingCachedData = true;
//         Serial.print("USING CACHED DATA: ");
//         Serial.print(serverTitle);
//         Serial.print(serverIsRegistered ? " (Registered)" : " (Not Registered)");
//         Serial.println(usingCachedData ? " [CACHE]" : "");
//       }
//     }
//   }
  
//   // If still no data, use defaults
//   if (serverTitle.length() == 0) {
//     if (isBookRegistered(cardUID)) {
//       serverTitle = "Book Title Unknown";
//       serverAuthor = "Author Unknown";
//       serverLocation = "Location Unknown";
//       serverIsRegistered = true;
//       Serial.println("Using local registration (no cache)");
//     } else {
//       serverTitle = "New Book";
//       serverAuthor = "Unknown";
//       serverLocation = "Unassigned";
//       serverIsRegistered = false;
//       Serial.println("New unregistered book");
//     }
//   }
  
//   // Create JSON
//   DynamicJsonDocument doc(1024);
  
//   doc["title"] = serverTitle;
//   doc["author"] = serverAuthor;
//   doc["location"] = serverLocation;
//   doc["isRegistered"] = serverIsRegistered;
//   doc["lastSeen"] = String(millis() / 1000);
//   doc["sensor"] = sensorIndex;
//   doc["espStatus"] = sensorStatus;
  
//   if (sensorStatus == "AVL") doc["status"] = "Available";
//   else if (sensorStatus == "BRW") doc["status"] = "Borrowed";
//   else if (sensorStatus == "OVD") doc["status"] = "Overdue";
//   else if (sensorStatus == "MIS") doc["status"] = "Misplaced";
//   else doc["status"] = "Available";
  
//   doc["lastUpdated"] = String(millis() / 1000);
  
//   if (usingCachedData) {
//     doc["source"] = "cached";
//   }
  
//   String outJson;
//   serializeJson(doc, outJson);
  
//   // Validate JSON
//   Serial.print("JSON SIZE: ");
//   Serial.println(outJson.length());
  
//   if (!outJson.endsWith("}")) {
//     Serial.println("ERROR: JSON missing closing } - FIXING");
//     outJson += "}";
//   }
  
//   // Step 3: Send to Firebase or queue
//   String sendCmd = "SEND:/books/" + cardUID + "|" + outJson;
  
//   if (wifiConnected) {
//     sendToESP(sendCmd);
    
//     String response;
//     if (waitForResponse(response, REQUEST_TIMEOUT)) {
//       if (response.startsWith("SUCCESS:")) {
//         // Update cache with new data
//         cacheBookData(cardUID, outJson);
        
//         // Save registration if not already saved
//         if (serverIsRegistered && !isBookRegistered(cardUID)) {
//           saveRegisteredBook(cardUID, sensorIndex);
//         }
//         Serial.println("Firebase update OK");
//       } else {
//         addPendingEntry(sendCmd);
//       }
//     } else {
//       addPendingEntry(sendCmd);
//     }
//   } else {
//     addPendingEntry(sendCmd);
//     Serial.println("WiFi offline - queued");
//   }
// }

// // ==================== SETUP ====================
// void setup() {
//   Serial.begin(115200);
//   Serial1.begin(115200, SERIAL_8N1);  // Add parity for better reliability
// //   Serial1.begin(115200); FOR ESP8266
  
//   // Clear initial garbage
//   delay(2000);
//   while (Serial1.available()) Serial1.read();
  
//   Serial.println();
//   Serial.println("LIBRARY SYSTEM - MEGA 2560 WITH OFFLINE CACHE");
//   Serial.println("==============================================");
  
//   // Initialize sensor pins
//   for (int i = 0; i < NUM_SENSORS; i++) {
//     pinMode(sensorPins[i], INPUT_PULLUP);
//   }
  
//   // Check for EEPROM corruption
//   if (isEEPROMCorrupted()) {
//     Serial.println("EEPROM corrupted - clearing all data");
//     clearCorruptedEEPROM();
//   }
  
//   // Load registered books
//   loadRegisteredBooks();
  
//   // Initialize RFID
//   SPI.begin();
//   mfrc522.PCD_Init();
//   delay(200);
  
//   // Test ESP connection
//   Serial.println("Testing ESP connection...");
//   wifiConnected = false;
  
//   for (int i = 0; i < 3; i++) {
//     sendToESP("PING");
//     String response;
//     if (waitForResponse(response, 2000)) {
//       if (response == "PONG") {
//         wifiConnected = true;
//         Serial.println("ESP connected");
//         break;
//       }
//     }
//     delay(1000);
//   }
  
//   if (!wifiConnected) {
//     Serial.println("ESP not responding - OFFLINE MODE");
//   } else {
//     flushPendingQueue();
//   }
  
//   Serial.println("System Ready");
// }

// // ==================== LOOP ====================
// void loop() {
//   // Read RFID
//   String uid = readRFID();
//   if (uid.length() == 8) {
//     processCard(uid);
//   }
  
//   // Handle ESP messages
//   static String espBuffer = "";
//   while (Serial1.available()) {
//     char c = Serial1.read();
//     if (c == '\n') {
//       espBuffer.trim();
//       if (espBuffer.length() > 0) {
//         if (espBuffer.startsWith("WIFI:")) {
//           if (espBuffer == "WIFI:OK") {
//             if (!wifiConnected) {
//               wifiConnected = true;
//               Serial.println("WiFi connected - FLUSHING QUEUE");
//               flushPendingQueue();
//             }
//           } else if (espBuffer == "WIFI:OFFLINE") {
//             wifiConnected = false;
//             Serial.println("WiFi offline - CACHE MODE");
//           }
//         } else if (espBuffer == "PONG") {
//           wifiConnected = true;
//         }
//       }
//       espBuffer = "";
//     } else if (c != '\r') {
//       espBuffer += c;
//     }
//   }
  
//   // Periodic ping
//   static unsigned long lastPing = 0;
//   if (millis() - lastPing > PING_INTERVAL) {
//     lastPing = millis();
//     sendToESP("PING");
//     String response;
//     if (waitForResponse(response, 3000)) {
//       if (response == "PONG" && !wifiConnected) {
//         wifiConnected = true;
//         Serial.println("WiFi restored - FLUSHING QUEUE");
//         flushPendingQueue();
//       }
//     } else if (wifiConnected) {
//       wifiConnected = false;
//       Serial.println("WiFi lost - CACHE MODE");
//     }
//   }
  
//   delay(10);
// }
// //NEED CALIBRATION//ISSUE I NOTICE:
// The sensor status is showing as AVL (Available) but the physical sensor is reading BRW (Borrowed). This is a sensor reading issue, not a communication issue.













// //*************************************NUMBER ONE 111111111111111111111111111111111111 *//
// // //=========================================================================================================================
// // //==========================================================================================================================
// // //==========================================================================================================================
// // //==========================================================================================================================
// // //==========================================================================================================================
// // //==========================================================================================================================
// // //==========================================================================================================================
// // //==========================================================================================================================
// // //=========================================================================================================================
// // //==========================================================================================================================
// // //==========================================================================================================================
// // //==========================================================================================================================
// // //==========================================================================================================================
// // //==========================================================================================================================
// // //==========================================================================================================================
// // //==========================================================================================================================
// // //==========================================================================================================================
// #include <Arduino.h>
// #include <SPI.h>
// #include <MFRC522.h>
// #include <EEPROM.h>
// #include <ArduinoJson.h>

// // ==================== CONFIG ====================
// #define RC522_SS_PIN 53
// #define RC522_RST_PIN 49

// #define REGISTERED_BOOKS_START 0
// #define MAX_REGISTERED_BOOKS 50
// #define UID_LENGTH 8

// #define BOOK_CACHE_START (REGISTERED_BOOKS_START + MAX_REGISTERED_BOOKS * (UID_LENGTH + 1))
// #define BOOK_CACHE_SIZE 512  // Enough for book data
// #define MAX_CACHED_BOOKS 20

// #define PENDING_START (BOOK_CACHE_START + MAX_CACHED_BOOKS * BOOK_CACHE_SIZE)
// #define PENDING_ENTRY_SIZE 768  // Increased from 512 for large JSON
// #define PENDING_MAX 8

// #define DEBOUNCE_TIME 3000UL
// #define REQUEST_TIMEOUT 8000UL
// #define PING_INTERVAL 30000UL

// #define NUM_SENSORS 15
// const byte sensorPins[NUM_SENSORS] = {22,23,24,25,26,27,28,29,30,31,32,33,34,35,36};

// // ==================== GLOBALS ====================
// MFRC522 mfrc522(RC522_SS_PIN, RC522_RST_PIN);
// bool wifiConnected = false;
// String lastCardUID = "";
// unsigned long lastCardTime = 0;
// String registeredUIDs[MAX_REGISTERED_BOOKS];
// byte sensorAssignments[MAX_REGISTERED_BOOKS];

// // ==================== EEPROM HELPERS - FIXED ====================
// String eepromReadString(int addr, int maxLen) {
//   char buffer[maxLen + 1];
//   int i;
//   for (i = 0; i < maxLen; i++) {
//     byte c = EEPROM.read(addr + i);
    
//     // Only stop on actual null terminator (0x00)
//     if (c == 0x00) {
//       break;
//     }
//     // Convert 0x01 back to 0x00 if we're using escape codes
//     if (c == 0x01) {
//       buffer[i] = (char)0x00;
//     } else {
//       buffer[i] = (char)c;
//     }
//   }
//   buffer[i] = '\0';
//   return String(buffer);
// }

// void eepromWriteString(int addr, const String &str, int maxLen) {
//   unsigned int len = min(str.length(), (unsigned int)(maxLen - 1));
  
//   // Write the string with escape handling for 0x00
//   for (unsigned int i = 0; i < len; i++) {
//     char c = str.charAt(i);
    
//     if (c == (char)0x00) {
//       // Escape 0x00 as 0x01
//       EEPROM.write(addr + i, 0x01);
//     } else {
//       EEPROM.write(addr + i, (byte)c);
//     }
//   }
  
//   // Write null terminator
//   EEPROM.write(addr + len, 0x00);
  
//   // Fill remaining with 0xFF (but only after null terminator!)
//   for (unsigned int i = len + 1; i < maxLen; i++) {
//     EEPROM.write(addr + i, 0xFF);
//   }
// }

// // ==================== JSON VALIDATION ====================
// bool isValidJSON(const String &json) {
//   // Quick validation
//   if (json.length() < 10) return false;
//   if (!json.startsWith("{")) return false;
//   if (!json.endsWith("}")) return false;
  
//   // Count braces to ensure it's balanced
//   int braceCount = 0;
//   bool inString = false;
  
//   for (unsigned int i = 0; i < json.length(); i++) {
//     char c = json.charAt(i);
    
//     // Handle escaped quotes in strings
//     if (c == '\\' && i + 1 < json.length()) {
//       i++; // Skip next character
//       continue;
//     }
    
//     // Track string boundaries
//     if (c == '"') {
//       inString = !inString;
//     }
    
//     // Count braces (but only outside strings)
//     if (!inString) {
//       if (c == '{') braceCount++;
//       else if (c == '}') braceCount--;
      
//       // If braces go negative, something's wrong
//       if (braceCount < 0) return false;
//     }
//   }
  
//   // Should end with braceCount = 0 and not in a string
//   return (braceCount == 0 && !inString);
// }

// // ==================== CORRUPTION DETECTION - PROPERLY FIXED ====================
// bool isEEPROMCorrupted() {
//   // Check if EEPROM is COMPLETELY empty (all 0xFF) - that's OK, not corrupted
//   bool allEmpty = true;
//   for (int i = 0; i < 20; i++) {
//     if (EEPROM.read(REGISTERED_BOOKS_START + i) != 0xFF) {
//       allEmpty = false;
//       break;
//     }
//   }
  
//   if (allEmpty) {
//     return false;
//   }
  
//   // Check for actual corruption (non-hex characters in UID positions)
//   for (int i = 0; i < 5; i++) {
//     int addr = REGISTERED_BOOKS_START + i * (UID_LENGTH + 1);
    
//     // First check if this slot is empty (0xFF or 0x00)
//     bool slotEmpty = true;
//     for (int j = 0; j < UID_LENGTH; j++) {
//       byte c = EEPROM.read(addr + j);
//       if (c != 0xFF && c != 0x00) {
//         slotEmpty = false;
//         break;
//       }
//     }
    
//     if (slotEmpty) continue; // Empty slot, move to next
    
//     // Read the UID string
//     String uid = eepromReadString(addr, UID_LENGTH);
    
//     // A valid UID should be exactly 8 hex characters (0-9, A-F)
//     if (uid.length() == UID_LENGTH) {
//       bool validHex = true;
//       for (int j = 0; j < uid.length(); j++) {
//         char c = uid.charAt(j);
//         if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) {
//           validHex = false;
//           break;
//         }
//       }
      
//       if (!validHex) {
//         return true;
//       }
//     } else if (uid.length() > 0 && uid.length() < UID_LENGTH) {
//       // Partial UID (not 8 chars) - corrupted
//       return true;
//     }
//   }
  
//   return false;
// }

// void clearCorruptedEEPROM() {
//   Serial.println("Clearing corrupted EEPROM data...");
  
//   // Clear registered books area
//   for (int i = 0; i < MAX_REGISTERED_BOOKS * (UID_LENGTH + 1); i++) {
//     EEPROM.write(REGISTERED_BOOKS_START + i, 0xFF);
//   }
  
//   // Clear book cache area
//   for (int i = 0; i < MAX_CACHED_BOOKS * BOOK_CACHE_SIZE; i++) {
//     EEPROM.write(BOOK_CACHE_START + i, 0xFF);
//   }
  
//   // Clear pending queue area
//   for (int i = 0; i < PENDING_MAX * PENDING_ENTRY_SIZE; i++) {
//     EEPROM.write(PENDING_START + i, 0xFF);
//   }
  
//   Serial.println("EEPROM cleared successfully");
// }

// // ==================== BOOK CACHE SYSTEM - FIXED ====================
// int getBookCacheAddr(const String &uid) {
//   // Simple hash function to find cache slot
//   unsigned int hash = 0;
//   for (unsigned int i = 0; i < uid.length(); i++) {
//     hash = (hash * 31) + uid.charAt(i);
//   }
//   int slot = hash % MAX_CACHED_BOOKS;
//   return BOOK_CACHE_START + slot * BOOK_CACHE_SIZE;
// }

// void cacheBookData(const String &uid, const String &jsonData) {
//   if (uid.length() != 8 || jsonData.length() == 0) return;
  
//   // IMPORTANT: Validate JSON is complete before storing
//   if (!isValidJSON(jsonData)) {
//     return;
//   }
  
//   int addr = getBookCacheAddr(uid);
  
//   // Store with length prefix for reliability
//   unsigned int len = jsonData.length();
//   len = min(len, (unsigned int)(BOOK_CACHE_SIZE - 3));
  
//   // Store length (2 bytes)
//   EEPROM.write(addr, (len >> 8) & 0xFF);
//   EEPROM.write(addr + 1, len & 0xFF);
  
//   // Store the data
//   for (unsigned int i = 0; i < len; i++) {
//     char c = jsonData.charAt(i);
//     if (c == (char)0x00) {
//       EEPROM.write(addr + 2 + i, 0x01); // Escape 0x00
//     } else {
//       EEPROM.write(addr + 2 + i, (byte)c);
//     }
//   }
  
//   // Terminate with 0xFF
//   EEPROM.write(addr + 2 + len, 0xFF);
// }

// String getCachedBookData(const String &uid) {
//   if (uid.length() != 8) return "";
  
//   int addr = getBookCacheAddr(uid);
  
//   // Try to read with length prefix first
//   int len = (EEPROM.read(addr) << 8) | EEPROM.read(addr + 1);
  
//   // Validate length
//   if (len > 0 && len <= (BOOK_CACHE_SIZE - 3)) {
//     // Read the data using length prefix
//     String cached = "";
//     for (int i = 0; i < len; i++) {
//       byte c = EEPROM.read(addr + 2 + i);
//       if (c == 0x01) {
//         cached += (char)0x00; // Unescape
//       } else {
//         cached += (char)c;
//       }
//     }
    
//     // Validate it's JSON
//     if (isValidJSON(cached)) {
//       return cached;
//     }
//   }
  
//   // Fallback: Try old string method for backward compatibility
//   String cached = eepromReadString(addr, BOOK_CACHE_SIZE - 1);
//   if (isValidJSON(cached)) {
//     return cached;
//   }
  
//   return "";
// }

// // ==================== REGISTERED BOOKS MANAGEMENT ====================
// bool isBookRegistered(const String &uid) {
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     if (registeredUIDs[i] == uid) return true;
//   }
//   return false;
// }

// int getBookIndex(const String &uid) {
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     if (registeredUIDs[i] == uid) return i;
//   }
//   return -1;
// }

// void saveRegisteredBook(const String &uid, int sensorIndex = -1) {
//   if (isBookRegistered(uid)) return;
  
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     if (registeredUIDs[i].length() == 0) {
//       registeredUIDs[i] = uid;
      
//       // Assign sensor
//       if (sensorIndex >= 0 && sensorIndex < NUM_SENSORS) {
//         sensorAssignments[i] = sensorIndex;
//       } else {
//         // Find available sensor
//         for (int s = 0; s < NUM_SENSORS; s++) {
//           bool sensorUsed = false;
//           for (int j = 0; j < MAX_REGISTERED_BOOKS; j++) {
//             if (sensorAssignments[j] == s) {
//               sensorUsed = true;
//               break;
//             }
//           }
//           if (!sensorUsed) {
//             sensorAssignments[i] = s;
//             break;
//           }
//         }
//       }
      
//       // Save to EEPROM
//       int addr = REGISTERED_BOOKS_START + i * (UID_LENGTH + 1);
//       eepromWriteString(addr, uid, UID_LENGTH);
//       return;
//     }
//   }
// }

// void loadRegisteredBooks() {
//   Serial.println("Loading registered books from EEPROM...");
//   int bookCount = 0;
  
//   // Initialize arrays
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     registeredUIDs[i] = "";
//     sensorAssignments[i] = 255; // 255 means "not assigned yet"
//   }
  
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     int addr = REGISTERED_BOOKS_START + i * (UID_LENGTH + 1);
//     String uid = eepromReadString(addr, UID_LENGTH);
    
//     // Only accept valid 8-character hex UIDs
//     if (uid.length() == UID_LENGTH) {
//       bool validHex = true;
//       for (int j = 0; j < uid.length(); j++) {
//         char c = uid.charAt(j);
//         if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) {
//           validHex = false;
//           break;
//         }
//       }
      
//       if (validHex) {
//         registeredUIDs[bookCount] = uid;
//         sensorAssignments[bookCount] = 255; // Will be assigned when scanned
//         bookCount++;
//       }
//     }
//   }
  
//   if (bookCount == 0) {
//     Serial.println("No registered books found in EEPROM");
//   } else {
//     Serial.print("Loaded ");
//     Serial.print(bookCount);
//     Serial.println(" registered books (sensors not assigned yet)");
//   }
// }

// // ==================== SENSOR FUNCTIONS - FIXED ====================



// // ==================== SENSOR FUNCTIONS - REPAIRED ====================
// String readSensor(int pin) {
//   static String lastState[NUM_SENSORS];               // Keep last stable state
//   static unsigned long lastChangeTime[NUM_SENSORS];  // For debouncing

//   int lowCount = 0;

//   // Sample the sensor multiple times quickly
//   for (int i = 0; i < 50; i++) {
//     if (digitalRead(pin) == LOW) lowCount++;  // LOW = sensor active (book present)
//     delayMicroseconds(400);
//   }

//   String currentState;

//   // Decide current state based on LOW count
//   if (lowCount > 40) {
//     // Mostly LOW → book present → AVL
//     currentState = "AVL";
//   } else if (lowCount < 10) {
//     // Mostly HIGH → book removed → BRW
//     currentState = "BRW";
//   } else {
//     // Unstable readings → keep last stable state
//     int sensorIndex = -1;
//     for (int i = 0; i < NUM_SENSORS; i++) {
//       if (sensorPins[i] == pin) {
//         sensorIndex = i;
//         break;
//       }
//     }
//     if (sensorIndex != -1) {
//       currentState = lastState[sensorIndex];
//     } else {
//       currentState = "BRW"; // fallback if sensor not found
//     }
//   }

//   // Update last stable state
//   for (int i = 0; i < NUM_SENSORS; i++) {
//     if (sensorPins[i] == pin) {
//       lastState[i] = currentState;
//       break;
//     }
//   }

//   return currentState;
// }


// // String readSensor(int pin) {
// //   int lowCount = 0;
// //   for (int i = 0; i < 10; i++) {
// //     if (digitalRead(pin) == LOW) lowCount++;
// //     delay(1);
// //   }
  
// //   // FIXED: Return "BRW" when LOW count >= 7 (book borrowed/not present)
// //   // Return "AVL" when LOW count < 7 (book available/present)
// //   // Your logs show sensor is working correctly - LOW = book borrowed, HIGH = book available
// //   return (lowCount >= 7) ? "BRW" : "AVL";
// // }

// // ==================== FIND SENSOR WITHOUT PRINTING - NEW FUNCTION ====================
// int getSensorIndexForBook(const String &uid) {
//   // Check if book already has a sensor assigned
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     if (registeredUIDs[i] == uid) {
//       // If sensor is already assigned (0-14), return it
//       if (sensorAssignments[i] >= 0 && sensorAssignments[i] < NUM_SENSORS) {
//         return sensorAssignments[i];
//       }
//       // If sensor is 255 (not assigned), break and assign new one
//       break;
//     }
//   }
//   return -1; // No sensor assigned yet
// }

// int findSensorForBook(const String &uid) {
//   // FIRST: Check if this book already has a sensor assigned
//   int existingSensor = getSensorIndexForBook(uid);
//   if (existingSensor != -1) {
//     return existingSensor;
//   }
  
//   // SECOND: Book needs a sensor - find first UNUSED sensor
//   for (int sensorNum = 0; sensorNum < NUM_SENSORS; sensorNum++) {
//     bool sensorInUse = false;
    
//     // Check if any book is already using this sensor
//     for (int j = 0; j < MAX_REGISTERED_BOOKS; j++) {
//       if (sensorAssignments[j] == sensorNum) {
//         sensorInUse = true;
//         break;
//       }
//     }
    
//     // Found an available sensor!
//     if (!sensorInUse) {
//       // Find empty slot to store this book
//       for (int j = 0; j < MAX_REGISTERED_BOOKS; j++) {
//         if (registeredUIDs[j].length() == 0) {
//           registeredUIDs[j] = uid;
//           sensorAssignments[j] = sensorNum;
          
//           // Save to EEPROM
//           int addr = REGISTERED_BOOKS_START + j * (UID_LENGTH + 1);
//           eepromWriteString(addr, uid, UID_LENGTH);
          
//           Serial.print("ASSIGNED: Book ");
//           Serial.print(uid);
//           Serial.print(" → Sensor ");
//           Serial.print(sensorNum);
//           Serial.print(" (Pin ");
//           Serial.print(sensorPins[sensorNum]);
//           Serial.println(")");
//           return sensorNum;
//         }
//       }
//     }
//   }
  
//   // If we get here, all sensors are in use
//   Serial.println("WARNING: All sensors in use! Using sensor 0 as fallback");
//   return 0;
// }



// String getBookStatus(const String &uid) {
//   // Get assigned sensor index for this book
//   int sensorIndex = getSensorIndexForBook(uid);

//   if (sensorIndex == -1) {
//     // No sensor assigned yet → assume book present by default
//     Serial.print("BOOK ");
//     Serial.print(uid);
//     Serial.println(" → No sensor assigned yet, defaulting to AVL");
//     return "AVL";
//   }

//   if (sensorIndex >= 0 && sensorIndex < NUM_SENSORS) {
//     // Read the sensor
//     String status = readSensor(sensorPins[sensorIndex]);

//     // Print debug info: sensor index, pin, and status
//     Serial.print("BOOK ");
//     Serial.print(uid);
//     Serial.print(" → SENSOR INDEX: ");
//     Serial.print(sensorIndex);
//     Serial.print(", PIN: ");
//     Serial.print(sensorPins[sensorIndex]);
//     Serial.print(", STATUS: ");
//     Serial.println(status);

//     return status;
//   }

//   // Fallback if sensor index is invalid
//   Serial.print("BOOK ");
//   Serial.print(uid);
//   Serial.println(" → Invalid sensor index, defaulting to AVL");
//   return "AVL";
// }


// // String getBookStatus(const String &uid) {
// //   // FIXED: Use getSensorIndexForBook instead of findSensorForBook
// //   // This prevents duplicate prints and unwanted sensor assignments
// //   int sensorIndex = getSensorIndexForBook(uid);
// //   if (sensorIndex == -1) {
// //     // No sensor assigned yet, return default
// //     return "AVL";
// //   }
  
// //   if (sensorIndex >= 0 && sensorIndex < NUM_SENSORS) {
// //     return readSensor(sensorPins[sensorIndex]);
// //   }
// //   return "AVL";
// // }

// // ==================== SERIAL COMMUNICATION ====================
// void sendToESP(const String &msg) {
//   // Clear any leftover data in serial buffer
//   while (Serial1.available()) Serial1.read();
//   delay(10);
  
//   // Send the complete message
//   Serial1.println(msg);
  
//   // Debug output
//   Serial.print("TO_ESP: ");
  
//   if (msg.startsWith("SEND:")) {
//     // For SEND messages, show compact version
//     int pipePos = msg.indexOf('|');
//     if (pipePos != -1) {
//       Serial.print(msg.substring(0, pipePos + 1));
//       Serial.print("... [JSON length: ");
//       Serial.print(msg.length() - pipePos - 1);
//       Serial.println("]");
//     } else {
//       // No pipe - show first part
//       int showLen = min(msg.length(), 60);
//       Serial.println(msg.substring(0, showLen));
//     }
//   } else {
//     // For other messages (PING, GET, etc.)
//     Serial.println(msg);
//   }
  
//   // Extra delay for long messages
//   if (msg.length() > 200) {
//     delay(50);
//   }
// }

// bool waitForResponse(String &response, unsigned long timeout) {
//   unsigned long start = millis();
//   response = "";
  
//   while (millis() - start < timeout) {
//     while (Serial1.available()) {
//       char c = Serial1.read();
//       if (c == '\n') {
//         response.trim();
//         if (response.length() > 0) {
//           if (!response.startsWith("DEBUG:") && !response.startsWith("WIFI:")) {
//             Serial.print("FROM_ESP: ");
//             Serial.println(response);
//           }
//           return true;
//         }
//       } else if (c != '\r' && response.length() < 2048) {
//         response += c;
//       }
//     }
//     delay(1);
//   }
//   return false;
// }

// // ==================== PENDING QUEUE ====================
// int pendingIndexAddr(int index) { 
//   return PENDING_START + index * PENDING_ENTRY_SIZE; 
// }

// bool addPendingEntry(const String &payload) {
//   if (!payload.startsWith("SEND:")) {
//     return false;
//   }
  
//   // Validate the JSON part
//   int pipePos = payload.indexOf('|');
//   if (pipePos != -1) {
//     String jsonPart = payload.substring(pipePos + 1);
//     if (!isValidJSON(jsonPart)) {
//       return false;
//     }
//   }
  
//   for (int i = 0; i < PENDING_MAX; i++) {
//     int addr = pendingIndexAddr(i);
//     if (EEPROM.read(addr) == 0xFF) {
//       eepromWriteString(addr, payload, PENDING_ENTRY_SIZE - 1);
//       Serial.print("QUEUED: ");
      
//       // Show first 60 chars
//       int showLen = min(payload.length(), 60);
//       Serial.println(payload.substring(0, showLen));
      
//       return true;
//     }
//   }
//   Serial.println("QUEUE FULL!");
//   return false;
// }

// bool popPendingEntry(String &outPayload) {
//   for (int i = 0; i < PENDING_MAX; i++) {
//     int addr = pendingIndexAddr(i);
//     if (EEPROM.read(addr) != 0xFF && EEPROM.read(addr) != 0x00) {
//       outPayload = eepromReadString(addr, PENDING_ENTRY_SIZE - 1);
      
//       // Clear slot
//       for (int j = 0; j < PENDING_ENTRY_SIZE; j++) {
//         EEPROM.write(addr + j, 0xFF);
//       }
      
//       // VALIDATE: Must start with "SEND:" and contain valid JSON
//       if (outPayload.length() > 10 && 
//           outPayload.startsWith("SEND:") && 
//           outPayload.indexOf('|') > 5) {
        
//         // Validate JSON part
//         int pipePos = outPayload.indexOf('|');
//         String jsonPart = outPayload.substring(pipePos + 1);
//         if (isValidJSON(jsonPart)) {
//           return true;
//         }
//       }
//     }
//   }
//   return false;
// }

// void flushPendingQueue() {
//   if (!wifiConnected) return;
  
//   String payload;
//   int flushed = 0;
  
//   while (popPendingEntry(payload)) {
//     if (payload.length() > 10) {
//       // Skip malformed entries
//       if (!payload.startsWith("SEND:")) {
//         continue;
//       }
      
//       Serial.print("FLUSHING: ");
//       Serial.println(payload.substring(0, 60));
//       sendToESP(payload);
      
//       String response;
//       if (waitForResponse(response, REQUEST_TIMEOUT)) {
//         if (response.startsWith("SUCCESS:")) {
//           flushed++;
//           delay(100);
//         } else {
//           addPendingEntry(payload);
//           break;
//         }
//       } else {
//         addPendingEntry(payload);
//         break;
//       }
//     }
//   }
  
//   if (flushed > 0) {
//     Serial.print("Successfully flushed ");
//     Serial.print(flushed);
//     Serial.println(" queued entries");
//   }
// }

// // ==================== RFID ====================
// String readRFID() {
//   if (!mfrc522.PICC_IsNewCardPresent()) return "";
//   if (!mfrc522.PICC_ReadCardSerial()) return "";
  
//   String uid = "";
//   for (byte i = 0; i < mfrc522.uid.size; i++) {
//     if (mfrc522.uid.uidByte[i] < 0x10) uid += "0";
//     uid += String(mfrc522.uid.uidByte[i], HEX);
//   }
//   uid.toUpperCase();
//   mfrc522.PICC_HaltA();
//   return uid;
// }

// // ==================== MAIN PROCESSING - FIXED ====================
// void processCard(const String &cardUID) {
//   // Debounce
//   if (cardUID == lastCardUID && millis() - lastCardTime < DEBOUNCE_TIME) {
//     return;
//   }
  
//   lastCardUID = cardUID;
//   lastCardTime = millis();
  
//   // FIXED: Get sensor index first (without printing)
//   int sensorIndex = findSensorForBook(cardUID);
  
//   // FIXED: Get sensor status using the known sensor index
//   String sensorStatus = "AVL";
//   if (sensorIndex >= 0 && sensorIndex < NUM_SENSORS) {
//     sensorStatus = readSensor(sensorPins[sensorIndex]);
//   }
  
//   Serial.println("=================================");
//   Serial.print("CARD: ");
//   Serial.println(cardUID);
//   Serial.print("SENSOR INDEX: ");
//   Serial.println(sensorIndex);
//   Serial.print("SENSOR STATUS: ");
//   Serial.println(sensorStatus);
//   Serial.println("=================================");
  
//   // Try to get data (online or from cache)
//   bool serverIsRegistered = false;
//   String serverTitle = "";
//   String serverAuthor = "";
//   String serverLocation = "";
//   String serverEspStatus = "AVL";
//   int serverSensor = 0;
//   bool usingCachedData = false;
  
//   if (wifiConnected) {
//     // Try to get from Firebase
//     sendToESP("GET:" + cardUID);
//     String response;
//     if (waitForResponse(response, REQUEST_TIMEOUT)) {
//       if (response.startsWith("BOOKDATA:")) {
//         int p = response.indexOf('|');
//         if (p != -1) {
//           String serverData = response.substring(p + 1);
          
//           if (serverData != "null") {
//             DynamicJsonDocument doc(1024);
//             DeserializationError err = deserializeJson(doc, serverData);
//             if (!err) {
//               serverIsRegistered = doc["isRegistered"] | false;
//               serverTitle = String(doc["title"] | "");
//               serverAuthor = String(doc["author"] | "");
//               serverLocation = String(doc["location"] | "");
//               serverSensor = doc["sensor"] | 0;
              
//               if (doc.containsKey("espStatus")) {
//                 serverEspStatus = String(doc["espStatus"] | "AVL");
//               }
              
//               Serial.print("FIREBASE: Registered=");
//               Serial.print(serverIsRegistered ? "YES" : "NO");
//               Serial.print(", Status=");
//               Serial.println(serverEspStatus);
              
//               // Cache this data for offline use
//               cacheBookData(cardUID, serverData);
              
//               // Save locally if registered
//               if (serverIsRegistered && !isBookRegistered(cardUID)) {
//                 saveRegisteredBook(cardUID, serverSensor);
//               }
//             }
//           } else {
//             Serial.println("Book not found in Firebase");
//           }
//         }
//       }
//     } else {
//       wifiConnected = false; // Mark as offline
//     }
//   }
  
//   // If offline or GET failed, check cache
//   if (!wifiConnected || serverTitle.length() == 0) {
//     String cachedData = getCachedBookData(cardUID);
//     if (cachedData.length() > 0) {
//       DynamicJsonDocument doc(1024);
//       DeserializationError err = deserializeJson(doc, cachedData);
//       if (!err) {
//         serverIsRegistered = doc["isRegistered"] | false;
//         serverTitle = String(doc["title"] | "");
//         serverAuthor = String(doc["author"] | "");
//         serverLocation = String(doc["location"] | "");
//         serverSensor = doc["sensor"] | 0;
        
//         if (doc.containsKey("espStatus")) {
//           serverEspStatus = String(doc["espStatus"] | "AVL");
//         }
        
//         usingCachedData = true;
//         Serial.println("USING CACHED DATA");
//       }
//     }
//   }
  
//   // If still no data, use defaults
//   if (serverTitle.length() == 0) {
//     if (isBookRegistered(cardUID)) {
//       serverTitle = "Book Title Unknown";
//       serverAuthor = "Author Unknown";
//       serverLocation = "Location Unknown";
//       serverIsRegistered = true;
//       Serial.println("Using local registration (no cache)");
//     } else {
//       serverTitle = "New Book";
//       serverAuthor = "Unknown";
//       serverLocation = "Unassigned";
//       serverIsRegistered = false;
//       Serial.println("New unregistered book");
//     }
//   }
  
//   // Create JSON
//   DynamicJsonDocument doc(1024);
  
//   doc["title"] = serverTitle;
//   doc["author"] = serverAuthor;
//   doc["location"] = serverLocation;
//   doc["isRegistered"] = serverIsRegistered;
//   doc["lastSeen"] = String(millis() / 1000);
//   doc["sensor"] = sensorIndex;
//   doc["espStatus"] = sensorStatus;
  
//   if (sensorStatus == "AVL") doc["status"] = "Available";
//   else if (sensorStatus == "BRW") doc["status"] = "Borrowed";
//   else if (sensorStatus == "OVD") doc["status"] = "Overdue";
//   else if (sensorStatus == "MIS") doc["status"] = "Misplaced";
//   else doc["status"] = "Available";
  
//   doc["lastUpdated"] = String(millis() / 1000);
  
//   if (usingCachedData) {
//     doc["source"] = "cached";
//   }
  
//   String outJson;
//   serializeJson(doc, outJson);
  
//   // Validate JSON
//   Serial.print("JSON SIZE: ");
//   Serial.println(outJson.length());
  
//   if (!outJson.endsWith("}")) {
//     outJson += "}";
//   }
  
//   // Step 3: Send to Firebase or queue
//   String sendCmd = "SEND:/books/" + cardUID + "|" + outJson;
  
//   if (wifiConnected) {
//     sendToESP(sendCmd);
    
//     String response;
//     if (waitForResponse(response, REQUEST_TIMEOUT)) {
//       if (response.startsWith("SUCCESS:")) {
//         // Update cache with new data
//         cacheBookData(cardUID, outJson);
        
//         // Save registration if not already saved
//         if (serverIsRegistered && !isBookRegistered(cardUID)) {
//           saveRegisteredBook(cardUID, sensorIndex);
//         }
//         Serial.println("Firebase update OK");
//       } else {
//         addPendingEntry(sendCmd);
//       }
//     } else {
//       addPendingEntry(sendCmd);
//     }
//   } else {
//     addPendingEntry(sendCmd);
//     Serial.println("WiFi offline - queued");
//   }
// }

// // ==================== SETUP ====================
// void setup() {
//   Serial.begin(115200);
//   Serial1.begin(115200);
  
//   // Clear initial garbage
//   delay(2000);
//   while (Serial1.available()) Serial1.read();
  
//   Serial.println();
//   Serial.println("LIBRARY SYSTEM - MEGA 2560 WITH OFFLINE CACHE");
//   Serial.println("==============================================");
  
//   // Initialize sensor pins
//   for (int i = 0; i < NUM_SENSORS; i++) {
//     pinMode(sensorPins[i], INPUT_PULLUP);
//   }
  
//   // Check for EEPROM corruption
//   if (isEEPROMCorrupted()) {
//     Serial.println("EEPROM corrupted - clearing all data");
//     clearCorruptedEEPROM();
//   }
  
//   // Load registered books
//   loadRegisteredBooks();
  
//   // Initialize RFID
//   SPI.begin();
//   mfrc522.PCD_Init();
//   delay(200);
  
//   // Test ESP connection
//   Serial.println("Testing ESP connection...");
//   wifiConnected = false;
  
//   for (int i = 0; i < 3; i++) {
//     sendToESP("PING");
//     String response;
//     if (waitForResponse(response, 2000)) {
//       if (response == "PONG") {
//         wifiConnected = true;
//         Serial.println("ESP connected");
//         break;
//       }
//     }
//     delay(1000);
//   }
  
//   if (!wifiConnected) {
//     Serial.println("ESP not responding - OFFLINE MODE");
//   } else {
//     flushPendingQueue();
//   }
  
//   Serial.println("System Ready - Sensors will be assigned sequentially");
// }

// // ==================== LOOP ====================
// void loop() {
//   // Read RFID
//   String uid = readRFID();
//   if (uid.length() == 8) {
//     processCard(uid);
//   }
  
//   // Handle ESP messages
//   static String espBuffer = "";
//   while (Serial1.available()) {
//     char c = Serial1.read();
//     if (c == '\n') {
//       espBuffer.trim();
//       if (espBuffer.length() > 0) {
//         if (espBuffer.startsWith("WIFI:")) {
//           if (espBuffer == "WIFI:OK") {
//             if (!wifiConnected) {
//               wifiConnected = true;
//               Serial.println("WiFi connected - FLUSHING QUEUE");
//               flushPendingQueue();
//             }
//           } else if (espBuffer == "WIFI:OFFLINE") {
//             wifiConnected = false;
//             Serial.println("WiFi offline - CACHE MODE");
//           }
//         } else if (espBuffer == "PONG") {
//           wifiConnected = true;
//         }
//       }
//       espBuffer = "";
//     } else if (c != '\r') {
//       espBuffer += c;
//     }
//   }
  
//   // Periodic ping
//   static unsigned long lastPing = 0;
//   if (millis() - lastPing > PING_INTERVAL) {
//     lastPing = millis();
//     sendToESP("PING");
//     String response;
//     if (waitForResponse(response, 3000)) {
//       if (response == "PONG" && !wifiConnected) {
//         wifiConnected = true;
//         Serial.println("WiFi restored - FLUSHING QUEUE");
//         flushPendingQueue();
//       }
//     } else if (wifiConnected) {
//       wifiConnected = false;
//       Serial.println("WiFi lost - CACHE MODE");
//     }
//   }
  
//   delay(10);
// }
// // // //=========================================================================================================================
// // // //==========================================================================================================================
// // // //==========================================================================================================================
// // // //==========================================================================================================================
// // // //==========================================================================================================================
// // // //==========================================================================================================================
// // // //==========================================================================================================================
// // // //==========================================================================================================================
// // // //=========================================================================================================================
// // // //==========================================================================================================================
// // // //==========================================================================================================================
// // // //==========================================================================================================================
// // // //==========================================================================================================================
// // // //==========================================================================================================================
// // // //==========================================================================================================================
// // // //==============POOPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPOWER============================================================================================================
// // // //===============ppppppppppppppppppppppppppppppower===========================================================================================================

// #include <Arduino.h>
// #include <SPI.h>
// #include <MFRC522.h>
// #include <EEPROM.h>
// #include <ArduinoJson.h>
// #include <Wire.h>
// #include <LiquidCrystal_I2C.h>

// // ==================== CONFIG ====================
// #define RC522_SS_PIN 53
// #define RC522_RST_PIN 49
// #define BUZZER_PIN 48  // Buzzer pin

// // LCD Configuration
// #define LCD_COLS 20
// #define LCD_ROWS 4
// LiquidCrystal_I2C lcd(0x27, LCD_COLS, LCD_ROWS);

// // EEPROM Configuration
// #define REGISTERED_BOOKS_START 0
// #define MAX_REGISTERED_BOOKS 50
// #define UID_LENGTH 8

// #define BOOK_CACHE_START (REGISTERED_BOOKS_START + MAX_REGISTERED_BOOKS * (UID_LENGTH + 1))
// #define BOOK_CACHE_SIZE 512
// #define MAX_CACHED_BOOKS 20

// #define PENDING_START (BOOK_CACHE_START + MAX_CACHED_BOOKS * BOOK_CACHE_SIZE)
// #define PENDING_ENTRY_SIZE 768
// #define PENDING_MAX 8

// // Timing Configuration
// #define DEBOUNCE_TIME 3000UL
// #define REQUEST_TIMEOUT 8000UL
// #define PING_INTERVAL 30000UL
// #define LCD_UPDATE_INTERVAL 1000UL
// #define WELCOME_ANIMATION_TIME 3000UL

// // Display timing
// #define BOOK_TITLE_DISPLAY_TIME 3000UL
// #define BOOK_STATUS_DISPLAY_TIME 3000UL
// #define NETWORK_STATUS_DISPLAY_TIME 3000UL
// #define ERROR_DISPLAY_TIME 2000UL
// #define SCANNING_DISPLAY_TIME 1000UL
// #define IDLE_RETURN_TIMEOUT 10000UL  // Return to idle after 10 seconds inactivity

// // Sensor Configuration
// #define NUM_SENSORS 15

// const byte sensorPins[NUM_SENSORS] = {22,23,24,25,26,27,28,29,30,31,32,33,34,35,36};

// // ==================== GLOBALS ====================
// MFRC522 mfrc522(RC522_SS_PIN, RC522_RST_PIN);
// bool wifiConnected = false;
// String lastCardUID = "";
// unsigned long lastCardTime = 0;
// String registeredUIDs[MAX_REGISTERED_BOOKS];
// byte sensorAssignments[MAX_REGISTERED_BOOKS];

// // LCD Display States
// enum DisplayState {
//   WELCOME,
//   IDLE,
//   SCANNING,
//   BOOK_TITLE,
//   BOOK_STATUS,
//   NETWORK_STATUS,
//   ERROR_STATE
// };

// DisplayState currentDisplayState = WELCOME;
// DisplayState previousDisplayState = WELCOME;
// unsigned long lastStateChange = 0;
// unsigned long lastLCDUpdate = 0;
// unsigned long welcomeStartTime = 0;
// unsigned long lastActivityTime = 0;

// String currentBookTitle = "";
// String currentBookAuthor = "";
// String currentBookLocation = "";
// String currentBookStatus = "";
// bool currentBookRegistered = false;
// int currentBookSensor = 0;

// // ==================== FUNCTION PROTOTYPES ====================
// void changeDisplayState(DisplayState newState);
// void lcdPrintCenter(int row, String text);
// void clearLCDLine(int row);
// void clearLCDScreen();
// void playTone(int frequency, int duration);
// void displayWelcomeAnimation();
// void displayIdleScreen();
// void displayScanningScreen(const String &uid);
// void displayBookTitleScreen();
// void displayBookStatusScreen();
// void displayNetworkStatus();
// void displayError(const String &error);
// void updateDisplay();
// String eepromReadString(int addr, int maxLen);
// void eepromWriteString(int addr, const String &str, int maxLen);
// bool isValidJSON(const String &json);
// bool isEEPROMCorrupted();
// void clearCorruptedEEPROM();
// int getBookCacheAddr(const String &uid);
// void cacheBookData(const String &uid, const String &jsonData);
// String getCachedBookData(const String &uid);
// bool isBookRegistered(const String &uid);
// int getBookIndex(const String &uid);
// void saveRegisteredBook(const String &uid, int sensorIndex);
// void loadRegisteredBooks();
// String readSensor(int pin);
// int getSensorIndexForBook(const String &uid);
// int findSensorForBook(const String &uid);
// String getBookStatus(const String &uid);
// void sendToESP(const String &msg);
// bool waitForResponse(String &response, unsigned long timeout);
// int pendingIndexAddr(int index);
// bool addPendingEntry(const String &payload);
// bool popPendingEntry(String &outPayload);
// void flushPendingQueue();
// String readRFID();
// void processCard(const String &cardUID);

// // ==================== LCD FUNCTIONS ====================

// void lcdPrintCenter(int row, String text) {
//   text.trim();
  
//   if (text.length() > LCD_COLS) {
//     text = text.substring(0, LCD_COLS);
//   }
  
//   int col = max(0, (LCD_COLS - text.length()) / 2);
  
//   // Clear the line first
//   lcd.setCursor(0, row);
//   for (int i = 0; i < LCD_COLS; i++) {
//     lcd.print(" ");
//   }
  
//   // Print at calculated position
//   lcd.setCursor(col, row);
//   lcd.print(text);
// }

// void clearLCDLine(int row) {
//   lcd.setCursor(0, row);
//   for (int i = 0; i < LCD_COLS; i++) {
//     lcd.print(" ");
//   }
// }

// void clearLCDScreen() {
//   for (int row = 0; row < LCD_ROWS; row++) {
//     clearLCDLine(row);
//   }
// }

// void playTone(int frequency, int duration) {
//   if (BUZZER_PIN > 0) {
//     tone(BUZZER_PIN, frequency, duration);
//   }
// }

// // ==================== DISPLAY STATE FUNCTIONS ====================

// void displayWelcomeAnimation() {
//   unsigned long elapsed = millis() - welcomeStartTime;
  
//   if (elapsed < 500) {
//     String line1 = "LEEJINBOTICS";
//     int showChars = map(elapsed, 0, 500, 0, line1.length());
//     lcd.setCursor((LCD_COLS - showChars) / 2, 0);
//     for (int i = 0; i < showChars; i++) {
//       lcd.print(line1[i]);
//     }
//   } 
//   else if (elapsed < 1000) {
//     String line2 = "LIBRARY SYSTEM";
//     int showChars = map(elapsed, 500, 1000, 0, line2.length());
//     lcd.setCursor((LCD_COLS - showChars) / 2, 1);
//     for (int i = 0; i < showChars; i++) {
//       lcd.print(line2[i]);
//     }
//     lcdPrintCenter(0, "LEEJINBOTICS");
//   }
//   else if (elapsed < 1500) {
//     String line3 = "v2.0 PRO";
//     int showChars = map(elapsed, 1000, 1500, 0, line3.length());
//     lcd.setCursor((LCD_COLS - showChars) / 2, 2);
//     for (int i = 0; i < showChars; i++) {
//       lcd.print(line3[i]);
//     }
//     lcdPrintCenter(0, "LEEJINBOTICS");
//     lcdPrintCenter(1, "LIBRARY SYSTEM");
//   }
//   else if (elapsed < 2000) {
//     lcdPrintCenter(0, "LEEJINBOTICS");
//     lcdPrintCenter(1, "LIBRARY SYSTEM");
//     lcdPrintCenter(2, "v2.0 PRO");
    
//     int dots = ((elapsed - 1500) / 200) % 4;
//     String loading = "INITIALIZING";
//     for (int i = 0; i < dots; i++) loading += ".";
//     lcdPrintCenter(3, loading);
    
//     if (elapsed < 1600 && elapsed > 1550) {
//       playTone(1000, 100);
//     }
//   }
//   else if (elapsed < 2500) {
//     lcdPrintCenter(0, "LEEJINBOTICS");
//     lcdPrintCenter(1, "LIBRARY SYSTEM");
//     lcdPrintCenter(2, "v2.0 PRO");
//     lcdPrintCenter(3, "READY");
//   }
//   else {
//     changeDisplayState(IDLE);
//   }
// }

// void displayIdleScreen() {
//   static unsigned long lastScroll = 0;
  
//   if (millis() - lastScroll > 500) {
//     lastScroll = millis();
    
//     // Line 1: System title
//     lcdPrintCenter(0, "LIBRARY SYSTEM");
    
//     // Line 2: Scan prompt with dots
//     String scanMsg = "SCAN CARD";
//     int dots = (millis() / 500) % 4;
//     for (int i = 0; i < dots; i++) scanMsg += ".";
//     lcdPrintCenter(1, scanMsg);
    
//     // Line 3: Registered books count and WiFi
//     int registeredCount = 0;
//     for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//       if (registeredUIDs[i].length() > 0) registeredCount++;
//     }
//     String wifiStatus = wifiConnected ? "WiFi:ON" : "WiFi:OFF";
//     String line3 = "REG:" + String(registeredCount) + " " + wifiStatus;
//     lcdPrintCenter(2, line3);
    
//     // Line 4: Sensors and Queue
//     int usedSensors = 0;
//     for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//       if (sensorAssignments[i] < NUM_SENSORS) usedSensors++;
//     }
//     int queueCount = 0;
//     for (int i = 0; i < PENDING_MAX; i++) {
//       int addr = PENDING_START + i * PENDING_ENTRY_SIZE;
//       if (EEPROM.read(addr) != 0xFF) queueCount++;
//     }
//     String line4 = "SENS:" + String(usedSensors) + "/" + String(NUM_SENSORS) + 
//                    " QUE:" + String(queueCount);
//     lcdPrintCenter(3, line4);
//   }
// }

// void displayScanningScreen(const String &uid) {
//   lcdPrintCenter(0, "SCANNING CARD");
  
//   String uidDisplay = "UID:" + uid.substring(0, 8);
//   lcdPrintCenter(1, uidDisplay);
  
//   static int animPos = 0;
//   String animChars[] = {"|", "/", "-", "\\"};
//   String processing = "PROCESSING " + animChars[animPos % 4];
//   lcdPrintCenter(2, processing);
//   animPos++;
  
//   String statusLine = wifiConnected ? "ONLINE" : "OFFLINE";
//   if (!wifiConnected) statusLine += " CACHE";
//   lcdPrintCenter(3, statusLine);
// }

// void displayBookTitleScreen() {
//   // Clear screen when first entering this state
//   if (previousDisplayState != BOOK_TITLE) {
//     clearLCDScreen();
//   }
  
//   // Line 1: Book Title (truncated if needed)
//   String titleDisplay = currentBookTitle;
//   if (titleDisplay.length() > LCD_COLS) {
//     titleDisplay = titleDisplay.substring(0, LCD_COLS - 2) + "..";
//   }
//   lcdPrintCenter(0, titleDisplay);
  
//   // Line 2: Author
//   String authorDisplay = "Author: " + currentBookAuthor;
//   if (authorDisplay.length() > LCD_COLS) {
//     authorDisplay = authorDisplay.substring(0, LCD_COLS - 2) + "..";
//   }
//   lcdPrintCenter(1, authorDisplay);
  
//   // Line 3: Location
//   String locDisplay = "Location: " + currentBookLocation;
//   if (locDisplay.length() > LCD_COLS) {
//     locDisplay = locDisplay.substring(0, LCD_COLS - 2) + "..";
//   }
//   lcdPrintCenter(2, locDisplay);
  
//   // Line 4: Registration status
//   String regStatus = currentBookRegistered ? "REGISTERED" : "NOT REGISTERED";
//   lcdPrintCenter(3, regStatus);
// }

// void displayBookStatusScreen() {
//   // Clear screen when first entering this state
//   if (previousDisplayState != BOOK_STATUS) {
//     clearLCDScreen();
//   }
  
//   // Line 1: Status header
//   lcdPrintCenter(0, "BOOK STATUS");
  
//   // Line 2: Status value
//   String statusDisplay = "Status: " + currentBookStatus;
//   lcdPrintCenter(1, statusDisplay);
  
//   // Line 3: Sensor information
//   String sensorDisplay = "Sensor: S" + String(currentBookSensor);
//   lcdPrintCenter(2, sensorDisplay);
  
//   // Line 4: Network status
//   String networkDisplay = wifiConnected ? "ONLINE SYNC" : "OFFLINE MODE";
//   lcdPrintCenter(3, networkDisplay);
// }

// void displayNetworkStatus() {
//   lcdPrintCenter(0, "NETWORK STATUS");
  
//   String wifiStatus = wifiConnected ? "WiFi: CONNECTED" : "WiFi: OFFLINE";
//   lcdPrintCenter(1, wifiStatus);
  
//   int queueCount = 0;
//   for (int i = 0; i < PENDING_MAX; i++) {
//     int addr = PENDING_START + i * PENDING_ENTRY_SIZE;
//     if (EEPROM.read(addr) != 0xFF) queueCount++;
//   }
//   String queueStatus = "QUEUE: " + String(queueCount);
//   lcdPrintCenter(2, queueStatus);
  
//   if (wifiConnected) {
//     lcdPrintCenter(3, "SYNC ACTIVE");
//   } else {
//     lcdPrintCenter(3, "CACHE MODE");
//   }
// }

// void displayError(const String &error) {
//   clearLCDScreen();
//   lcdPrintCenter(0, "ERROR");
  
//   String errorMsg = error;
//   if (errorMsg.length() > LCD_COLS) {
//     errorMsg = errorMsg.substring(0, LCD_COLS);
//   }
//   lcdPrintCenter(1, errorMsg);
  
//   lcdPrintCenter(2, "PLEASE RETRY");
//   lcdPrintCenter(3, "Returning...");
  
//   playTone(300, 300);
//   delay(100);
//   playTone(200, 300);
// }

// // ==================== DISPLAY STATE MANAGEMENT ====================

// void changeDisplayState(DisplayState newState) {
//   if (currentDisplayState != newState) {
//     previousDisplayState = currentDisplayState;
//     currentDisplayState = newState;
//     lastStateChange = millis();
    
//     // Clear screen for most state transitions (except IDLE which handles its own updates)
//     if (newState != IDLE) {
//       clearLCDScreen();
//     }
    
//     // Record activity for timeout
//     if (newState != IDLE) {
//       lastActivityTime = millis();
//     }
    
//     Serial.print("Display State Changed: ");
//     Serial.print(previousDisplayState);
//     Serial.print(" -> ");
//     Serial.println(newState);
//   }
// }

// void updateDisplay() {
//   unsigned long now = millis();
  
//   // Check for timeout to return to idle
//   if (currentDisplayState != IDLE && currentDisplayState != WELCOME) {
//     if (now - lastActivityTime > IDLE_RETURN_TIMEOUT) {
//       changeDisplayState(IDLE);
//       return;
//     }
//   }
  
//   // Handle state-specific display logic
//   switch (currentDisplayState) {
//     case WELCOME:
//       displayWelcomeAnimation();
//       break;
      
//     case IDLE:
//       displayIdleScreen();
//       break;
      
//     case SCANNING:
//       displayScanningScreen(lastCardUID);
//       // After scanning time, move to book title
//       if (now - lastStateChange > SCANNING_DISPLAY_TIME) {
//         changeDisplayState(BOOK_TITLE);
//       }
//       break;
      
//     case BOOK_TITLE:
//       displayBookTitleScreen();
//       // After display time, move to book status
//       if (now - lastStateChange > BOOK_TITLE_DISPLAY_TIME) {
//         changeDisplayState(BOOK_STATUS);
//       }
//       break;
      
//     case BOOK_STATUS:
//       displayBookStatusScreen();
//       // After display time, return to idle
//       if (now - lastStateChange > BOOK_STATUS_DISPLAY_TIME) {
//         changeDisplayState(IDLE);
//       }
//       break;
      
//     case NETWORK_STATUS:
//       displayNetworkStatus();
//       // After display time, return to idle
//       if (now - lastStateChange > NETWORK_STATUS_DISPLAY_TIME) {
//         changeDisplayState(IDLE);
//       }
//       break;
      
//     case ERROR_STATE:
//       // Error state handles its own display
//       if (now - lastStateChange > ERROR_DISPLAY_TIME) {
//         changeDisplayState(IDLE);
//       }
//       break;
//   }
// }

// // ==================== EEPROM FUNCTIONS ====================
// String eepromReadString(int addr, int maxLen) {
//   char buffer[maxLen + 1];
//   int i;
//   for (i = 0; i < maxLen; i++) {
//     byte c = EEPROM.read(addr + i);
//     if (c == 0x00) break;
//     if (c == 0x01) buffer[i] = (char)0x00;
//     else buffer[i] = (char)c;
//   }
//   buffer[i] = '\0';
//   return String(buffer);
// }

// void eepromWriteString(int addr, const String &str, int maxLen) {
//   int len = min(str.length(), (unsigned int)(maxLen - 1));
//   for (int i = 0; i < len; i++) {
//     char c = str.charAt(i);
//     if (c == (char)0x00) EEPROM.write(addr + i, 0x01);
//     else EEPROM.write(addr + i, (byte)c);
//   }
//   EEPROM.write(addr + len, 0x00);
//   for (int i = len + 1; i < maxLen; i++) {
//     EEPROM.write(addr + i, 0xFF);
//   }
// }

// bool isValidJSON(const String &json) {
//   if (json.length() < 10) return false;
//   if (!json.startsWith("{")) return false;
//   if (!json.endsWith("}")) return false;
//   int braceCount = 0;
//   bool inString = false;
//   for (unsigned int i = 0; i < json.length(); i++) {
//     char c = json.charAt(i);
//     if (c == '\\' && i + 1 < json.length()) {
//       i++;
//       continue;
//     }
//     if (c == '"') inString = !inString;
//     if (!inString) {
//       if (c == '{') braceCount++;
//       else if (c == '}') braceCount--;
//       if (braceCount < 0) return false;
//     }
//   }
//   return (braceCount == 0 && !inString);
// }

// bool isEEPROMCorrupted() {
//   bool allEmpty = true;
//   for (int i = 0; i < 20; i++) {
//     if (EEPROM.read(REGISTERED_BOOKS_START + i) != 0xFF) {
//       allEmpty = false;
//       break;
//     }
//   }
//   if (allEmpty) return false;
//   for (int i = 0; i < 5; i++) {
//     int addr = REGISTERED_BOOKS_START + i * (UID_LENGTH + 1);
//     bool slotEmpty = true;
//     for (int j = 0; j < UID_LENGTH; j++) {
//       byte c = EEPROM.read(addr + j);
//       if (c != 0xFF && c != 0x00) {
//         slotEmpty = false;
//         break;
//       }
//     }
//     if (slotEmpty) continue;
//     String uid = eepromReadString(addr, UID_LENGTH);
//     if (uid.length() == UID_LENGTH) {
//       bool validHex = true;
//       for (unsigned int j = 0; j < uid.length(); j++) {
//         char c = uid.charAt(j);
//         if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) {
//           validHex = false;
//           break;
//         }
//       }
//       if (!validHex) return true;
//     } else if (uid.length() > 0 && uid.length() < UID_LENGTH) {
//       return true;
//     }
//   }
//   return false;
// }

// void clearCorruptedEEPROM() {
//   Serial.println("Clearing corrupted EEPROM data...");
//   lcd.clear();
//   lcdPrintCenter(0, "EEPROM CLEAR");
//   lcdPrintCenter(1, "Removing corrupted");
//   lcdPrintCenter(2, "data...");
//   for (int i = 0; i < MAX_REGISTERED_BOOKS * (UID_LENGTH + 1); i++) {
//     EEPROM.write(REGISTERED_BOOKS_START + i, 0xFF);
//   }
//   for (int i = 0; i < MAX_CACHED_BOOKS * BOOK_CACHE_SIZE; i++) {
//     EEPROM.write(BOOK_CACHE_START + i, 0xFF);
//   }
//   for (int i = 0; i < PENDING_MAX * PENDING_ENTRY_SIZE; i++) {
//     EEPROM.write(PENDING_START + i, 0xFF);
//   }
//   lcdPrintCenter(3, "CLEARED OK");
//   Serial.println("EEPROM cleared successfully");
//   delay(1000);
// }

// // ==================== BOOK CACHE SYSTEM ====================
// int getBookCacheAddr(const String &uid) {
//   unsigned int hash = 0;
//   for (unsigned int i = 0; i < uid.length(); i++) {
//     hash = (hash * 31) + uid.charAt(i);
//   }
//   int slot = hash % MAX_CACHED_BOOKS;
//   return BOOK_CACHE_START + slot * BOOK_CACHE_SIZE;
// }

// void cacheBookData(const String &uid, const String &jsonData) {
//   if (uid.length() != 8 || jsonData.length() == 0) return;
//   if (!isValidJSON(jsonData)) return;
//   int addr = getBookCacheAddr(uid);
//   unsigned int len = jsonData.length();
//   len = min(len, (unsigned int)(BOOK_CACHE_SIZE - 3));
//   EEPROM.write(addr, (len >> 8) & 0xFF);
//   EEPROM.write(addr + 1, len & 0xFF);
//   for (unsigned int i = 0; i < len; i++) {
//     char c = jsonData.charAt(i);
//     if (c == (char)0x00) EEPROM.write(addr + 2 + i, 0x01);
//     else EEPROM.write(addr + 2 + i, (byte)c);
//   }
//   EEPROM.write(addr + 2 + len, 0xFF);
// }

// String getCachedBookData(const String &uid) {
//   if (uid.length() != 8) return "";
//   int addr = getBookCacheAddr(uid);
//   int len = (EEPROM.read(addr) << 8) | EEPROM.read(addr + 1);
//   if (len > 0 && len <= (BOOK_CACHE_SIZE - 3)) {
//     String cached = "";
//     for (int i = 0; i < len; i++) {
//       byte c = EEPROM.read(addr + 2 + i);
//       if (c == 0x01) cached += (char)0x00;
//       else cached += (char)c;
//     }
//     if (isValidJSON(cached)) return cached;
//   }
//   String cached = eepromReadString(addr, BOOK_CACHE_SIZE - 1);
//   if (isValidJSON(cached)) return cached;
//   return "";
// }

// // ==================== REGISTERED BOOKS MANAGEMENT ====================
// bool isBookRegistered(const String &uid) {
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     if (registeredUIDs[i] == uid) return true;
//   }
//   return false;
// }

// int getBookIndex(const String &uid) {
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     if (registeredUIDs[i] == uid) return i;
//   }
//   return -1;
// }

// void saveRegisteredBook(const String &uid, int sensorIndex) {
//   if (isBookRegistered(uid)) return;
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     if (registeredUIDs[i].length() == 0) {
//       registeredUIDs[i] = uid;
//       if (sensorIndex >= 0 && sensorIndex < NUM_SENSORS) {
//         sensorAssignments[i] = sensorIndex;
//       } else {
//         for (int s = 0; s < NUM_SENSORS; s++) {
//           bool sensorUsed = false;
//           for (int j = 0; j < MAX_REGISTERED_BOOKS; j++) {
//             if (sensorAssignments[j] == s) {
//               sensorUsed = true;
//               break;
//             }
//           }
//           if (!sensorUsed) {
//             sensorAssignments[i] = s;
//             break;
//           }
//         }
//       }
//       int addr = REGISTERED_BOOKS_START + i * (UID_LENGTH + 1);
//       eepromWriteString(addr, uid, UID_LENGTH);
//       return;
//     }
//   }
// }

// void loadRegisteredBooks() {
//   Serial.println("Loading registered books from EEPROM...");
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     registeredUIDs[i] = "";
//     sensorAssignments[i] = 255;
//   }
//   int bookCount = 0;
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     int addr = REGISTERED_BOOKS_START + i * (UID_LENGTH + 1);
//     String uid = eepromReadString(addr, UID_LENGTH);
//     if (uid.length() == UID_LENGTH) {
//       bool validHex = true;
//       for (unsigned int j = 0; j < uid.length(); j++) {
//         char c = uid.charAt(j);
//         if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) {
//           validHex = false;
//           break;
//         }
//       }
//       if (validHex) {
//         registeredUIDs[bookCount] = uid;
//         sensorAssignments[bookCount] = 255;
//         bookCount++;
//       }
//     }
//   }
//   if (bookCount == 0) {
//     Serial.println("No registered books found in EEPROM");
//   } else {
//     Serial.print("Loaded ");
//     Serial.print(bookCount);
//     Serial.println(" registered books");
//   }
// }

// // ==================== SENSOR FUNCTIONS ====================
// String readSensor(int pin) {
//   static String lastState[NUM_SENSORS];
//   static unsigned long lastChangeTime[NUM_SENSORS];
//   int lowCount = 0;
//   for (int i = 0; i < 50; i++) {
//     if (digitalRead(pin) == LOW) lowCount++;
//     delayMicroseconds(400);
//   }
//   String currentState;
//   if (lowCount > 40) currentState = "AVL";
//   else if (lowCount < 10) currentState = "BRW";
//   else {
//     int sensorIndex = -1;
//     for (int i = 0; i < NUM_SENSORS; i++) {
//       if (sensorPins[i] == pin) {
//         sensorIndex = i;
//         break;
//       }
//     }
//     if (sensorIndex != -1) currentState = lastState[sensorIndex];
//     else currentState = "BRW";
//   }
//   for (int i = 0; i < NUM_SENSORS; i++) {
//     if (sensorPins[i] == pin) {
//       lastState[i] = currentState;
//       break;
//     }
//   }
//   return currentState;
// }

// int getSensorIndexForBook(const String &uid) {
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     if (registeredUIDs[i] == uid) {
//       if (sensorAssignments[i] >= 0 && sensorAssignments[i] < NUM_SENSORS) {
//         return sensorAssignments[i];
//       }
//       break;
//     }
//   }
//   return -1;
// }

// int findSensorForBook(const String &uid) {
//   int existingSensor = getSensorIndexForBook(uid);
//   if (existingSensor != -1) return existingSensor;
//   for (int sensorNum = 0; sensorNum < NUM_SENSORS; sensorNum++) {
//     bool sensorInUse = false;
//     for (int j = 0; j < MAX_REGISTERED_BOOKS; j++) {
//       if (sensorAssignments[j] == sensorNum) {
//         sensorInUse = true;
//         break;
//       }
//     }
//     if (!sensorInUse) {
//       for (int j = 0; j < MAX_REGISTERED_BOOKS; j++) {
//         if (registeredUIDs[j].length() == 0) {
//           registeredUIDs[j] = uid;
//           sensorAssignments[j] = sensorNum;
//           int addr = REGISTERED_BOOKS_START + j * (UID_LENGTH + 1);
//           eepromWriteString(addr, uid, UID_LENGTH);
//           Serial.print("ASSIGNED: Book ");
//           Serial.print(uid);
//           Serial.print(" → Sensor ");
//           Serial.print(sensorNum);
//           Serial.print(" (Pin ");
//           Serial.print(sensorPins[sensorNum]);
//           Serial.println(")");
//           return sensorNum;
//         }
//       }
//     }
//   }
//   Serial.println("WARNING: All sensors in use! Using sensor 0 as fallback");
//   return 0;
// }

// String getBookStatus(const String &uid) {
//   int sensorIndex = getSensorIndexForBook(uid);
//   if (sensorIndex == -1) {
//     Serial.print("BOOK ");
//     Serial.print(uid);
//     Serial.println(" → No sensor assigned yet, defaulting to AVL");
//     return "AVL";
//   }
//   if (sensorIndex >= 0 && sensorIndex < NUM_SENSORS) {
//     String status = readSensor(sensorPins[sensorIndex]);
//     Serial.print("BOOK ");
//     Serial.print(uid);
//     Serial.print(" → SENSOR INDEX: ");
//     Serial.print(sensorIndex);
//     Serial.print(", PIN: ");
//     Serial.print(sensorPins[sensorIndex]);
//     Serial.print(", STATUS: ");
//     Serial.println(status);
//     return status;
//   }
//   Serial.print("BOOK ");
//   Serial.print(uid);
//   Serial.println(" → Invalid sensor index, defaulting to AVL");
//   return "AVL";
// }

// // ==================== SERIAL COMMUNICATION ====================
// void sendToESP(const String &msg) {
//   while (Serial1.available()) Serial1.read();
//   delay(10);
//   Serial1.println(msg);
//   Serial.print("TO_ESP: ");
//   if (msg.startsWith("SEND:")) {
//     int pipePos = msg.indexOf('|');
//     if (pipePos != -1) {
//       Serial.print(msg.substring(0, pipePos + 1));
//       Serial.print("... [JSON length: ");
//       Serial.print(msg.length() - pipePos - 1);
//       Serial.println("]");
//     } else {
//       int showLen = min(msg.length(), 60);
//       Serial.println(msg.substring(0, showLen));
//     }
//   } else {
//     Serial.println(msg);
//   }
//   if (msg.length() > 200) delay(50);
// }

// bool waitForResponse(String &response, unsigned long timeout) {
//   unsigned long start = millis();
//   response = "";
//   while (millis() - start < timeout) {
//     while (Serial1.available()) {
//       char c = Serial1.read();
//       if (c == '\n') {
//         response.trim();
//         if (response.length() > 0) {
//           if (!response.startsWith("DEBUG:") && !response.startsWith("WIFI:")) {
//             Serial.print("FROM_ESP: ");
//             Serial.println(response);
//           }
//           return true;
//         }
//       } else if (c != '\r' && response.length() < 2048) {
//         response += c;
//       }
//     }
//     delay(1);
//   }
//   return false;
// }

// // ==================== PENDING QUEUE ====================
// int pendingIndexAddr(int index) { 
//   return PENDING_START + index * PENDING_ENTRY_SIZE; 
// }

// bool addPendingEntry(const String &payload) {
//   if (!payload.startsWith("SEND:")) return false;
//   int pipePos = payload.indexOf('|');
//   if (pipePos != -1) {
//     String jsonPart = payload.substring(pipePos + 1);
//     if (!isValidJSON(jsonPart)) return false;
//   }
//   for (int i = 0; i < PENDING_MAX; i++) {
//     int addr = pendingIndexAddr(i);
//     if (EEPROM.read(addr) == 0xFF) {
//       eepromWriteString(addr, payload, PENDING_ENTRY_SIZE - 1);
//       Serial.print("QUEUED: ");
//       int showLen = min(payload.length(), 60);
//       Serial.println(payload.substring(0, showLen));
//       return true;
//     }
//   }
//   Serial.println("QUEUE FULL!");
//   return false;
// }

// bool popPendingEntry(String &outPayload) {
//   for (int i = 0; i < PENDING_MAX; i++) {
//     int addr = pendingIndexAddr(i);
//     if (EEPROM.read(addr) != 0xFF && EEPROM.read(addr) != 0x00) {
//       outPayload = eepromReadString(addr, PENDING_ENTRY_SIZE - 1);
//       for (int j = 0; j < PENDING_ENTRY_SIZE; j++) {
//         EEPROM.write(addr + j, 0xFF);
//       }
//       if (outPayload.length() > 10 && 
//           outPayload.startsWith("SEND:") && 
//           outPayload.indexOf('|') > 5) {
//         int pipePos = outPayload.indexOf('|');
//         String jsonPart = outPayload.substring(pipePos + 1);
//         if (isValidJSON(jsonPart)) return true;
//       }
//     }
//   }
//   return false;
// }

// void flushPendingQueue() {
//   if (!wifiConnected) return;
//   String payload;
//   int flushed = 0;
//   while (popPendingEntry(payload)) {
//     if (payload.length() > 10) {
//       if (!payload.startsWith("SEND:")) continue;
//       Serial.print("FLUSHING: ");
//       Serial.println(payload.substring(0, 60));
//       sendToESP(payload);
//       String response;
//       if (waitForResponse(response, REQUEST_TIMEOUT)) {
//         if (response.startsWith("SUCCESS:")) {
//           flushed++;
//           delay(100);
//         } else {
//           addPendingEntry(payload);
//           break;
//         }
//       } else {
//         addPendingEntry(payload);
//         break;
//       }
//     }
//   }
//   if (flushed > 0) {
//     Serial.print("Successfully flushed ");
//     Serial.print(flushed);
//     Serial.println(" queued entries");
//   }
// }

// // ==================== RFID ====================
// String readRFID() {
//   if (!mfrc522.PICC_IsNewCardPresent()) return "";
//   if (!mfrc522.PICC_ReadCardSerial()) return "";
//   String uid = "";
//   for (byte i = 0; i < mfrc522.uid.size; i++) {
//     if (mfrc522.uid.uidByte[i] < 0x10) uid += "0";
//     uid += String(mfrc522.uid.uidByte[i], HEX);
//   }
//   uid.toUpperCase();
//   mfrc522.PICC_HaltA();
//   return uid;
// }

// // ==================== MAIN PROCESSING ====================
// void processCard(const String &cardUID) {
//   if (cardUID == lastCardUID && millis() - lastCardTime < DEBOUNCE_TIME) return;
//   lastCardUID = cardUID;
//   lastCardTime = millis();
  
//   playTone(1200, 100);
//   changeDisplayState(SCANNING);
//   lastActivityTime = millis();
  
//   int sensorIndex = findSensorForBook(cardUID);
//   String sensorStatus = "AVL";
//   if (sensorIndex >= 0 && sensorIndex < NUM_SENSORS) {
//     sensorStatus = readSensor(sensorPins[sensorIndex]);
//   }
  
//   Serial.println("=================================");
//   Serial.print("CARD: ");
//   Serial.println(cardUID);
//   Serial.print("SENSOR INDEX: ");
//   Serial.println(sensorIndex);
//   Serial.print("SENSOR STATUS: ");
//   Serial.println(sensorStatus);
//   Serial.println("=================================");
  
//   bool serverIsRegistered = false;
//   String serverTitle = "";
//   String serverAuthor = "";
//   String serverLocation = "";
//   String serverEspStatus = "AVL";
//   int serverSensor = 0;
//   bool usingCachedData = false;
//   bool fromFirebase = false;
//   bool isLocallyRegistered = isBookRegistered(cardUID);
  
//   if (wifiConnected) {
//     sendToESP("GET:" + cardUID);
//     String response;
//     if (waitForResponse(response, REQUEST_TIMEOUT)) {
//       if (response.startsWith("BOOKDATA:")) {
//         int p = response.indexOf('|');
//         if (p != -1) {
//           String serverData = response.substring(p + 1);
//           if (serverData != "null") {
//             JsonDocument doc;
//             DeserializationError err = deserializeJson(doc, serverData);
//             if (!err) {
//               serverIsRegistered = doc["isRegistered"] | false;
//               serverTitle = String(doc["title"] | "");
//               serverAuthor = String(doc["author"] | "");
//               serverLocation = String(doc["location"] | "");
//               serverSensor = doc["sensor"] | 0;
//               // FIXED: Check if espStatus exists and get it
//               if (doc["espStatus"].is<String>()) {
//                 serverEspStatus = doc["espStatus"].as<String>();
//               } else {
//                 serverEspStatus = "AVL";
//               }
              
//               if (!serverIsRegistered) {
//                 if (serverTitle.length() > 0 && 
//                     serverTitle != "New Book" && 
//                     serverTitle != "Book Title Unknown" &&
//                     serverAuthor != "Unknown" &&
//                     serverLocation != "Unassigned") {
//                   serverIsRegistered = true;
//                   Serial.println("FIREBASE AUTO-REGISTER: Book has real data");
//                 }
//               }
//               Serial.print("FIREBASE: Registered=");
//               Serial.print(serverIsRegistered ? "YES" : "NO");
//               Serial.print(", Title='");
//               Serial.print(serverTitle);
//               Serial.print("', Status=");
//               Serial.println(serverEspStatus);
//               cacheBookData(cardUID, serverData);
//               if (serverIsRegistered && !isLocallyRegistered) {
//                 saveRegisteredBook(cardUID, serverSensor);
//                 isLocallyRegistered = true;
//                 Serial.println("FIREBASE SAVED: Book registered in local storage");
//               }
//               fromFirebase = true;
//             }
//           } else {
//             Serial.println("Book not found in Firebase (null response)");
//           }
//         }
//       }
//     } else {
//       wifiConnected = false;
//       Serial.println("WiFi timeout - switching to offline mode");
//     }
//   }
  
//   if (!fromFirebase) {
//     String cachedData = getCachedBookData(cardUID);
//     if (cachedData.length() > 0) {
//       JsonDocument doc;
//       DeserializationError err = deserializeJson(doc, cachedData);
//       if (!err) {
//         serverIsRegistered = doc["isRegistered"] | false;
//         serverTitle = String(doc["title"] | "");
//         serverAuthor = String(doc["author"] | "");
//         serverLocation = String(doc["location"] | "");
//         serverSensor = doc["sensor"] | 0;
//         // FIXED: Check if espStatus exists and get it
//         if (doc["espStatus"].is<String>()) {
//           serverEspStatus = doc["espStatus"].as<String>();
//         } else {
//           serverEspStatus = "AVL";
//         }
        
//         if (!serverIsRegistered) {
//           if (serverTitle.length() > 0 && 
//               serverTitle != "New Book" && 
//               serverTitle != "Book Title Unknown" &&
//               serverAuthor != "Unknown" &&
//               serverLocation != "Unassigned") {
//             serverIsRegistered = true;
//             Serial.println("CACHE AUTO-REGISTER: Book has real data");
//           }
//         }
//         usingCachedData = true;
//         Serial.print("USING CACHED DATA: Registered=");
//         Serial.println(serverIsRegistered ? "YES" : "NO");
//       }
//     }
//   }
  
//   if (!serverIsRegistered && !isLocallyRegistered) {
//     if (serverTitle.length() > 0 && 
//         serverTitle != "New Book" && 
//         serverTitle != "Book Title Unknown" &&
//         serverAuthor != "Unknown" &&
//         serverLocation != "Unassigned") {
//       serverIsRegistered = true;
//       Serial.println("CONTENT AUTO-REGISTER: Book has real data");
//     }
//   }
  
//   if (serverTitle.length() == 0) {
//     if (isLocallyRegistered) {
//       serverTitle = "Book Title Unknown";
//       serverAuthor = "Author Unknown";
//       serverLocation = "Location Unknown";
//       serverIsRegistered = true;
//       Serial.println("Using local registration (no cache)");
//     } else {
//       serverTitle = "New Book";
//       serverAuthor = "Unknown";
//       serverLocation = "Unassigned";
//       serverIsRegistered = false;
//       Serial.println("New unregistered book");
//     }
//   } else if (serverTitle == "New Book" && serverIsRegistered) {
//     serverTitle = "Registered Book";
//     Serial.println("Updating 'New Book' title to 'Registered Book'");
//   }
  
//   // Store book info for LCD display
//   currentBookTitle = serverTitle;
//   currentBookAuthor = serverAuthor;
//   currentBookLocation = serverLocation;
//   currentBookStatus = sensorStatus;  // AVL, BRW, etc.
//   currentBookRegistered = serverIsRegistered;
//   currentBookSensor = sensorIndex;
  
//   JsonDocument doc;
//   doc["title"] = serverTitle;
//   doc["author"] = serverAuthor;
//   doc["location"] = serverLocation;
//   doc["isRegistered"] = serverIsRegistered;
//   doc["lastSeen"] = String(millis() / 1000);
//   doc["sensor"] = sensorIndex;
//   doc["espStatus"] = sensorStatus;
  
//   if (sensorStatus == "AVL") doc["status"] = "Available";
//   else if (sensorStatus == "BRW") doc["status"] = "Borrowed";
//   else if (sensorStatus == "OVD") doc["status"] = "Overdue";
//   else if (sensorStatus == "MIS") doc["status"] = "Misplaced";
//   else doc["status"] = "Available";
  
//   doc["lastUpdated"] = String(millis() / 1000);
  
//   if (serverSensor > 0) doc["sensor"] = serverSensor;
//   if (usingCachedData) doc["source"] = "cached";
//   doc["sensor"] = sensorIndex;
  
//   if (serverIsRegistered && !isLocallyRegistered) {
//     saveRegisteredBook(cardUID, sensorIndex);
//     Serial.println("SAVED TO EEPROM: Book marked as registered");
//   }
  
//   String outJson;
//   serializeJson(doc, outJson);
//   Serial.print("JSON SIZE: ");
//   Serial.println(outJson.length());
//   if (!outJson.endsWith("}")) outJson += "}";
//   if (!isValidJSON(outJson)) {
//     Serial.println("ERROR: Generated invalid JSON!");
//     outJson = "{\"title\":\"" + serverTitle + "\",\"author\":\"" + serverAuthor + "\",\"location\":\"" + serverLocation + "\",\"isRegistered\":" + (serverIsRegistered ? "true" : "false") + ",\"espStatus\":\"" + sensorStatus + "\"}";
//   }
  
//   String sendCmd = "SEND:/books/" + cardUID + "|" + outJson;
//   if (wifiConnected) {
//     sendToESP(sendCmd);
//     String response;
//     if (waitForResponse(response, REQUEST_TIMEOUT)) {
//       if (response.startsWith("SUCCESS:")) {
//         cacheBookData(cardUID, outJson);
//         if (serverIsRegistered && !isBookRegistered(cardUID)) {
//           saveRegisteredBook(cardUID, sensorIndex);
//         }
//         Serial.println("Firebase update SUCCESS");
//       } else if (response.startsWith("ERROR:")) {
//         Serial.print("Firebase error: ");
//         Serial.println(response);
//         addPendingEntry(sendCmd);
//       } else {
//         addPendingEntry(sendCmd);
//         Serial.println("Unknown response - queued");
//       }
//     } else {
//       addPendingEntry(sendCmd);
//       Serial.println("No response from ESP - queued");
//     }
//   } else {
//     addPendingEntry(sendCmd);
//     Serial.println("WiFi offline - queued");
//   }
  
//   Serial.print("FINAL STATUS: ");
//   Serial.print(cardUID);
//   Serial.print(" - ");
//   Serial.print(serverTitle);
//   Serial.print(" - Registered: ");
//   Serial.print(serverIsRegistered ? "YES" : "NO");
//   Serial.print(" - Sensor: ");
//   Serial.println(sensorIndex);
//   Serial.println("=================================");
// }

// // ==================== SETUP ====================
// void setup() {
//   Serial.begin(115200);
//   Serial1.begin(115200);
  
//   Wire.begin();
//   lcd.init();
//   lcd.backlight();
//   lcd.clear();
  
//   if (BUZZER_PIN > 0) {
//     pinMode(BUZZER_PIN, OUTPUT);
//   }
  
//   delay(2000);
//   while (Serial1.available()) Serial1.read();
  
//   Serial.println();
//   Serial.println("LIBRARY SYSTEM - MEGA 2560 WITH OFFLINE CACHE");
//   Serial.println("==============================================");
  
//   currentDisplayState = WELCOME;
//   welcomeStartTime = millis();
//   lastActivityTime = millis();
  
//   for (int i = 0; i < NUM_SENSORS; i++) {
//     pinMode(sensorPins[i], INPUT_PULLUP);
//   }
  
//   if (isEEPROMCorrupted()) {
//     Serial.println("EEPROM corrupted - clearing all data");
//     clearCorruptedEEPROM();
//   }
  
//   loadRegisteredBooks();
  
//   SPI.begin();
//   mfrc522.PCD_Init();
//   delay(200);
  
//   Serial.println("Testing ESP connection...");
//   wifiConnected = false;
//   for (int i = 0; i < 3; i++) {
//     sendToESP("PING");
//     String response;
//     if (waitForResponse(response, 2000)) {
//       if (response == "PONG") {
//         wifiConnected = true;
//         Serial.println("ESP connected");
//         break;
//       }
//     }
//     delay(1000);
//   }
  
//   if (!wifiConnected) {
//     Serial.println("ESP not responding - OFFLINE MODE");
//   } else {
//     flushPendingQueue();
//   }
  
//   Serial.println("System Ready - Sensors will be assigned sequentially");
// }

// // ==================== LOOP ====================
// void loop() {
//   // Update display
//   updateDisplay();
  
//   // Check for RFID cards
//   String uid = readRFID();
//   if (uid.length() == 8) {
//     processCard(uid);
//   }
  
//   // Handle serial communication with ESP
//   static String espBuffer = "";
//   while (Serial1.available()) {
//     char c = Serial1.read();
//     if (c == '\n') {
//       espBuffer.trim();
//       if (espBuffer.length() > 0) {
//         if (espBuffer.startsWith("WIFI:")) {
//           if (espBuffer == "WIFI:OK") {
//             if (!wifiConnected) {
//               wifiConnected = true;
//               Serial.println("WiFi connected - FLUSHING QUEUE");
//               flushPendingQueue();
//               changeDisplayState(NETWORK_STATUS);
//             }
//           } else if (espBuffer == "WIFI:OFFLINE") {
//             wifiConnected = false;
//             Serial.println("WiFi offline - CACHE MODE");
//           }
//         } else if (espBuffer == "PONG") {
//           wifiConnected = true;
//         }
//       }
//       espBuffer = "";
//     } else if (c != '\r') {
//       espBuffer += c;
//     }
//   }
  
//   // Periodic ping to ESP
//   static unsigned long lastPing = 0;
//   if (millis() - lastPing > PING_INTERVAL) {
//     lastPing = millis();
//     sendToESP("PING");
//     String response;
//     if (waitForResponse(response, 3000)) {
//       if (response == "PONG" && !wifiConnected) {
//         wifiConnected = true;
//         Serial.println("WiFi restored - FLUSHING QUEUE");
//         flushPendingQueue();
//         changeDisplayState(NETWORK_STATUS);
//       }
//     } else if (wifiConnected) {
//       wifiConnected = false;
//       Serial.println("WiFi lost - CACHE MODE");
//     }
//   }
  
//   delay(10);
// }








// //
// //*************************************NUMBER TWO 22222222222222222222222222222 */
// //*************************************NUMBER TWO 22222222222222222222222222222 */
// //*************************************NUMBER TWO 22222222222222222222222222222 */
// #include <Arduino.h>
// #include <SPI.h>
// #include <MFRC522.h>
// #include <EEPROM.h>
// #include <ArduinoJson.h>
// #include <LiquidCrystal_I2C.h>  // For I2C LCD

// // ==================== LCD CONFIGURATION ====================
// #define LCD_ADDRESS 0x27      // Default I2C address for 20x4 LCD
// #define LCD_COLS 20
// #define LCD_ROWS 4
// LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLS, LCD_ROWS);

// // ==================== BUZZER CONFIGURATION ====================
// #define BUZZER_PIN 48

// // ==================== GLOBAL VARIABLES (DECLARED BEFORE LCD FUNCTIONS) ====================
// bool wifiConnected = false;
// String lastCardUID = "";
// unsigned long lastCardTime = 0;
// String registeredUIDs[50];  // MAX_REGISTERED_BOOKS
// byte sensorAssignments[50]; // MAX_REGISTERED_BOOKS

// // ==================== LCD STATES ====================
// enum LCDState {
//   LCD_WELCOME,
//   LCD_READY,
//   LCD_CARD_DETECTED,
//   LCD_BOOK_INFO,
//   LCD_OFFLINE,
//   LCD_NEW_BOOK,
//   LCD_CLEARING_EEPROM,
//   LCD_CORRUPTED_DATA
// };

// LCDState currentLCDState = LCD_WELCOME;
// unsigned long lcdStateStartTime = 0;
// int animationFrame = 0;
// String currentBookTitle = "";
// String currentBookAuthor = "";
// String currentBookLocation = "";
// String currentBookStatus = "";

// // ==================== BEEP TONES ====================
// #define BEEP_AVL 2000    // High pitch for Available
// #define BEEP_BRW 1500    // Medium pitch for Borrowed
// #define BEEP_SWIPE 1000  // Low pitch for card swipe
// #define BEEP_WIFI_CONNECT 2500  // Rising tone for WiFi connect
// #define BEEP_WIFI_DISCONNECT 800  // Falling tone for WiFi disconnect
// #define BEEP_ERROR 300   // Error beep
// #define BEEP_CONFIRM 1800 // Confirmation beep

// // ==================== BUZZER FUNCTIONS ====================
// void beep(int frequency, int duration) {
//   if (frequency <= 0) return;
//   tone(BUZZER_PIN, frequency, duration);
//   delay(duration + 50);
// }

// void beepSequence(int freq1, int dur1, int freq2 = 0, int dur2 = 0) {
//   beep(freq1, dur1);
//   if (freq2 > 0) {
//     delay(50);
//     beep(freq2, dur2);
//   }
// }

// // ==================== LCD HELPER FUNCTIONS ====================
// void clearLine(int row) {
//   lcd.setCursor(0, row);
//   for (int i = 0; i < LCD_COLS; i++) {
//     lcd.print(" ");
//   }
// }

// void centerText(String text, int row) {
//   int padding = (LCD_COLS - text.length()) / 2;
//   if (padding < 0) padding = 0;
//   lcd.setCursor(padding, row);
//   lcd.print(text);
// }

// void setLCDState(LCDState newState) {
//   if (currentLCDState != newState) {
//     currentLCDState = newState;
//     lcdStateStartTime = millis();
//     animationFrame = 0;
//     lcd.clear();
//   }
// }

// void updateLCD() {
//   static unsigned long lastUpdate = 0;
//   unsigned long currentMillis = millis();
  
//   if (currentMillis - lastUpdate < 200) return;
//   lastUpdate = currentMillis;
  
//   unsigned long stateDuration = currentMillis - lcdStateStartTime;
  
//   switch (currentLCDState) {
//     case LCD_WELCOME:
//       if (animationFrame == 0) {
//         lcd.clear();
//         centerText("LIBRARY SYSTEM", 0);
//         centerText("v5.0", 1);
//         centerText("LeejinBotics", 2);
//         beep(BEEP_CONFIRM, 200);
//         animationFrame = 1;
//       }
//       if (stateDuration > 3000) {
//         setLCDState(LCD_READY);
//       }
//       break;
      
//     case LCD_READY:
//       if (animationFrame == 0) {
//         lcd.clear();
//         centerText("SYSTEM READY", 0);
//         lcd.setCursor(0, 1);
//         lcd.print("Scan a book card...");
//         lcd.setCursor(0, 2);
//         lcd.print("WiFi: ");
//         lcd.print(wifiConnected ? "ONLINE " : "OFFLINE");
        
//         // Create dots animation
//         int dotCount = (stateDuration / 500) % 4;
//         String dots = "";
//         for (int i = 0; i < dotCount; i++) {
//           dots += ".";
//         }
//         lcd.setCursor(17, 1);
//         lcd.print(dots);
//         animationFrame = 1;
//       }
//       break;
      
//     case LCD_CARD_DETECTED:
//       if (animationFrame == 0) {
//         lcd.clear();
//         centerText("CARD DETECTED!", 0);
//         lcd.setCursor(0, 1);
//         lcd.print("UID: ");
//         lcd.print(lastCardUID);
//         lcd.setCursor(0, 2);
//         lcd.print("Status: READING");
        
//         // Create reading dots animation
//         int readingDotCount = (stateDuration / 300) % 4;
//         String readingDots = "";
//         for (int i = 0; i < readingDotCount; i++) {
//           readingDots += ".";
//         }
//         lcd.setCursor(16, 2);
//         lcd.print(readingDots);
//         beep(BEEP_SWIPE, 150);
//         animationFrame = 1;
//       }
//       break;
      
//     case LCD_BOOK_INFO:
//       if (animationFrame == 0) {
//         lcd.clear();
        
//         // Display title (truncate if too long)
//         String displayTitle = currentBookTitle;
//         if (displayTitle.length() > 20) {
//           displayTitle = displayTitle.substring(0, 17) + "...";
//         }
//         lcd.setCursor(0, 0);
//         lcd.print(displayTitle);
        
//         // Display author (truncate if too long)
//         String displayAuthor = currentBookAuthor;
//         if (displayAuthor.length() > 20) {
//           displayAuthor = displayAuthor.substring(0, 17) + "...";
//         }
//         lcd.setCursor(0, 1);
//         lcd.print(displayAuthor);
        
//         // Display location
//         lcd.setCursor(0, 2);
//         lcd.print("LOC: ");
//         lcd.print(currentBookLocation);
        
//         // Display status with sensor info
//         lcd.setCursor(0, 3);
//         lcd.print("Status: ");
//         lcd.print(currentBookStatus);
        
//         // Play appropriate beep
//         if (currentBookStatus == "AVL") {
//           beep(BEEP_AVL, 300);
//         } else if (currentBookStatus == "BRW") {
//           beep(BEEP_BRW, 300);
//         }
        
//         animationFrame = 1;
//       }
      
//       if (stateDuration > 5000) {
//         setLCDState(LCD_READY);
//       }
//       break;
      
//     case LCD_OFFLINE:
//       if (animationFrame == 0) {
//         lcd.clear();
//         centerText("OFFLINE MODE", 0);
//         lcd.setCursor(0, 1);
//         lcd.print("Data cached for:");
//         lcd.setCursor(0, 2);
//         if (lastCardUID.length() > 0) {
//           lcd.print(lastCardUID);
//         } else {
//           lcd.print("No recent card");
//         }
//         lcd.setCursor(0, 3);
//         lcd.print("Scan to queue...");
        
//         beep(BEEP_WIFI_DISCONNECT, 200);
//         animationFrame = 1;
//       }
//       break;
      
//     case LCD_NEW_BOOK:
//       if (animationFrame == 0) {
//         lcd.clear();
//         centerText("NEW BOOK DETECTED", 0);
//         lcd.setCursor(0, 1);
//         lcd.print("UID: ");
//         lcd.print(lastCardUID);
//         lcd.setCursor(0, 2);
//         lcd.print("Assigning Sensor...");
//         lcd.setCursor(0, 3);
//         lcd.print("Please register!");
        
//         if ((stateDuration / 500) % 2 == 0) {
//           lcd.setCursor(17, 0);
//           lcd.print("NEW");
//         } else {
//           lcd.setCursor(17, 0);
//           lcd.print("   ");
//         }
        
//         beepSequence(BEEP_SWIPE, 100, BEEP_CONFIRM, 100);
//         animationFrame = 1;
//       }
//       break;
      
//     case LCD_CLEARING_EEPROM:
//       if (animationFrame == 0) {
//         lcd.clear();
//         centerText("CLEARING EEPROM", 0);
//         lcd.setCursor(0, 1);
//         lcd.print("Please wait...");
        
//         int progress = (stateDuration / 100) % 21;
//         lcd.setCursor(0, 2);
//         for (int i = 0; i < 20; i++) {
//           if (i < progress) {
//             lcd.print("=");
//           } else {
//             lcd.print(" ");
//           }
//         }
        
//         lcd.setCursor(18, 2);
//         if (progress * 5 < 100) {
//           lcd.print(" ");
//         }
//         lcd.print(progress * 5);
//         lcd.print("%");
        
//         beep(BEEP_ERROR, 50);
//         animationFrame = 1;
//       }
//       break;
      
//     case LCD_CORRUPTED_DATA:
//       if (animationFrame == 0) {
//         lcd.clear();
//         centerText("CORRUPTED DATA", 0);
//         lcd.setCursor(0, 1);
//         lcd.print("EEPROM Error!");
//         lcd.setCursor(0, 2);
//         lcd.print("System recovering");
        
//         if ((stateDuration / 250) % 2 == 0) {
//           lcd.setCursor(3, 3);
//           lcd.print("!!! WARNING !!!");
//         } else {
//           clearLine(3);
//         }
        
//         beepSequence(BEEP_ERROR, 100, BEEP_ERROR, 100);
//         animationFrame = 1;
//       }
//       break;
//   }
// }

// // ==================== ORIGINAL CONFIG ====================
// #define RC522_SS_PIN 53
// #define RC522_RST_PIN 49

// #define REGISTERED_BOOKS_START 0
// #define MAX_REGISTERED_BOOKS 50
// #define UID_LENGTH 8

// #define BOOK_CACHE_START (REGISTERED_BOOKS_START + MAX_REGISTERED_BOOKS * (UID_LENGTH + 1))
// #define BOOK_CACHE_SIZE 512
// #define MAX_CACHED_BOOKS 20

// #define PENDING_START (BOOK_CACHE_START + MAX_CACHED_BOOKS * BOOK_CACHE_SIZE)
// #define PENDING_ENTRY_SIZE 768
// #define PENDING_MAX 8

// #define DEBOUNCE_TIME 3000UL
// #define REQUEST_TIMEOUT 8000UL
// #define PING_INTERVAL 30000UL

// #define NUM_SENSORS 15
// const byte sensorPins[NUM_SENSORS] = {22,23,24,25,26,27,28,29,30,31,32,33,34,35,36};

// // ==================== MFRC522 OBJECT ====================
// MFRC522 mfrc522(RC522_SS_PIN, RC522_RST_PIN);

// // ==================== EEPROM HELPERS ====================
// String eepromReadString(int addr, int maxLen) {
//   char buffer[maxLen + 1];
//   int i;
//   for (i = 0; i < maxLen; i++) {
//     byte c = EEPROM.read(addr + i);
//     if (c == 0x00) {
//       break;
//     }
//     if (c == 0x01) {
//       buffer[i] = (char)0x00;
//     } else {
//       buffer[i] = (char)c;
//     }
//   }
//   buffer[i] = '\0';
//   return String(buffer);
// }

// void eepromWriteString(int addr, const String &str, int maxLen) {
//   unsigned int len = min(str.length(), (unsigned int)(maxLen - 1));
  
//   for (unsigned int i = 0; i < len; i++) {
//     char c = str.charAt(i);
//     if (c == (char)0x00) {
//       EEPROM.write(addr + i, 0x01);
//     } else {
//       EEPROM.write(addr + i, (byte)c);
//     }
//   }
  
//   EEPROM.write(addr + len, 0x00);
  
//   for (unsigned int i = len + 1; i < (unsigned int)maxLen; i++) {
//     EEPROM.write(addr + i, 0xFF);
//   }
// }

// // ==================== JSON VALIDATION ====================
// bool isValidJSON(const String &json) {
//   if (json.length() < 10) return false;
//   if (!json.startsWith("{")) return false;
//   if (!json.endsWith("}")) return false;
  
//   int braceCount = 0;
//   bool inString = false;
  
//   for (unsigned int i = 0; i < json.length(); i++) {
//     char c = json.charAt(i);
    
//     if (c == '\\' && i + 1 < json.length()) {
//       i++;
//       continue;
//     }
    
//     if (c == '"') {
//       inString = !inString;
//     }
    
//     if (!inString) {
//       if (c == '{') braceCount++;
//       else if (c == '}') braceCount--;
      
//       if (braceCount < 0) return false;
//     }
//   }
  
//   return (braceCount == 0 && !inString);
// }

// // ==================== CORRUPTION DETECTION ====================
// bool isEEPROMCorrupted() {
//   bool allEmpty = true;
//   for (int i = 0; i < 20; i++) {
//     if (EEPROM.read(REGISTERED_BOOKS_START + i) != 0xFF) {
//       allEmpty = false;
//       break;
//     }
//   }
  
//   if (allEmpty) {
//     return false;
//   }
  
//   for (int i = 0; i < 5; i++) {
//     int addr = REGISTERED_BOOKS_START + i * (UID_LENGTH + 1);
    
//     bool slotEmpty = true;
//     for (int j = 0; j < UID_LENGTH; j++) {
//       byte c = EEPROM.read(addr + j);
//       if (c != 0xFF && c != 0x00) {
//         slotEmpty = false;
//         break;
//       }
//     }
    
//     if (slotEmpty) continue;
    
//     String uid = eepromReadString(addr, UID_LENGTH);
    
//     if (uid.length() == UID_LENGTH) {
//       bool validHex = true;
//       for (unsigned int j = 0; j < uid.length(); j++) {
//         char c = uid.charAt(j);
//         if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) {
//           validHex = false;
//           break;
//         }
//       }
      
//       if (!validHex) {
//         return true;
//       }
//     } else if (uid.length() > 0 && uid.length() < UID_LENGTH) {
//       return true;
//     }
//   }
  
//   return false;
// }

// void clearCorruptedEEPROM() {
//   setLCDState(LCD_CLEARING_EEPROM);
//   Serial.println("Clearing corrupted EEPROM data...");
  
//   for (int i = 0; i < MAX_REGISTERED_BOOKS * (UID_LENGTH + 1); i++) {
//     EEPROM.write(REGISTERED_BOOKS_START + i, 0xFF);
//   }
  
//   for (int i = 0; i < MAX_CACHED_BOOKS * BOOK_CACHE_SIZE; i++) {
//     EEPROM.write(BOOK_CACHE_START + i, 0xFF);
//   }
  
//   for (int i = 0; i < PENDING_MAX * PENDING_ENTRY_SIZE; i++) {
//     EEPROM.write(PENDING_START + i, 0xFF);
//   }
  
//   Serial.println("EEPROM cleared successfully");
//   delay(2000);
//   setLCDState(LCD_WELCOME);
// }

// // ==================== BOOK CACHE SYSTEM ====================
// int getBookCacheAddr(const String &uid) {
//   unsigned int hash = 0;
//   for (unsigned int i = 0; i < uid.length(); i++) {
//     hash = (hash * 31) + uid.charAt(i);
//   }
//   int slot = hash % MAX_CACHED_BOOKS;
//   return BOOK_CACHE_START + slot * BOOK_CACHE_SIZE;
// }

// void cacheBookData(const String &uid, const String &jsonData) {
//   if (uid.length() != 8 || jsonData.length() == 0) return;
  
//   if (!isValidJSON(jsonData)) {
//     return;
//   }
  
//   int addr = getBookCacheAddr(uid);
  
//   unsigned int len = jsonData.length();
//   len = min(len, (unsigned int)(BOOK_CACHE_SIZE - 3));
  
//   EEPROM.write(addr, (len >> 8) & 0xFF);
//   EEPROM.write(addr + 1, len & 0xFF);
  
//   for (unsigned int i = 0; i < len; i++) {
//     char c = jsonData.charAt(i);
//     if (c == (char)0x00) {
//       EEPROM.write(addr + 2 + i, 0x01);
//     } else {
//       EEPROM.write(addr + 2 + i, (byte)c);
//     }
//   }
  
//   EEPROM.write(addr + 2 + len, 0xFF);
// }

// String getCachedBookData(const String &uid) {
//   if (uid.length() != 8) return "";
  
//   int addr = getBookCacheAddr(uid);
  
//   int len = (EEPROM.read(addr) << 8) | EEPROM.read(addr + 1);
  
//   if (len > 0 && len <= (BOOK_CACHE_SIZE - 3)) {
//     String cached = "";
//     for (int i = 0; i < len; i++) {
//       byte c = EEPROM.read(addr + 2 + i);
//       if (c == 0x01) {
//         cached += (char)0x00;
//       } else {
//         cached += (char)c;
//       }
//     }
    
//     if (isValidJSON(cached)) {
//       return cached;
//     }
//   }
  
//   String cached = eepromReadString(addr, BOOK_CACHE_SIZE - 1);
//   if (isValidJSON(cached)) {
//     return cached;
//   }
  
//   return "";
// }

// // ==================== REGISTERED BOOKS MANAGEMENT ====================
// bool isBookRegistered(const String &uid) {
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     if (registeredUIDs[i] == uid) return true;
//   }
//   return false;
// }

// int getBookIndex(const String &uid) {
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     if (registeredUIDs[i] == uid) return i;
//   }
//   return -1;
// }

// void saveRegisteredBook(const String &uid, int sensorIndex = -1) {
//   if (isBookRegistered(uid)) return;
  
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     if (registeredUIDs[i].length() == 0) {
//       registeredUIDs[i] = uid;
      
//       if (sensorIndex >= 0 && sensorIndex < NUM_SENSORS) {
//         sensorAssignments[i] = sensorIndex;
//       } else {
//         for (int s = 0; s < NUM_SENSORS; s++) {
//           bool sensorUsed = false;
//           for (int j = 0; j < MAX_REGISTERED_BOOKS; j++) {
//             if (sensorAssignments[j] == s) {
//               sensorUsed = true;
//               break;
//             }
//           }
//           if (!sensorUsed) {
//             sensorAssignments[i] = s;
//             break;
//           }
//         }
//       }
      
//       int addr = REGISTERED_BOOKS_START + i * (UID_LENGTH + 1);
//       eepromWriteString(addr, uid, UID_LENGTH);
//       return;
//     }
//   }
// }

// void loadRegisteredBooks() {
//   setLCDState(LCD_READY);
//   Serial.println("Loading registered books from EEPROM...");
//   int bookCount = 0;
  
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     registeredUIDs[i] = "";
//     sensorAssignments[i] = 255;
//   }
  
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     int addr = REGISTERED_BOOKS_START + i * (UID_LENGTH + 1);
//     String uid = eepromReadString(addr, UID_LENGTH);
    
//     if (uid.length() == UID_LENGTH) {
//       bool validHex = true;
//       for (unsigned int j = 0; j < uid.length(); j++) {
//         char c = uid.charAt(j);
//         if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) {
//           validHex = false;
//           break;
//         }
//       }
      
//       if (validHex) {
//         registeredUIDs[bookCount] = uid;
//         sensorAssignments[bookCount] = 255;
//         bookCount++;
//       }
//     }
//   }
  
//   if (bookCount == 0) {
//     Serial.println("No registered books found in EEPROM");
//   } else {
//     Serial.print("Loaded ");
//     Serial.print(bookCount);
//     Serial.println(" registered books (sensors not assigned yet)");
//   }
// }

// // ==================== SENSOR FUNCTIONS ====================
// String readSensor(int pin) {
//   static String lastState[NUM_SENSORS];
//   static unsigned long lastChangeTime[NUM_SENSORS];

//   int lowCount = 0;

//   for (int i = 0; i < 50; i++) {
//     if (digitalRead(pin) == LOW) lowCount++;
//     delayMicroseconds(400);
//   }

//   String currentState;

//   if (lowCount > 40) {
//     currentState = "AVL";
//   } else if (lowCount < 10) {
//     currentState = "BRW";
//   } else {
//     int sensorIndex = -1;
//     for (int i = 0; i < NUM_SENSORS; i++) {
//       if (sensorPins[i] == pin) {
//         sensorIndex = i;
//         break;
//       }
//     }
//     if (sensorIndex != -1) {
//       currentState = lastState[sensorIndex];
//     } else {
//       currentState = "BRW";
//     }
//   }

//   for (int i = 0; i < NUM_SENSORS; i++) {
//     if (sensorPins[i] == pin) {
//       lastState[i] = currentState;
//       break;
//     }
//   }

//   return currentState;
// }

// int getSensorIndexForBook(const String &uid) {
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     if (registeredUIDs[i] == uid) {
//       if (sensorAssignments[i] >= 0 && sensorAssignments[i] < NUM_SENSORS) {
//         return sensorAssignments[i];
//       }
//       break;
//     }
//   }
//   return -1;
// }

// int findSensorForBook(const String &uid) {
//   int existingSensor = getSensorIndexForBook(uid);
//   if (existingSensor != -1) {
//     return existingSensor;
//   }
  
//   for (int sensorNum = 0; sensorNum < NUM_SENSORS; sensorNum++) {
//     bool sensorInUse = false;
    
//     for (int j = 0; j < MAX_REGISTERED_BOOKS; j++) {
//       if (sensorAssignments[j] == sensorNum) {
//         sensorInUse = true;
//         break;
//       }
//     }
    
//     if (!sensorInUse) {
//       for (int j = 0; j < MAX_REGISTERED_BOOKS; j++) {
//         if (registeredUIDs[j].length() == 0) {
//           registeredUIDs[j] = uid;
//           sensorAssignments[j] = sensorNum;
          
//           int addr = REGISTERED_BOOKS_START + j * (UID_LENGTH + 1);
//           eepromWriteString(addr, uid, UID_LENGTH);
          
//           Serial.print("ASSIGNED: Book ");
//           Serial.print(uid);
//           Serial.print(" → Sensor ");
//           Serial.print(sensorNum);
//           Serial.print(" (Pin ");
//           Serial.print(sensorPins[sensorNum]);
//           Serial.println(")");
//           return sensorNum;
//         }
//       }
//     }
//   }
  
//   Serial.println("WARNING: All sensors in use! Using sensor 0 as fallback");
//   return 0;
// }

// String getBookStatus(const String &uid) {
//   int sensorIndex = getSensorIndexForBook(uid);

//   if (sensorIndex == -1) {
//     Serial.print("BOOK ");
//     Serial.print(uid);
//     Serial.println(" → No sensor assigned yet, defaulting to AVL");
//     return "AVL";
//   }

//   if (sensorIndex >= 0 && sensorIndex < NUM_SENSORS) {
//     String status = readSensor(sensorPins[sensorIndex]);

//     Serial.print("BOOK ");
//     Serial.print(uid);
//     Serial.print(" → SENSOR INDEX: ");
//     Serial.print(sensorIndex);
//     Serial.print(", PIN: ");
//     Serial.print(sensorPins[sensorIndex]);
//     Serial.print(", STATUS: ");
//     Serial.println(status);

//     return status;
//   }

//   Serial.print("BOOK ");
//   Serial.print(uid);
//   Serial.println(" → Invalid sensor index, defaulting to AVL");
//   return "AVL";
// }

// // ==================== SERIAL COMMUNICATION ====================
// void sendToESP(const String &msg) {
//   while (Serial1.available()) Serial1.read();
//   delay(10);
  
//   Serial1.println(msg);
  
//   Serial.print("TO_ESP: ");
  
//   if (msg.startsWith("SEND:")) {
//     int pipePos = msg.indexOf('|');
//     if (pipePos != -1) {
//       Serial.print(msg.substring(0, pipePos + 1));
//       Serial.print("... [JSON length: ");
//       Serial.print(msg.length() - pipePos - 1);
//       Serial.println("]");
//     } else {
//       int showLen = min(msg.length(), 60);
//       Serial.println(msg.substring(0, showLen));
//     }
//   } else {
//     Serial.println(msg);
//   }
  
//   if (msg.length() > 200) {
//     delay(50);
//   }
// }

// bool waitForResponse(String &response, unsigned long timeout) {
//   unsigned long start = millis();
//   response = "";
  
//   while (millis() - start < timeout) {
//     while (Serial1.available()) {
//       char c = Serial1.read();
//       if (c == '\n') {
//         response.trim();
//         if (response.length() > 0) {
//           if (!response.startsWith("DEBUG:") && !response.startsWith("WIFI:")) {
//             Serial.print("FROM_ESP: ");
//             Serial.println(response);
//           }
//           return true;
//         }
//       } else if (c != '\r' && response.length() < 2048) {
//         response += c;
//       }
//     }
//     delay(1);
//   }
//   return false;
// }

// // ==================== PENDING QUEUE ====================
// int pendingIndexAddr(int index) { 
//   return PENDING_START + index * PENDING_ENTRY_SIZE; 
// }

// bool addPendingEntry(const String &payload) {
//   if (!payload.startsWith("SEND:")) {
//     return false;
//   }
  
//   int pipePos = payload.indexOf('|');
//   if (pipePos != -1) {
//     String jsonPart = payload.substring(pipePos + 1);
//     if (!isValidJSON(jsonPart)) {
//       return false;
//     }
//   }
  
//   for (int i = 0; i < PENDING_MAX; i++) {
//     int addr = pendingIndexAddr(i);
//     if (EEPROM.read(addr) == 0xFF) {
//       eepromWriteString(addr, payload, PENDING_ENTRY_SIZE - 1);
//       Serial.print("QUEUED: ");
      
//       int showLen = min(payload.length(), 60);
//       Serial.println(payload.substring(0, showLen));
      
//       return true;
//     }
//   }
//   Serial.println("QUEUE FULL!");
//   return false;
// }

// bool popPendingEntry(String &outPayload) {
//   for (int i = 0; i < PENDING_MAX; i++) {
//     int addr = pendingIndexAddr(i);
//     if (EEPROM.read(addr) != 0xFF && EEPROM.read(addr) != 0x00) {
//       outPayload = eepromReadString(addr, PENDING_ENTRY_SIZE - 1);
      
//       for (int j = 0; j < PENDING_ENTRY_SIZE; j++) {
//         EEPROM.write(addr + j, 0xFF);
//       }
      
//       if (outPayload.length() > 10 && 
//           outPayload.startsWith("SEND:") && 
//           outPayload.indexOf('|') > 5) {
        
//         int pipePos = outPayload.indexOf('|');
//         String jsonPart = outPayload.substring(pipePos + 1);
//         if (isValidJSON(jsonPart)) {
//           return true;
//         }
//       }
//     }
//   }
//   return false;
// }

// void flushPendingQueue() {
//   if (!wifiConnected) return;
  
//   String payload;
//   int flushed = 0;
  
//   while (popPendingEntry(payload)) {
//     if (payload.length() > 10) {
//       if (!payload.startsWith("SEND:")) {
//         continue;
//       }
      
//       Serial.print("FLUSHING: ");
//       Serial.println(payload.substring(0, 60));
//       sendToESP(payload);
      
//       String response;
//       if (waitForResponse(response, REQUEST_TIMEOUT)) {
//         if (response.startsWith("SUCCESS:")) {
//           flushed++;
//           delay(100);
//         } else {
//           addPendingEntry(payload);
//           break;
//         }
//       } else {
//         addPendingEntry(payload);
//         break;
//       }
//     }
//   }
  
//   if (flushed > 0) {
//     Serial.print("Successfully flushed ");
//     Serial.print(flushed);
//     Serial.println(" queued entries");
//   }
// }

// // ==================== RFID ====================
// String readRFID() {
//   if (!mfrc522.PICC_IsNewCardPresent()) return "";
//   if (!mfrc522.PICC_ReadCardSerial()) return "";
  
//   String uid = "";
//   for (byte i = 0; i < mfrc522.uid.size; i++) {
//     if (mfrc522.uid.uidByte[i] < 0x10) uid += "0";
//     uid += String(mfrc522.uid.uidByte[i], HEX);
//   }
//   uid.toUpperCase();
//   mfrc522.PICC_HaltA();
//   return uid;
// }

// // ==================== MAIN PROCESSING ====================
// void processCard(const String &cardUID) {
//   if (cardUID == lastCardUID && millis() - lastCardTime < DEBOUNCE_TIME) {
//     return;
//   }
  
//   lastCardUID = cardUID;
//   lastCardTime = millis();
//   setLCDState(LCD_CARD_DETECTED);
  
//   int sensorIndex = findSensorForBook(cardUID);
//   String sensorStatus = "AVL";
//   if (sensorIndex >= 0 && sensorIndex < NUM_SENSORS) {
//     sensorStatus = readSensor(sensorPins[sensorIndex]);
//   }
  
//   Serial.println("=================================");
//   Serial.print("CARD: ");
//   Serial.println(cardUID);
//   Serial.print("SENSOR INDEX: ");
//   Serial.println(sensorIndex);
//   Serial.print("SENSOR STATUS: ");
//   Serial.println(sensorStatus);
//   Serial.println("=================================");
  
//   bool serverIsRegistered = false;
//   String serverTitle = "";
//   String serverAuthor = "";
//   String serverLocation = "";
//   String serverEspStatus = "AVL";
//   int serverSensor = 0;
//   bool usingCachedData = false;
  
//   if (wifiConnected) {
//     sendToESP("GET:" + cardUID);
//     String response;
//     if (waitForResponse(response, REQUEST_TIMEOUT)) {
//       if (response.startsWith("BOOKDATA:")) {
//         int p = response.indexOf('|');
//         if (p != -1) {
//           String serverData = response.substring(p + 1);
          
//           if (serverData != "null") {
//             JsonDocument doc;
//             DeserializationError err = deserializeJson(doc, serverData);
//             if (!err) {
//               serverIsRegistered = doc["isRegistered"] | false;
//               serverTitle = doc["title"] | "";
//               serverAuthor = doc["author"] | "";
//               serverLocation = doc["location"] | "";
//               serverSensor = doc["sensor"] | 0;
              
//               if (doc["espStatus"]) {
//                 serverEspStatus = String(doc["espStatus"].as<const char*>());
//               }
              
//               Serial.print("FIREBASE: Registered=");
//               Serial.print(serverIsRegistered ? "YES" : "NO");
//               Serial.print(", Status=");
//               Serial.println(serverEspStatus);
              
//               cacheBookData(cardUID, serverData);
              
//               if (serverIsRegistered && !isBookRegistered(cardUID)) {
//                 saveRegisteredBook(cardUID, serverSensor);
//               }
//             }
//           } else {
//             Serial.println("Book not found in Firebase");
//           }
//         }
//       }
//     } else {
//       wifiConnected = false;
//       setLCDState(LCD_OFFLINE);
//     }
//   }
  
//   if (!wifiConnected || serverTitle.length() == 0) {
//     String cachedData = getCachedBookData(cardUID);
//     if (cachedData.length() > 0) {
//       JsonDocument doc;
//       DeserializationError err = deserializeJson(doc, cachedData);
//       if (!err) {
//         serverIsRegistered = doc["isRegistered"] | false;
//         serverTitle = doc["title"] | "";
//         serverAuthor = doc["author"] | "";
//         serverLocation = doc["location"] | "";
//         serverSensor = doc["sensor"] | 0;
        
//         if (doc["espStatus"]) {
//           serverEspStatus = String(doc["espStatus"].as<const char*>());
//         }
        
//         usingCachedData = true;
//         Serial.println("USING CACHED DATA");
//       }
//     }
//   }
  
//   if (serverTitle.length() == 0) {
//     if (isBookRegistered(cardUID)) {
//       serverTitle = "Book Title Unknown";
//       serverAuthor = "Author Unknown";
//       serverLocation = "Location Unknown";
//       serverIsRegistered = true;
//       Serial.println("Using local registration (no cache)");
//     } else {
//       serverTitle = "New Book";
//       serverAuthor = "Unknown";
//       serverLocation = "Unassigned";
//       serverIsRegistered = false;
//       setLCDState(LCD_NEW_BOOK);
//       Serial.println("New unregistered book");
//       delay(3000);
//       setLCDState(LCD_READY);
//       return;
//     }
//   }
  
//   // Update global variables for LCD display
//   currentBookTitle = serverTitle;
//   currentBookAuthor = serverAuthor;
//   currentBookLocation = serverLocation;
//   currentBookStatus = sensorStatus;
  
//   // Show book info on LCD
//   setLCDState(LCD_BOOK_INFO);
  
//   JsonDocument doc;
  
//   doc["title"] = serverTitle;
//   doc["author"] = serverAuthor;
//   doc["location"] = serverLocation;
//   doc["isRegistered"] = serverIsRegistered;
//   doc["lastSeen"] = String(millis() / 1000);
//   doc["sensor"] = sensorIndex;
//   doc["espStatus"] = sensorStatus;
  
//   if (sensorStatus == "AVL") doc["status"] = "Available";
//   else if (sensorStatus == "BRW") doc["status"] = "Borrowed";
//   else if (sensorStatus == "OVD") doc["status"] = "Overdue";
//   else if (sensorStatus == "MIS") doc["status"] = "Misplaced";
//   else doc["status"] = "Available";
  
//   doc["lastUpdated"] = String(millis() / 1000);
  
//   if (usingCachedData) {
//     doc["source"] = "cached";
//   }
  
//   String outJson;
//   serializeJson(doc, outJson);
  
//   Serial.print("JSON SIZE: ");
//   Serial.println(outJson.length());
  
//   if (!outJson.endsWith("}")) {
//     outJson += "}";
//   }
  
//   String sendCmd = "SEND:/books/" + cardUID + "|" + outJson;
  
//   if (wifiConnected) {
//     sendToESP(sendCmd);
    
//     String response;
//     if (waitForResponse(response, REQUEST_TIMEOUT)) {
//       if (response.startsWith("SUCCESS:")) {
//         cacheBookData(cardUID, outJson);
        
//         if (serverIsRegistered && !isBookRegistered(cardUID)) {
//           saveRegisteredBook(cardUID, sensorIndex);
//         }
//         Serial.println("Firebase update OK");
//       } else {
//         addPendingEntry(sendCmd);
//       }
//     } else {
//       addPendingEntry(sendCmd);
//     }
//   } else {
//     addPendingEntry(sendCmd);
//     Serial.println("WiFi offline - queued");
//   }
// }

// // ==================== SETUP ====================
// void setup() {
//   Serial.begin(115200);
//   Serial1.begin(115200);
  
//   // Initialize LCD
//   lcd.init();
//   lcd.backlight();
//   lcd.clear();
  
//   // Initialize buzzer pin
//   pinMode(BUZZER_PIN, OUTPUT);
//   digitalWrite(BUZZER_PIN, LOW);
  
//   delay(2000);
//   while (Serial1.available()) Serial1.read();
  
//   Serial.println();
//   Serial.println("LIBRARY SYSTEM - MEGA 2560 WITH OFFLINE CACHE");
//   Serial.println("==============================================");
  
//   for (int i = 0; i < NUM_SENSORS; i++) {
//     pinMode(sensorPins[i], INPUT_PULLUP);
//   }
  
//   // Check for EEPROM corruption
//   if (isEEPROMCorrupted()) {
//     setLCDState(LCD_CORRUPTED_DATA);
//     Serial.println("EEPROM corrupted - clearing all data");
//     delay(3000);
//     clearCorruptedEEPROM();
//   }
  
//   loadRegisteredBooks();
  
//   SPI.begin();
//   mfrc522.PCD_Init();
//   delay(200);
  
//   Serial.println("Testing ESP connection...");
//   wifiConnected = false;
  
//   for (int i = 0; i < 3; i++) {
//     sendToESP("PING");
//     String response;
//     if (waitForResponse(response, 2000)) {
//       if (response == "PONG") {
//         wifiConnected = true;
//         Serial.println("ESP connected");
//         beep(BEEP_WIFI_CONNECT, 200);
//         break;
//       }
//     }
//     delay(1000);
//   }
  
//   if (!wifiConnected) {
//     Serial.println("ESP not responding - OFFLINE MODE");
//     beep(BEEP_WIFI_DISCONNECT, 200);
//     setLCDState(LCD_OFFLINE);
//   } else {
//     flushPendingQueue();
//     setLCDState(LCD_WELCOME);
//   }
  
//   Serial.println("System Ready - Sensors will be assigned sequentially");
// }

// // ==================== LOOP ====================
// void loop() {
//   // Update LCD animations
//   updateLCD();
  
//   // Read RFID
//   String uid = readRFID();
//   if (uid.length() == 8) {
//     processCard(uid);
//   }
  
//   // Handle ESP messages
//   static String espBuffer = "";
//   while (Serial1.available()) {
//     char c = Serial1.read();
//     if (c == '\n') {
//       espBuffer.trim();
//       if (espBuffer.length() > 0) {
//         if (espBuffer.startsWith("WIFI:")) {
//           if (espBuffer == "WIFI:OK") {
//             if (!wifiConnected) {
//               wifiConnected = true;
//               Serial.println("WiFi connected - FLUSHING QUEUE");
//               beep(BEEP_WIFI_CONNECT, 200);
//               flushPendingQueue();
//               setLCDState(LCD_READY);
//             }
//           } else if (espBuffer == "WIFI:OFFLINE") {
//             wifiConnected = false;
//             Serial.println("WiFi offline - CACHE MODE");
//             beep(BEEP_WIFI_DISCONNECT, 200);
//             setLCDState(LCD_OFFLINE);
//           }
//         } else if (espBuffer == "PONG") {
//           wifiConnected = true;
//         }
//       }
//       espBuffer = "";
//     } else if (c != '\r') {
//       espBuffer += c;
//     }
//   }
  
//   // Periodic ping
//   static unsigned long lastPing = 0;
//   if (millis() - lastPing > PING_INTERVAL) {
//     lastPing = millis();
//     sendToESP("PING");
//     String response;
//     if (waitForResponse(response, 3000)) {
//       if (response == "PONG" && !wifiConnected) {
//         wifiConnected = true;
//         Serial.println("WiFi restored - FLUSHING QUEUE");
//         beep(BEEP_WIFI_CONNECT, 200);
//         flushPendingQueue();
//         setLCDState(LCD_READY);
//       }
//     } else if (wifiConnected) {
//       wifiConnected = false;
//       Serial.println("WiFi lost - CACHE MODE");
//       beep(BEEP_WIFI_DISCONNECT, 200);
//       setLCDState(LCD_OFFLINE);
//     }
//   }
  
//   delay(10);
// }


// #include <Arduino.h>
// #include <SPI.h>
// #include <MFRC522.h>
// #include <EEPROM.h>
// #include <ArduinoJson.h>
// #include <LiquidCrystal_I2C.h>

// // ==================== LCD & BEEPER CONFIG ====================
// #define LCD_I2C_ADDR 0x27  // Default I2C address for 20x4 LCD
// #define LCD_COLS 20
// #define LCD_ROWS 4

// #define BUZZER_PIN 48
// #define BEEP_SHORT 100
// #define BEEP_MEDIUM 200
// #define BEEP_LONG 400

// // LCD Animation frames for scanning
// const char* scanningFrames[] = {
//     "[Scanning   ]",
//     "[Scanning.  ]",
//     "[Scanning.. ]",
//     "[Scanning...]"
// };

// // LCD Screens
// enum LCDScreen {
//     SCREEN_WELCOME,
//     SCREEN_READY,
//     SCREEN_CARD_DETECTED,
//     SCREEN_BOOK_INFO,
//     SCREEN_NEW_BOOK,
//     SCREEN_OFFLINE,
//     SCREEN_WIFI_CONNECT,
//     SCREEN_WIFI_DISCONNECT,
//     SCREEN_EEPROM_CLEAR,
//     SCREEN_CORRUPTED
// };

// // ==================== CONFIG ====================
// #define RC522_SS_PIN 53
// #define RC522_RST_PIN 49

// #define REGISTERED_BOOKS_START 0
// #define MAX_REGISTERED_BOOKS 50
// #define UID_LENGTH 8

// #define BOOK_CACHE_START (REGISTERED_BOOKS_START + MAX_REGISTERED_BOOKS * (UID_LENGTH + 1))
// #define BOOK_CACHE_SIZE 512
// #define MAX_CACHED_BOOKS 20

// #define PENDING_START (BOOK_CACHE_START + MAX_CACHED_BOOKS * BOOK_CACHE_SIZE)
// #define PENDING_ENTRY_SIZE 768
// #define PENDING_MAX 8

// #define DEBOUNCE_TIME 3000UL
// #define REQUEST_TIMEOUT 8000UL
// #define PING_INTERVAL 30000UL

// #define NUM_SENSORS 15
// const byte sensorPins[NUM_SENSORS] = {22,23,24,25,26,27,28,29,30,31,32,33,34,35,36};

// // ==================== GLOBAL VARIABLES ====================
// MFRC522 mfrc522(RC522_SS_PIN, RC522_RST_PIN);
// LiquidCrystal_I2C lcd(LCD_I2C_ADDR, LCD_COLS, LCD_ROWS);

// bool wifiConnected = false;
// String lastCardUID = "";
// unsigned long lastCardTime = 0;
// String registeredUIDs[MAX_REGISTERED_BOOKS];
// byte sensorAssignments[MAX_REGISTERED_BOOKS];

// // LCD Display Variables
// LCDScreen currentScreen = SCREEN_WELCOME;
// LCDScreen lastScreen = SCREEN_WELCOME;
// String screenTitle = "";
// String screenLine1 = "";
// String screenLine2 = "";
// String screenLine3 = "";
// unsigned long screenTimeout = 0;
// bool screenChanged = true;
// uint8_t scanningFrame = 0;
// unsigned long lastAnimUpdate = 0;
// const unsigned long ANIM_INTERVAL = 300;

// // ==================== FUNCTION PROTOTYPES ====================
// void beep(int duration, int count, int pause);
// void beepSuccess();
// void beepError();
// void beepCardSwipe();
// void beepAvailable();
// void beepBorrowed();
// void beepWifiConnect();
// void beepWifiDisconnect();
// void beepEepromClear();
// void updateLCD();
// void changeScreen(LCDScreen newScreen);
// void showBookInfo(String title, String author, String location, String status, int sensor);
// void showTemporaryScreen(LCDScreen screen, unsigned long duration);
// String eepromReadString(int addr, int maxLen);
// void eepromWriteString(int addr, const String &str, int maxLen);
// bool isValidJSON(const String &json);
// bool isEEPROMCorrupted();
// void clearCorruptedEEPROM();
// int getBookCacheAddr(const String &uid);
// void cacheBookData(const String &uid, const String &jsonData);
// String getCachedBookData(const String &uid);
// bool isBookRegistered(const String &uid);
// int getBookIndex(const String &uid);
// void saveRegisteredBook(const String &uid, int sensorIndex);
// void loadRegisteredBooks();
// String readSensor(int pin);
// int getSensorIndexForBook(const String &uid);
// int findSensorForBook(const String &uid);
// String getBookStatus(const String &uid);
// void sendToESP(const String &msg);
// bool waitForResponse(String &response, unsigned long timeout);
// int pendingIndexAddr(int index);
// bool addPendingEntry(const String &payload);
// bool popPendingEntry(String &outPayload);
// void flushPendingQueue();
// String readRFID();
// void processCard(const String &cardUID);

// // ==================== BEEPER FUNCTIONS ====================
// void beep(int duration, int count, int pause) {
//     for (int i = 0; i < count; i++) {
//         digitalWrite(BUZZER_PIN, HIGH);
//         delay(duration);
//         digitalWrite(BUZZER_PIN, LOW);
//         if (i < count - 1) delay(pause);
//     }
// }

// void beepSuccess() {
//     beep(BEEP_SHORT, 1, 100);
// }

// void beepError() {
//     beep(BEEP_LONG, 1, 100);
// }

// void beepCardSwipe() {
//     beep(BEEP_SHORT, 2, 50);
// }

// void beepAvailable() {
//     beep(BEEP_SHORT, 1, 100);
// }

// void beepBorrowed() {
//     beep(BEEP_SHORT, 2, 80);
// }

// void beepWifiConnect() {
//     for (int i = 0; i < 3; i++) {
//         beep(BEEP_SHORT + (i * 50), 1, 100);
//         delay(50);
//     }
// }

// void beepWifiDisconnect() {
//     for (int i = 2; i >= 0; i--) {
//         beep(BEEP_SHORT + (i * 50), 1, 100);
//         delay(50);
//     }
// }

// void beepEepromClear() {
//     beep(BEEP_LONG, 3, 200);
// }

// // ==================== LCD FUNCTIONS ====================
// void changeScreen(LCDScreen newScreen) {
//     if (currentScreen != newScreen) {
//         lastScreen = currentScreen;
//         currentScreen = newScreen;
//         screenChanged = true;
//         lastAnimUpdate = millis();
//     }
// }

// void updateLCD() {
//     unsigned long currentTime = millis();
    
//     if (screenChanged) {
//         lcd.clear();
//         screenChanged = false;
        
//         lcd.setCursor(0, 0);
//         for (int i = 0; i < LCD_COLS; i++) {
//             lcd.print("-");
//         }
        
//         lcd.setCursor(0, 3);
//         for (int i = 0; i < LCD_COLS; i++) {
//             lcd.print("-");
//         }
//     }
    
//     switch (currentScreen) {
//         case SCREEN_WELCOME:
//             lcd.setCursor(0, 1);
//             lcd.print("  LIBRARY SYSTEM  ");
//             lcd.setCursor(0, 2);
//             lcd.print("     v5.0         ");
//             break;
            
//         case SCREEN_READY:
//             lcd.setCursor(0, 1);
//             lcd.print("READY             ");
            
//             lcd.setCursor(0, 2);
//             if (wifiConnected) {
//                 lcd.print("WiFi: ONLINE      ");
//             } else {
//                 lcd.print("WiFi: OFFLINE     ");
//             }
//             break;
            
//         case SCREEN_CARD_DETECTED:
//             if (currentTime - lastAnimUpdate > ANIM_INTERVAL) {
//                 scanningFrame = (scanningFrame + 1) % 4;
//                 lastAnimUpdate = currentTime;
                
//                 lcd.setCursor(0, 1);
//                 lcd.print("CARD DETECTED!    ");
                
//                 lcd.setCursor(0, 2);
//                 lcd.print("UID: ");
//                 if (lastCardUID.length() >= 8) {
//                     lcd.print(lastCardUID.substring(0, 8));
//                 } else {
//                     lcd.print(lastCardUID);
//                 }
//                 lcd.print("      ");
                
//                 lcd.setCursor(0, 1);
//                 lcd.print("                  ");
//                 lcd.setCursor(0, 1);
//                 lcd.print(scanningFrames[scanningFrame]);
//             }
//             break;
            
//         case SCREEN_BOOK_INFO:
//             lcd.setCursor(0, 0);
//             lcd.print("-------------------");
            
//             lcd.setCursor(0, 1);
//             if (screenTitle.length() > 19) {
//                 lcd.print(screenTitle.substring(0, 17) + "..");
//             } else {
//                 lcd.print(screenTitle);
//                 int spaces = 20 - screenTitle.length();
//                 for (int i = 0; i < spaces; i++) lcd.print(" ");
//             }
            
//             lcd.setCursor(0, 2);
//             if (screenLine1.length() > 19) {
//                 lcd.print(screenLine1.substring(0, 17) + "..");
//             } else {
//                 lcd.print(screenLine1);
//                 int spaces = 20 - screenLine1.length();
//                 for (int i = 0; i < spaces; i++) lcd.print(" ");
//             }
            
//             lcd.setCursor(0, 3);
//             if (screenLine2.length() > 19) {
//                 lcd.print(screenLine2.substring(0, 17) + "..");
//             } else {
//                 lcd.print(screenLine2);
//                 int spaces = 20 - screenLine2.length();
//                 for (int i = 0; i < spaces; i++) lcd.print(" ");
//             }
//             break;
            
//         case SCREEN_NEW_BOOK:
//             lcd.setCursor(0, 1);
//             lcd.print("NEW BOOK DETECTED ");
            
//             lcd.setCursor(0, 2);
//             lcd.print("UID: ");
//             if (lastCardUID.length() >= 8) {
//                 lcd.print(lastCardUID.substring(0, 8));
//             }
//             lcd.print("      ");
//             break;
            
//         case SCREEN_OFFLINE:
//             lcd.setCursor(0, 1);
//             lcd.print("OFFLINE MODE      ");
            
//             lcd.setCursor(0, 2);
//             lcd.print("Data cached       ");
//             break;
            
//         case SCREEN_WIFI_CONNECT:
//             lcd.setCursor(0, 1);
//             lcd.print("WiFi CONNECTED    ");
            
//             lcd.setCursor(0, 2);
//             lcd.print("Synchronizing...  ");
//             break;
            
//         case SCREEN_WIFI_DISCONNECT:
//             lcd.setCursor(0, 1);
//             lcd.print("WiFi DISCONNECTED ");
            
//             lcd.setCursor(0, 2);
//             lcd.print("Cache Mode Active ");
//             break;
            
//         case SCREEN_EEPROM_CLEAR:
//             lcd.setCursor(0, 1);
//             lcd.print("EEPROM CLEARING   ");
            
//             lcd.setCursor(0, 2);
//             lcd.print("Please wait...    ");
//             break;
            
//         case SCREEN_CORRUPTED:
//             lcd.setCursor(0, 1);
//             lcd.print("CORRUPTED DATA    ");
            
//             lcd.setCursor(0, 2);
//             lcd.print("Clearing EEPROM   ");
//             break;
//     }
    
//     if (screenTimeout > 0 && currentTime > screenTimeout) {
//         changeScreen(SCREEN_READY);
//         screenTimeout = 0;
//     }
// }

// void showBookInfo(String title, String author, String location, String status, int sensor) {
//     screenTitle = title;
//     screenLine1 = author;
//     screenLine2 = "LOC:" + location;
//     screenLine3 = "Sensor:" + String(sensor) + " " + status;
    
//     changeScreen(SCREEN_BOOK_INFO);
//     screenTimeout = millis() + 5000;
    
//     if (status == "AVL") {
//         beepAvailable();
//     } else if (status == "BRW") {
//         beepBorrowed();
//     }
// }

// void showTemporaryScreen(LCDScreen screen, unsigned long duration) {
//     changeScreen(screen);
//     screenTimeout = millis() + duration;
// }

// // ==================== EEPROM HELPERS ====================
// String eepromReadString(int addr, int maxLen) {
//     char buffer[maxLen + 1];
//     int i;
//     for (i = 0; i < maxLen; i++) {
//         byte c = EEPROM.read(addr + i);
//         if (c == 0x00) {
//             break;
//         }
//         if (c == 0x01) {
//             buffer[i] = (char)0x00;
//         } else {
//             buffer[i] = (char)c;
//         }
//     }
//     buffer[i] = '\0';
//     return String(buffer);
// }

// void eepromWriteString(int addr, const String &str, int maxLen) {
//     unsigned int len = min((unsigned int)str.length(), (unsigned int)(maxLen - 1));
    
//     for (unsigned int i = 0; i < len; i++) {
//         char c = str.charAt(i);
//         if (c == (char)0x00) {
//             EEPROM.write(addr + i, 0x01);
//         } else {
//             EEPROM.write(addr + i, (byte)c);
//         }
//     }
    
//     EEPROM.write(addr + len, 0x00);
    
//     for (unsigned int i = len + 1; i < (unsigned int)maxLen; i++) {
//         EEPROM.write(addr + i, 0xFF);
//     }
// }

// // ==================== JSON VALIDATION ====================
// bool isValidJSON(const String &json) {
//     if (json.length() < 10) return false;
//     if (!json.startsWith("{")) return false;
//     if (!json.endsWith("}")) return false;
    
//     int braceCount = 0;
//     bool inString = false;
    
//     for (unsigned int i = 0; i < json.length(); i++) {
//         char c = json.charAt(i);
        
//         if (c == '\\' && i + 1 < json.length()) {
//             i++;
//             continue;
//         }
        
//         if (c == '"') {
//             inString = !inString;
//         }
        
//         if (!inString) {
//             if (c == '{') braceCount++;
//             else if (c == '}') braceCount--;
            
//             if (braceCount < 0) return false;
//         }
//     }
    
//     return (braceCount == 0 && !inString);
// }

// // ==================== CORRUPTION DETECTION ====================
// bool isEEPROMCorrupted() {
//     bool allEmpty = true;
//     for (int i = 0; i < 20; i++) {
//         if (EEPROM.read(REGISTERED_BOOKS_START + i) != 0xFF) {
//             allEmpty = false;
//             break;
//         }
//     }
    
//     if (allEmpty) {
//         return false;
//     }
    
//     for (int i = 0; i < 5; i++) {
//         int addr = REGISTERED_BOOKS_START + i * (UID_LENGTH + 1);
        
//         bool slotEmpty = true;
//         for (int j = 0; j < UID_LENGTH; j++) {
//             byte c = EEPROM.read(addr + j);
//             if (c != 0xFF && c != 0x00) {
//                 slotEmpty = false;
//                 break;
//             }
//         }
        
//         if (slotEmpty) continue;
        
//         String uid = eepromReadString(addr, UID_LENGTH);
        
//         if (uid.length() == UID_LENGTH) {
//             bool validHex = true;
//             for (unsigned int j = 0; j < uid.length(); j++) {
//                 char c = uid.charAt(j);
//                 if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) {
//                     validHex = false;
//                     break;
//                 }
//             }
            
//             if (!validHex) {
//                 return true;
//             }
//         } else if (uid.length() > 0 && uid.length() < UID_LENGTH) {
//             return true;
//         }
//     }
    
//     return false;
// }

// void clearCorruptedEEPROM() {
//     showTemporaryScreen(SCREEN_CORRUPTED, 3000);
//     beepEepromClear();
    
//     Serial.println("Clearing corrupted EEPROM data...");
    
//     for (int i = 0; i < MAX_REGISTERED_BOOKS * (UID_LENGTH + 1); i++) {
//         EEPROM.write(REGISTERED_BOOKS_START + i, 0xFF);
//     }
    
//     for (int i = 0; i < MAX_CACHED_BOOKS * BOOK_CACHE_SIZE; i++) {
//         EEPROM.write(BOOK_CACHE_START + i, 0xFF);
//     }
    
//     for (int i = 0; i < PENDING_MAX * PENDING_ENTRY_SIZE; i++) {
//         EEPROM.write(PENDING_START + i, 0xFF);
//     }
    
//     showTemporaryScreen(SCREEN_EEPROM_CLEAR, 2000);
//     Serial.println("EEPROM cleared successfully");
// }

// // ==================== BOOK CACHE SYSTEM ====================
// int getBookCacheAddr(const String &uid) {
//     unsigned int hash = 0;
//     for (unsigned int i = 0; i < uid.length(); i++) {
//         hash = (hash * 31) + uid.charAt(i);
//     }
//     int slot = hash % MAX_CACHED_BOOKS;
//     return BOOK_CACHE_START + slot * BOOK_CACHE_SIZE;
// }

// void cacheBookData(const String &uid, const String &jsonData) {
//     if (uid.length() != 8 || jsonData.length() == 0) return;
    
//     if (!isValidJSON(jsonData)) {
//         return;
//     }
    
//     int addr = getBookCacheAddr(uid);
    
//     unsigned int len = jsonData.length();
//     len = min(len, (unsigned int)(BOOK_CACHE_SIZE - 3));
    
//     EEPROM.write(addr, (len >> 8) & 0xFF);
//     EEPROM.write(addr + 1, len & 0xFF);
    
//     for (unsigned int i = 0; i < len; i++) {
//         char c = jsonData.charAt(i);
//         if (c == (char)0x00) {
//             EEPROM.write(addr + 2 + i, 0x01);
//         } else {
//             EEPROM.write(addr + 2 + i, (byte)c);
//         }
//     }
    
//     EEPROM.write(addr + 2 + len, 0xFF);
// }

// String getCachedBookData(const String &uid) {
//     if (uid.length() != 8) return "";
    
//     int addr = getBookCacheAddr(uid);
    
//     int len = (EEPROM.read(addr) << 8) | EEPROM.read(addr + 1);
    
//     if (len > 0 && len <= (BOOK_CACHE_SIZE - 3)) {
//         String cached = "";
//         for (int i = 0; i < len; i++) {
//             byte c = EEPROM.read(addr + 2 + i);
//             if (c == 0x01) {
//                 cached += (char)0x00;
//             } else {
//                 cached += (char)c;
//             }
//         }
        
//         if (isValidJSON(cached)) {
//             return cached;
//         }
//     }
    
//     String cached = eepromReadString(addr, BOOK_CACHE_SIZE - 1);
//     if (isValidJSON(cached)) {
//         return cached;
//     }
    
//     return "";
// }

// // ==================== REGISTERED BOOKS MANAGEMENT ====================
// bool isBookRegistered(const String &uid) {
//     for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//         if (registeredUIDs[i] == uid) return true;
//     }
//     return false;
// }

// int getBookIndex(const String &uid) {
//     for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//         if (registeredUIDs[i] == uid) return i;
//     }
//     return -1;
// }

// void saveRegisteredBook(const String &uid, int sensorIndex) {
//     if (isBookRegistered(uid)) return;
    
//     for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//         if (registeredUIDs[i].length() == 0) {
//             registeredUIDs[i] = uid;
            
//             if (sensorIndex >= 0 && sensorIndex < NUM_SENSORS) {
//                 sensorAssignments[i] = sensorIndex;
//             } else {
//                 for (int s = 0; s < NUM_SENSORS; s++) {
//                     bool sensorUsed = false;
//                     for (int j = 0; j < MAX_REGISTERED_BOOKS; j++) {
//                         if (sensorAssignments[j] == s) {
//                             sensorUsed = true;
//                             break;
//                         }
//                     }
//                     if (!sensorUsed) {
//                         sensorAssignments[i] = s;
//                         break;
//                     }
//                 }
//             }
            
//             int addr = REGISTERED_BOOKS_START + i * (UID_LENGTH + 1);
//             eepromWriteString(addr, uid, UID_LENGTH);
//             return;
//         }
//     }
// }

// void loadRegisteredBooks() {
//     Serial.println("Loading registered books from EEPROM...");
//     int bookCount = 0;
    
//     for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//         registeredUIDs[i] = "";
//         sensorAssignments[i] = 255;
//     }
    
//     for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//         int addr = REGISTERED_BOOKS_START + i * (UID_LENGTH + 1);
//         String uid = eepromReadString(addr, UID_LENGTH);
        
//         if (uid.length() == UID_LENGTH) {
//             bool validHex = true;
//             for (unsigned int j = 0; j < uid.length(); j++) {
//                 char c = uid.charAt(j);
//                 if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) {
//                     validHex = false;
//                     break;
//                 }
//             }
            
//             if (validHex) {
//                 registeredUIDs[bookCount] = uid;
//                 sensorAssignments[bookCount] = 255;
//                 bookCount++;
//             }
//         }
//     }
    
//     if (bookCount == 0) {
//         Serial.println("No registered books found in EEPROM");
//     } else {
//         Serial.print("Loaded ");
//         Serial.print(bookCount);
//         Serial.println(" registered books");
//     }
// }

// // ==================== SENSOR FUNCTIONS ====================
// String readSensor(int pin) {
//     static String lastState[NUM_SENSORS];
//     static unsigned long lastChangeTime[NUM_SENSORS];

//     int lowCount = 0;

//     for (int i = 0; i < 50; i++) {
//         if (digitalRead(pin) == LOW) lowCount++;
//         delayMicroseconds(400);
//     }

//     String currentState;

//     if (lowCount > 40) {
//         currentState = "AVL";
//     } else if (lowCount < 10) {
//         currentState = "BRW";
//     } else {
//         int sensorIndex = -1;
//         for (int i = 0; i < NUM_SENSORS; i++) {
//             if (sensorPins[i] == pin) {
//                 sensorIndex = i;
//                 break;
//             }
//         }
//         if (sensorIndex != -1) {
//             currentState = lastState[sensorIndex];
//         } else {
//             currentState = "BRW";
//         }
//     }

//     for (int i = 0; i < NUM_SENSORS; i++) {
//         if (sensorPins[i] == pin) {
//             lastState[i] = currentState;
//             break;
//         }
//     }

//     return currentState;
// }

// int getSensorIndexForBook(const String &uid) {
//     for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//         if (registeredUIDs[i] == uid) {
//             if (sensorAssignments[i] >= 0 && sensorAssignments[i] < NUM_SENSORS) {
//                 return sensorAssignments[i];
//             }
//             break;
//         }
//     }
//     return -1;
// }

// int findSensorForBook(const String &uid) {
//     int existingSensor = getSensorIndexForBook(uid);
//     if (existingSensor != -1) {
//         return existingSensor;
//     }
    
//     for (int sensorNum = 0; sensorNum < NUM_SENSORS; sensorNum++) {
//         bool sensorInUse = false;
        
//         for (int j = 0; j < MAX_REGISTERED_BOOKS; j++) {
//             if (sensorAssignments[j] == sensorNum) {
//                 sensorInUse = true;
//                 break;
//             }
//         }
        
//         if (!sensorInUse) {
//             for (int j = 0; j < MAX_REGISTERED_BOOKS; j++) {
//                 if (registeredUIDs[j].length() == 0) {
//                     registeredUIDs[j] = uid;
//                     sensorAssignments[j] = sensorNum;
                    
//                     int addr = REGISTERED_BOOKS_START + j * (UID_LENGTH + 1);
//                     eepromWriteString(addr, uid, UID_LENGTH);
                    
//                     Serial.print("ASSIGNED: Book ");
//                     Serial.print(uid);
//                     Serial.print(" → Sensor ");
//                     Serial.println(sensorNum);
//                     return sensorNum;
//                 }
//             }
//         }
//     }
    
//     Serial.println("WARNING: All sensors in use! Using sensor 0 as fallback");
//     return 0;
// }

// String getBookStatus(const String &uid) {
//     int sensorIndex = getSensorIndexForBook(uid);

//     if (sensorIndex == -1) {
//         return "AVL";
//     }

//     if (sensorIndex >= 0 && sensorIndex < NUM_SENSORS) {
//         return readSensor(sensorPins[sensorIndex]);
//     }

//     return "AVL";
// }

// // ==================== SERIAL COMMUNICATION ====================
// void sendToESP(const String &msg) {
//     while (Serial1.available()) Serial1.read();
//     delay(10);
    
//     Serial1.println(msg);
    
//     Serial.print("TO_ESP: ");
    
//     if (msg.startsWith("SEND:")) {
//         int pipePos = msg.indexOf('|');
//         if (pipePos != -1) {
//             Serial.print(msg.substring(0, pipePos + 1));
//             Serial.print("... [JSON length: ");
//             Serial.print(msg.length() - pipePos - 1);
//             Serial.println("]");
//         } else {
//             int showLen = min(msg.length(), 60);
//             Serial.println(msg.substring(0, showLen));
//         }
//     } else {
//         Serial.println(msg);
//     }
    
//     if (msg.length() > 200) {
//         delay(50);
//     }
// }

// bool waitForResponse(String &response, unsigned long timeout) {
//     unsigned long start = millis();
//     response = "";
    
//     while (millis() - start < timeout) {
//         while (Serial1.available()) {
//             char c = Serial1.read();
//             if (c == '\n') {
//                 response.trim();
//                 if (response.length() > 0) {
//                     if (!response.startsWith("DEBUG:") && !response.startsWith("WIFI:")) {
//                         Serial.print("FROM_ESP: ");
//                         Serial.println(response);
//                     }
//                     return true;
//                 }
//             } else if (c != '\r' && response.length() < 2048) {
//                 response += c;
//             }
//         }
//         delay(1);
//     }
//     return false;
// }

// // ==================== PENDING QUEUE ====================
// int pendingIndexAddr(int index) { 
//     return PENDING_START + index * PENDING_ENTRY_SIZE; 
// }

// bool addPendingEntry(const String &payload) {
//     if (!payload.startsWith("SEND:")) {
//         return false;
//     }
    
//     int pipePos = payload.indexOf('|');
//     if (pipePos != -1) {
//         String jsonPart = payload.substring(pipePos + 1);
//         if (!isValidJSON(jsonPart)) {
//             return false;
//         }
//     }
    
//     for (int i = 0; i < PENDING_MAX; i++) {
//         int addr = pendingIndexAddr(i);
//         if (EEPROM.read(addr) == 0xFF) {
//             eepromWriteString(addr, payload, PENDING_ENTRY_SIZE - 1);
            
//             if (!wifiConnected) {
//                 changeScreen(SCREEN_OFFLINE);
//             }
            
//             return true;
//         }
//     }
//     Serial.println("QUEUE FULL!");
//     return false;
// }

// bool popPendingEntry(String &outPayload) {
//     for (int i = 0; i < PENDING_MAX; i++) {
//         int addr = pendingIndexAddr(i);
//         if (EEPROM.read(addr) != 0xFF && EEPROM.read(addr) != 0x00) {
//             outPayload = eepromReadString(addr, PENDING_ENTRY_SIZE - 1);
            
//             for (int j = 0; j < PENDING_ENTRY_SIZE; j++) {
//                 EEPROM.write(addr + j, 0xFF);
//             }
            
//             if (outPayload.length() > 10 && 
//                 outPayload.startsWith("SEND:") && 
//                 outPayload.indexOf('|') > 5) {
                
//                 int pipePos = outPayload.indexOf('|');
//                 String jsonPart = outPayload.substring(pipePos + 1);
//                 if (isValidJSON(jsonPart)) {
//                     return true;
//                 }
//             }
//         }
//     }
//     return false;
// }

// void flushPendingQueue() {
//     if (!wifiConnected) return;
    
//     String payload;
//     int flushed = 0;
    
//     while (popPendingEntry(payload)) {
//         if (payload.length() > 10) {
//             if (!payload.startsWith("SEND:")) {
//                 continue;
//             }
            
//             sendToESP(payload);
            
//             String response;
//             if (waitForResponse(response, REQUEST_TIMEOUT)) {
//                 if (response.startsWith("SUCCESS:")) {
//                     flushed++;
//                     delay(100);
//                 } else {
//                     addPendingEntry(payload);
//                     break;
//                 }
//             } else {
//                 addPendingEntry(payload);
//                 break;
//             }
//         }
//     }
    
//     if (flushed > 0) {
//         Serial.print("Successfully flushed ");
//         Serial.print(flushed);
//         Serial.println(" queued entries");
//     }
// }

// // ==================== RFID ====================
// String readRFID() {
//     if (!mfrc522.PICC_IsNewCardPresent()) return "";
//     if (!mfrc522.PICC_ReadCardSerial()) return "";
    
//     String uid = "";
//     for (byte i = 0; i < mfrc522.uid.size; i++) {
//         if (mfrc522.uid.uidByte[i] < 0x10) uid += "0";
//         uid += String(mfrc522.uid.uidByte[i], HEX);
//     }
//     uid.toUpperCase();
//     mfrc522.PICC_HaltA();
//     return uid;
// }

// // ==================== MAIN PROCESSING ====================
// void processCard(const String &cardUID) {
//     if (cardUID == lastCardUID && millis() - lastCardTime < DEBOUNCE_TIME) {
//         return;
//     }
    
//     lastCardUID = cardUID;
//     lastCardTime = millis();
    
//     beepCardSwipe();
//     changeScreen(SCREEN_CARD_DETECTED);
    
//     int sensorIndex = findSensorForBook(cardUID);
//     String sensorStatus = "AVL";
//     if (sensorIndex >= 0 && sensorIndex < NUM_SENSORS) {
//         sensorStatus = readSensor(sensorPins[sensorIndex]);
//     }
    
//     Serial.println("=================================");
//     Serial.print("CARD: ");
//     Serial.println(cardUID);
//     Serial.print("SENSOR INDEX: ");
//     Serial.println(sensorIndex);
//     Serial.print("SENSOR STATUS: ");
//     Serial.println(sensorStatus);
//     Serial.println("=================================");
    
//     bool serverIsRegistered = false;
//     String serverTitle = "";
//     String serverAuthor = "";
//     String serverLocation = "";
//     String serverEspStatus = "AVL";
//     int serverSensor = 0;
//     bool usingCachedData = false;
    
//     if (wifiConnected) {
//         sendToESP("GET:" + cardUID);
//         String response;
//         if (waitForResponse(response, REQUEST_TIMEOUT)) {
//             if (response.startsWith("BOOKDATA:")) {
//                 int p = response.indexOf('|');
//                 if (p != -1) {
//                     String serverData = response.substring(p + 1);
                    
//                     if (serverData != "null") {
//                         JsonDocument doc;
//                         DeserializationError err = deserializeJson(doc, serverData);
//                         if (!err) {
//                             serverIsRegistered = doc["isRegistered"] | false;
//                             serverTitle = String(doc["title"] | "");
//                             serverAuthor = String(doc["author"] | "");
//                             serverLocation = String(doc["location"] | "");
//                             serverSensor = doc["sensor"] | 0;
                            
//                             if (doc["espStatus"].is<String>()) {
//                                 serverEspStatus = doc["espStatus"].as<String>();
//                             }
                            
//                             cacheBookData(cardUID, serverData);
                            
//                             if (serverIsRegistered && !isBookRegistered(cardUID)) {
//                                 saveRegisteredBook(cardUID, serverSensor);
//                             }
//                         }
//                     } else {
//                         showTemporaryScreen(SCREEN_NEW_BOOK, 3000);
//                         Serial.println("New book detected - please register");
//                     }
//                 }
//             }
//         } else {
//             wifiConnected = false;
//         }
//     }
    
//     if (!wifiConnected || serverTitle.length() == 0) {
//         String cachedData = getCachedBookData(cardUID);
//         if (cachedData.length() > 0) {
//             JsonDocument doc;
//             DeserializationError err = deserializeJson(doc, cachedData);
//             if (!err) {
//                 serverIsRegistered = doc["isRegistered"] | false;
//                 serverTitle = String(doc["title"] | "");
//                 serverAuthor = String(doc["author"] | "");
//                 serverLocation = String(doc["location"] | "");
//                 serverSensor = doc["sensor"] | 0;
                
//                 if (doc["espStatus"].is<String>()) {
//                     serverEspStatus = doc["espStatus"].as<String>();
//                 }
                
//                 usingCachedData = true;
//             }
//         }
//     }
    
//     if (serverTitle.length() == 0) {
//         if (isBookRegistered(cardUID)) {
//             serverTitle = "Registered Book";
//             serverAuthor = "Unknown Author";
//             serverLocation = "A0";
//             serverIsRegistered = true;
//         } else {
//             serverTitle = "New Book";
//             serverAuthor = "Unknown";
//             serverLocation = "Unassigned";
//             serverIsRegistered = false;
//             showTemporaryScreen(SCREEN_NEW_BOOK, 3000);
//         }
//     }
    
//     if (serverIsRegistered) {
//         showBookInfo(serverTitle, serverAuthor, serverLocation, sensorStatus, sensorIndex);
//     }
    
//     JsonDocument doc;
    
//     doc["title"] = serverTitle;
//     doc["author"] = serverAuthor;
//     doc["location"] = serverLocation;
//     doc["isRegistered"] = serverIsRegistered;
//     doc["lastSeen"] = String(millis() / 1000);
//     doc["sensor"] = sensorIndex;
//     doc["espStatus"] = sensorStatus;
    
//     if (sensorStatus == "AVL") doc["status"] = "Available";
//     else if (sensorStatus == "BRW") doc["status"] = "Borrowed";
//     else doc["status"] = "Available";
    
//     doc["lastUpdated"] = String(millis() / 1000);
    
//     if (usingCachedData) {
//         doc["source"] = "cached";
//     }
    
//     String outJson;
//     serializeJson(doc, outJson);
    
//     String sendCmd = "SEND:/books/" + cardUID + "|" + outJson;
    
//     if (wifiConnected) {
//         sendToESP(sendCmd);
        
//         String response;
//         if (waitForResponse(response, REQUEST_TIMEOUT)) {
//             if (response.startsWith("SUCCESS:")) {
//                 cacheBookData(cardUID, outJson);
                
//                 if (serverIsRegistered && !isBookRegistered(cardUID)) {
//                     saveRegisteredBook(cardUID, sensorIndex);
//                 }
//                 beepSuccess();
//             } else {
//                 addPendingEntry(sendCmd);
//                 beepError();
//             }
//         } else {
//             addPendingEntry(sendCmd);
//             beepError();
//         }
//     } else {
//         addPendingEntry(sendCmd);
//         beepError();
//     }
// }

// // ==================== SETUP ====================
// void setup() {
//     Serial.begin(115200);
//     Serial1.begin(115200);
    
//     delay(2000);
//     while (Serial1.available()) Serial1.read();
    
//     Serial.println();
//     Serial.println("LIBRARY SYSTEM - MEGA 2560 WITH OFFLINE CACHE");
//     Serial.println("==============================================");
    
//     lcd.init();
//     lcd.backlight();
//     lcd.clear();
//     changeScreen(SCREEN_WELCOME);
    
//     pinMode(BUZZER_PIN, OUTPUT);
//     digitalWrite(BUZZER_PIN, LOW);
    
//     for (int i = 0; i < NUM_SENSORS; i++) {
//         pinMode(sensorPins[i], INPUT_PULLUP);
//     }
    
//     if (isEEPROMCorrupted()) {
//         clearCorruptedEEPROM();
//     } else {
//         delay(2000);
//     }
    
//     loadRegisteredBooks();
    
//     SPI.begin();
//     mfrc522.PCD_Init();
//     delay(200);
    
//     Serial.println("Testing ESP connection...");
//     wifiConnected = false;
    
//     for (int i = 0; i < 3; i++) {
//         sendToESP("PING");
//         String response;
//         if (waitForResponse(response, 2000)) {
//             if (response == "PONG") {
//                 wifiConnected = true;
//                 Serial.println("ESP connected");
//                 beepWifiConnect();
//                 showTemporaryScreen(SCREEN_WIFI_CONNECT, 2000);
//                 break;
//             }
//         }
//         delay(1000);
//     }
    
//     if (!wifiConnected) {
//         Serial.println("ESP not responding - OFFLINE MODE");
//         beepWifiDisconnect();
//         showTemporaryScreen(SCREEN_WIFI_DISCONNECT, 2000);
//     } else {
//         flushPendingQueue();
//     }
    
//     delay(1000);
//     changeScreen(SCREEN_READY);
//     Serial.println("System Ready");
// }

// // ==================== LOOP ====================
// void loop() {
//     updateLCD();
    
//     String uid = readRFID();
//     if (uid.length() == 8) {
//         processCard(uid);
//     }
    
//     static String espBuffer = "";
//     while (Serial1.available()) {
//         char c = Serial1.read();
//         if (c == '\n') {
//             espBuffer.trim();
//             if (espBuffer.length() > 0) {
//                 if (espBuffer.startsWith("WIFI:")) {
//                     if (espBuffer == "WIFI:OK") {
//                         if (!wifiConnected) {
//                             wifiConnected = true;
//                             Serial.println("WiFi connected - FLUSHING QUEUE");
//                             beepWifiConnect();
//                             showTemporaryScreen(SCREEN_WIFI_CONNECT, 2000);
//                             flushPendingQueue();
//                         }
//                     } else if (espBuffer == "WIFI:OFFLINE") {
//                         wifiConnected = false;
//                         Serial.println("WiFi offline - CACHE MODE");
//                         beepWifiDisconnect();
//                         showTemporaryScreen(SCREEN_WIFI_DISCONNECT, 2000);
//                     }
//                 } else if (espBuffer == "PONG") {
//                     wifiConnected = true;
//                 }
//             }
//             espBuffer = "";
//         } else if (c != '\r') {
//             espBuffer += c;
//         }
//     }
    
//     static unsigned long lastPing = 0;
//     if (millis() - lastPing > PING_INTERVAL) {
//         lastPing = millis();
//         sendToESP("PING");
//         String response;
//         if (waitForResponse(response, 3000)) {
//             if (response == "PONG" && !wifiConnected) {
//                 wifiConnected = true;
//                 Serial.println("WiFi restored - FLUSHING QUEUE");
//                 beepWifiConnect();
//                 showTemporaryScreen(SCREEN_WIFI_CONNECT, 2000);
//                 flushPendingQueue();
//             }
//         } else if (wifiConnected) {
//             wifiConnected = false;
//             Serial.println("WiFi lost - CACHE MODE");
//             beepWifiDisconnect();
//             showTemporaryScreen(SCREEN_WIFI_DISCONNECT, 2000);
//         }
//     }
    
//     delay(10);
// }




///// WiFi offline
// WiFi offline  beep not show on screen
// #include <Arduino.h>
// #include <SPI.h>
// #include <MFRC522.h>
// #include <EEPROM.h>
// #include <ArduinoJson.h>
// #include <LiquidCrystal_I2C.h>  // For I2C LCD

// // ==================== LCD CONFIGURATION ====================
// #define LCD_ADDRESS 0x27      // Default I2C address for 20x4 LCD
// #define LCD_COLS 20
// #define LCD_ROWS 4
// LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLS, LCD_ROWS);

// // ==================== BUZZER CONFIGURATION ====================
// #define BUZZER_PIN 48

// // ==================== GLOBAL VARIABLES ====================
// bool wifiConnected = false;
// bool systemReady = false;
// bool lcdInitialized = false;
// String lastCardUID = "";
// unsigned long lastCardTime = 0;
// String registeredUIDs[50];  // MAX_REGISTERED_BOOKS
// byte sensorAssignments[50]; // MAX_REGISTERED_BOOKS

// // ==================== LCD STATES ====================
// enum LCDState {
//   LCD_INIT,
//   LCD_WELCOME,
//   LCD_READY,
//   LCD_CARD_DETECTED,
//   LCD_BOOK_INFO,
//   LCD_OFFLINE,
//   LCD_NEW_BOOK,
//   LCD_CLEARING_EEPROM,
//   LCD_CORRUPTED_DATA
// };

// LCDState currentLCDState = LCD_INIT;
// unsigned long lcdStateStartTime = 0;
// int animationFrame = 0;
// String currentBookTitle = "";
// String currentBookAuthor = "";
// String currentBookLocation = "";
// String currentBookStatus = "";

// // ==================== BEEP TONES ====================
// #define BEEP_AVL 2000    // High pitch for Available
// #define BEEP_BRW 1500    // Medium pitch for Borrowed
// #define BEEP_SWIPE 1000  // Low pitch for card swipe
// #define BEEP_WIFI_CONNECT 2500  // Rising tone for WiFi connect
// #define BEEP_WIFI_DISCONNECT 800  // Falling tone for WiFi disconnect
// #define BEEP_ERROR 300   // Error beep
// #define BEEP_CONFIRM 1800 // Confirmation beep

// // ==================== BUZZER FUNCTIONS ====================
// void beep(int frequency, int duration) {
//   if (frequency <= 0) return;
//   tone(BUZZER_PIN, frequency, duration);
//   // Non-blocking - tone() handles duration
// }

// // ==================== LCD HELPER FUNCTIONS ====================
// void clearLine(int row) {
//   lcd.setCursor(0, row);
//   for (int i = 0; i < LCD_COLS; i++) {
//     lcd.print(" ");
//   }
// }

// void centerText(String text, int row) {
//   int padding = (LCD_COLS - text.length()) / 2;
//   if (padding < 0) padding = 0;
//   lcd.setCursor(padding, row);
//   lcd.print(text);
// }

// void setLCDState(LCDState newState) {
//   if (currentLCDState != newState) {
//     currentLCDState = newState;
//     lcdStateStartTime = millis();
//     animationFrame = 0;
//     if (lcdInitialized) {
//       lcd.clear();
//     }
//   }
// }

// void updateLCD() {
//   static unsigned long lastUpdate = 0;
//   unsigned long currentMillis = millis();
  
//   if (!lcdInitialized) return;
  
//   // Update LCD every 100ms (non-blocking)
//   if (currentMillis - lastUpdate < 100) return;
//   lastUpdate = currentMillis;
  
//   unsigned long stateDuration = currentMillis - lcdStateStartTime;
  
//   switch (currentLCDState) {
//     case LCD_INIT:
//       if (animationFrame == 0) {
//         lcd.clear();
//         centerText("INITIALIZING...", 1);
//         lcd.setCursor(0, 2);
//         lcd.print("LeejinBotics v5.0");
//         animationFrame = 1;
//       }
//       // Auto-proceed to welcome after 1 second
//       if (stateDuration > 1000) {
//         setLCDState(LCD_WELCOME);
//       }
//       break;
      
//     case LCD_WELCOME:
//       if (animationFrame == 0) {
//         lcd.clear();
//         centerText("LIBRARY SYSTEM", 0);
//         centerText("v5.0", 1);
//         centerText("LeejinBotics", 2);
//         beep(BEEP_CONFIRM, 200);
//         animationFrame = 1;
//       }
//       // Auto-proceed to ready after 2 seconds
//       if (stateDuration > 2000) {
//         setLCDState(LCD_READY);
//         systemReady = true;
//       }
//       break;
      
//     case LCD_READY:
//       if (animationFrame == 0) {
//         lcd.clear();
//         centerText("SYSTEM READY", 0);
//         lcd.setCursor(0, 1);
//         lcd.print("Scan a book card...");
//         lcd.setCursor(0, 2);
//         lcd.print("WiFi: ");
//         lcd.print(wifiConnected ? "ONLINE " : "OFFLINE");
        
//         // Create dots animation
//         int dotCount = (stateDuration / 500) % 4;
//         String dots = "";
//         for (int i = 0; i < dotCount && i < 4; i++) {
//           dots += ".";
//         }
//         lcd.setCursor(17, 1);
//         lcd.print(dots);
//         animationFrame = 1;
//       }
//       break;
      
//     case LCD_CARD_DETECTED:
//       if (animationFrame == 0) {
//         lcd.clear();
//         centerText("CARD DETECTED!", 0);
//         lcd.setCursor(0, 1);
//         lcd.print("UID: ");
//         lcd.print(lastCardUID);
//         lcd.setCursor(0, 2);
//         lcd.print("Status: READING");
        
//         // Create reading dots animation
//         int readingDotCount = (stateDuration / 300) % 4;
//         String readingDots = "";
//         for (int i = 0; i < readingDotCount && i < 4; i++) {
//           readingDots += ".";
//         }
//         lcd.setCursor(16, 2);
//         lcd.print(readingDots);
//         beep(BEEP_SWIPE, 150);
//         animationFrame = 1;
//       }
//       // Auto timeout after 3 seconds if stuck
//       if (stateDuration > 3000) {
//         setLCDState(LCD_READY);
//       }
//       break;
      
//     case LCD_BOOK_INFO:
//       if (animationFrame == 0) {
//         lcd.clear();
        
//         // Display title (truncate if too long)
//         String displayTitle = currentBookTitle;
//         if (displayTitle.length() > 20) {
//           displayTitle = displayTitle.substring(0, 17) + "...";
//         }
//         lcd.setCursor(0, 0);
//         lcd.print(displayTitle);
        
//         // Display author (truncate if too long)
//         String displayAuthor = currentBookAuthor;
//         if (displayAuthor.length() > 20) {
//           displayAuthor = displayAuthor.substring(0, 17) + "...";
//         }
//         lcd.setCursor(0, 1);
//         lcd.print(displayAuthor);
        
//         // Display location
//         lcd.setCursor(0, 2);
//         lcd.print("LOC: ");
//         lcd.print(currentBookLocation);
        
//         // Display status
//         lcd.setCursor(0, 3);
//         lcd.print("Status: ");
//         lcd.print(currentBookStatus);
        
//         // Play appropriate beep
//         if (currentBookStatus == "AVL") {
//           beep(BEEP_AVL, 300);
//         } else if (currentBookStatus == "BRW") {
//           beep(BEEP_BRW, 300);
//         }
        
//         animationFrame = 1;
//       }
      
//       // Auto return to ready after 4 seconds
//       if (stateDuration > 4000) {
//         setLCDState(LCD_READY);
//       }
//       break;
      
//     case LCD_OFFLINE:
//       if (animationFrame == 0) {
//         lcd.clear();
//         centerText("OFFLINE MODE", 0);
//         lcd.setCursor(0, 1);
//         lcd.print("Data cached for:");
//         lcd.setCursor(0, 2);
//         if (lastCardUID.length() > 0) {
//           lcd.print(lastCardUID.substring(0, 16));
//         } else {
//           lcd.print("No recent card");
//         }
//         lcd.setCursor(0, 3);
//         lcd.print("Scan to queue...");
        
//         beep(BEEP_WIFI_DISCONNECT, 200);
//         animationFrame = 1;
//       }
//       break;
      
//     case LCD_NEW_BOOK:
//       if (animationFrame == 0) {
//         lcd.clear();
//         centerText("NEW BOOK DETECTED", 0);
//         lcd.setCursor(0, 1);
//         lcd.print("UID: ");
//         lcd.print(lastCardUID.substring(0, 16));
//         lcd.setCursor(0, 2);
//         lcd.print("Assigning Sensor...");
//         lcd.setCursor(0, 3);
//         lcd.print("Please register!");
        
//         // Blinking "NEW" text
//         if ((stateDuration / 500) % 2 == 0) {
//           lcd.setCursor(17, 0);
//           lcd.print("NEW");
//         } else {
//           lcd.setCursor(17, 0);
//           lcd.print("   ");
//         }
        
//         beep(BEEP_SWIPE, 100);
//         delay(50);
//         beep(BEEP_CONFIRM, 100);
//         animationFrame = 1;
//       }
//       // Auto timeout after 3 seconds
//       if (stateDuration > 3000) {
//         setLCDState(LCD_READY);
//       }
//       break;
      
//     case LCD_CLEARING_EEPROM:
//       if (animationFrame == 0) {
//         lcd.clear();
//         centerText("CLEARING EEPROM", 0);
//         lcd.setCursor(0, 1);
//         lcd.print("Please wait...");
        
//         int progress = (stateDuration / 100) % 21;
//         lcd.setCursor(0, 2);
//         for (int i = 0; i < 20; i++) {
//           if (i < progress) {
//             lcd.print("=");
//           } else {
//             lcd.print(" ");
//           }
//         }
        
//         lcd.setCursor(18, 2);
//         if (progress * 5 < 100) {
//           lcd.print(" ");
//         }
//         lcd.print(progress * 5);
//         lcd.print("%");
        
//         beep(BEEP_ERROR, 50);
//         animationFrame = 1;
//       }
//       break;
      
//     case LCD_CORRUPTED_DATA:
//       if (animationFrame == 0) {
//         lcd.clear();
//         centerText("CORRUPTED DATA", 0);
//         lcd.setCursor(0, 1);
//         lcd.print("EEPROM Error!");
//         lcd.setCursor(0, 2);
//         lcd.print("System recovering");
        
//         if ((stateDuration / 250) % 2 == 0) {
//           lcd.setCursor(3, 3);
//           lcd.print("!!! WARNING !!!");
//         } else {
//           clearLine(3);
//         }
        
//         beep(BEEP_ERROR, 100);
//         delay(50);
//         beep(BEEP_ERROR, 100);
//         animationFrame = 1;
//       }
//       break;
//   }
// }

// // ==================== ORIGINAL CONFIG ====================
// #define RC522_SS_PIN 53
// #define RC522_RST_PIN 49

// #define REGISTERED_BOOKS_START 0
// #define MAX_REGISTERED_BOOKS 50
// #define UID_LENGTH 8

// #define BOOK_CACHE_START (REGISTERED_BOOKS_START + MAX_REGISTERED_BOOKS * (UID_LENGTH + 1))
// #define BOOK_CACHE_SIZE 512
// #define MAX_CACHED_BOOKS 20

// #define PENDING_START (BOOK_CACHE_START + MAX_CACHED_BOOKS * BOOK_CACHE_SIZE)
// #define PENDING_ENTRY_SIZE 768
// #define PENDING_MAX 8

// #define DEBOUNCE_TIME 3000UL
// #define REQUEST_TIMEOUT 8000UL
// #define PING_INTERVAL 30000UL

// #define NUM_SENSORS 15
// const byte sensorPins[NUM_SENSORS] = {22,23,24,25,26,27,28,29,30,31,32,33,34,35,36};

// // ==================== MFRC522 OBJECT ====================
// MFRC522 mfrc522(RC522_SS_PIN, RC522_RST_PIN);

// // ==================== EEPROM HELPERS ====================
// String eepromReadString(int addr, int maxLen) {
//   char buffer[maxLen + 1];
//   int i;
//   for (i = 0; i < maxLen; i++) {
//     byte c = EEPROM.read(addr + i);
//     if (c == 0x00) {
//       break;
//     }
//     if (c == 0x01) {
//       buffer[i] = (char)0x00;
//     } else {
//       buffer[i] = (char)c;
//     }
//   }
//   buffer[i] = '\0';
//   return String(buffer);
// }

// void eepromWriteString(int addr, const String &str, int maxLen) {
//   unsigned int len = min(str.length(), (unsigned int)(maxLen - 1));
  
//   for (unsigned int i = 0; i < len; i++) {
//     char c = str.charAt(i);
//     if (c == (char)0x00) {
//       EEPROM.write(addr + i, 0x01);
//     } else {
//       EEPROM.write(addr + i, (byte)c);
//     }
//   }
  
//   EEPROM.write(addr + len, 0x00);
  
//   for (unsigned int i = len + 1; i < (unsigned int)maxLen; i++) {
//     EEPROM.write(addr + i, 0xFF);
//   }
// }

// // ==================== JSON VALIDATION ====================
// bool isValidJSON(const String &json) {
//   if (json.length() < 10) return false;
//   if (!json.startsWith("{")) return false;
//   if (!json.endsWith("}")) return false;
  
//   int braceCount = 0;
//   bool inString = false;
  
//   for (unsigned int i = 0; i < json.length(); i++) {
//     char c = json.charAt(i);
    
//     if (c == '\\' && i + 1 < json.length()) {
//       i++;
//       continue;
//     }
    
//     if (c == '"') {
//       inString = !inString;
//     }
    
//     if (!inString) {
//       if (c == '{') braceCount++;
//       else if (c == '}') braceCount--;
      
//       if (braceCount < 0) return false;
//     }
//   }
  
//   return (braceCount == 0 && !inString);
// }

// // ==================== CORRUPTION DETECTION ====================
// bool isEEPROMCorrupted() {
//   bool allEmpty = true;
//   for (int i = 0; i < 20; i++) {
//     if (EEPROM.read(REGISTERED_BOOKS_START + i) != 0xFF) {
//       allEmpty = false;
//       break;
//     }
//   }
  
//   if (allEmpty) {
//     return false;
//   }
  
//   for (int i = 0; i < 5; i++) {
//     int addr = REGISTERED_BOOKS_START + i * (UID_LENGTH + 1);
    
//     bool slotEmpty = true;
//     for (int j = 0; j < UID_LENGTH; j++) {
//       byte c = EEPROM.read(addr + j);
//       if (c != 0xFF && c != 0x00) {
//         slotEmpty = false;
//         break;
//       }
//     }
    
//     if (slotEmpty) continue;
    
//     String uid = eepromReadString(addr, UID_LENGTH);
    
//     if (uid.length() == UID_LENGTH) {
//       bool validHex = true;
//       for (unsigned int j = 0; j < uid.length(); j++) {
//         char c = uid.charAt(j);
//         if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) {
//           validHex = false;
//           break;
//         }
//       }
      
//       if (!validHex) {
//         return true;
//       }
//     } else if (uid.length() > 0 && uid.length() < UID_LENGTH) {
//       return true;
//     }
//   }
  
//   return false;
// }

// void clearCorruptedEEPROM() {
//   setLCDState(LCD_CLEARING_EEPROM);
//   Serial.println("Clearing corrupted EEPROM data...");
  
//   for (int i = 0; i < MAX_REGISTERED_BOOKS * (UID_LENGTH + 1); i++) {
//     EEPROM.write(REGISTERED_BOOKS_START + i, 0xFF);
//   }
  
//   for (int i = 0; i < MAX_CACHED_BOOKS * BOOK_CACHE_SIZE; i++) {
//     EEPROM.write(BOOK_CACHE_START + i, 0xFF);
//   }
  
//   for (int i = 0; i < PENDING_MAX * PENDING_ENTRY_SIZE; i++) {
//     EEPROM.write(PENDING_START + i, 0xFF);
//   }
  
//   Serial.println("EEPROM cleared successfully");
//   delay(2000);
//   setLCDState(LCD_WELCOME);
// }

// // ==================== BOOK CACHE SYSTEM ====================
// int getBookCacheAddr(const String &uid) {
//   unsigned int hash = 0;
//   for (unsigned int i = 0; i < uid.length(); i++) {
//     hash = (hash * 31) + uid.charAt(i);
//   }
//   int slot = hash % MAX_CACHED_BOOKS;
//   return BOOK_CACHE_START + slot * BOOK_CACHE_SIZE;
// }

// void cacheBookData(const String &uid, const String &jsonData) {
//   if (uid.length() != 8 || jsonData.length() == 0) return;
  
//   if (!isValidJSON(jsonData)) {
//     return;
//   }
  
//   int addr = getBookCacheAddr(uid);
  
//   unsigned int len = jsonData.length();
//   len = min(len, (unsigned int)(BOOK_CACHE_SIZE - 3));
  
//   EEPROM.write(addr, (len >> 8) & 0xFF);
//   EEPROM.write(addr + 1, len & 0xFF);
  
//   for (unsigned int i = 0; i < len; i++) {
//     char c = jsonData.charAt(i);
//     if (c == (char)0x00) {
//       EEPROM.write(addr + 2 + i, 0x01);
//     } else {
//       EEPROM.write(addr + 2 + i, (byte)c);
//     }
//   }
  
//   EEPROM.write(addr + 2 + len, 0xFF);
// }

// String getCachedBookData(const String &uid) {
//   if (uid.length() != 8) return "";
  
//   int addr = getBookCacheAddr(uid);
  
//   int len = (EEPROM.read(addr) << 8) | EEPROM.read(addr + 1);
  
//   if (len > 0 && len <= (BOOK_CACHE_SIZE - 3)) {
//     String cached = "";
//     for (int i = 0; i < len; i++) {
//       byte c = EEPROM.read(addr + 2 + i);
//       if (c == 0x01) {
//         cached += (char)0x00;
//       } else {
//         cached += (char)c;
//       }
//     }
    
//     if (isValidJSON(cached)) {
//       return cached;
//     }
//   }
  
//   String cached = eepromReadString(addr, BOOK_CACHE_SIZE - 1);
//   if (isValidJSON(cached)) {
//     return cached;
//   }
  
//   return "";
// }

// // ==================== REGISTERED BOOKS MANAGEMENT ====================
// bool isBookRegistered(const String &uid) {
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     if (registeredUIDs[i] == uid) return true;
//   }
//   return false;
// }

// int getBookIndex(const String &uid) {
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     if (registeredUIDs[i] == uid) return i;
//   }
//   return -1;
// }

// void saveRegisteredBook(const String &uid, int sensorIndex = -1) {
//   if (isBookRegistered(uid)) return;
  
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     if (registeredUIDs[i].length() == 0) {
//       registeredUIDs[i] = uid;
      
//       if (sensorIndex >= 0 && sensorIndex < NUM_SENSORS) {
//         sensorAssignments[i] = sensorIndex;
//       } else {
//         for (int s = 0; s < NUM_SENSORS; s++) {
//           bool sensorUsed = false;
//           for (int j = 0; j < MAX_REGISTERED_BOOKS; j++) {
//             if (sensorAssignments[j] == s) {
//               sensorUsed = true;
//               break;
//             }
//           }
//           if (!sensorUsed) {
//             sensorAssignments[i] = s;
//             break;
//           }
//         }
//       }
      
//       int addr = REGISTERED_BOOKS_START + i * (UID_LENGTH + 1);
//       eepromWriteString(addr, uid, UID_LENGTH);
//       return;
//     }
//   }
// }

// void loadRegisteredBooks() {
//   Serial.println("Loading registered books from EEPROM...");
//   int bookCount = 0;
  
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     registeredUIDs[i] = "";
//     sensorAssignments[i] = 255;
//   }
  
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     int addr = REGISTERED_BOOKS_START + i * (UID_LENGTH + 1);
//     String uid = eepromReadString(addr, UID_LENGTH);
    
//     if (uid.length() == UID_LENGTH) {
//       bool validHex = true;
//       for (unsigned int j = 0; j < uid.length(); j++) {
//         char c = uid.charAt(j);
//         if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) {
//           validHex = false;
//           break;
//         }
//       }
      
//       if (validHex) {
//         registeredUIDs[bookCount] = uid;
//         sensorAssignments[bookCount] = 255;
//         bookCount++;
//       }
//     }
//   }
  
//   if (bookCount == 0) {
//     Serial.println("No registered books found in EEPROM");
//   } else {
//     Serial.print("Loaded ");
//     Serial.print(bookCount);
//     Serial.println(" registered books (sensors not assigned yet)");
//   }
// }

// // ==================== SENSOR FUNCTIONS ====================
// String readSensor(int pin) {
//   static String lastState[NUM_SENSORS];

//   int lowCount = 0;

//   // Quick non-blocking sensor read
//   for (int i = 0; i < 10; i++) {
//     if (digitalRead(pin) == LOW) lowCount++;
//     delayMicroseconds(100);
//   }

//   if (lowCount > 8) {
//     return "AVL";
//   } else if (lowCount < 2) {
//     return "BRW";
//   } else {
//     // Return last known state
//     for (int i = 0; i < NUM_SENSORS; i++) {
//       if (sensorPins[i] == pin) {
//         return lastState[i];
//       }
//     }
//     return "BRW";
//   }
// }

// int getSensorIndexForBook(const String &uid) {
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     if (registeredUIDs[i] == uid) {
//       if (sensorAssignments[i] >= 0 && sensorAssignments[i] < NUM_SENSORS) {
//         return sensorAssignments[i];
//       }
//       break;
//     }
//   }
//   return -1;
// }

// int findSensorForBook(const String &uid) {
//   int existingSensor = getSensorIndexForBook(uid);
//   if (existingSensor != -1) {
//     return existingSensor;
//   }
  
//   for (int sensorNum = 0; sensorNum < NUM_SENSORS; sensorNum++) {
//     bool sensorInUse = false;
    
//     for (int j = 0; j < MAX_REGISTERED_BOOKS; j++) {
//       if (sensorAssignments[j] == sensorNum) {
//         sensorInUse = true;
//         break;
//       }
//     }
    
//     if (!sensorInUse) {
//       for (int j = 0; j < MAX_REGISTERED_BOOKS; j++) {
//         if (registeredUIDs[j].length() == 0) {
//           registeredUIDs[j] = uid;
//           sensorAssignments[j] = sensorNum;
          
//           int addr = REGISTERED_BOOKS_START + j * (UID_LENGTH + 1);
//           eepromWriteString(addr, uid, UID_LENGTH);
          
//           Serial.print("ASSIGNED: Book ");
//           Serial.print(uid);
//           Serial.print(" → Sensor ");
//           Serial.print(sensorNum);
//           Serial.print(" (Pin ");
//           Serial.print(sensorPins[sensorNum]);
//           Serial.println(")");
//           return sensorNum;
//         }
//       }
//     }
//   }
  
//   Serial.println("WARNING: All sensors in use! Using sensor 0 as fallback");
//   return 0;
// }

// String getBookStatus(const String &uid) {
//   int sensorIndex = getSensorIndexForBook(uid);

//   if (sensorIndex == -1) {
//     return "AVL";
//   }

//   if (sensorIndex >= 0 && sensorIndex < NUM_SENSORS) {
//     return readSensor(sensorPins[sensorIndex]);
//   }

//   return "AVL";
// }

// // ==================== SERIAL COMMUNICATION ====================
// void sendToESP(const String &msg) {
//   while (Serial1.available()) Serial1.read();
//   delay(5);
  
//   Serial1.println(msg);
  
//   Serial.print("TO_ESP: ");
  
//   if (msg.startsWith("SEND:")) {
//     int pipePos = msg.indexOf('|');
//     if (pipePos != -1) {
//       Serial.print(msg.substring(0, pipePos + 1));
//       Serial.print("... [JSON length: ");
//       Serial.print(msg.length() - pipePos - 1);
//       Serial.println("]");
//     } else {
//       int showLen = min(msg.length(), 60);
//       Serial.println(msg.substring(0, showLen));
//     }
//   } else {
//     Serial.println(msg);
//   }
// }

// bool waitForResponse(String &response, unsigned long timeout) {
//   unsigned long start = millis();
//   response = "";
  
//   while (millis() - start < timeout) {
//     while (Serial1.available()) {
//       char c = Serial1.read();
//       if (c == '\n') {
//         response.trim();
//         if (response.length() > 0) {
//           if (!response.startsWith("DEBUG:") && !response.startsWith("WIFI:")) {
//             Serial.print("FROM_ESP: ");
//             Serial.println(response);
//           }
//           return true;
//         }
//       } else if (c != '\r' && response.length() < 2048) {
//         response += c;
//       }
//     }
//     delay(1);
//   }
//   return false;
// }

// // ==================== PENDING QUEUE ====================
// int pendingIndexAddr(int index) { 
//   return PENDING_START + index * PENDING_ENTRY_SIZE; 
// }

// bool addPendingEntry(const String &payload) {
//   if (!payload.startsWith("SEND:")) {
//     return false;
//   }
  
//   int pipePos = payload.indexOf('|');
//   if (pipePos != -1) {
//     String jsonPart = payload.substring(pipePos + 1);
//     if (!isValidJSON(jsonPart)) {
//       return false;
//     }
//   }
  
//   for (int i = 0; i < PENDING_MAX; i++) {
//     int addr = pendingIndexAddr(i);
//     if (EEPROM.read(addr) == 0xFF) {
//       eepromWriteString(addr, payload, PENDING_ENTRY_SIZE - 1);
//       Serial.print("QUEUED: ");
      
//       int showLen = min(payload.length(), 60);
//       Serial.println(payload.substring(0, showLen));
      
//       return true;
//     }
//   }
//   Serial.println("QUEUE FULL!");
//   return false;
// }

// bool popPendingEntry(String &outPayload) {
//   for (int i = 0; i < PENDING_MAX; i++) {
//     int addr = pendingIndexAddr(i);
//     if (EEPROM.read(addr) != 0xFF && EEPROM.read(addr) != 0x00) {
//       outPayload = eepromReadString(addr, PENDING_ENTRY_SIZE - 1);
      
//       for (int j = 0; j < PENDING_ENTRY_SIZE; j++) {
//         EEPROM.write(addr + j, 0xFF);
//       }
      
//       if (outPayload.length() > 10 && 
//           outPayload.startsWith("SEND:") && 
//           outPayload.indexOf('|') > 5) {
        
//         int pipePos = outPayload.indexOf('|');
//         String jsonPart = outPayload.substring(pipePos + 1);
//         if (isValidJSON(jsonPart)) {
//           return true;
//         }
//       }
//     }
//   }
//   return false;
// }

// void flushPendingQueue() {
//   if (!wifiConnected) return;
  
//   String payload;
//   int flushed = 0;
  
//   while (popPendingEntry(payload)) {
//     if (payload.length() > 10) {
//       if (!payload.startsWith("SEND:")) {
//         continue;
//       }
      
//       Serial.print("FLUSHING: ");
//       Serial.println(payload.substring(0, 60));
//       sendToESP(payload);
      
//       String response;
//       if (waitForResponse(response, REQUEST_TIMEOUT)) {
//         if (response.startsWith("SUCCESS:")) {
//           flushed++;
//           delay(50);
//         } else {
//           addPendingEntry(payload);
//           break;
//         }
//       } else {
//         addPendingEntry(payload);
//         break;
//       }
//     }
//   }
  
//   if (flushed > 0) {
//     Serial.print("Successfully flushed ");
//     Serial.print(flushed);
//     Serial.println(" queued entries");
//   }
// }

// // ==================== RFID ====================
// String readRFID() {
//   if (!mfrc522.PICC_IsNewCardPresent()) return "";
//   if (!mfrc522.PICC_ReadCardSerial()) return "";
  
//   String uid = "";
//   for (byte i = 0; i < mfrc522.uid.size; i++) {
//     if (mfrc522.uid.uidByte[i] < 0x10) uid += "0";
//     uid += String(mfrc522.uid.uidByte[i], HEX);
//   }
//   uid.toUpperCase();
//   mfrc522.PICC_HaltA();
//   return uid;
// }

// // ==================== MAIN PROCESSING ====================
// void processCard(const String &cardUID) {
//   if (!systemReady) return;
  
//   if (cardUID == lastCardUID && millis() - lastCardTime < DEBOUNCE_TIME) {
//     return;
//   }
  
//   lastCardUID = cardUID;
//   lastCardTime = millis();
//   setLCDState(LCD_CARD_DETECTED);
  
//   int sensorIndex = findSensorForBook(cardUID);
//   String sensorStatus = "AVL";
//   if (sensorIndex >= 0 && sensorIndex < NUM_SENSORS) {
//     sensorStatus = readSensor(sensorPins[sensorIndex]);
//   }
  
//   Serial.println("=================================");
//   Serial.print("CARD: ");
//   Serial.println(cardUID);
//   Serial.print("SENSOR INDEX: ");
//   Serial.println(sensorIndex);
//   Serial.print("SENSOR STATUS: ");
//   Serial.println(sensorStatus);
//   Serial.println("=================================");
  
//   bool serverIsRegistered = false;
//   String serverTitle = "";
//   String serverAuthor = "";
//   String serverLocation = "";
//   String serverEspStatus = "AVL";
//   int serverSensor = 0;
//   bool usingCachedData = false;
  
//   if (wifiConnected) {
//     sendToESP("GET:" + cardUID);
//     String response;
//     if (waitForResponse(response, REQUEST_TIMEOUT)) {
//       if (response.startsWith("BOOKDATA:")) {
//         int p = response.indexOf('|');
//         if (p != -1) {
//           String serverData = response.substring(p + 1);
          
//           if (serverData != "null") {
//             JsonDocument doc;
//             DeserializationError err = deserializeJson(doc, serverData);
//             if (!err) {
//               serverIsRegistered = doc["isRegistered"] | false;
//               serverTitle = doc["title"] | "";
//               serverAuthor = doc["author"] | "";
//               serverLocation = doc["location"] | "";
//               serverSensor = doc["sensor"] | 0;
              
//               if (doc["espStatus"]) {
//                 serverEspStatus = String(doc["espStatus"].as<const char*>());
//               }
              
//               Serial.print("FIREBASE: Registered=");
//               Serial.print(serverIsRegistered ? "YES" : "NO");
//               Serial.print(", Status=");
//               Serial.println(serverEspStatus);
              
//               cacheBookData(cardUID, serverData);
              
//               if (serverIsRegistered && !isBookRegistered(cardUID)) {
//                 saveRegisteredBook(cardUID, serverSensor);
//               }
//             }
//           } else {
//             Serial.println("Book not found in Firebase");
//           }
//         }
//       }
//     } else {
//       wifiConnected = false;
//       setLCDState(LCD_OFFLINE);
//     }
//   }
  
//   if (!wifiConnected || serverTitle.length() == 0) {
//     String cachedData = getCachedBookData(cardUID);
//     if (cachedData.length() > 0) {
//       JsonDocument doc;
//       DeserializationError err = deserializeJson(doc, cachedData);
//       if (!err) {
//         serverIsRegistered = doc["isRegistered"] | false;
//         serverTitle = doc["title"] | "";
//         serverAuthor = doc["author"] | "";
//         serverLocation = doc["location"] | "";
//         serverSensor = doc["sensor"] | 0;
        
//         if (doc["espStatus"]) {
//           serverEspStatus = String(doc["espStatus"].as<const char*>());
//         }
        
//         usingCachedData = true;
//         Serial.println("USING CACHED DATA");
//       }
//     }
//   }
  
//   if (serverTitle.length() == 0) {
//     if (isBookRegistered(cardUID)) {
//       serverTitle = "Book Title Unknown";
//       serverAuthor = "Author Unknown";
//       serverLocation = "Location Unknown";
//       serverIsRegistered = true;
//       Serial.println("Using local registration (no cache)");
//     } else {
//       serverTitle = "New Book";
//       serverAuthor = "Unknown";
//       serverLocation = "Unassigned";
//       serverIsRegistered = false;
//       setLCDState(LCD_NEW_BOOK);
//       Serial.println("New unregistered book");
//       return;
//     }
//   }
  
//   // Update global variables for LCD display
//   currentBookTitle = serverTitle;
//   currentBookAuthor = serverAuthor;
//   currentBookLocation = serverLocation;
//   currentBookStatus = sensorStatus;
  
//   // Show book info on LCD
//   setLCDState(LCD_BOOK_INFO);
  
//   JsonDocument doc;
  
//   doc["title"] = serverTitle;
//   doc["author"] = serverAuthor;
//   doc["location"] = serverLocation;
//   doc["isRegistered"] = serverIsRegistered;
//   doc["lastSeen"] = String(millis() / 1000);
//   doc["sensor"] = sensorIndex;
//   doc["espStatus"] = sensorStatus;
  
//   if (sensorStatus == "AVL") doc["status"] = "Available";
//   else if (sensorStatus == "BRW") doc["status"] = "Borrowed";
//   else if (sensorStatus == "OVD") doc["status"] = "Overdue";
//   else if (sensorStatus == "MIS") doc["status"] = "Misplaced";
//   else doc["status"] = "Available";
  
//   doc["lastUpdated"] = String(millis() / 1000);
  
//   if (usingCachedData) {
//     doc["source"] = "cached";
//   }
  
//   String outJson;
//   serializeJson(doc, outJson);
  
//   Serial.print("JSON SIZE: ");
//   Serial.println(outJson.length());
  
//   if (!outJson.endsWith("}")) {
//     outJson += "}";
//   }
  
//   String sendCmd = "SEND:/books/" + cardUID + "|" + outJson;
  
//   if (wifiConnected) {
//     sendToESP(sendCmd);
    
//     String response;
//     if (waitForResponse(response, REQUEST_TIMEOUT)) {
//       if (response.startsWith("SUCCESS:")) {
//         cacheBookData(cardUID, outJson);
        
//         if (serverIsRegistered && !isBookRegistered(cardUID)) {
//           saveRegisteredBook(cardUID, sensorIndex);
//         }
//         Serial.println("Firebase update OK");
//       } else {
//         addPendingEntry(sendCmd);
//       }
//     } else {
//       addPendingEntry(sendCmd);
//     }
//   } else {
//     addPendingEntry(sendCmd);
//     Serial.println("WiFi offline - queued");
//   }
// }

// // ==================== SETUP ====================
// void setup() {
//   Serial.begin(115200);
//   Serial1.begin(115200);
  
//   // Initialize buzzer pin first (always works)
//   pinMode(BUZZER_PIN, OUTPUT);
//   digitalWrite(BUZZER_PIN, LOW);
  
//   // Initialize LCD with simple approach
//   lcd.init();
//   lcd.backlight();
//   lcd.clear();
//   lcdInitialized = true;
  
//   // Show immediate feedback
//   lcd.setCursor(0, 0);
//   lcd.print("Booting...");
  
//   delay(100); // Minimal delay for serial
  
//   Serial.println();
//   Serial.println("LIBRARY SYSTEM - MEGA 2560");
//   Serial.println("==========================");
  
//   // Initialize sensor pins
//   for (int i = 0; i < NUM_SENSORS; i++) {
//     pinMode(sensorPins[i], INPUT_PULLUP);
//   }
  
//   // Quick EEPROM check
//   if (isEEPROMCorrupted()) {
//     Serial.println("EEPROM corrupted - clearing");
//     clearCorruptedEEPROM();
//   } else {
//     loadRegisteredBooks();
//   }
  
//   // Initialize RFID
//   SPI.begin();
//   mfrc522.PCD_Init();
  
//   // Quick RFID test
//   byte v = mfrc522.PCD_ReadRegister(MFRC522::VersionReg);
//   if (v == 0x00 || v == 0xFF) {
//     Serial.println("RFID module not found!");
//     lcd.setCursor(0, 1);
//     lcd.print("RFID: NOT FOUND");
//   } else {
//     Serial.print("RFID module v");
//     Serial.println(v, HEX);
//     lcd.setCursor(0, 1);
//     lcd.print("RFID: OK");
//   }
  
//   // Test ESP connection (non-blocking)
//   Serial.println("Testing ESP connection...");
//   lcd.setCursor(0, 2);
//   lcd.print("WiFi: Checking...");
  
//   sendToESP("PING");
  
//   // Set initial LCD state
//   setLCDState(LCD_INIT);
  
//   Serial.println("Setup complete!");
// }

// // ==================== LOOP ====================
// void loop() {
//   // Always update LCD (non-blocking)
//   updateLCD();
  
//   // Handle RFID if system is ready
//   if (systemReady) {
//     String uid = readRFID();
//     if (uid.length() == 8) {
//       processCard(uid);
//     }
//   }
  
//   // Handle ESP messages
//   static String espBuffer = "";
//   while (Serial1.available()) {
//     char c = Serial1.read();
//     if (c == '\n') {
//       espBuffer.trim();
//       if (espBuffer.length() > 0) {
//         if (espBuffer.startsWith("WIFI:")) {
//           if (espBuffer == "WIFI:OK") {
//             if (!wifiConnected) {
//               wifiConnected = true;
//               Serial.println("WiFi connected");
//               beep(BEEP_WIFI_CONNECT, 200);
//               flushPendingQueue();
//             }
//           } else if (espBuffer == "WIFI:OFFLINE") {
//             wifiConnected = false;
//             Serial.println("WiFi offline");
//             beep(BEEP_WIFI_DISCONNECT, 200);
//           }
//         } else if (espBuffer == "PONG") {
//           if (!wifiConnected) {
//             wifiConnected = true;
//             Serial.println("WiFi connected via PONG");
//           }
//         }
//       }
//       espBuffer = "";
//     } else if (c != '\r') {
//       espBuffer += c;
//     }
//   }
  
//   // Small delay to prevent watchdog issues
//   delay(10);
// }



//=============ALTER AVR/BRW INITIAL & CHANGE------ INITIAL MESSAGE RUN FASTER----SECOND DISPLAYED & STUCK-CODE WORKS UNDERGROUND
// #include <Arduino.h>
// #include <SPI.h>
// #include <MFRC522.h>
// #include <EEPROM.h>
// #include <ArduinoJson.h>
// #include <LiquidCrystal_I2C.h>

// // ==================== LCD CONFIGURATION ====================
// #define LCD_ADDRESS 0x27
// #define LCD_COLS 20
// #define LCD_ROWS 4
// LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLS, LCD_ROWS);

// // ==================== BUZZER CONFIGURATION ====================
// #define BUZZER_PIN 48

// // ==================== GLOBAL VARIABLES ====================
// bool wifiConnected = false;
// bool systemReady = false;
// bool lcdInitialized = false;
// String lastCardUID = "";
// unsigned long lastCardTime = 0;
// String registeredUIDs[50];
// byte sensorAssignments[50];

// // ==================== LCD STATES ====================
// enum LCDState {
//   LCD_INIT,
//   LCD_WELCOME,
//   LCD_READY,
//   LCD_CARD_DETECTED,
//   LCD_BOOK_INFO_PAGE1,
//   LCD_BOOK_INFO_PAGE2,
//   LCD_BOOK_INFO_PAGE3,
//   LCD_OFFLINE,
//   LCD_NEW_BOOK,
//   LCD_CLEARING_EEPROM,
//   LCD_CORRUPTED_DATA
// };

// LCDState currentLCDState = LCD_INIT;
// unsigned long lcdStateStartTime = 0;
// int animationFrame = 0;
// String currentBookTitle = "";
// String currentBookAuthor = "";
// String currentBookLocation = "";
// String currentBookStatus = "";
// int currentSensorIndex = -1;
// int currentPage = 1;

// // ==================== BEEP TONES ====================
// #define BEEP_AVL 2000
// #define BEEP_BRW 1200
// #define BEEP_SWIPE 800
// #define BEEP_WIFI_CONNECT 2500
// #define BEEP_WIFI_DISCONNECT 600
// #define BEEP_ERROR 400
// #define BEEP_CONFIRM 1800
// #define BEEP_PAGE_CHANGE 1500

// // ==================== ORIGINAL CONFIG ====================
// #define RC522_SS_PIN 53
// #define RC522_RST_PIN 49

// #define REGISTERED_BOOKS_START 0
// #define MAX_REGISTERED_BOOKS 50
// #define UID_LENGTH 8

// #define BOOK_CACHE_START (REGISTERED_BOOKS_START + MAX_REGISTERED_BOOKS * (UID_LENGTH + 1))
// #define BOOK_CACHE_SIZE 512
// #define MAX_CACHED_BOOKS 20

// #define PENDING_START (BOOK_CACHE_START + MAX_CACHED_BOOKS * BOOK_CACHE_SIZE)
// #define PENDING_ENTRY_SIZE 768
// #define PENDING_MAX 8

// #define DEBOUNCE_TIME 3000UL
// #define REQUEST_TIMEOUT 8000UL
// #define PING_INTERVAL 30000UL

// #define NUM_SENSORS 15
// const byte sensorPins[NUM_SENSORS] = {22,23,24,25,26,27,28,29,30,31,32,33,34,35,36};

// // ==================== MFRC522 OBJECT ====================
// MFRC522 mfrc522(RC522_SS_PIN, RC522_RST_PIN);

// // ==================== BUZZER FUNCTIONS ====================
// void beep(int frequency, int duration) {
//   if (frequency <= 0) return;
//   tone(BUZZER_PIN, frequency, duration);
//   delay(duration);
//   noTone(BUZZER_PIN);
// }

// void beepSequence(int freq1, int dur1, int freq2 = 0, int dur2 = 0) {
//   beep(freq1, dur1);
//   if (freq2 > 0) {
//     delay(50);
//     beep(freq2, dur2);
//   }
// }

// // ==================== LCD HELPER FUNCTIONS ====================
// void clearLine(int row) {
//   lcd.setCursor(0, row);
//   for (int i = 0; i < LCD_COLS; i++) {
//     lcd.print(" ");
//   }
// }

// void centerText(String text, int row) {
//   int textLen = text.length();
//   if (textLen > LCD_COLS) {
//     text = text.substring(0, LCD_COLS);
//     textLen = LCD_COLS;
//   }
//   int padding = (LCD_COLS - textLen) / 2;
//   if (padding < 0) padding = 0;
//   lcd.setCursor(padding, row);
//   lcd.print(text);
// }

// void printBounded(String text, int maxLength) {
//   if (text.length() > (unsigned int)maxLength) {
//     lcd.print(text.substring(0, maxLength));
//   } else {
//     lcd.print(text);
//     for (unsigned int i = text.length(); i < (unsigned int)maxLength; i++) {
//       lcd.print(" ");
//     }
//   }
// }

// void drawSeparator(int row) {
//   lcd.setCursor(0, row);
//   for (int i = 0; i < LCD_COLS; i++) {
//     lcd.print("-");
//   }
// }

// void drawDoubleSeparator(int row) {
//   lcd.setCursor(0, row);
//   for (int i = 0; i < LCD_COLS; i++) {
//     lcd.print("=");
//   }
// }

// void setLCDState(LCDState newState) {
//   if (currentLCDState != newState) {
//     if (!(currentLCDState == LCD_BOOK_INFO_PAGE1 && newState == LCD_BOOK_INFO_PAGE2) &&
//         !(currentLCDState == LCD_BOOK_INFO_PAGE2 && newState == LCD_BOOK_INFO_PAGE3) &&
//         !(currentLCDState == LCD_BOOK_INFO_PAGE3 && newState == LCD_BOOK_INFO_PAGE1)) {
//       if (lcdInitialized) {
//         lcd.clear();
//       }
//     }
    
//     currentLCDState = newState;
//     lcdStateStartTime = millis();
//     animationFrame = 0;
//   }
// }

// void updateLCD() {
//   static unsigned long lastUpdate = 0;
//   unsigned long currentMillis = millis();
  
//   if (!lcdInitialized) return;
  
//   if (currentMillis - lastUpdate < 150) return;
//   lastUpdate = currentMillis;
  
//   unsigned long stateDuration = currentMillis - lcdStateStartTime;
  
//   switch (currentLCDState) {
//     case LCD_INIT:
//       if (animationFrame == 0) {
//         lcd.clear();
//         drawDoubleSeparator(0);
//         centerText("LIBRARY SYSTEM", 1);
//         centerText("v5.0", 2);
//         drawDoubleSeparator(3);
//         animationFrame = 1;
//       }
//       if (stateDuration > 1500) {
//         setLCDState(LCD_WELCOME);
//       }
//       break;
      
//     case LCD_WELCOME:
//       if (animationFrame == 0) {
//         lcd.clear();
//         centerText("LEEJINBOTICS", 0);
//         drawSeparator(1);
//         centerText("INITIALIZING", 2);
//         animationFrame = 1;
//       }
      
//       // Animated progress dots
//       String dots = "";
//       int dotCount = (stateDuration / 300) % 8;
//       for (int i = 0; i < 8; i++) {
//         dots += (i < dotCount) ? "." : " ";
//       }
//       lcd.setCursor(6, 3);
//       lcd.print(dots);
      
//       if (stateDuration > 2000) {
//         setLCDState(LCD_READY);
//         systemReady = true;
//         beep(BEEP_CONFIRM, 200);
//       }
//       break;
      
//     case LCD_READY:
//       if (animationFrame == 0) {
//         lcd.clear();
//         drawDoubleSeparator(0);
//         centerText("READY TO SCAN", 1);
//         lcd.setCursor(0, 2);
//         lcd.print("WiFi:");
//         lcd.setCursor(6, 2);
//         if (wifiConnected) {
//           lcd.print("ONLINE  ");
//           lcd.write(223);
//         } else {
//           lcd.print("OFFLINE ");
//           lcd.print("X");
//         }
//         drawDoubleSeparator(3);
//         animationFrame = 1;
//       }
      
//       if ((stateDuration / 500) % 2 == 0) {
//         lcd.setCursor(8, 1);
//         lcd.print("SCAN");
//       } else {
//         lcd.setCursor(8, 1);
//         lcd.print("    ");
//       }
//       break;
      
//     case LCD_CARD_DETECTED:
//       if (animationFrame == 0) {
//         lcd.clear();
//         drawDoubleSeparator(0);
//         centerText("SCANNING CARD", 1);
//         lcd.setCursor(0, 2);
//         lcd.print("UID: ");
//         String shortUID = lastCardUID;
//         if (shortUID.length() > 12) shortUID = shortUID.substring(0, 12);
//         lcd.print(shortUID);
//         drawDoubleSeparator(3);
//         beep(BEEP_SWIPE, 150);
//         animationFrame = 1;
//       }
      
//       // Animated scanning indicator
//       int scanPos = (stateDuration / 200) % 16;
//       lcd.setCursor(2, 3);
//       for (int i = 0; i < 16; i++) {
//         if (i == scanPos) {
//           lcd.print(">");
//         } else {
//           lcd.print(" ");
//         }
//       }
      
//       if (stateDuration > 2000) {
//         setLCDState(LCD_BOOK_INFO_PAGE1);
//       }
//       break;
      
//     case LCD_BOOK_INFO_PAGE1:
//       if (animationFrame == 0) {
//         lcd.clear();
//         drawDoubleSeparator(0);
//         centerText("BOOK DETAILS", 1);
//         lcd.setCursor(0, 2);
//         lcd.print("Page 1/3");
//         lcd.setCursor(12, 2);
//         lcd.print(">>");
//         drawDoubleSeparator(3);
//         animationFrame = 1;
//         beep(BEEP_PAGE_CHANGE, 100);
//       }
      
//       // Display title with smart truncation
//       lcd.setCursor(0, 1);
//       String displayTitle = currentBookTitle;
//       if (displayTitle.length() > 20) {
//         // For very long titles, show first part with ellipsis
//         displayTitle = displayTitle.substring(0, 17) + "...";
//       }
//       centerText(displayTitle, 1);
      
//       if (stateDuration > 3000) {
//         setLCDState(LCD_BOOK_INFO_PAGE2);
//       }
//       break;
      
//     case LCD_BOOK_INFO_PAGE2:
//       if (animationFrame == 0) {
//         lcd.clear();
//         drawDoubleSeparator(0);
//         centerText("AUTHOR & STATUS", 1);
//         lcd.setCursor(0, 2);
//         lcd.print("Page 2/3");
//         lcd.setCursor(12, 2);
//         lcd.print(">>");
//         drawDoubleSeparator(3);
//         animationFrame = 1;
//         beep(BEEP_PAGE_CHANGE, 100);
//       }
      
//       // Display author with smart truncation
//       lcd.setCursor(0, 1);
//       String displayAuthor = currentBookAuthor;
//       if (displayAuthor.length() > 20) {
//         displayAuthor = displayAuthor.substring(0, 17) + "...";
//       }
//       centerText(displayAuthor, 1);
      
//       // Display status with icon
//       lcd.setCursor(0, 2);
//       lcd.print("Status: ");
//       if (currentBookStatus == "AVL") {
//         lcd.print("AVAILABLE ");
//         lcd.write(3);
//         if (animationFrame == 1) {
//           beep(BEEP_AVL, 300);
//           animationFrame = 2;
//         }
//       } else if (currentBookStatus == "BRW") {
//         lcd.print("BORROWED  ");
//         lcd.write(1);
//         if (animationFrame == 1) {
//           beep(BEEP_BRW, 300);
//           animationFrame = 2;
//         }
//       } else {
//         lcd.print(currentBookStatus);
//       }
      
//       if (stateDuration > 3000) {
//         setLCDState(LCD_BOOK_INFO_PAGE3);
//       }
//       break;
      
//     case LCD_BOOK_INFO_PAGE3:
//       if (animationFrame == 0) {
//         lcd.clear();
//         drawDoubleSeparator(0);
//         centerText("LOCATION INFO", 1);
//         lcd.setCursor(0, 2);
//         lcd.print("Page 3/3");
//         lcd.setCursor(12, 2);
//         lcd.print(">>");
//         drawDoubleSeparator(3);
//         animationFrame = 1;
//         beep(BEEP_PAGE_CHANGE, 100);
//       }
      
//       // Display location with smart truncation
//       lcd.setCursor(0, 1);
//       String displayLoc = currentBookLocation;
//       if (displayLoc.length() > 20) {
//         displayLoc = displayLoc.substring(0, 17) + "...";
//       }
//       centerText(displayLoc, 1);
      
//       // Display sensor info
//       lcd.setCursor(0, 2);
//       lcd.print("Sensor: ");
//       if (currentSensorIndex >= 0 && currentSensorIndex < NUM_SENSORS) {
//         lcd.print(currentSensorIndex);
//         lcd.print(" (Pin ");
//         lcd.print(sensorPins[currentSensorIndex]);
//         lcd.print(")");
//       } else {
//         lcd.print("N/A");
//       }
      
//       if (stateDuration > 3000) {
//         setLCDState(LCD_READY);
//       }
//       break;
      
//     case LCD_OFFLINE:
//       if (animationFrame == 0) {
//         lcd.clear();
//         drawDoubleSeparator(0);
//         centerText("OFFLINE MODE", 1);
//         lcd.setCursor(0, 2);
//         lcd.print("Data cached locally");
//         drawDoubleSeparator(3);
//         beep(BEEP_WIFI_DISCONNECT, 300);
//         animationFrame = 1;
//       }
      
//       if ((stateDuration / 400) % 2 == 0) {
//         lcd.setCursor(9, 1);
//         lcd.print("OFFLINE");
//       } else {
//         lcd.setCursor(9, 1);
//         lcd.print("       ");
//       }
//       break;
      
//     case LCD_NEW_BOOK:
//       if (animationFrame == 0) {
//         lcd.clear();
//         drawDoubleSeparator(0);
//         centerText("NEW BOOK DETECTED", 1);
//         lcd.setCursor(0, 2);
//         lcd.print("UID: ");
//         String shortUID2 = lastCardUID;
//         if (shortUID2.length() > 12) shortUID2 = shortUID2.substring(0, 12);
//         lcd.print(shortUID2);
//         drawDoubleSeparator(3);
//         beepSequence(BEEP_SWIPE, 100, BEEP_CONFIRM, 150);
//         animationFrame = 1;
//       }
      
//       if ((stateDuration / 300) % 2 == 0) {
//         lcd.setCursor(2, 3);
//         lcd.print("*** REGISTER ME ***");
//       } else {
//         clearLine(3);
//       }
      
//       if (stateDuration > 4000) {
//         setLCDState(LCD_READY);
//       }
//       break;
      
//     case LCD_CLEARING_EEPROM:
//       if (animationFrame == 0) {
//         lcd.clear();
//         drawDoubleSeparator(0);
//         centerText("EEPROM MAINTENANCE", 1);
//         lcd.setCursor(0, 2);
//         lcd.print("Cleaning storage...");
//         drawDoubleSeparator(3);
//         animationFrame = 1;
//       }
      
//       int progressBars = (stateDuration / 100) % 21;
//       lcd.setCursor(0, 3);
//       for (int i = 0; i < 20; i++) {
//         if (i < progressBars) {
//           lcd.print("#");
//         } else {
//           lcd.print(" ");
//         }
//       }
      
//       if (stateDuration > 2000) {
//         setLCDState(LCD_WELCOME);
//       }
//       break;
      
//     case LCD_CORRUPTED_DATA:
//       if (animationFrame == 0) {
//         lcd.clear();
//         drawDoubleSeparator(0);
//         centerText("DATA INTEGRITY", 1);
//         lcd.setCursor(0, 2);
//         lcd.print("EEPROM Recovery...");
//         drawDoubleSeparator(3);
//         beepSequence(BEEP_ERROR, 200, BEEP_ERROR, 200);
//         animationFrame = 1;
//       }
      
//       if ((stateDuration / 200) % 2 == 0) {
//         lcd.setCursor(6, 1);
//         lcd.print("INTEGRITY");
//       } else {
//         lcd.setCursor(6, 1);
//         lcd.print("         ");
//       }
      
//       if (stateDuration > 3000) {
//         setLCDState(LCD_WELCOME);
//       }
//       break;
//   }
// }

// // ==================== EEPROM HELPERS ====================
// String eepromReadString(int addr, int maxLen) {
//   char buffer[maxLen + 1];
//   int i;
//   for (i = 0; i < maxLen; i++) {
//     byte c = EEPROM.read(addr + i);
//     if (c == 0x00) {
//       break;
//     }
//     if (c == 0x01) {
//       buffer[i] = (char)0x00;
//     } else {
//       buffer[i] = (char)c;
//     }
//   }
//   buffer[i] = '\0';
//   return String(buffer);
// }

// void eepromWriteString(int addr, const String &str, int maxLen) {
//   unsigned int len = min(str.length(), (unsigned int)(maxLen - 1));
  
//   for (unsigned int i = 0; i < len; i++) {
//     char c = str.charAt(i);
//     if (c == (char)0x00) {
//       EEPROM.write(addr + i, 0x01);
//     } else {
//       EEPROM.write(addr + i, (byte)c);
//     }
//   }
  
//   EEPROM.write(addr + len, 0x00);
  
//   for (unsigned int i = len + 1; i < (unsigned int)maxLen; i++) {
//     EEPROM.write(addr + i, 0xFF);
//   }
// }

// // ==================== JSON VALIDATION ====================
// bool isValidJSON(const String &json) {
//   if (json.length() < 10) return false;
//   if (!json.startsWith("{")) return false;
//   if (!json.endsWith("}")) return false;
  
//   int braceCount = 0;
//   bool inString = false;
  
//   for (unsigned int i = 0; i < json.length(); i++) {
//     char c = json.charAt(i);
    
//     if (c == '\\' && i + 1 < json.length()) {
//       i++;
//       continue;
//     }
    
//     if (c == '"') {
//       inString = !inString;
//     }
    
//     if (!inString) {
//       if (c == '{') braceCount++;
//       else if (c == '}') braceCount--;
      
//       if (braceCount < 0) return false;
//     }
//   }
  
//   return (braceCount == 0 && !inString);
// }

// // ==================== CORRUPTION DETECTION ====================
// bool isEEPROMCorrupted() {
//   bool allEmpty = true;
//   for (int i = 0; i < 20; i++) {
//     if (EEPROM.read(REGISTERED_BOOKS_START + i) != 0xFF) {
//       allEmpty = false;
//       break;
//     }
//   }
  
//   if (allEmpty) {
//     return false;
//   }
  
//   for (int i = 0; i < 5; i++) {
//     int addr = REGISTERED_BOOKS_START + i * (UID_LENGTH + 1);
    
//     bool slotEmpty = true;
//     for (int j = 0; j < UID_LENGTH; j++) {
//       byte c = EEPROM.read(addr + j);
//       if (c != 0xFF && c != 0x00) {
//         slotEmpty = false;
//         break;
//       }
//     }
    
//     if (slotEmpty) continue;
    
//     String uid = eepromReadString(addr, UID_LENGTH);
    
//     if (uid.length() == UID_LENGTH) {
//       bool validHex = true;
//       for (unsigned int j = 0; j < uid.length(); j++) {
//         char c = uid.charAt(j);
//         if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) {
//           validHex = false;
//           break;
//         }
//       }
      
//       if (!validHex) {
//         return true;
//       }
//     } else if (uid.length() > 0 && uid.length() < UID_LENGTH) {
//       return true;
//     }
//   }
  
//   return false;
// }

// void clearCorruptedEEPROM() {
//   setLCDState(LCD_CLEARING_EEPROM);
//   Serial.println("Clearing corrupted EEPROM data...");
  
//   for (int i = 0; i < MAX_REGISTERED_BOOKS * (UID_LENGTH + 1); i++) {
//     EEPROM.write(REGISTERED_BOOKS_START + i, 0xFF);
//   }
  
//   for (int i = 0; i < MAX_CACHED_BOOKS * BOOK_CACHE_SIZE; i++) {
//     EEPROM.write(BOOK_CACHE_START + i, 0xFF);
//   }
  
//   for (int i = 0; i < PENDING_MAX * PENDING_ENTRY_SIZE; i++) {
//     EEPROM.write(PENDING_START + i, 0xFF);
//   }
  
//   Serial.println("EEPROM cleared successfully");
//   delay(2000);
//   setLCDState(LCD_WELCOME);
// }

// // ==================== BOOK CACHE SYSTEM ====================
// int getBookCacheAddr(const String &uid) {
//   unsigned int hash = 0;
//   for (unsigned int i = 0; i < uid.length(); i++) {
//     hash = (hash * 31) + uid.charAt(i);
//   }
//   int slot = hash % MAX_CACHED_BOOKS;
//   return BOOK_CACHE_START + slot * BOOK_CACHE_SIZE;
// }

// void cacheBookData(const String &uid, const String &jsonData) {
//   if (uid.length() != 8 || jsonData.length() == 0) return;
  
//   if (!isValidJSON(jsonData)) {
//     return;
//   }
  
//   int addr = getBookCacheAddr(uid);
  
//   unsigned int len = jsonData.length();
//   len = min(len, (unsigned int)(BOOK_CACHE_SIZE - 3));
  
//   EEPROM.write(addr, (len >> 8) & 0xFF);
//   EEPROM.write(addr + 1, len & 0xFF);
  
//   for (unsigned int i = 0; i < len; i++) {
//     char c = jsonData.charAt(i);
//     if (c == (char)0x00) {
//       EEPROM.write(addr + 2 + i, 0x01);
//     } else {
//       EEPROM.write(addr + 2 + i, (byte)c);
//     }
//   }
  
//   EEPROM.write(addr + 2 + len, 0xFF);
// }

// String getCachedBookData(const String &uid) {
//   if (uid.length() != 8) return "";
  
//   int addr = getBookCacheAddr(uid);
  
//   int len = (EEPROM.read(addr) << 8) | EEPROM.read(addr + 1);
  
//   if (len > 0 && len <= (BOOK_CACHE_SIZE - 3)) {
//     String cached = "";
//     for (int i = 0; i < len; i++) {
//       byte c = EEPROM.read(addr + 2 + i);
//       if (c == 0x01) {
//         cached += (char)0x00;
//       } else {
//         cached += (char)c;
//       }
//     }
    
//     if (isValidJSON(cached)) {
//       return cached;
//     }
//   }
  
//   String cached = eepromReadString(addr, BOOK_CACHE_SIZE - 1);
//   if (isValidJSON(cached)) {
//     return cached;
//   }
  
//   return "";
// }

// // ==================== REGISTERED BOOKS MANAGEMENT ====================
// bool isBookRegistered(const String &uid) {
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     if (registeredUIDs[i] == uid) return true;
//   }
//   return false;
// }

// int getBookIndex(const String &uid) {
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     if (registeredUIDs[i] == uid) return i;
//   }
//   return -1;
// }

// void saveRegisteredBook(const String &uid, int sensorIndex = -1) {
//   if (isBookRegistered(uid)) return;
  
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     if (registeredUIDs[i].length() == 0) {
//       registeredUIDs[i] = uid;
      
//       if (sensorIndex >= 0 && sensorIndex < NUM_SENSORS) {
//         sensorAssignments[i] = sensorIndex;
//       } else {
//         for (int s = 0; s < NUM_SENSORS; s++) {
//           bool sensorUsed = false;
//           for (int j = 0; j < MAX_REGISTERED_BOOKS; j++) {
//             if (sensorAssignments[j] == s) {
//               sensorUsed = true;
//               break;
//             }
//           }
//           if (!sensorUsed) {
//             sensorAssignments[i] = s;
//             break;
//           }
//         }
//       }
      
//       int addr = REGISTERED_BOOKS_START + i * (UID_LENGTH + 1);
//       eepromWriteString(addr, uid, UID_LENGTH);
//       return;
//     }
//   }
// }

// void loadRegisteredBooks() {
//   Serial.println("Loading registered books from EEPROM...");
//   int bookCount = 0;
  
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     registeredUIDs[i] = "";
//     sensorAssignments[i] = 255;
//   }
  
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     int addr = REGISTERED_BOOKS_START + i * (UID_LENGTH + 1);
//     String uid = eepromReadString(addr, UID_LENGTH);
    
//     if (uid.length() == UID_LENGTH) {
//       bool validHex = true;
//       for (unsigned int j = 0; j < uid.length(); j++) {
//         char c = uid.charAt(j);
//         if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) {
//           validHex = false;
//           break;
//         }
//       }
      
//       if (validHex) {
//         registeredUIDs[bookCount] = uid;
//         sensorAssignments[bookCount] = 255;
//         bookCount++;
//       }
//     }
//   }
  
//   if (bookCount == 0) {
//     Serial.println("No registered books found in EEPROM");
//   } else {
//     Serial.print("Loaded ");
//     Serial.print(bookCount);
//     Serial.println(" registered books (sensors not assigned yet)");
//   }
// }

// // ==================== SENSOR FUNCTIONS ====================
// String readSensor(int pin) {
//   static String lastState[NUM_SENSORS];

//   int lowCount = 0;

//   for (int i = 0; i < 10; i++) {
//     if (digitalRead(pin) == LOW) lowCount++;
//     delayMicroseconds(100);
//   }

//   if (lowCount > 8) {
//     return "AVL";
//   } else if (lowCount < 2) {
//     return "BRW";
//   } else {
//     for (int i = 0; i < NUM_SENSORS; i++) {
//       if (sensorPins[i] == pin) {
//         return lastState[i];
//       }
//     }
//     return "BRW";
//   }
// }

// int getSensorIndexForBook(const String &uid) {
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     if (registeredUIDs[i] == uid) {
//       if (sensorAssignments[i] >= 0 && sensorAssignments[i] < NUM_SENSORS) {
//         return sensorAssignments[i];
//       }
//       break;
//     }
//   }
//   return -1;
// }

// int findSensorForBook(const String &uid) {
//   int existingSensor = getSensorIndexForBook(uid);
//   if (existingSensor != -1) {
//     return existingSensor;
//   }
  
//   for (int sensorNum = 0; sensorNum < NUM_SENSORS; sensorNum++) {
//     bool sensorInUse = false;
    
//     for (int j = 0; j < MAX_REGISTERED_BOOKS; j++) {
//       if (sensorAssignments[j] == sensorNum) {
//         sensorInUse = true;
//         break;
//       }
//     }
    
//     if (!sensorInUse) {
//       for (int j = 0; j < MAX_REGISTERED_BOOKS; j++) {
//         if (registeredUIDs[j].length() == 0) {
//           registeredUIDs[j] = uid;
//           sensorAssignments[j] = sensorNum;
          
//           int addr = REGISTERED_BOOKS_START + j * (UID_LENGTH + 1);
//           eepromWriteString(addr, uid, UID_LENGTH);
          
//           Serial.print("ASSIGNED: Book ");
//           Serial.print(uid);
//           Serial.print(" → Sensor ");
//           Serial.print(sensorNum);
//           Serial.print(" (Pin ");
//           Serial.print(sensorPins[sensorNum]);
//           Serial.println(")");
//           return sensorNum;
//         }
//       }
//     }
//   }
  
//   Serial.println("WARNING: All sensors in use! Using sensor 0 as fallback");
//   return 0;
// }

// String getBookStatus(const String &uid) {
//   int sensorIndex = getSensorIndexForBook(uid);

//   if (sensorIndex == -1) {
//     return "AVL";
//   }

//   if (sensorIndex >= 0 && sensorIndex < NUM_SENSORS) {
//     return readSensor(sensorPins[sensorIndex]);
//   }

//   return "AVL";
// }

// // ==================== SERIAL COMMUNICATION ====================
// void sendToESP(const String &msg) {
//   while (Serial1.available()) Serial1.read();
//   delay(5);
  
//   Serial1.println(msg);
  
//   Serial.print("TO_ESP: ");
  
//   if (msg.startsWith("SEND:")) {
//     int pipePos = msg.indexOf('|');
//     if (pipePos != -1) {
//       Serial.print(msg.substring(0, pipePos + 1));
//       Serial.print("... [JSON length: ");
//       Serial.print(msg.length() - pipePos - 1);
//       Serial.println("]");
//     } else {
//       int showLen = min(msg.length(), 60);
//       Serial.println(msg.substring(0, showLen));
//     }
//   } else {
//     Serial.println(msg);
//   }
// }

// bool waitForResponse(String &response, unsigned long timeout) {
//   unsigned long start = millis();
//   response = "";
  
//   while (millis() - start < timeout) {
//     while (Serial1.available()) {
//       char c = Serial1.read();
//       if (c == '\n') {
//         response.trim();
//         if (response.length() > 0) {
//           if (!response.startsWith("DEBUG:") && !response.startsWith("WIFI:")) {
//             Serial.print("FROM_ESP: ");
//             Serial.println(response);
//           }
//           return true;
//         }
//       } else if (c != '\r' && response.length() < 2048) {
//         response += c;
//       }
//     }
//     delay(1);
//   }
//   return false;
// }

// // ==================== PENDING QUEUE ====================
// int pendingIndexAddr(int index) { 
//   return PENDING_START + index * PENDING_ENTRY_SIZE; 
// }

// bool addPendingEntry(const String &payload) {
//   if (!payload.startsWith("SEND:")) {
//     return false;
//   }
  
//   int pipePos = payload.indexOf('|');
//   if (pipePos != -1) {
//     String jsonPart = payload.substring(pipePos + 1);
//     if (!isValidJSON(jsonPart)) {
//       return false;
//     }
//   }
  
//   for (int i = 0; i < PENDING_MAX; i++) {
//     int addr = pendingIndexAddr(i);
//     if (EEPROM.read(addr) == 0xFF) {
//       eepromWriteString(addr, payload, PENDING_ENTRY_SIZE - 1);
//       Serial.print("QUEUED: ");
      
//       int showLen = min(payload.length(), 60);
//       Serial.println(payload.substring(0, showLen));
      
//       return true;
//     }
//   }
//   Serial.println("QUEUE FULL!");
//   return false;
// }

// bool popPendingEntry(String &outPayload) {
//   for (int i = 0; i < PENDING_MAX; i++) {
//     int addr = pendingIndexAddr(i);
//     if (EEPROM.read(addr) != 0xFF && EEPROM.read(addr) != 0x00) {
//       outPayload = eepromReadString(addr, PENDING_ENTRY_SIZE - 1);
      
//       for (int j = 0; j < PENDING_ENTRY_SIZE; j++) {
//         EEPROM.write(addr + j, 0xFF);
//       }
      
//       if (outPayload.length() > 10 && 
//           outPayload.startsWith("SEND:") && 
//           outPayload.indexOf('|') > 5) {
        
//         int pipePos = outPayload.indexOf('|');
//         String jsonPart = outPayload.substring(pipePos + 1);
//         if (isValidJSON(jsonPart)) {
//           return true;
//         }
//       }
//     }
//   }
//   return false;
// }

// void flushPendingQueue() {
//   if (!wifiConnected) return;
  
//   String payload;
//   int flushed = 0;
  
//   while (popPendingEntry(payload)) {
//     if (payload.length() > 10) {
//       if (!payload.startsWith("SEND:")) {
//         continue;
//       }
      
//       Serial.print("FLUSHING: ");
//       Serial.println(payload.substring(0, 60));
//       sendToESP(payload);
      
//       String response;
//       if (waitForResponse(response, REQUEST_TIMEOUT)) {
//         if (response.startsWith("SUCCESS:")) {
//           flushed++;
//           delay(50);
//         } else {
//           addPendingEntry(payload);
//           break;
//         }
//       } else {
//         addPendingEntry(payload);
//         break;
//       }
//     }
//   }
  
//   if (flushed > 0) {
//     Serial.print("Successfully flushed ");
//     Serial.print(flushed);
//     Serial.println(" queued entries");
//   }
// }

// // ==================== RFID ====================
// String readRFID() {
//   if (!mfrc522.PICC_IsNewCardPresent()) return "";
//   if (!mfrc522.PICC_ReadCardSerial()) return "";
  
//   String uid = "";
//   for (byte i = 0; i < mfrc522.uid.size; i++) {
//     if (mfrc522.uid.uidByte[i] < 0x10) uid += "0";
//     uid += String(mfrc522.uid.uidByte[i], HEX);
//   }
//   uid.toUpperCase();
//   mfrc522.PICC_HaltA();
//   return uid;
// }

// // ==================== MAIN PROCESSING ====================
// void processCard(const String &cardUID) {
//   if (!systemReady) return;
  
//   if (cardUID == lastCardUID && millis() - lastCardTime < DEBOUNCE_TIME) {
//     return;
//   }
  
//   lastCardUID = cardUID;
//   lastCardTime = millis();
//   setLCDState(LCD_CARD_DETECTED);
  
//   int sensorIndex = findSensorForBook(cardUID);
//   String sensorStatus = "AVL";
//   if (sensorIndex >= 0 && sensorIndex < NUM_SENSORS) {
//     sensorStatus = readSensor(sensorPins[sensorIndex]);
//   }
  
//   Serial.println("=================================");
//   Serial.print("CARD: ");
//   Serial.println(cardUID);
//   Serial.print("SENSOR INDEX: ");
//   Serial.println(sensorIndex);
//   Serial.print("SENSOR STATUS: ");
//   Serial.println(sensorStatus);
//   Serial.println("=================================");
  
//   bool serverIsRegistered = false;
//   String serverTitle = "";
//   String serverAuthor = "";
//   String serverLocation = "";
//   String serverEspStatus = "AVL";
//   int serverSensor = 0;
//   bool usingCachedData = false;
  
//   if (wifiConnected) {
//     sendToESP("GET:" + cardUID);
//     String response;
//     if (waitForResponse(response, REQUEST_TIMEOUT)) {
//       if (response.startsWith("BOOKDATA:")) {
//         int p = response.indexOf('|');
//         if (p != -1) {
//           String serverData = response.substring(p + 1);
          
//           if (serverData != "null") {
//             JsonDocument doc;
//             DeserializationError err = deserializeJson(doc, serverData);
//             if (!err) {
//               serverIsRegistered = doc["isRegistered"] | false;
//               serverTitle = doc["title"] | "";
//               serverAuthor = doc["author"] | "";
//               serverLocation = doc["location"] | "";
//               serverSensor = doc["sensor"] | 0;
              
//               if (doc["espStatus"]) {
//                 serverEspStatus = String(doc["espStatus"].as<const char*>());
//               }
              
//               Serial.print("FIREBASE: Registered=");
//               Serial.print(serverIsRegistered ? "YES" : "NO");
//               Serial.print(", Status=");
//               Serial.println(serverEspStatus);
              
//               cacheBookData(cardUID, serverData);
              
//               if (serverIsRegistered && !isBookRegistered(cardUID)) {
//                 saveRegisteredBook(cardUID, serverSensor);
//               }
//             }
//           } else {
//             Serial.println("Book not found in Firebase");
//           }
//         }
//       }
//     } else {
//       wifiConnected = false;
//       setLCDState(LCD_OFFLINE);
//     }
//   }
  
//   if (!wifiConnected || serverTitle.length() == 0) {
//     String cachedData = getCachedBookData(cardUID);
//     if (cachedData.length() > 0) {
//       JsonDocument doc;
//       DeserializationError err = deserializeJson(doc, cachedData);
//       if (!err) {
//         serverIsRegistered = doc["isRegistered"] | false;
//         serverTitle = doc["title"] | "";
//         serverAuthor = doc["author"] | "";
//         serverLocation = doc["location"] | "";
//         serverSensor = doc["sensor"] | 0;
        
//         if (doc["espStatus"]) {
//           serverEspStatus = String(doc["espStatus"].as<const char*>());
//         }
        
//         usingCachedData = true;
//         Serial.println("USING CACHED DATA");
//       }
//     }
//   }
  
//   if (serverTitle.length() == 0) {
//     if (isBookRegistered(cardUID)) {
//       serverTitle = "Book Title Unknown";
//       serverAuthor = "Author Unknown";
//       serverLocation = "Location Unknown";
//       serverIsRegistered = true;
//       Serial.println("Using local registration (no cache)");
//     } else {
//       serverTitle = "New Book";
//       serverAuthor = "Unknown";
//       serverLocation = "Unassigned";
//       serverIsRegistered = false;
//       setLCDState(LCD_NEW_BOOK);
//       Serial.println("New unregistered book");
//       return;
//     }
//   }
  
//   // Update global variables for LCD display
//   currentBookTitle = serverTitle;
//   currentBookAuthor = serverAuthor;
//   currentBookLocation = serverLocation;
//   currentBookStatus = sensorStatus;
//   currentSensorIndex = sensorIndex;
  
//   // Show book info on LCD (Page 1)
//   setLCDState(LCD_BOOK_INFO_PAGE1);
  
//   JsonDocument doc;
  
//   doc["title"] = serverTitle;
//   doc["author"] = serverAuthor;
//   doc["location"] = serverLocation;
//   doc["isRegistered"] = serverIsRegistered;
//   doc["lastSeen"] = String(millis() / 1000);
//   doc["sensor"] = sensorIndex;
//   doc["espStatus"] = sensorStatus;
  
//   if (sensorStatus == "AVL") doc["status"] = "Available";
//   else if (sensorStatus == "BRW") doc["status"] = "Borrowed";
//   else if (sensorStatus == "OVD") doc["status"] = "Overdue";
//   else if (sensorStatus == "MIS") doc["status"] = "Misplaced";
//   else doc["status"] = "Available";
  
//   doc["lastUpdated"] = String(millis() / 1000);
  
//   if (usingCachedData) {
//     doc["source"] = "cached";
//   }
  
//   String outJson;
//   serializeJson(doc, outJson);
  
//   Serial.print("JSON SIZE: ");
//   Serial.println(outJson.length());
  
//   if (!outJson.endsWith("}")) {
//     outJson += "}";
//   }
  
//   String sendCmd = "SEND:/books/" + cardUID + "|" + outJson;
  
//   if (wifiConnected) {
//     sendToESP(sendCmd);
    
//     String response;
//     if (waitForResponse(response, REQUEST_TIMEOUT)) {
//       if (response.startsWith("SUCCESS:")) {
//         cacheBookData(cardUID, outJson);
        
//         if (serverIsRegistered && !isBookRegistered(cardUID)) {
//           saveRegisteredBook(cardUID, sensorIndex);
//         }
//         Serial.println("Firebase update OK");
//       } else {
//         addPendingEntry(sendCmd);
//       }
//     } else {
//       addPendingEntry(sendCmd);
//     }
//   } else {
//     addPendingEntry(sendCmd);
//     Serial.println("WiFi offline - queued");
//   }
// }

// // ==================== SETUP ====================
// void setup() {
//   Serial.begin(115200);
//   Serial1.begin(115200);
  
//   // Initialize buzzer pin
//   pinMode(BUZZER_PIN, OUTPUT);
//   digitalWrite(BUZZER_PIN, LOW);
  
//   // Initialize LCD
//   lcd.init();
//   lcd.backlight();
//   lcd.clear();
//   lcdInitialized = true;
  
//   // Show professional boot screen
//   lcd.setCursor(0, 0);
//   lcd.print("====================");
//   lcd.setCursor(4, 1);
//   lcd.print("LIBRARY SYSTEM");
//   lcd.setCursor(5, 2);
//   lcd.print("INITIALIZING");
//   lcd.setCursor(0, 3);
//   lcd.print("====================");
  
//   delay(100);
  
//   Serial.println();
//   Serial.println("========================================");
//   Serial.println("    LIBRARY SYSTEM v5.0 - MEGA 2560     ");
//   Serial.println("========================================");
  
//   // Initialize sensor pins
//   for (int i = 0; i < NUM_SENSORS; i++) {
//     pinMode(sensorPins[i], INPUT_PULLUP);
//   }
  
//   // Quick EEPROM check
//   if (isEEPROMCorrupted()) {
//     Serial.println("EEPROM corrupted - clearing");
//     clearCorruptedEEPROM();
//   } else {
//     loadRegisteredBooks();
//   }
  
//   // Initialize RFID
//   SPI.begin();
//   mfrc522.PCD_Init();
  
//   // Quick RFID test
//   byte v = mfrc522.PCD_ReadRegister(MFRC522::VersionReg);
//   if (v == 0x00 || v == 0xFF) {
//     Serial.println("RFID module not found!");
//   } else {
//     Serial.print("RFID module v");
//     Serial.println(v, HEX);
//   }
  
//   // Test ESP connection
//   Serial.println("Testing ESP connection...");
//   sendToESP("PING");
  
//   // Set initial LCD state
//   setLCDState(LCD_INIT);
  
//   Serial.println("Setup complete! System ready.");
// }

// // ==================== LOOP ====================
// void loop() {
//   updateLCD();
  
//   if (systemReady) {
//     String uid = readRFID();
//     if (uid.length() == 8) {
//       processCard(uid);
//     }
//   }
  
//   static String espBuffer = "";
//   while (Serial1.available()) {
//     char c = Serial1.read();
//     if (c == '\n') {
//       espBuffer.trim();
//       if (espBuffer.length() > 0) {
//         if (espBuffer.startsWith("WIFI:")) {
//           if (espBuffer == "WIFI:OK") {
//             if (!wifiConnected) {
//               wifiConnected = true;
//               Serial.println("WiFi connected");
//               beep(BEEP_WIFI_CONNECT, 300);
//               flushPendingQueue();
//             }
//           } else if (espBuffer == "WIFI:OFFLINE") {
//             wifiConnected = false;
//             Serial.println("WiFi offline");
//             beep(BEEP_WIFI_DISCONNECT, 300);
//           }
//         } else if (espBuffer == "PONG") {
//           if (!wifiConnected) {
//             wifiConnected = true;
//             Serial.println("WiFi connected via PONG");
//           }
//         }
//       }
//       espBuffer = "";
//     } else if (c != '\r') {
//       espBuffer += c;
//     }
//   }
  
//   delay(10);
// }


















// #include <Arduino.h>
// #include <SPI.h>
// #include <MFRC522.h>
// #include <EEPROM.h>
// #include <ArduinoJson.h>
// #include <LiquidCrystal_I2C.h>

// // ==================== LCD CONFIGURATION ====================
// #define LCD_ADDRESS 0x27
// #define LCD_COLS 20
// #define LCD_ROWS 4
// LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLS, LCD_ROWS);

// // ==================== BUZZER CONFIGURATION ====================
// #define BUZZER_PIN 48

// // ==================== GLOBAL VARIABLES ====================
// bool wifiConnected = false;
// bool systemReady = false;
// bool lcdInitialized = false;
// bool showingBookInfo = false;  // NEW: Track if we're showing book info
// String lastCardUID = "";
// unsigned long lastCardTime = 0;
// String registeredUIDs[50];
// byte sensorAssignments[50];
// unsigned long beepEndTime = 0;
// int currentBeepFrequency = 0;

// // ==================== LCD STATES ====================
// enum LCDState {
//   LCD_INIT,
//   LCD_WELCOME,
//   LCD_READY,
//   LCD_CARD_DETECTED,
//   LCD_BOOK_INFO_PAGE1,
//   LCD_BOOK_INFO_PAGE2,
//   LCD_BOOK_INFO_PAGE3,
//   LCD_OFFLINE,
//   LCD_NEW_BOOK,
//   LCD_CLEARING_EEPROM,
//   LCD_CORRUPTED_DATA
// };

// LCDState currentLCDState = LCD_INIT;
// unsigned long lcdStateStartTime = 0;
// int animationFrame = 0;
// String currentBookTitle = "";
// String currentBookAuthor = "";
// String currentBookLocation = "";
// String currentBookStatus = "";
// int currentSensorIndex = -1;

// // ==================== BEEP TONES ====================
// #define BEEP_AVL 2000        // High happy tone for Available
// #define BEEP_BRW 1200        // Medium alert tone for Borrowed  
// #define BEEP_SWIPE 800       // Low tone for card swipe
// #define BEEP_WIFI_CONNECT 2500 // Rising tone for WiFi connect
// #define BEEP_WIFI_DISCONNECT 600 // Falling tone for WiFi disconnect
// #define BEEP_ERROR 400       // Error beep
// #define BEEP_CONFIRM 1800    // Confirmation beep
// #define BEEP_PAGE_CHANGE 1500 // Page change beep

// // ==================== ORIGINAL CONFIG ====================
// #define RC522_SS_PIN 53
// #define RC522_RST_PIN 49

// #define REGISTERED_BOOKS_START 0
// #define MAX_REGISTERED_BOOKS 50
// #define UID_LENGTH 8

// #define BOOK_CACHE_START (REGISTERED_BOOKS_START + MAX_REGISTERED_BOOKS * (UID_LENGTH + 1))
// #define BOOK_CACHE_SIZE 512
// #define MAX_CACHED_BOOKS 20

// #define PENDING_START (BOOK_CACHE_START + MAX_CACHED_BOOKS * BOOK_CACHE_SIZE)
// #define PENDING_ENTRY_SIZE 768
// #define PENDING_MAX 8

// #define DEBOUNCE_TIME 3000UL
// #define REQUEST_TIMEOUT 8000UL
// #define PING_INTERVAL 30000UL

// #define NUM_SENSORS 15
// const byte sensorPins[NUM_SENSORS] = {22,23,24,25,26,27,28,29,30,31,32,33,34,35,36};

// // ==================== MFRC522 OBJECT ====================
// MFRC522 mfrc522(RC522_SS_PIN, RC522_RST_PIN);

// // ==================== NON-BLOCKING BUZZER FUNCTIONS ====================
// void startBeep(int frequency, int duration) {
//   if (frequency <= 0) return;
//   tone(BUZZER_PIN, frequency, duration);
//   beepEndTime = millis() + duration;
//   currentBeepFrequency = frequency;
// }

// void stopBeep() {
//   noTone(BUZZER_PIN);
//   currentBeepFrequency = 0;
// }

// void updateBeep() {
//   if (currentBeepFrequency > 0 && millis() > beepEndTime) {
//     stopBeep();
//   }
// }

// void beep(int frequency, int duration) {
//   startBeep(frequency, duration);
// }

// // ==================== LCD HELPER FUNCTIONS ====================
// void clearLine(int row) {
//   lcd.setCursor(0, row);
//   for (int i = 0; i < LCD_COLS; i++) {
//     lcd.print(" ");
//   }
// }

// void centerText(String text, int row) {
//   int textLen = text.length();
//   if (textLen > LCD_COLS) {
//     text = text.substring(0, LCD_COLS);
//     textLen = LCD_COLS;
//   }
//   int padding = (LCD_COLS - textLen) / 2;
//   if (padding < 0) padding = 0;
//   lcd.setCursor(padding, row);
//   lcd.print(text);
// }

// void drawSeparator(int row) {
//   lcd.setCursor(0, row);
//   for (int i = 0; i < LCD_COLS; i++) {
//     lcd.print("-");
//   }
// }

// void drawDoubleSeparator(int row) {
//   lcd.setCursor(0, row);
//   for (int i = 0; i < LCD_COLS; i++) {
//     lcd.print("=");
//   }
// }

// void setLCDState(LCDState newState) {
//   if (currentLCDState != newState) {
//     // Clear screen for all state changes except page transitions within book info
//     if (!((currentLCDState >= LCD_BOOK_INFO_PAGE1 && currentLCDState <= LCD_BOOK_INFO_PAGE3) &&
//           (newState >= LCD_BOOK_INFO_PAGE1 && newState <= LCD_BOOK_INFO_PAGE3))) {
//       if (lcdInitialized) {
//         lcd.clear();
//       }
//     }
    
//     currentLCDState = newState;
//     lcdStateStartTime = millis();
//     animationFrame = 0;
    
//     // Set showingBookInfo flag
//     if (newState >= LCD_BOOK_INFO_PAGE1 && newState <= LCD_BOOK_INFO_PAGE3) {
//       showingBookInfo = true;
//     } else if (newState == LCD_READY) {
//       showingBookInfo = false;
//     }
//   }
// }

// void updateLCD() {
//   static unsigned long lastUpdate = 0;
//   unsigned long currentMillis = millis();
  
//   if (!lcdInitialized) return;
  
//   if (currentMillis - lastUpdate < 150) return;
//   lastUpdate = currentMillis;
  
//   unsigned long stateDuration = currentMillis - lcdStateStartTime;
  
//   // Update beep (non-blocking)
//   updateBeep();
  
//   switch (currentLCDState) {
//     case LCD_INIT: {
//       if (animationFrame == 0) {
//         lcd.clear();
//         drawDoubleSeparator(0);
//         centerText("LIBRARY SYSTEM", 1);
//         centerText("v5.0", 2);
//         drawDoubleSeparator(3);
//         animationFrame = 1;
//       }
//       if (stateDuration > 2000) {
//         setLCDState(LCD_WELCOME);
//       }
//       break;
//     }
      
//     case LCD_WELCOME: {
//       if (animationFrame == 0) {
//         lcd.clear();
//         centerText("LEEJINBOTICS", 0);
//         drawSeparator(1);
//         centerText("INITIALIZING", 2);
//         animationFrame = 1;
//       }
      
//       // Animated progress dots
//       lcd.setCursor(6, 3);
//       int dotCount = (stateDuration / 300) % 8;
//       for (int i = 0; i < 8; i++) {
//         if (i < dotCount) {
//           lcd.print(".");
//         } else {
//           lcd.print(" ");
//         }
//       }
      
//       if (stateDuration > 3000) {
//         setLCDState(LCD_READY);
//         systemReady = true;
//         beep(BEEP_CONFIRM, 200);
//       }
//       break;
//     }
      
//     case LCD_READY: {
//       if (animationFrame == 0) {
//         lcd.clear();
//         drawDoubleSeparator(0);
//         centerText("READY TO SCAN", 1);
//         lcd.setCursor(0, 2);
//         lcd.print("WiFi:");
//         lcd.setCursor(6, 2);
//         if (wifiConnected) {
//           lcd.print("ONLINE  ");
//           lcd.write(223); // Degree symbol
//         } else {
//           lcd.print("OFFLINE ");
//           lcd.print("X");
//         }
//         drawDoubleSeparator(3);
//         animationFrame = 1;
//       }
      
//       // Blinking "SCAN" text
//       if ((stateDuration / 500) % 2 == 0) {
//         lcd.setCursor(8, 1);
//         lcd.print("SCAN");
//       } else {
//         lcd.setCursor(8, 1);
//         lcd.print("    ");
//       }
//       break;
//     }
      
//     case LCD_CARD_DETECTED: {
//       if (animationFrame == 0) {
//         lcd.clear();
//         drawDoubleSeparator(0);
//         centerText("SCANNING CARD", 1);
//         lcd.setCursor(0, 2);
//         lcd.print("UID: ");
//         if (lastCardUID.length() > 12) {
//           lcd.print(lastCardUID.substring(0, 12));
//         } else {
//           lcd.print(lastCardUID);
//         }
//         drawDoubleSeparator(3);
//         beep(BEEP_SWIPE, 150);
//         animationFrame = 1;
//       }
      
//       // Animated scanning indicator
//       int scanPos = (stateDuration / 200) % 16;
//       lcd.setCursor(2, 3);
//       for (int i = 0; i < 16; i++) {
//         if (i == scanPos) {
//           lcd.print(">");
//         } else {
//           lcd.print(" ");
//         }
//       }
      
//       if (stateDuration > 2000) {
//         setLCDState(LCD_BOOK_INFO_PAGE1);
//       }
//       break;
//     }
      
//     case LCD_BOOK_INFO_PAGE1: {
//       if (animationFrame == 0) {
//         lcd.clear();
//         drawDoubleSeparator(0);
//         centerText("BOOK DETAILS", 1);
//         lcd.setCursor(0, 2);
//         lcd.print("Page 1/3");
//         lcd.setCursor(12, 2);
//         lcd.print(">>");
//         drawDoubleSeparator(3);
//         animationFrame = 1;
//         beep(BEEP_PAGE_CHANGE, 100);
//       }
      
//       // Display title with smart truncation
//       lcd.setCursor(0, 1);
//       String displayTitle = currentBookTitle;
//       if (displayTitle.length() > 20) {
//         displayTitle = displayTitle.substring(0, 17) + "...";
//       }
//       centerText(displayTitle, 1);
      
//       if (stateDuration > 3000) {
//         setLCDState(LCD_BOOK_INFO_PAGE2);
//       }
//       break;
//     }
      
//     case LCD_BOOK_INFO_PAGE2: {
//       if (animationFrame == 0) {
//         lcd.clear();
//         drawDoubleSeparator(0);
//         centerText("AUTHOR & STATUS", 1);
//         lcd.setCursor(0, 2);
//         lcd.print("Page 2/3");
//         lcd.setCursor(12, 2);
//         lcd.print(">>");
//         drawDoubleSeparator(3);
//         animationFrame = 1;
//         beep(BEEP_PAGE_CHANGE, 100);
//       }
      
//       // Display author with smart truncation
//       lcd.setCursor(0, 1);
//       String displayAuthor = currentBookAuthor;
//       if (displayAuthor.length() > 20) {
//         displayAuthor = displayAuthor.substring(0, 17) + "...";
//       }
//       centerText(displayAuthor, 1);
      
//       // Display status with icon
//       lcd.setCursor(0, 2);
//       lcd.print("Status: ");
//       if (currentBookStatus == "AVL") {
//         lcd.print("AVAILABLE ");
//         lcd.write(3); // Heart symbol
//         if (animationFrame == 1) {
//           beep(BEEP_AVL, 300);
//           animationFrame = 2;
//         }
//       } else if (currentBookStatus == "BRW") {
//         lcd.print("BORROWED  ");
//         lcd.write(1); // Smiley symbol
//         if (animationFrame == 1) {
//           beep(BEEP_BRW, 300);
//           animationFrame = 2;
//         }
//       } else {
//         lcd.print(currentBookStatus);
//       }
      
//       if (stateDuration > 3000) {
//         setLCDState(LCD_BOOK_INFO_PAGE3);
//       }
//       break;
//     }
      
//     case LCD_BOOK_INFO_PAGE3: {
//       if (animationFrame == 0) {
//         lcd.clear();
//         drawDoubleSeparator(0);
//         centerText("LOCATION INFO", 1);
//         lcd.setCursor(0, 2);
//         lcd.print("Page 3/3");
//         lcd.setCursor(12, 2);
//         lcd.print(">>");
//         drawDoubleSeparator(3);
//         animationFrame = 1;
//         beep(BEEP_PAGE_CHANGE, 100);
//       }
      
//       // Display location with smart truncation
//       lcd.setCursor(0, 1);
//       String displayLoc = currentBookLocation;
//       if (displayLoc.length() > 20) {
//         displayLoc = displayLoc.substring(0, 17) + "...";
//       }
//       centerText(displayLoc, 1);
      
//       // Display sensor info
//       lcd.setCursor(0, 2);
//       lcd.print("Sensor: ");
//       if (currentSensorIndex >= 0 && currentSensorIndex < NUM_SENSORS) {
//         lcd.print(currentSensorIndex);
//         lcd.print(" (Pin ");
//         lcd.print(sensorPins[currentSensorIndex]);
//         lcd.print(")");
//       } else {
//         lcd.print("N/A");
//       }
      
//       if (stateDuration > 3000) {
//         setLCDState(LCD_READY);
//       }
//       break;
//     }
      
//     case LCD_OFFLINE: {
//       if (animationFrame == 0) {
//         lcd.clear();
//         drawDoubleSeparator(0);
//         centerText("OFFLINE MODE", 1);
//         lcd.setCursor(0, 2);
//         lcd.print("Data cached locally");
//         drawDoubleSeparator(3);
//         beep(BEEP_WIFI_DISCONNECT, 300);
//         animationFrame = 1;
//       }
      
//       if ((stateDuration / 400) % 2 == 0) {
//         lcd.setCursor(9, 1);
//         lcd.print("OFFLINE");
//       } else {
//         lcd.setCursor(9, 1);
//         lcd.print("       ");
//       }
//       break;
//     }
      
//     case LCD_NEW_BOOK: {
//       if (animationFrame == 0) {
//         lcd.clear();
//         drawDoubleSeparator(0);
//         centerText("NEW BOOK DETECTED", 1);
//         lcd.setCursor(0, 2);
//         lcd.print("UID: ");
//         if (lastCardUID.length() > 12) {
//           lcd.print(lastCardUID.substring(0, 12));
//         } else {
//           lcd.print(lastCardUID);
//         }
//         drawDoubleSeparator(3);
//         beep(BEEP_SWIPE, 100);
//         delay(50);
//         beep(BEEP_CONFIRM, 150);
//         animationFrame = 1;
//       }
      
//       if ((stateDuration / 300) % 2 == 0) {
//         lcd.setCursor(2, 3);
//         lcd.print("*** REGISTER ME ***");
//       } else {
//         clearLine(3);
//       }
      
//       if (stateDuration > 4000) {
//         setLCDState(LCD_READY);
//       }
//       break;
//     }
      
//     case LCD_CLEARING_EEPROM: {
//       if (animationFrame == 0) {
//         lcd.clear();
//         drawDoubleSeparator(0);
//         centerText("EEPROM MAINTENANCE", 1);
//         lcd.setCursor(0, 2);
//         lcd.print("Cleaning storage...");
//         drawDoubleSeparator(3);
//         animationFrame = 1;
//       }
      
//       int progressBars = (stateDuration / 100) % 21;
//       lcd.setCursor(0, 3);
//       for (int i = 0; i < 20; i++) {
//         if (i < progressBars) {
//           lcd.print("#");
//         } else {
//           lcd.print(" ");
//         }
//       }
      
//       if (stateDuration > 2000) {
//         setLCDState(LCD_WELCOME);
//       }
//       break;
//     }
      
//     case LCD_CORRUPTED_DATA: {
//       if (animationFrame == 0) {
//         lcd.clear();
//         drawDoubleSeparator(0);
//         centerText("DATA INTEGRITY", 1);
//         lcd.setCursor(0, 2);
//         lcd.print("EEPROM Recovery...");
//         drawDoubleSeparator(3);
//         beep(BEEP_ERROR, 200);
//         delay(50);
//         beep(BEEP_ERROR, 200);
//         animationFrame = 1;
//       }
      
//       if ((stateDuration / 200) % 2 == 0) {
//         lcd.setCursor(6, 1);
//         lcd.print("INTEGRITY");
//       } else {
//         lcd.setCursor(6, 1);
//         lcd.print("         ");
//       }
      
//       if (stateDuration > 3000) {
//         setLCDState(LCD_WELCOME);
//       }
//       break;
//     }
//   }
// }

// // ==================== EEPROM HELPERS ====================
// String eepromReadString(int addr, int maxLen) {
//   char buffer[maxLen + 1];
//   int i;
//   for (i = 0; i < maxLen; i++) {
//     byte c = EEPROM.read(addr + i);
//     if (c == 0x00) {
//       break;
//     }
//     if (c == 0x01) {
//       buffer[i] = (char)0x00;
//     } else {
//       buffer[i] = (char)c;
//     }
//   }
//   buffer[i] = '\0';
//   return String(buffer);
// }

// void eepromWriteString(int addr, const String &str, int maxLen) {
//   unsigned int len = min(str.length(), (unsigned int)(maxLen - 1));
  
//   for (unsigned int i = 0; i < len; i++) {
//     char c = str.charAt(i);
//     if (c == (char)0x00) {
//       EEPROM.write(addr + i, 0x01);
//     } else {
//       EEPROM.write(addr + i, (byte)c);
//     }
//   }
  
//   EEPROM.write(addr + len, 0x00);
  
//   for (unsigned int i = len + 1; i < (unsigned int)maxLen; i++) {
//     EEPROM.write(addr + i, 0xFF);
//   }
// }

// // ==================== JSON VALIDATION ====================
// bool isValidJSON(const String &json) {
//   if (json.length() < 10) return false;
//   if (!json.startsWith("{")) return false;
//   if (!json.endsWith("}")) return false;
  
//   int braceCount = 0;
//   bool inString = false;
  
//   for (unsigned int i = 0; i < json.length(); i++) {
//     char c = json.charAt(i);
    
//     if (c == '\\' && i + 1 < json.length()) {
//       i++;
//       continue;
//     }
    
//     if (c == '"') {
//       inString = !inString;
//     }
    
//     if (!inString) {
//       if (c == '{') braceCount++;
//       else if (c == '}') braceCount--;
      
//       if (braceCount < 0) return false;
//     }
//   }
  
//   return (braceCount == 0 && !inString);
// }

// // ==================== CORRUPTION DETECTION ====================
// bool isEEPROMCorrupted() {
//   bool allEmpty = true;
//   for (int i = 0; i < 20; i++) {
//     if (EEPROM.read(REGISTERED_BOOKS_START + i) != 0xFF) {
//       allEmpty = false;
//       break;
//     }
//   }
  
//   if (allEmpty) {
//     return false;
//   }
  
//   for (int i = 0; i < 5; i++) {
//     int addr = REGISTERED_BOOKS_START + i * (UID_LENGTH + 1);
    
//     bool slotEmpty = true;
//     for (int j = 0; j < UID_LENGTH; j++) {
//       byte c = EEPROM.read(addr + j);
//       if (c != 0xFF && c != 0x00) {
//         slotEmpty = false;
//         break;
//       }
//     }
    
//     if (slotEmpty) continue;
    
//     String uid = eepromReadString(addr, UID_LENGTH);
    
//     if (uid.length() == UID_LENGTH) {
//       bool validHex = true;
//       for (unsigned int j = 0; j < uid.length(); j++) {
//         char c = uid.charAt(j);
//         if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) {
//           validHex = false;
//           break;
//         }
//       }
      
//       if (!validHex) {
//         return true;
//       }
//     } else if (uid.length() > 0 && uid.length() < UID_LENGTH) {
//       return true;
//     }
//   }
  
//   return false;
// }

// void clearCorruptedEEPROM() {
//   setLCDState(LCD_CLEARING_EEPROM);
//   Serial.println("Clearing corrupted EEPROM data...");
  
//   for (int i = 0; i < MAX_REGISTERED_BOOKS * (UID_LENGTH + 1); i++) {
//     EEPROM.write(REGISTERED_BOOKS_START + i, 0xFF);
//   }
  
//   for (int i = 0; i < MAX_CACHED_BOOKS * BOOK_CACHE_SIZE; i++) {
//     EEPROM.write(BOOK_CACHE_START + i, 0xFF);
//   }
  
//   for (int i = 0; i < PENDING_MAX * PENDING_ENTRY_SIZE; i++) {
//     EEPROM.write(PENDING_START + i, 0xFF);
//   }
  
//   Serial.println("EEPROM cleared successfully");
//   delay(2000);
//   setLCDState(LCD_WELCOME);
// }

// // ==================== BOOK CACHE SYSTEM ====================
// int getBookCacheAddr(const String &uid) {
//   unsigned int hash = 0;
//   for (unsigned int i = 0; i < uid.length(); i++) {
//     hash = (hash * 31) + uid.charAt(i);
//   }
//   int slot = hash % MAX_CACHED_BOOKS;
//   return BOOK_CACHE_START + slot * BOOK_CACHE_SIZE;
// }

// void cacheBookData(const String &uid, const String &jsonData) {
//   if (uid.length() != 8 || jsonData.length() == 0) return;
  
//   if (!isValidJSON(jsonData)) {
//     return;
//   }
  
//   int addr = getBookCacheAddr(uid);
  
//   unsigned int len = jsonData.length();
//   len = min(len, (unsigned int)(BOOK_CACHE_SIZE - 3));
  
//   EEPROM.write(addr, (len >> 8) & 0xFF);
//   EEPROM.write(addr + 1, len & 0xFF);
  
//   for (unsigned int i = 0; i < len; i++) {
//     char c = jsonData.charAt(i);
//     if (c == (char)0x00) {
//       EEPROM.write(addr + 2 + i, 0x01);
//     } else {
//       EEPROM.write(addr + 2 + i, (byte)c);
//     }
//   }
  
//   EEPROM.write(addr + 2 + len, 0xFF);
// }

// String getCachedBookData(const String &uid) {
//   if (uid.length() != 8) return "";
  
//   int addr = getBookCacheAddr(uid);
  
//   int len = (EEPROM.read(addr) << 8) | EEPROM.read(addr + 1);
  
//   if (len > 0 && len <= (BOOK_CACHE_SIZE - 3)) {
//     String cached = "";
//     for (int i = 0; i < len; i++) {
//       byte c = EEPROM.read(addr + 2 + i);
//       if (c == 0x01) {
//         cached += (char)0x00;
//       } else {
//         cached += (char)c;
//       }
//     }
    
//     if (isValidJSON(cached)) {
//       return cached;
//     }
//   }
  
//   String cached = eepromReadString(addr, BOOK_CACHE_SIZE - 1);
//   if (isValidJSON(cached)) {
//     return cached;
//   }
  
//   return "";
// }

// // ==================== REGISTERED BOOKS MANAGEMENT ====================
// bool isBookRegistered(const String &uid) {
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     if (registeredUIDs[i] == uid) return true;
//   }
//   return false;
// }

// int getBookIndex(const String &uid) {
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     if (registeredUIDs[i] == uid) return i;
//   }
//   return -1;
// }

// void saveRegisteredBook(const String &uid, int sensorIndex = -1) {
//   if (isBookRegistered(uid)) return;
  
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     if (registeredUIDs[i].length() == 0) {
//       registeredUIDs[i] = uid;
      
//       if (sensorIndex >= 0 && sensorIndex < NUM_SENSORS) {
//         sensorAssignments[i] = sensorIndex;
//       } else {
//         for (int s = 0; s < NUM_SENSORS; s++) {
//           bool sensorUsed = false;
//           for (int j = 0; j < MAX_REGISTERED_BOOKS; j++) {
//             if (sensorAssignments[j] == s) {
//               sensorUsed = true;
//               break;
//             }
//           }
//           if (!sensorUsed) {
//             sensorAssignments[i] = s;
//             break;
//           }
//         }
//       }
      
//       int addr = REGISTERED_BOOKS_START + i * (UID_LENGTH + 1);
//       eepromWriteString(addr, uid, UID_LENGTH);
//       return;
//     }
//   }
// }

// void loadRegisteredBooks() {
//   Serial.println("Loading registered books from EEPROM...");
//   int bookCount = 0;
  
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     registeredUIDs[i] = "";
//     sensorAssignments[i] = 255;
//   }
  
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     int addr = REGISTERED_BOOKS_START + i * (UID_LENGTH + 1);
//     String uid = eepromReadString(addr, UID_LENGTH);
    
//     if (uid.length() == UID_LENGTH) {
//       bool validHex = true;
//       for (unsigned int j = 0; j < uid.length(); j++) {
//         char c = uid.charAt(j);
//         if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) {
//           validHex = false;
//           break;
//         }
//       }
      
//       if (validHex) {
//         registeredUIDs[bookCount] = uid;
//         sensorAssignments[bookCount] = 255;
//         bookCount++;
//       }
//     }
//   }
  
//   if (bookCount == 0) {
//     Serial.println("No registered books found in EEPROM");
//   } else {
//     Serial.print("Loaded ");
//     Serial.print(bookCount);
//     Serial.println(" registered books (sensors not assigned yet)");
//   }
// }

// // ==================== SENSOR FUNCTIONS ====================
// String readSensor(int pin) {
//   static String lastState[NUM_SENSORS];

//   int lowCount = 0;

//   for (int i = 0; i < 10; i++) {
//     if (digitalRead(pin) == LOW) lowCount++;
//     delayMicroseconds(100);
//   }

//   if (lowCount > 8) {
//     return "AVL";
//   } else if (lowCount < 2) {
//     return "BRW";
//   } else {
//     for (int i = 0; i < NUM_SENSORS; i++) {
//       if (sensorPins[i] == pin) {
//         return lastState[i];
//       }
//     }
//     return "BRW";
//   }
// }

// int getSensorIndexForBook(const String &uid) {
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     if (registeredUIDs[i] == uid) {
//       if (sensorAssignments[i] >= 0 && sensorAssignments[i] < NUM_SENSORS) {
//         return sensorAssignments[i];
//       }
//       break;
//     }
//   }
//   return -1;
// }

// int findSensorForBook(const String &uid) {
//   int existingSensor = getSensorIndexForBook(uid);
//   if (existingSensor != -1) {
//     return existingSensor;
//   }
  
//   for (int sensorNum = 0; sensorNum < NUM_SENSORS; sensorNum++) {
//     bool sensorInUse = false;
    
//     for (int j = 0; j < MAX_REGISTERED_BOOKS; j++) {
//       if (sensorAssignments[j] == sensorNum) {
//         sensorInUse = true;
//         break;
//       }
//     }
    
//     if (!sensorInUse) {
//       for (int j = 0; j < MAX_REGISTERED_BOOKS; j++) {
//         if (registeredUIDs[j].length() == 0) {
//           registeredUIDs[j] = uid;
//           sensorAssignments[j] = sensorNum;
          
//           int addr = REGISTERED_BOOKS_START + j * (UID_LENGTH + 1);
//           eepromWriteString(addr, uid, UID_LENGTH);
          
//           Serial.print("ASSIGNED: Book ");
//           Serial.print(uid);
//           Serial.print(" → Sensor ");
//           Serial.print(sensorNum);
//           Serial.print(" (Pin ");
//           Serial.print(sensorPins[sensorNum]);
//           Serial.println(")");
//           return sensorNum;
//         }
//       }
//     }
//   }
  
//   Serial.println("WARNING: All sensors in use! Using sensor 0 as fallback");
//   return 0;
// }

// String getBookStatus(const String &uid) {
//   int sensorIndex = getSensorIndexForBook(uid);

//   if (sensorIndex == -1) {
//     return "AVL";
//   }

//   if (sensorIndex >= 0 && sensorIndex < NUM_SENSORS) {
//     return readSensor(sensorPins[sensorIndex]);
//   }

//   return "AVL";
// }

// // ==================== SERIAL COMMUNICATION ====================
// void sendToESP(const String &msg) {
//   while (Serial1.available()) Serial1.read();
//   delay(5);
  
//   Serial1.println(msg);
  
//   Serial.print("TO_ESP: ");
  
//   if (msg.startsWith("SEND:")) {
//     int pipePos = msg.indexOf('|');
//     if (pipePos != -1) {
//       Serial.print(msg.substring(0, pipePos + 1));
//       Serial.print("... [JSON length: ");
//       Serial.print(msg.length() - pipePos - 1);
//       Serial.println("]");
//     } else {
//       int showLen = min(msg.length(), 60);
//       Serial.println(msg.substring(0, showLen));
//     }
//   } else {
//     Serial.println(msg);
//   }
// }

// bool waitForResponse(String &response, unsigned long timeout) {
//   unsigned long start = millis();
//   response = "";
  
//   while (millis() - start < timeout) {
//     while (Serial1.available()) {
//       char c = Serial1.read();
//       if (c == '\n') {
//         response.trim();
//         if (response.length() > 0) {
//           if (!response.startsWith("DEBUG:") && !response.startsWith("WIFI:")) {
//             Serial.print("FROM_ESP: ");
//             Serial.println(response);
//           }
//           return true;
//         }
//       } else if (c != '\r' && response.length() < 2048) {
//         response += c;
//       }
//     }
//     delay(1);
//   }
//   return false;
// }

// // ==================== PENDING QUEUE ====================
// int pendingIndexAddr(int index) { 
//   return PENDING_START + index * PENDING_ENTRY_SIZE; 
// }

// bool addPendingEntry(const String &payload) {
//   if (!payload.startsWith("SEND:")) {
//     return false;
//   }
  
//   int pipePos = payload.indexOf('|');
//   if (pipePos != -1) {
//     String jsonPart = payload.substring(pipePos + 1);
//     if (!isValidJSON(jsonPart)) {
//       return false;
//     }
//   }
  
//   for (int i = 0; i < PENDING_MAX; i++) {
//     int addr = pendingIndexAddr(i);
//     if (EEPROM.read(addr) == 0xFF) {
//       eepromWriteString(addr, payload, PENDING_ENTRY_SIZE - 1);
//       Serial.print("QUEUED: ");
      
//       int showLen = min(payload.length(), 60);
//       Serial.println(payload.substring(0, showLen));
      
//       return true;
//     }
//   }
//   Serial.println("QUEUE FULL!");
//   return false;
// }

// bool popPendingEntry(String &outPayload) {
//   for (int i = 0; i < PENDING_MAX; i++) {
//     int addr = pendingIndexAddr(i);
//     if (EEPROM.read(addr) != 0xFF && EEPROM.read(addr) != 0x00) {
//       outPayload = eepromReadString(addr, PENDING_ENTRY_SIZE - 1);
      
//       for (int j = 0; j < PENDING_ENTRY_SIZE; j++) {
//         EEPROM.write(addr + j, 0xFF);
//       }
      
//       if (outPayload.length() > 10 && 
//           outPayload.startsWith("SEND:") && 
//           outPayload.indexOf('|') > 5) {
        
//         int pipePos = outPayload.indexOf('|');
//         String jsonPart = outPayload.substring(pipePos + 1);
//         if (isValidJSON(jsonPart)) {
//           return true;
//         }
//       }
//     }
//   }
//   return false;
// }

// void flushPendingQueue() {
//   if (!wifiConnected) return;
  
//   String payload;
//   int flushed = 0;
  
//   while (popPendingEntry(payload)) {
//     if (payload.length() > 10) {
//       if (!payload.startsWith("SEND:")) {
//         continue;
//       }
      
//       Serial.print("FLUSHING: ");
//       Serial.println(payload.substring(0, 60));
//       sendToESP(payload);
      
//       String response;
//       if (waitForResponse(response, REQUEST_TIMEOUT)) {
//         if (response.startsWith("SUCCESS:")) {
//           flushed++;
//           delay(50);
//         } else {
//           addPendingEntry(payload);
//           break;
//         }
//       } else {
//         addPendingEntry(payload);
//         break;
//       }
//     }
//   }
  
//   if (flushed > 0) {
//     Serial.print("Successfully flushed ");
//     Serial.print(flushed);
//     Serial.println(" queued entries");
//   }
// }

// // ==================== RFID ====================
// String readRFID() {
//   if (!mfrc522.PICC_IsNewCardPresent()) return "";
//   if (!mfrc522.PICC_ReadCardSerial()) return "";
  
//   String uid = "";
//   for (byte i = 0; i < mfrc522.uid.size; i++) {
//     if (mfrc522.uid.uidByte[i] < 0x10) uid += "0";
//     uid += String(mfrc522.uid.uidByte[i], HEX);
//   }
//   uid.toUpperCase();
//   mfrc522.PICC_HaltA();
//   return uid;
// }

// // ==================== MAIN PROCESSING ====================
// void processCard(const String &cardUID) {
//   if (!systemReady) return;
  
//   // Don't process new cards while still showing book info
//   if (showingBookInfo) {
//     return;
//   }
  
//   if (cardUID == lastCardUID && millis() - lastCardTime < DEBOUNCE_TIME) {
//     return;
//   }
  
//   lastCardUID = cardUID;
//   lastCardTime = millis();
//   setLCDState(LCD_CARD_DETECTED);
  
//   int sensorIndex = findSensorForBook(cardUID);
//   String sensorStatus = "AVL";
//   if (sensorIndex >= 0 && sensorIndex < NUM_SENSORS) {
//     sensorStatus = readSensor(sensorPins[sensorIndex]);
//   }
  
//   Serial.println("=================================");
//   Serial.print("CARD: ");
//   Serial.println(cardUID);
//   Serial.print("SENSOR INDEX: ");
//   Serial.println(sensorIndex);
//   Serial.print("SENSOR STATUS: ");
//   Serial.println(sensorStatus);
//   Serial.println("=================================");
  
//   bool serverIsRegistered = false;
//   String serverTitle = "";
//   String serverAuthor = "";
//   String serverLocation = "";
//   String serverEspStatus = "AVL";
//   int serverSensor = 0;
//   bool usingCachedData = false;
  
//   if (wifiConnected) {
//     sendToESP("GET:" + cardUID);
//     String response;
//     if (waitForResponse(response, REQUEST_TIMEOUT)) {
//       if (response.startsWith("BOOKDATA:")) {
//         int p = response.indexOf('|');
//         if (p != -1) {
//           String serverData = response.substring(p + 1);
          
//           if (serverData != "null") {
//             JsonDocument doc;
//             DeserializationError err = deserializeJson(doc, serverData);
//             if (!err) {
//               serverIsRegistered = doc["isRegistered"] | false;
//               serverTitle = doc["title"] | "";
//               serverAuthor = doc["author"] | "";
//               serverLocation = doc["location"] | "";
//               serverSensor = doc["sensor"] | 0;


//               // ADD THESE TRIM LINES:
//               serverTitle.trim();
//               serverAuthor.trim(); 
//               serverLocation.trim();
              
//               if (doc["espStatus"]) {
//                 serverEspStatus = String(doc["espStatus"].as<const char*>());
//               }
              
//               Serial.print("FIREBASE: Registered=");
//               Serial.print(serverIsRegistered ? "YES" : "NO");
//               Serial.print(", Status=");
//               Serial.println(serverEspStatus);
              
//               cacheBookData(cardUID, serverData);
              
//               if (serverIsRegistered && !isBookRegistered(cardUID)) {
//                 saveRegisteredBook(cardUID, serverSensor);
//               }
//             }
//           } else {
//             Serial.println("Book not found in Firebase");
//           }
//         }
//       }
//     } else {
//       wifiConnected = false;
//       setLCDState(LCD_OFFLINE);
//     }
//   }
  
//   if (!wifiConnected || serverTitle.length() == 0) {
//     String cachedData = getCachedBookData(cardUID);
//     if (cachedData.length() > 0) {
//       JsonDocument doc;
//       DeserializationError err = deserializeJson(doc, cachedData);
//       if (!err) {
//         serverIsRegistered = doc["isRegistered"] | false;
//         serverTitle = doc["title"] | "";
//         serverAuthor = doc["author"] | "";
//         serverLocation = doc["location"] | "";
//         serverSensor = doc["sensor"] | 0;
        
//         if (doc["espStatus"]) {
//           serverEspStatus = String(doc["espStatus"].as<const char*>());
//         }
        
//         usingCachedData = true;
//         Serial.println("USING CACHED DATA");
//       }
//     }
//   }
  
//   if (serverTitle.length() == 0) {
//     if (isBookRegistered(cardUID)) {
//       serverTitle = "Book Title Unknown";
//       serverAuthor = "Author Unknown";
//       serverLocation = "Location Unknown";
//       serverIsRegistered = true;
//       Serial.println("Using local registration (no cache)");
//     } else {
//       serverTitle = "New Book";
//       serverAuthor = "Unknown";
//       serverLocation = "Unassigned";
//       serverIsRegistered = false;
//       setLCDState(LCD_NEW_BOOK);
//       Serial.println("New unregistered book");
//       return;
//     }
//   }
  
//   // Update global variables for LCD display
//   currentBookTitle = serverTitle;
//   currentBookAuthor = serverAuthor;
//   currentBookLocation = serverLocation;
//   currentBookStatus = sensorStatus;
//   currentSensorIndex = sensorIndex;
  
//   // The LCD state will now automatically progress through:
//   // 1. LCD_CARD_DETECTED (2 seconds)
//   // 2. LCD_BOOK_INFO_PAGE1 (3 seconds)  
//   // 3. LCD_BOOK_INFO_PAGE2 (3 seconds)
//   // 4. LCD_BOOK_INFO_PAGE3 (3 seconds)
//   // 5. Back to LCD_READY automatically
  
//   JsonDocument doc;
  
//   doc["title"] = serverTitle;
//   doc["author"] = serverAuthor;
//   doc["location"] = serverLocation;
//   doc["isRegistered"] = serverIsRegistered;
//   doc["lastSeen"] = String(millis() / 1000);
//   doc["sensor"] = sensorIndex;
//   doc["espStatus"] = sensorStatus;
  
//   if (sensorStatus == "AVL") doc["status"] = "Available";
//   else if (sensorStatus == "BRW") doc["status"] = "Borrowed";
//   else if (sensorStatus == "OVD") doc["status"] = "Overdue";
//   else if (sensorStatus == "MIS") doc["status"] = "Misplaced";
//   else doc["status"] = "Available";
  
//   doc["lastUpdated"] = String(millis() / 1000);
  
//   if (usingCachedData) {
//     doc["source"] = "cached";
//   }
  
//   String outJson;
//   serializeJson(doc, outJson);
  
//   Serial.print("JSON SIZE: ");
//   Serial.println(outJson.length());
  
//   if (!outJson.endsWith("}")) {
//     outJson += "}";
//   }
  
//   String sendCmd = "SEND:/books/" + cardUID + "|" + outJson;
  
//   if (wifiConnected) {
//     sendToESP(sendCmd);
    
//     String response;
//     if (waitForResponse(response, REQUEST_TIMEOUT)) {
//       if (response.startsWith("SUCCESS:")) {
//         cacheBookData(cardUID, outJson);
        
//         if (serverIsRegistered && !isBookRegistered(cardUID)) {
//           saveRegisteredBook(cardUID, sensorIndex);
//         }
//         Serial.println("Firebase update OK");
//       } else {
//         addPendingEntry(sendCmd);
//       }
//     } else {
//       addPendingEntry(sendCmd);
//     }
//   } else {
//     addPendingEntry(sendCmd);
//     Serial.println("WiFi offline - queued");
//   }
// }

// // ==================== SETUP ====================
// void setup() {
//   Serial.begin(115200);
//   Serial1.begin(115200);
  
//   // Initialize buzzer pin
//   pinMode(BUZZER_PIN, OUTPUT);
//   digitalWrite(BUZZER_PIN, LOW);
  
//   // Initialize LCD
//   lcd.init();
//   lcd.backlight();
//   lcd.clear();
//   lcdInitialized = true;
  
//   // Show immediate professional boot screen
//   lcd.setCursor(0, 0);
//   lcd.print("====================");
//   lcd.setCursor(4, 1);
//   lcd.print("LIBRARY SYSTEM");
//   lcd.setCursor(5, 2);
//   lcd.print("INITIALIZING");
//   lcd.setCursor(0, 3);
//   lcd.print("====================");
  
//   delay(100);
  
//   Serial.println();
//   Serial.println("========================================");
//   Serial.println("    LIBRARY SYSTEM v5.0 - MEGA 2560     ");
//   Serial.println("========================================");
  
//   // Initialize sensor pins
//   for (int i = 0; i < NUM_SENSORS; i++) {
//     pinMode(sensorPins[i], INPUT_PULLUP);
//   }
  
//   // Quick EEPROM check
//   if (isEEPROMCorrupted()) {
//     Serial.println("EEPROM corrupted - clearing");
//     clearCorruptedEEPROM();
//   } else {
//     loadRegisteredBooks();
//   }
  
//   // Initialize RFID
//   SPI.begin();
//   mfrc522.PCD_Init();
  
//   // Quick RFID test
//   byte v = mfrc522.PCD_ReadRegister(MFRC522::VersionReg);
//   if (v == 0x00 || v == 0xFF) {
//     Serial.println("RFID module not found!");
//   } else {
//     Serial.print("RFID module v");
//     Serial.println(v, HEX);
//   }
  
//   // Test ESP connection
//   Serial.println("Testing ESP connection...");
//   sendToESP("PING");
  
//   // Set initial LCD state
//   setLCDState(LCD_INIT);
  
//   Serial.println("Setup complete! System ready.");
// }

// // ==================== LOOP ====================
// void loop() {
//   // Update LCD and non-blocking beep
//   updateLCD();
  
//   // Handle RFID if system is ready
//   // if (systemReady && !showingBookInfo) {
//   //   String uid = readRFID();
//   //   if (uid.length() == 8) {
//   //     processCard(uid);
//   //   }
//   // }

//       if (systemReady && currentLCDState == LCD_READY) {
//       String uid = readRFID();
//       if (uid.length() == 8) {
//         processCard(uid);
//       }
//     }
  
//   static String espBuffer = "";
//   while (Serial1.available()) {
//     char c = Serial1.read();
//     if (c == '\n') {
//       espBuffer.trim();
//       if (espBuffer.length() > 0) {
//         if (espBuffer.startsWith("WIFI:")) {
//           if (espBuffer == "WIFI:OK") {
//             if (!wifiConnected) {
//               wifiConnected = true;
//               Serial.println("WiFi connected");
//               beep(BEEP_WIFI_CONNECT, 300);
//               flushPendingQueue();
//             }
//           } else if (espBuffer == "WIFI:OFFLINE") {
//             wifiConnected = false;
//             Serial.println("WiFi offline");
//             beep(BEEP_WIFI_DISCONNECT, 300);
//           }
//         } else if (espBuffer == "PONG") {
//           if (!wifiConnected) {
//             wifiConnected = true;
//             Serial.println("WiFi connected via PONG");
//           }
//         }
//       }
//       espBuffer = "";
//     } else if (c != '\r') {
//       espBuffer += c;
//     }
//   }
  
//   delay(10);
// }
 


















//=========================================LCD PERFECT-CLOUD/ OFFLINE PERFECT============================================
//=========================================LCD PERFECT-CLOUD/ OFFLINE PERFECT============================================
//=========================================LCD PERFECT-CLOUD/ OFFLINE PERFECT============================================
//=========================================LCD PERFECT-CLOUD/ OFFLINE PERFECT============================================
//=========================================LCD PERFECT-CLOUD/ OFFLINE PERFECT============================================
//=========================================LCD PERFECT-CLOUD/ OFFLINE PERFECT============================================
//=========================================LCD PERFECT-CLOUD/ OFFLINE PERFECT============================================
//=========================================LCD PERFECT-CLOUD/ OFFLINE PERFECT============================================
//=========================================LCD PERFECT-CLOUD/ OFFLINE PERFECT============================================
//=========================================LCD PERFECT-CLOUD/ OFFLINE PERFECT============================================
//=========================================LCD PERFECT-CLOUD/ OFFLINE PERFECT============================================
//=========================================LCD PERFECT-CLOUD/ OFFLINE PERFECT============================================
//=========================================LCD PERFECT-CLOUD/ OFFLINE PERFECT============================================
//=========================================LCD PERFECT-CLOUD/ OFFLINE PERFECT============================================
//=========================================LCD PERFECT-CLOUD/ OFFLINE PERFECT============================================
//=========================================LCD PERFECT-CLOUD/ OFFLINE PERFECT============================================
//=========================================LCD PERFECT-CLOUD/ OFFLINE PERFECT============================================
//=========================================LCD PERFECT-CLOUD/ OFFLINE PERFECT============================================
//=========================================LCD PERFECT-CLOUD/ OFFLINE PERFECT============================================
//=========================================LCD PERFECT-CLOUD/ OFFLINE PERFECT============================================
//=========================================LCD PERFECT-CLOUD/ OFFLINE PERFECT============================================
//=========================================LCD PERFECT-CLOUD/ OFFLINE PERFECT============================================
//=========================================LCD PERFECT-CLOUD/ OFFLINE PERFECT============================================
//=========================================LCD PERFECT-CLOUD/ OFFLINE PERFECT============================================

// #include <Arduino.h>
// #include <SPI.h>
// #include <MFRC522.h>
// #include <EEPROM.h>
// #include <ArduinoJson.h>
// #include <LiquidCrystal_I2C.h>

// // ==================== LCD CONFIGURATION ====================
// #define LCD_ADDRESS 0x27
// #define LCD_COLS 20
// #define LCD_ROWS 4
// LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLS, LCD_ROWS);

// // ==================== BUZZER CONFIGURATION ====================
// #define BUZZER_PIN 48

// // ==================== GLOBAL VARIABLES ====================
// bool wifiConnected = false;
// bool systemReady = false;
// bool lcdInitialized = false;
// bool showingBookInfo = false;
// String lastCardUID = "";
// unsigned long lastCardTime = 0;
// String registeredUIDs[50];
// byte sensorAssignments[50];
// unsigned long beepEndTime = 0;
// int currentBeepFrequency = 0;

// // ==================== LCD STATES ====================
// enum LCDState {
//   LCD_INIT,
//   LCD_WELCOME,
//   LCD_READY,
//   LCD_CARD_DETECTED,
//   LCD_BOOK_INFO_PAGE1,
//   LCD_BOOK_INFO_PAGE2,
//   LCD_BOOK_INFO_PAGE3,
//   LCD_OFFLINE,
//   LCD_NEW_BOOK,
//   LCD_CLEARING_EEPROM,
//   LCD_CORRUPTED_DATA
// };

// LCDState currentLCDState = LCD_INIT;
// unsigned long lcdStateStartTime = 0;
// int animationFrame = 0;
// String currentBookTitle = "";
// String currentBookAuthor = "";
// String currentBookLocation = "";
// String currentBookStatus = "";
// int currentSensorIndex = -1;

// // ==================== BEEP TONES ====================
// #define BEEP_AVL 2000
// #define BEEP_BRW 1200
// #define BEEP_SWIPE 800
// #define BEEP_WIFI_CONNECT 2500
// #define BEEP_WIFI_DISCONNECT 600
// #define BEEP_ERROR 400
// #define BEEP_CONFIRM 1800
// #define BEEP_PAGE_CHANGE 1500

// // ==================== ORIGINAL CONFIG ====================
// #define RC522_SS_PIN 53
// #define RC522_RST_PIN 49

// #define REGISTERED_BOOKS_START 0
// #define MAX_REGISTERED_BOOKS 50
// #define UID_LENGTH 8

// #define BOOK_CACHE_START (REGISTERED_BOOKS_START + MAX_REGISTERED_BOOKS * (UID_LENGTH + 1))
// #define BOOK_CACHE_SIZE 512
// #define MAX_CACHED_BOOKS 20

// #define PENDING_START (BOOK_CACHE_START + MAX_CACHED_BOOKS * BOOK_CACHE_SIZE)
// #define PENDING_ENTRY_SIZE 768
// #define PENDING_MAX 8

// #define DEBOUNCE_TIME 3000UL
// #define REQUEST_TIMEOUT 8000UL
// #define PING_INTERVAL 30000UL

// #define NUM_SENSORS 15
// const byte sensorPins[NUM_SENSORS] = {22,23,24,25,26,27,28,29,30,31,32,33,34,35,36};

// // ==================== MFRC522 OBJECT ====================
// MFRC522 mfrc522(RC522_SS_PIN, RC522_RST_PIN);

// // ==================== LCD HELPER FUNCTIONS - FIXED ====================
// String cleanDisplayText(String text) {
//   text.trim();
//   text.replace("\n", "");
//   text.replace("\r", "");
//   text.replace("\t", " ");
  
//   // Remove multiple spaces
//   while (text.indexOf("  ") != -1) {
//     text.replace("  ", " ");
//   }
  
//   return text;
// }

// void clearLine(int row) {
//   lcd.setCursor(0, row);
//   for (int i = 0; i < LCD_COLS; i++) {
//     lcd.print(" ");
//   }
//   lcd.setCursor(0, row);
// }

// void centerText(String text, int row) {
//   text = cleanDisplayText(text);
//   int textLen = text.length();
  
//   if (textLen > LCD_COLS) {
//     text = text.substring(0, LCD_COLS - 3) + "...";
//     textLen = text.length();
//   }
  
//   int padding = max(0, (LCD_COLS - textLen) / 2);
//   clearLine(row);
//   lcd.setCursor(padding, row);
//   lcd.print(text);
// }

// void drawSeparator(int row) {
//   lcd.setCursor(0, row);
//   for (int i = 0; i < LCD_COLS; i++) {
//     lcd.print("-");
//   }
// }

// void drawDoubleSeparator(int row) {
//   lcd.setCursor(0, row);
//   for (int i = 0; i < LCD_COLS; i++) {
//     lcd.print("=");
//   }
// }

// // ==================== NON-BLOCKING BUZZER FUNCTIONS ====================
// void startBeep(int frequency, int duration) {
//   if (frequency <= 0) return;
//   tone(BUZZER_PIN, frequency, duration);
//   beepEndTime = millis() + duration;
//   currentBeepFrequency = frequency;
// }

// void stopBeep() {
//   noTone(BUZZER_PIN);
//   currentBeepFrequency = 0;
// }

// void updateBeep() {
//   if (currentBeepFrequency > 0 && millis() > beepEndTime) {
//     stopBeep();
//   }
// }

// void beep(int frequency, int duration) {
//   startBeep(frequency, duration);
// }

// // ==================== LCD STATES ====================
// void setLCDState(LCDState newState) {
//   if (currentLCDState != newState) {
//     if (!((currentLCDState >= LCD_BOOK_INFO_PAGE1 && currentLCDState <= LCD_BOOK_INFO_PAGE3) &&
//           (newState >= LCD_BOOK_INFO_PAGE1 && newState <= LCD_BOOK_INFO_PAGE3))) {
//       if (lcdInitialized) {
//         lcd.clear();
//         delay(2);
//       }
//     }
    
//     currentLCDState = newState;
//     lcdStateStartTime = millis();
//     animationFrame = 0;
    
//     if (newState >= LCD_BOOK_INFO_PAGE1 && newState <= LCD_BOOK_INFO_PAGE3) {
//       showingBookInfo = true;
//     } else if (newState == LCD_READY) {
//       showingBookInfo = false;
//     }
//   }
// }

// void updateLCD() {
//   static unsigned long lastUpdate = 0;
//   unsigned long currentMillis = millis();
  
//   if (!lcdInitialized) return;
  
//   if (currentMillis - lastUpdate < 150) return;
//   lastUpdate = currentMillis;
  
//   unsigned long stateDuration = currentMillis - lcdStateStartTime;
  
//   updateBeep();
  
//   switch (currentLCDState) {
//     case LCD_INIT: {
//       if (animationFrame == 0) {
//         lcd.clear();
//         drawDoubleSeparator(0);
//         centerText("LIBRARY SYSTEM", 1);
//         centerText("v5.0", 2);
//         drawDoubleSeparator(3);
//         animationFrame = 1;
//       }
//       if (stateDuration > 2000) {
//         setLCDState(LCD_WELCOME);
//       }
//       break;
//     }
      
//     case LCD_WELCOME: {
//       if (animationFrame == 0) {
//         lcd.clear();
//         centerText("LEEJINBOTICS", 0);
//         drawSeparator(1);
//         centerText("INITIALIZING", 2);
//         animationFrame = 1;
//       }
      
//       lcd.setCursor(6, 3);
//       int dotCount = (stateDuration / 300) % 8;
//       for (int i = 0; i < 8; i++) {
//         if (i < dotCount) {
//           lcd.print(".");
//         } else {
//           lcd.print(" ");
//         }
//       }
      
//       if (stateDuration > 3000) {
//         setLCDState(LCD_READY);
//         systemReady = true;
//         beep(BEEP_CONFIRM, 200);
//       }
//       break;
//     }
      
//     case LCD_READY: {
//       static unsigned long lastWifiUpdate = 0;
//       static bool lastWifiState = wifiConnected;
      
//       if (animationFrame == 0) {
//         lcd.clear();
//         drawDoubleSeparator(0);
//         centerText("READY TO SCAN", 1);
//         lcd.setCursor(0, 2);
//         lcd.print("WiFi:");
        
//         // Initial display
//         lcd.setCursor(6, 2);
//         if (wifiConnected) {
//           lcd.print("ONLINE  ");
//           lcd.write(223);
//         } else {
//           lcd.print("OFFLINE ");
//           lcd.print("X");
//         }
        
//         drawDoubleSeparator(3);
//         animationFrame = 1;
//         lastWifiState = wifiConnected;
//         lastWifiUpdate = millis();
//       }
      
//       // Update WiFi status regularly (every 500ms)
//       if (millis() - lastWifiUpdate > 500) {
//         if (wifiConnected != lastWifiState) {
//           lcd.setCursor(6, 2);
//           if (wifiConnected) {
//             lcd.print("ONLINE  ");
//             lcd.write(223);
//           } else {
//             lcd.print("OFFLINE ");
//             lcd.print("X");
//           }
//           lastWifiState = wifiConnected;
//         }
//         lastWifiUpdate = millis();
//       }
      
//       // Blinking "SCAN" text
//       if ((stateDuration / 500) % 2 == 0) {
//         lcd.setCursor(8, 1);
//         lcd.print("SCAN");
//       } else {
//         lcd.setCursor(8, 1);
//         lcd.print("    ");
//       }
//       break;
//     }
      
//     case LCD_CARD_DETECTED: {
//       if (animationFrame == 0) {
//         lcd.clear();
//         drawDoubleSeparator(0);
//         centerText("SCANNING CARD", 1);
//         lcd.setCursor(0, 2);
//         lcd.print("UID: ");
//         if (lastCardUID.length() > 12) {
//           lcd.print(lastCardUID.substring(0, 12));
//         } else {
//           lcd.print(lastCardUID);
//         }
//         drawDoubleSeparator(3);
//         beep(BEEP_SWIPE, 150);
//         animationFrame = 1;
//       }
      
//       int scanPos = (stateDuration / 200) % 16;
//       lcd.setCursor(2, 3);
//       for (int i = 0; i < 16; i++) {
//         if (i == scanPos) {
//           lcd.print(">");
//         } else {
//           lcd.print(" ");
//         }
//       }
      
//       if (stateDuration > 2000) {
//         setLCDState(LCD_BOOK_INFO_PAGE1);
//       }
//       break;
//     }
      
//     case LCD_BOOK_INFO_PAGE1: {
//       if (animationFrame == 0) {
//         lcd.clear();
//         drawDoubleSeparator(0);
//         centerText("BOOK DETAILS", 1);
//         lcd.setCursor(0, 2);
//         lcd.print("Page 1/3");
//         lcd.setCursor(12, 2);
//         lcd.print(">>");
//         drawDoubleSeparator(3);
//         animationFrame = 1;
//         beep(BEEP_PAGE_CHANGE, 100);
//       }
      
//       String displayTitle = cleanDisplayText(currentBookTitle);
//       centerText(displayTitle, 1);
      
//       if (stateDuration > 3000) {
//         setLCDState(LCD_BOOK_INFO_PAGE2);
//       }
//       break;
//     }
      
//     case LCD_BOOK_INFO_PAGE2: {
//       if (animationFrame == 0) {
//         lcd.clear();
//         drawDoubleSeparator(0);
//         centerText("AUTHOR & STATUS", 1);
//         lcd.setCursor(0, 2);
//         lcd.print("Page 2/3");
//         lcd.setCursor(12, 2);
//         lcd.print(">>");
//         drawDoubleSeparator(3);
//         animationFrame = 1;
//         beep(BEEP_PAGE_CHANGE, 100);
//       }
      
//       String displayAuthor = cleanDisplayText(currentBookAuthor);
//       centerText(displayAuthor, 1);
      
//       lcd.setCursor(0, 2);
//       lcd.print("Status: ");
//       if (currentBookStatus == "AVL") {
//         lcd.print("AVAILABLE ");
//         lcd.write(3);
//         if (animationFrame == 1) {
//           beep(BEEP_AVL, 300);
//           animationFrame = 2;
//         }
//       } else if (currentBookStatus == "BRW") {
//         lcd.print("BORROWED  ");
//         lcd.write(1);
//         if (animationFrame == 1) {
//           beep(BEEP_BRW, 300);
//           animationFrame = 2;
//         }
//       } else {
//         lcd.print(currentBookStatus);
//       }
      
//       if (stateDuration > 3000) {
//         setLCDState(LCD_BOOK_INFO_PAGE3);
//       }
//       break;
//     }
      
//     case LCD_BOOK_INFO_PAGE3: {
//       if (animationFrame == 0) {
//         lcd.clear();
//         drawDoubleSeparator(0);
//         centerText("LOCATION INFO", 1);
//         lcd.setCursor(0, 2);
//         lcd.print("Page 3/3");
//         lcd.setCursor(12, 2);
//         lcd.print(">>");
//         drawDoubleSeparator(3);
//         animationFrame = 1;
//         beep(BEEP_PAGE_CHANGE, 100);
//       }
      
//       String displayLoc = cleanDisplayText(currentBookLocation);
//       centerText(displayLoc, 1);
      
//       lcd.setCursor(0, 2);
//       lcd.print("Sensor: ");
//       if (currentSensorIndex >= 0 && currentSensorIndex < NUM_SENSORS) {
//         lcd.print(currentSensorIndex);
//         lcd.print(" (Pin ");
//         lcd.print(sensorPins[currentSensorIndex]);
//         lcd.print(")");
//       } else {
//         lcd.print("N/A");
//       }
      
//       if (stateDuration > 3000) {
//         setLCDState(LCD_READY);
//       }
//       break;
//     }
      
//     case LCD_OFFLINE: {
//       if (animationFrame == 0) {
//         lcd.clear();
//         drawDoubleSeparator(0);
//         centerText("OFFLINE MODE", 1);
//         lcd.setCursor(0, 2);
//         lcd.print("Data cached locally");
//         drawDoubleSeparator(3);
//         beep(BEEP_WIFI_DISCONNECT, 300);
//         animationFrame = 1;
//       }
      
//       if ((stateDuration / 400) % 2 == 0) {
//         lcd.setCursor(9, 1);
//         lcd.print("OFFLINE");
//       } else {
//         lcd.setCursor(9, 1);
//         lcd.print("       ");
//       }
//       break;
//     }
      
//     case LCD_NEW_BOOK: {
//       if (animationFrame == 0) {
//         lcd.clear();
//         drawDoubleSeparator(0);
//         centerText("NEW BOOK DETECTED", 1);
//         lcd.setCursor(0, 2);
//         lcd.print("UID: ");
//         if (lastCardUID.length() > 12) {
//           lcd.print(lastCardUID.substring(0, 12));
//         } else {
//           lcd.print(lastCardUID);
//         }
//         drawDoubleSeparator(3);
//         beep(BEEP_SWIPE, 100);
//         delay(50);
//         beep(BEEP_CONFIRM, 150);
//         animationFrame = 1;
//       }
      
//       if ((stateDuration / 300) % 2 == 0) {
//         lcd.setCursor(2, 3);
//         lcd.print("*** REGISTER ME ***");
//       } else {
//         clearLine(3);
//       }
      
//       if (stateDuration > 4000) {
//         setLCDState(LCD_READY);
//       }
//       break;
//     }
      
//     case LCD_CLEARING_EEPROM: {
//       if (animationFrame == 0) {
//         lcd.clear();
//         drawDoubleSeparator(0);
//         centerText("EEPROM MAINTENANCE", 1);
//         lcd.setCursor(0, 2);
//         lcd.print("Cleaning storage...");
//         drawDoubleSeparator(3);
//         animationFrame = 1;
//       }
      
//       int progressBars = (stateDuration / 100) % 21;
//       lcd.setCursor(0, 3);
//       for (int i = 0; i < 20; i++) {
//         if (i < progressBars) {
//           lcd.print("#");
//         } else {
//           lcd.print(" ");
//         }
//       }
      
//       if (stateDuration > 2000) {
//         setLCDState(LCD_WELCOME);
//       }
//       break;
//     }
      
//     case LCD_CORRUPTED_DATA: {
//       if (animationFrame == 0) {
//         lcd.clear();
//         drawDoubleSeparator(0);
//         centerText("DATA INTEGRITY", 1);
//         lcd.setCursor(0, 2);
//         lcd.print("EEPROM Recovery...");
//         drawDoubleSeparator(3);
//         beep(BEEP_ERROR, 200);
//         delay(50);
//         beep(BEEP_ERROR, 200);
//         animationFrame = 1;
//       }
      
//       if ((stateDuration / 200) % 2 == 0) {
//         lcd.setCursor(6, 1);
//         lcd.print("INTEGRITY");
//       } else {
//         lcd.setCursor(6, 1);
//         lcd.print("         ");
//       }
      
//       if (stateDuration > 3000) {
//         setLCDState(LCD_WELCOME);
//       }
//       break;
//     }
//   }
// }

// // ==================== EEPROM HELPERS ====================
// String eepromReadString(int addr, int maxLen) {
//   char buffer[maxLen + 1];
//   int i;
//   for (i = 0; i < maxLen; i++) {
//     byte c = EEPROM.read(addr + i);
//     if (c == 0x00) {
//       break;
//     }
//     if (c == 0x01) {
//       buffer[i] = (char)0x00;
//     } else {
//       buffer[i] = (char)c;
//     }
//   }
//   buffer[i] = '\0';
//   return String(buffer);
// }

// void eepromWriteString(int addr, const String &str, int maxLen) {
//   unsigned int len = min(str.length(), (unsigned int)(maxLen - 1));
  
//   for (unsigned int i = 0; i < len; i++) {
//     char c = str.charAt(i);
//     if (c == (char)0x00) {
//       EEPROM.write(addr + i, 0x01);
//     } else {
//       EEPROM.write(addr + i, (byte)c);
//     }
//   }
  
//   EEPROM.write(addr + len, 0x00);
  
//   for (unsigned int i = len + 1; i < (unsigned int)maxLen; i++) {
//     EEPROM.write(addr + i, 0xFF);
//   }
// }

// // ==================== JSON VALIDATION ====================
// bool isValidJSON(const String &json) {
//   if (json.length() < 10) return false;
//   if (!json.startsWith("{")) return false;
//   if (!json.endsWith("}")) return false;
  
//   int braceCount = 0;
//   bool inString = false;
  
//   for (unsigned int i = 0; i < json.length(); i++) {
//     char c = json.charAt(i);
    
//     if (c == '\\' && i + 1 < json.length()) {
//       i++;
//       continue;
//     }
    
//     if (c == '"') {
//       inString = !inString;
//     }
    
//     if (!inString) {
//       if (c == '{') braceCount++;
//       else if (c == '}') braceCount--;
      
//       if (braceCount < 0) return false;
//     }
//   }
  
//   return (braceCount == 0 && !inString);
// }

// // ==================== CORRUPTION DETECTION ====================
// bool isEEPROMCorrupted() {
//   bool allEmpty = true;
//   for (int i = 0; i < 20; i++) {
//     if (EEPROM.read(REGISTERED_BOOKS_START + i) != 0xFF) {
//       allEmpty = false;
//       break;
//     }
//   }
  
//   if (allEmpty) {
//     return false;
//   }
  
//   for (int i = 0; i < 5; i++) {
//     int addr = REGISTERED_BOOKS_START + i * (UID_LENGTH + 1);
    
//     bool slotEmpty = true;
//     for (int j = 0; j < UID_LENGTH; j++) {
//       byte c = EEPROM.read(addr + j);
//       if (c != 0xFF && c != 0x00) {
//         slotEmpty = false;
//         break;
//       }
//     }
    
//     if (slotEmpty) continue;
    
//     String uid = eepromReadString(addr, UID_LENGTH);
    
//     if (uid.length() == UID_LENGTH) {
//       bool validHex = true;
//       for (unsigned int j = 0; j < uid.length(); j++) {
//         char c = uid.charAt(j);
//         if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) {
//           validHex = false;
//           break;
//         }
//       }
      
//       if (!validHex) {
//         return true;
//       }
//     } else if (uid.length() > 0 && uid.length() < UID_LENGTH) {
//       return true;
//     }
//   }
  
//   return false;
// }

// void clearCorruptedEEPROM() {
//   setLCDState(LCD_CLEARING_EEPROM);
//   Serial.println("Clearing corrupted EEPROM data...");
  
//   for (int i = 0; i < MAX_REGISTERED_BOOKS * (UID_LENGTH + 1); i++) {
//     EEPROM.write(REGISTERED_BOOKS_START + i, 0xFF);
//   }
  
//   for (int i = 0; i < MAX_CACHED_BOOKS * BOOK_CACHE_SIZE; i++) {
//     EEPROM.write(BOOK_CACHE_START + i, 0xFF);
//   }
  
//   for (int i = 0; i < PENDING_MAX * PENDING_ENTRY_SIZE; i++) {
//     EEPROM.write(PENDING_START + i, 0xFF);
//   }
  
//   Serial.println("EEPROM cleared successfully");
//   delay(2000);
//   setLCDState(LCD_WELCOME);
// }

// // ==================== BOOK CACHE SYSTEM ====================
// int getBookCacheAddr(const String &uid) {
//   unsigned int hash = 0;
//   for (unsigned int i = 0; i < uid.length(); i++) {
//     hash = (hash * 31) + uid.charAt(i);
//   }
//   int slot = hash % MAX_CACHED_BOOKS;
//   return BOOK_CACHE_START + slot * BOOK_CACHE_SIZE;
// }

// void cacheBookData(const String &uid, const String &jsonData) {
//   if (uid.length() != 8 || jsonData.length() == 0) return;
  
//   if (!isValidJSON(jsonData)) {
//     return;
//   }
  
//   int addr = getBookCacheAddr(uid);
  
//   unsigned int len = jsonData.length();
//   len = min(len, (unsigned int)(BOOK_CACHE_SIZE - 3));
  
//   EEPROM.write(addr, (len >> 8) & 0xFF);
//   EEPROM.write(addr + 1, len & 0xFF);
  
//   for (unsigned int i = 0; i < len; i++) {
//     char c = jsonData.charAt(i);
//     if (c == (char)0x00) {
//       EEPROM.write(addr + 2 + i, 0x01);
//     } else {
//       EEPROM.write(addr + 2 + i, (byte)c);
//     }
//   }
  
//   EEPROM.write(addr + 2 + len, 0xFF);
// }

// String getCachedBookData(const String &uid) {
//   if (uid.length() != 8) return "";
  
//   int addr = getBookCacheAddr(uid);
  
//   int len = (EEPROM.read(addr) << 8) | EEPROM.read(addr + 1);
  
//   if (len > 0 && len <= (BOOK_CACHE_SIZE - 3)) {
//     String cached = "";
//     for (int i = 0; i < len; i++) {
//       byte c = EEPROM.read(addr + 2 + i);
//       if (c == 0x01) {
//         cached += (char)0x00;
//       } else {
//         cached += (char)c;
//       }
//     }
    
//     if (isValidJSON(cached)) {
//       return cached;
//     }
//   }
  
//   String cached = eepromReadString(addr, BOOK_CACHE_SIZE - 1);
//   if (isValidJSON(cached)) {
//     return cached;
//   }
  
//   return "";
// }

// // ==================== REGISTERED BOOKS MANAGEMENT ====================
// bool isBookRegistered(const String &uid) {
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     if (registeredUIDs[i] == uid) return true;
//   }
//   return false;
// }

// int getBookIndex(const String &uid) {
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     if (registeredUIDs[i] == uid) return i;
//   }
//   return -1;
// }

// void saveRegisteredBook(const String &uid, int sensorIndex = -1) {
//   if (isBookRegistered(uid)) return;
  
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     if (registeredUIDs[i].length() == 0) {
//       registeredUIDs[i] = uid;
      
//       if (sensorIndex >= 0 && sensorIndex < NUM_SENSORS) {
//         sensorAssignments[i] = sensorIndex;
//       } else {
//         for (int s = 0; s < NUM_SENSORS; s++) {
//           bool sensorUsed = false;
//           for (int j = 0; j < MAX_REGISTERED_BOOKS; j++) {
//             if (sensorAssignments[j] == s) {
//               sensorUsed = true;
//               break;
//             }
//           }
//           if (!sensorUsed) {
//             sensorAssignments[i] = s;
//             break;
//           }
//         }
//       }
      
//       int addr = REGISTERED_BOOKS_START + i * (UID_LENGTH + 1);
//       eepromWriteString(addr, uid, UID_LENGTH);
//       return;
//     }
//   }
// }

// void loadRegisteredBooks() {
//   Serial.println("Loading registered books from EEPROM...");
//   int bookCount = 0;
  
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     registeredUIDs[i] = "";
//     sensorAssignments[i] = 255;
//   }
  
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     int addr = REGISTERED_BOOKS_START + i * (UID_LENGTH + 1);
//     String uid = eepromReadString(addr, UID_LENGTH);
    
//     if (uid.length() == UID_LENGTH) {
//       bool validHex = true;
//       for (unsigned int j = 0; j < uid.length(); j++) {
//         char c = uid.charAt(j);
//         if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) {
//           validHex = false;
//           break;
//         }
//       }
      
//       if (validHex) {
//         registeredUIDs[bookCount] = uid;
//         sensorAssignments[bookCount] = 255;
//         bookCount++;
//       }
//     }
//   }
  
//   if (bookCount == 0) {
//     Serial.println("No registered books found in EEPROM");
//   } else {
//     Serial.print("Loaded ");
//     Serial.print(bookCount);
//     Serial.println(" registered books (sensors not assigned yet)");
//   }
// }

// // ==================== SENSOR FUNCTIONS ====================
// String readSensor(int pin) {
//   static String lastState[NUM_SENSORS];

//   int lowCount = 0;

//   for (int i = 0; i < 10; i++) {
//     if (digitalRead(pin) == LOW) lowCount++;
//     delayMicroseconds(100);
//   }

//   if (lowCount > 8) {
//     return "AVL";
//   } else if (lowCount < 2) {
//     return "BRW";
//   } else {
//     for (int i = 0; i < NUM_SENSORS; i++) {
//       if (sensorPins[i] == pin) {
//         return lastState[i];
//       }
//     }
//     return "BRW";
//   }
// }

// int getSensorIndexForBook(const String &uid) {
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     if (registeredUIDs[i] == uid) {
//       if (sensorAssignments[i] >= 0 && sensorAssignments[i] < NUM_SENSORS) {
//         return sensorAssignments[i];
//       }
//       break;
//     }
//   }
//   return -1;
// }

// int findSensorForBook(const String &uid) {
//   int existingSensor = getSensorIndexForBook(uid);
//   if (existingSensor != -1) {
//     return existingSensor;
//   }
  
//   for (int sensorNum = 0; sensorNum < NUM_SENSORS; sensorNum++) {
//     bool sensorInUse = false;
    
//     for (int j = 0; j < MAX_REGISTERED_BOOKS; j++) {
//       if (sensorAssignments[j] == sensorNum) {
//         sensorInUse = true;
//         break;
//       }
//     }
    
//     if (!sensorInUse) {
//       for (int j = 0; j < MAX_REGISTERED_BOOKS; j++) {
//         if (registeredUIDs[j].length() == 0) {
//           registeredUIDs[j] = uid;
//           sensorAssignments[j] = sensorNum;
          
//           int addr = REGISTERED_BOOKS_START + j * (UID_LENGTH + 1);
//           eepromWriteString(addr, uid, UID_LENGTH);
          
//           Serial.print("ASSIGNED: Book ");
//           Serial.print(uid);
//           Serial.print(" → Sensor ");
//           Serial.print(sensorNum);
//           Serial.print(" (Pin ");
//           Serial.print(sensorPins[sensorNum]);
//           Serial.println(")");
//           return sensorNum;
//         }
//       }
//     }
//   }
  
//   Serial.println("WARNING: All sensors in use! Using sensor 0 as fallback");
//   return 0;
// }

// String getBookStatus(const String &uid) {
//   int sensorIndex = getSensorIndexForBook(uid);

//   if (sensorIndex == -1) {
//     return "AVL";
//   }

//   if (sensorIndex >= 0 && sensorIndex < NUM_SENSORS) {
//     return readSensor(sensorPins[sensorIndex]);
//   }

//   return "AVL";
// }

// // ==================== SERIAL COMMUNICATION ====================
// void sendToESP(const String &msg) {
//   while (Serial1.available()) Serial1.read();
//   delay(5);
  
//   Serial1.println(msg);
  
//   Serial.print("TO_ESP: ");
  
//   if (msg.startsWith("SEND:")) {
//     int pipePos = msg.indexOf('|');
//     if (pipePos != -1) {
//       Serial.print(msg.substring(0, pipePos + 1));
//       Serial.print("... [JSON length: ");
//       Serial.print(msg.length() - pipePos - 1);
//       Serial.println("]");
//     } else {
//       int showLen = min(msg.length(), 60);
//       Serial.println(msg.substring(0, showLen));
//     }
//   } else {
//     Serial.println(msg);
//   }
// }

// bool waitForResponse(String &response, unsigned long timeout) {
//   unsigned long start = millis();
//   response = "";
  
//   while (millis() - start < timeout) {
//     while (Serial1.available()) {
//       char c = Serial1.read();
//       if (c == '\n') {
//         response.trim();
//         if (response.length() > 0) {
//           if (!response.startsWith("DEBUG:") && !response.startsWith("WIFI:")) {
//             Serial.print("FROM_ESP: ");
//             Serial.println(response);
//           }
//           return true;
//         }
//       } else if (c != '\r' && response.length() < 2048) {
//         response += c;
//       }
//     }
//     delay(1);
//   }
//   return false;
// }

// // ==================== PENDING QUEUE ====================
// int pendingIndexAddr(int index) { 
//   return PENDING_START + index * PENDING_ENTRY_SIZE; 
// }

// bool addPendingEntry(const String &payload) {
//   if (!payload.startsWith("SEND:")) {
//     return false;
//   }
  
//   int pipePos = payload.indexOf('|');
//   if (pipePos != -1) {
//     String jsonPart = payload.substring(pipePos + 1);
//     if (!isValidJSON(jsonPart)) {
//       return false;
//     }
//   }
  
//   for (int i = 0; i < PENDING_MAX; i++) {
//     int addr = pendingIndexAddr(i);
//     if (EEPROM.read(addr) == 0xFF) {
//       eepromWriteString(addr, payload, PENDING_ENTRY_SIZE - 1);
//       Serial.print("QUEUED: ");
      
//       int showLen = min(payload.length(), 60);
//       Serial.println(payload.substring(0, showLen));
      
//       return true;
//     }
//   }
//   Serial.println("QUEUE FULL!");
//   return false;
// }

// bool popPendingEntry(String &outPayload) {
//   for (int i = 0; i < PENDING_MAX; i++) {
//     int addr = pendingIndexAddr(i);
//     if (EEPROM.read(addr) != 0xFF && EEPROM.read(addr) != 0x00) {
//       outPayload = eepromReadString(addr, PENDING_ENTRY_SIZE - 1);
      
//       for (int j = 0; j < PENDING_ENTRY_SIZE; j++) {
//         EEPROM.write(addr + j, 0xFF);
//       }
      
//       if (outPayload.length() > 10 && 
//           outPayload.startsWith("SEND:") && 
//           outPayload.indexOf('|') > 5) {
        
//         int pipePos = outPayload.indexOf('|');
//         String jsonPart = outPayload.substring(pipePos + 1);
//         if (isValidJSON(jsonPart)) {
//           return true;
//         }
//       }
//     }
//   }
//   return false;
// }

// void flushPendingQueue() {
//   if (!wifiConnected) return;
  
//   String payload;
//   int flushed = 0;
  
//   while (popPendingEntry(payload)) {
//     if (payload.length() > 10) {
//       if (!payload.startsWith("SEND:")) {
//         continue;
//       }
      
//       Serial.print("FLUSHING: ");
//       Serial.println(payload.substring(0, 60));
//       sendToESP(payload);
      
//       String response;
//       if (waitForResponse(response, REQUEST_TIMEOUT)) {
//         if (response.startsWith("SUCCESS:")) {
//           flushed++;
//           delay(50);
//         } else {
//           addPendingEntry(payload);
//           break;
//         }
//       } else {
//         addPendingEntry(payload);
//         break;
//       }
//     }
//   }
  
//   if (flushed > 0) {
//     Serial.print("Successfully flushed ");
//     Serial.print(flushed);
//     Serial.println(" queued entries");
//   }
// }

// // ==================== RFID ====================
// String readRFID() {
//   if (!mfrc522.PICC_IsNewCardPresent()) return "";
//   if (!mfrc522.PICC_ReadCardSerial()) return "";
  
//   String uid = "";
//   for (byte i = 0; i < mfrc522.uid.size; i++) {
//     if (mfrc522.uid.uidByte[i] < 0x10) uid += "0";
//     uid += String(mfrc522.uid.uidByte[i], HEX);
//   }
//   uid.toUpperCase();
//   mfrc522.PICC_HaltA();
//   return uid;
// }

// // ==================== MAIN PROCESSING ====================
// void processCard(const String &cardUID) {
//   if (!systemReady) return;
  
//   if (currentLCDState != LCD_READY) {
//     return;
//   }
  
//   if (cardUID == lastCardUID && millis() - lastCardTime < DEBOUNCE_TIME) {
//     return;
//   }
  
//   lastCardUID = cardUID;
//   lastCardTime = millis();
//   setLCDState(LCD_CARD_DETECTED);
  
//   int sensorIndex = findSensorForBook(cardUID);
//   String sensorStatus = "AVL";
//   if (sensorIndex >= 0 && sensorIndex < NUM_SENSORS) {
//     sensorStatus = readSensor(sensorPins[sensorIndex]);
//   }
  
//   Serial.println("=================================");
//   Serial.print("CARD: ");
//   Serial.println(cardUID);
//   Serial.print("SENSOR INDEX: ");
//   Serial.println(sensorIndex);
//   Serial.print("SENSOR STATUS: ");
//   Serial.println(sensorStatus);
//   Serial.println("=================================");
  
//   bool serverIsRegistered = false;
//   String serverTitle = "";
//   String serverAuthor = "";
//   String serverLocation = "";
//   String serverEspStatus = "AVL";
//   int serverSensor = 0;
//   bool usingCachedData = false;
  
//   if (wifiConnected) {
//     sendToESP("GET:" + cardUID);
//     String response;
//     if (waitForResponse(response, REQUEST_TIMEOUT)) {
//       if (response.startsWith("BOOKDATA:")) {
//         int p = response.indexOf('|');
//         if (p != -1) {
//           String serverData = response.substring(p + 1);
          
//           if (serverData != "null") {
//             JsonDocument doc;
//             DeserializationError err = deserializeJson(doc, serverData);
//             if (!err) {
//               serverIsRegistered = doc["isRegistered"] | false;
//               serverTitle = doc["title"] | "";
//               serverAuthor = doc["author"] | "";
//               serverLocation = doc["location"] | "";
//               serverSensor = doc["sensor"] | 0;
              
//               serverTitle.trim();
//               serverAuthor.trim(); 
//               serverLocation.trim();
              
//               if (doc["espStatus"]) {
//                 serverEspStatus = String(doc["espStatus"].as<const char*>());
//               }
              
//               Serial.print("FIREBASE: Registered=");
//               Serial.print(serverIsRegistered ? "YES" : "NO");
//               Serial.print(", Status=");
//               Serial.println(serverEspStatus);
              
//               cacheBookData(cardUID, serverData);
              
//               if (serverIsRegistered && !isBookRegistered(cardUID)) {
//                 saveRegisteredBook(cardUID, serverSensor);
//               }
//             }
//           } else {
//             Serial.println("Book not found in Firebase");
//           }
//         }
//       }
//     } else {
//       wifiConnected = false;
//       setLCDState(LCD_OFFLINE);
//     }
//   }
  
//   if (!wifiConnected || serverTitle.length() == 0) {
//     String cachedData = getCachedBookData(cardUID);
//     if (cachedData.length() > 0) {
//       JsonDocument doc;
//       DeserializationError err = deserializeJson(doc, cachedData);
//       if (!err) {
//         serverIsRegistered = doc["isRegistered"] | false;
//         serverTitle = doc["title"] | "";
//         serverAuthor = doc["author"] | "";
//         serverLocation = doc["location"] | "";
//         serverSensor = doc["sensor"] | 0;
        
//         if (doc["espStatus"]) {
//           serverEspStatus = String(doc["espStatus"].as<const char*>());
//         }
        
//         usingCachedData = true;
//         Serial.println("USING CACHED DATA");
//       }
//     }
//   }
  
//   if (serverTitle.length() == 0) {
//     if (isBookRegistered(cardUID)) {
//       serverTitle = "Book Title Unknown";
//       serverAuthor = "Author Unknown";
//       serverLocation = "Location Unknown";
//       serverIsRegistered = true;
//       Serial.println("Using local registration (no cache)");
//     } else {
//       serverTitle = "New Book";
//       serverAuthor = "Unknown";
//       serverLocation = "Unassigned";
//       serverIsRegistered = false;
//       setLCDState(LCD_NEW_BOOK);
//       Serial.println("New unregistered book");
//       return;
//     }
//   }
  
//   // Clean data before storing for LCD display
//   serverTitle = cleanDisplayText(serverTitle);
//   serverAuthor = cleanDisplayText(serverAuthor);
//   serverLocation = cleanDisplayText(serverLocation);
  
//   // Update global variables for LCD display
//   currentBookTitle = serverTitle;
//   currentBookAuthor = serverAuthor;
//   currentBookLocation = serverLocation;
//   currentBookStatus = sensorStatus;
//   currentSensorIndex = sensorIndex;
  
//   JsonDocument doc;
  
//   doc["title"] = serverTitle;
//   doc["author"] = serverAuthor;
//   doc["location"] = serverLocation;
//   doc["isRegistered"] = serverIsRegistered;
//   doc["lastSeen"] = String(millis() / 1000);
//   doc["sensor"] = sensorIndex;
//   doc["espStatus"] = sensorStatus;
  
//   if (sensorStatus == "AVL") doc["status"] = "Available";
//   else if (sensorStatus == "BRW") doc["status"] = "Borrowed";
//   else if (sensorStatus == "OVD") doc["status"] = "Overdue";
//   else if (sensorStatus == "MIS") doc["status"] = "Misplaced";
//   else doc["status"] = "Available";
  
//   doc["lastUpdated"] = String(millis() / 1000);
  
//   if (usingCachedData) {
//     doc["source"] = "cached";
//   }
  
//   String outJson;
//   serializeJson(doc, outJson);
  
//   Serial.print("JSON SIZE: ");
//   Serial.println(outJson.length());
  
//   if (!outJson.endsWith("}")) {
//     outJson += "}";
//   }
  
//   String sendCmd = "SEND:/books/" + cardUID + "|" + outJson;
  
//   if (wifiConnected) {
//     sendToESP(sendCmd);
    
//     String response;
//     if (waitForResponse(response, REQUEST_TIMEOUT)) {
//       if (response.startsWith("SUCCESS:")) {
//         cacheBookData(cardUID, outJson);
        
//         if (serverIsRegistered && !isBookRegistered(cardUID)) {
//           saveRegisteredBook(cardUID, sensorIndex);
//         }
//         Serial.println("Firebase update OK");
//       } else {
//         addPendingEntry(sendCmd);
//       }
//     } else {
//       addPendingEntry(sendCmd);
//     }
//   } else {
//     addPendingEntry(sendCmd);
//     Serial.println("WiFi offline - queued");
//   }
// }

// // ==================== SETUP ====================
// void setup() {
//   Serial.begin(115200);
//   Serial1.begin(115200);
  
//   pinMode(BUZZER_PIN, OUTPUT);
//   digitalWrite(BUZZER_PIN, LOW);
  
//   lcd.init();
//   lcd.backlight();
//   lcd.clear();
//   lcdInitialized = true;
  
//   lcd.setCursor(0, 0);
//   lcd.print("====================");
//   lcd.setCursor(4, 1);
//   lcd.print("LIBRARY SYSTEM");
//   lcd.setCursor(5, 2);
//   lcd.print("INITIALIZING");
//   lcd.setCursor(0, 3);
//   lcd.print("====================");
  
//   delay(100);
  
//   Serial.println();
//   Serial.println("========================================");
//   Serial.println("    LIBRARY SYSTEM v5.0 - MEGA 2560     ");
//   Serial.println("========================================");
  
//   for (int i = 0; i < NUM_SENSORS; i++) {
//     pinMode(sensorPins[i], INPUT_PULLUP);
//   }
  
//   if (isEEPROMCorrupted()) {
//     Serial.println("EEPROM corrupted - clearing");
//     clearCorruptedEEPROM();
//   } else {
//     loadRegisteredBooks();
//   }
  
//   SPI.begin();
//   mfrc522.PCD_Init();
  
//   byte v = mfrc522.PCD_ReadRegister(MFRC522::VersionReg);
//   if (v == 0x00 || v == 0xFF) {
//     Serial.println("RFID module not found!");
//   } else {
//     Serial.print("RFID module v");
//     Serial.println(v, HEX);
//   }
  
//   Serial.println("Testing ESP connection...");
//   sendToESP("PING");
  
//   setLCDState(LCD_INIT);
  
//   Serial.println("Setup complete! System ready.");
// }

// // ==================== LOOP ====================
// void loop() {
//   updateLCD();
  
//   if (systemReady && currentLCDState == LCD_READY) {
//     String uid = readRFID();
//     if (uid.length() == 8) {
//       processCard(uid);
//     }
//   }
  
//   static String espBuffer = "";
//   while (Serial1.available()) {
//     char c = Serial1.read();
//     if (c == '\n') {
//       espBuffer.trim();
//       if (espBuffer.length() > 0) {
//         if (espBuffer.startsWith("WIFI:")) {
//           if (espBuffer == "WIFI:OK") {
//             if (!wifiConnected) {
//               wifiConnected = true;
//               Serial.println("WiFi connected");
//               beep(BEEP_WIFI_CONNECT, 300);
//               flushPendingQueue();
//             }
//           } else if (espBuffer == "WIFI:OFFLINE") {
//             wifiConnected = false;
//             Serial.println("WiFi offline");
//             beep(BEEP_WIFI_DISCONNECT, 300);
//           }
//         } else if (espBuffer == "PONG") {
//           if (!wifiConnected) {
//             wifiConnected = true;
//             Serial.println("WiFi connected via PONG");
//           }
//         }
//       }
//       espBuffer = "";
//     } else if (c != '\r') {
//       espBuffer += c;
//     }
//   }
  
//   delay(10);
// }

//=========================================LCD PERFECT-CLOUD/ OFFLINE PERFECT============================================
//=========================================LCD PERFECT-CLOUD/ OFFLINE PERFECT============================================
//=========================================LCD PERFECT-CLOUD/ OFFLINE PERFECT============================================
//=========================================LCD PERFECT-CLOUD/ OFFLINE PERFECT============================================
//=========================================LCD PERFECT-CLOUD/ OFFLINE PERFECT============================================
//=========================================LCD PERFECT-CLOUD/ OFFLINE PERFECT============================================
//=========================================LCD PERFECT-CLOUD/ OFFLINE PERFECT============================================
//=========================================LCD PERFECT-CLOUD/ OFFLINE PERFECT============================================
//=========================================LCD PERFECT-CLOUD/ OFFLINE PERFECT============================================
//=========================================LCD PERFECT-CLOUD/ OFFLINE PERFECT============================================
//=========================================LCD PERFECT-CLOUD/ OFFLINE PERFECT============================================
//=========================================LCD PERFECT-CLOUD/ OFFLINE PERFECT============================================
//=========================================LCD PERFECT-CLOUD/ OFFLINE PERFECT============================================
//=========================================LCD PERFECT-CLOUD/ OFFLINE PERFECT============================================
//=========================================LCD PERFECT-CLOUD/ OFFLINE PERFECT============================================
//=========================================LCD PERFECT-CLOUD/ OFFLINE PERFECT============================================
//=========================================LCD PERFECT-CLOUD/ OFFLINE PERFECT============================================
//=========================================LCD PERFECT-CLOUD/ OFFLINE PERFECT============================================
//=========================================LCD PERFECT-CLOUD/ OFFLINE PERFECT============================================
//=========================================LCD PERFECT-CLOUD/ OFFLINE PERFECT============================================
//=========================================LCD PERFECT-CLOUD/ OFFLINE PERFECT============================================
//=========================================LCD PERFECT-CLOUD/ OFFLINE PERFECT============================================
//=========================================LCD PERFECT-CLOUD/ OFFLINE PERFECT============================================
//=========================================LCD PERFECT-CLOUD/ OFFLINE PERFECT============================================


























//===========iIIIIIIIIIIIIIIIIIISUE RAISED MIS/BRW ALTERNATE & MIS IS NOT PLAYING....
// //=====================LCD PERFECT-CLOUD/ OFFLINE PERFECT/ DF-PLAYER INTEGRATION NOW ==================
// //=====================LCD PERFECT-CLOUD/ OFFLINE PERFECT/ DF-PLAYER INTEGRATION NOW ==================
// //=====================LCD PERFECT-CLOUD/ OFFLINE PERFECT/ DF-PLAYER INTEGRATION NOW ==================
// //=====================LCD PERFECT-CLOUD/ OFFLINE PERFECT/ DF-PLAYER INTEGRATION NOW ==================
// //=====================LCD PERFECT-CLOUD/ OFFLINE PERFECT/ DF-PLAYER INTEGRATION NOW ==================
// //=====================LCD PERFECT-CLOUD/ OFFLINE PERFECT/ DF-PLAYER INTEGRATION NOW ==================
// //=====================LCD PERFECT-CLOUD/ OFFLINE PERFECT/ DF-PLAYER INTEGRATION NOW ==================
// //=====================LCD PERFECT-CLOUD/ OFFLINE PERFECT/ DF-PLAYER INTEGRATION NOW ==================
// //=====================LCD PERFECT-CLOUD/ OFFLINE PERFECT/ DF-PLAYER INTEGRATION NOW ==================
// //=====================LCD PERFECT-CLOUD/ OFFLINE PERFECT/ DF-PLAYER INTEGRATION NOW ==================
// //=====================LCD PERFECT-CLOUD/ OFFLINE PERFECT/ DF-PLAYER INTEGRATION NOW ==================
// //=====================LCD PERFECT-CLOUD/ OFFLINE PERFECT/ DF-PLAYER INTEGRATION NOW ==================
// //=====================LCD PERFECT-CLOUD/ OFFLINE PERFECT/ DF-PLAYER INTEGRATION NOW ==================
// //=====================LCD PERFECT-CLOUD/ OFFLINE PERFECT/ DF-PLAYER INTEGRATION NOW ==================
// //=====================LCD PERFECT-CLOUD/ OFFLINE PERFECT/ DF-PLAYER INTEGRATION NOW ==================
// //=====================LCD PERFECT-CLOUD/ OFFLINE PERFECT/ DF-PLAYER INTEGRATION NOW ==================

// #include <Arduino.h>
// #include <SPI.h>
// #include <MFRC522.h>
// #include <EEPROM.h>
// #include <ArduinoJson.h>
// #include <LiquidCrystal_I2C.h>
// #include <SoftwareSerial.h>
// #include <DFRobotDFPlayerMini.h>

// // ==================== LCD CONFIGURATION ====================
// #define LCD_ADDRESS 0x27
// #define LCD_COLS 20
// #define LCD_ROWS 4
// LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLS, LCD_ROWS);

// // ==================== BUZZER CONFIGURATION ====================
// #define BUZZER_PIN 48

// // ==================== DFPLAYER CONFIGURATION ====================
// #define DFPLAYER_RX_PIN 10  // Connect to DFPlayer TX
// #define DFPLAYER_TX_PIN 11  // Connect to DFPlayer RX
// SoftwareSerial dfplayerSerial(DFPLAYER_RX_PIN, DFPLAYER_TX_PIN); // RX, TX
// DFRobotDFPlayerMini dfplayer;
// bool dfplayerInitialized = false;

// // Audio track numbers (adjust based on your SD card files)
// #define AUDIO_SYSTEM_READY 1
// #define AUDIO_WIFI_CONNECTED 2
// #define AUDIO_WIFI_DISCONNECTED 3
// #define AUDIO_CARD_SCANNED 4
// #define AUDIO_BOOK_AVAILABLE 5
// #define AUDIO_BOOK_BORROWED 6
// #define AUDIO_BOOK_MISPLACED 7
// #define AUDIO_NEW_BOOK 8
// #define AUDIO_ERROR 9

// // ==================== GLOBAL VARIABLES ====================
// bool wifiConnected = false;
// bool systemReady = false;
// bool lcdInitialized = false;
// bool showingBookInfo = false;
// String lastCardUID = "";
// unsigned long lastCardTime = 0;
// String registeredUIDs[50];
// byte sensorAssignments[50];
// unsigned long beepEndTime = 0;
// int currentBeepFrequency = 0;

// // ==================== LCD STATES ====================
// enum LCDState {
//   LCD_INIT,
//   LCD_WELCOME,
//   LCD_READY,
//   LCD_CARD_DETECTED,
//   LCD_BOOK_INFO_PAGE1,
//   LCD_BOOK_INFO_PAGE2,
//   LCD_BOOK_INFO_PAGE3,
//   LCD_OFFLINE,
//   LCD_NEW_BOOK,
//   LCD_CLEARING_EEPROM,
//   LCD_CORRUPTED_DATA
// };

// LCDState currentLCDState = LCD_INIT;
// unsigned long lcdStateStartTime = 0;
// int animationFrame = 0;
// String currentBookTitle = "";
// String currentBookAuthor = "";
// String currentBookLocation = "";
// String currentBookStatus = "";
// int currentSensorIndex = -1;

// // ==================== BEEP TONES ====================
// #define BEEP_AVL 2000
// #define BEEP_BRW 1200
// #define BEEP_SWIPE 800
// #define BEEP_WIFI_CONNECT 2500
// #define BEEP_WIFI_DISCONNECT 600
// #define BEEP_ERROR 400
// #define BEEP_CONFIRM 1800
// #define BEEP_PAGE_CHANGE 1500

// // ==================== ORIGINAL CONFIG ====================
// #define RC522_SS_PIN 53
// #define RC522_RST_PIN 49

// #define REGISTERED_BOOKS_START 0
// #define MAX_REGISTERED_BOOKS 50
// #define UID_LENGTH 8

// #define BOOK_CACHE_START (REGISTERED_BOOKS_START + MAX_REGISTERED_BOOKS * (UID_LENGTH + 1))
// #define BOOK_CACHE_SIZE 512
// #define MAX_CACHED_BOOKS 20

// #define PENDING_START (BOOK_CACHE_START + MAX_CACHED_BOOKS * BOOK_CACHE_SIZE)
// #define PENDING_ENTRY_SIZE 768
// #define PENDING_MAX 8

// #define DEBOUNCE_TIME 3000UL
// #define REQUEST_TIMEOUT 8000UL
// #define PING_INTERVAL 30000UL

// #define NUM_SENSORS 15
// const byte sensorPins[NUM_SENSORS] = {22,23,24,25,26,27,28,29,30,31,32,33,34,35,36};

// // ==================== MFRC522 OBJECT ====================
// MFRC522 mfrc522(RC522_SS_PIN, RC522_RST_PIN);

// // ==================== LCD HELPER FUNCTIONS - FIXED ====================
// String cleanDisplayText(String text) {
//   text.trim();
//   text.replace("\n", "");
//   text.replace("\r", "");
//   text.replace("\t", " ");
  
//   // Remove multiple spaces
//   while (text.indexOf("  ") != -1) {
//     text.replace("  ", " ");
//   }
  
//   return text;
// }

// void clearLine(int row) {
//   lcd.setCursor(0, row);
//   for (int i = 0; i < LCD_COLS; i++) {
//     lcd.print(" ");
//   }
//   lcd.setCursor(0, row);
// }

// void centerText(String text, int row) {
//   text = cleanDisplayText(text);
//   int textLen = text.length();
  
//   if (textLen > LCD_COLS) {
//     text = text.substring(0, LCD_COLS - 3) + "...";
//     textLen = text.length();
//   }
  
//   int padding = max(0, (LCD_COLS - textLen) / 2);
//   clearLine(row);
//   lcd.setCursor(padding, row);
//   lcd.print(text);
// }

// void drawSeparator(int row) {
//   lcd.setCursor(0, row);
//   for (int i = 0; i < LCD_COLS; i++) {
//     lcd.print("-");
//   }
// }

// void drawDoubleSeparator(int row) {
//   lcd.setCursor(0, row);
//   for (int i = 0; i < LCD_COLS; i++) {
//     lcd.print("=");
//   }
// }

// // ==================== NON-BLOCKING BUZZER FUNCTIONS ====================
// void startBeep(int frequency, int duration) {
//   if (frequency <= 0) return;
//   tone(BUZZER_PIN, frequency, duration);
//   beepEndTime = millis() + duration;
//   currentBeepFrequency = frequency;
// }

// void stopBeep() {
//   noTone(BUZZER_PIN);
//   currentBeepFrequency = 0;
// }

// void updateBeep() {
//   if (currentBeepFrequency > 0 && millis() > beepEndTime) {
//     stopBeep();
//   }
// }

// void beep(int frequency, int duration) {
//   startBeep(frequency, duration);
// }

// // ==================== DFPLAYER AUDIO FUNCTIONS ====================
// void playAudio(int track) {
//   if (dfplayerInitialized && track >= 1 && track <= 9) {
//     dfplayer.play(track);
//     delay(50); // Small delay to ensure command is sent
//   }
// }

// void stopAudio() {
//   if (dfplayerInitialized) {
//     dfplayer.stop();
//     delay(20);
//   }
// }

// void initDFPlayer() {
//   dfplayerSerial.begin(9600);
//   Serial.println("Initializing DFPlayer Mini...");
  
//   // Try multiple initialization attempts
//   bool dfplayerFound = false;
//   for(int i = 0; i < 3; i++) {
//     Serial.print("DFPlayer init attempt ");
//     Serial.print(i+1);
//     Serial.print("... ");
    
//     if (dfplayer.begin(dfplayerSerial)) {
//       dfplayerFound = true;
//       Serial.println("SUCCESS!");
//       break;
//     } else {
//       Serial.println("FAILED");
//       delay(300);
//     }
//   }
  
//   if (!dfplayerFound) {
//     Serial.println("WARNING: DFPlayer Mini not found!");
//     Serial.println("Audio announcements will be disabled.");
//     dfplayerInitialized = false;
//     return;
//   }
  
//   dfplayerInitialized = true;
//   dfplayer.volume(30); // Set volume (0-30), 25 is about 83%
//   delay(100);
  
//   Serial.println("DFPlayer Mini initialized successfully");
// }

// // ==================== LCD STATES ====================
// void setLCDState(LCDState newState) {
//   if (currentLCDState != newState) {
//     if (!((currentLCDState >= LCD_BOOK_INFO_PAGE1 && currentLCDState <= LCD_BOOK_INFO_PAGE3) &&
//           (newState >= LCD_BOOK_INFO_PAGE1 && newState <= LCD_BOOK_INFO_PAGE3))) {
//       if (lcdInitialized) {
//         lcd.clear();
//         delay(2);
//       }
//     }
    
//     currentLCDState = newState;
//     lcdStateStartTime = millis();
//     animationFrame = 0;
    
//     if (newState >= LCD_BOOK_INFO_PAGE1 && newState <= LCD_BOOK_INFO_PAGE3) {
//       showingBookInfo = true;
//     } else if (newState == LCD_READY) {
//       showingBookInfo = false;
//     }
//   }
// }

// void updateLCD() {
//   static unsigned long lastUpdate = 0;
//   unsigned long currentMillis = millis();
  
//   if (!lcdInitialized) return;
  
//   if (currentMillis - lastUpdate < 150) return;
//   lastUpdate = currentMillis;
  
//   unsigned long stateDuration = currentMillis - lcdStateStartTime;
  
//   updateBeep();
  
//   switch (currentLCDState) {
//     case LCD_INIT: {
//       if (animationFrame == 0) {
//         lcd.clear();
//         drawDoubleSeparator(0);
//         centerText("LIBRARY SYSTEM", 1);
//         centerText("v5.0", 2);
//         drawDoubleSeparator(3);
//         animationFrame = 1;
//       }
//       if (stateDuration > 2000) {
//         setLCDState(LCD_WELCOME);
//       }
//       break;
//     }
      
//     case LCD_WELCOME: {
//       if (animationFrame == 0) {
//         lcd.clear();
//         centerText("LEEJINBOTICS", 0);
//         drawSeparator(1);
//         centerText("INITIALIZING", 2);
//         animationFrame = 1;
//       }
      
//       lcd.setCursor(6, 3);
//       int dotCount = (stateDuration / 300) % 8;
//       for (int i = 0; i < 8; i++) {
//         if (i < dotCount) {
//           lcd.print(".");
//         } else {
//           lcd.print(" ");
//         }
//       }
      
//       if (stateDuration > 3000) {
//         setLCDState(LCD_READY);
//         systemReady = true;
//         beep(BEEP_CONFIRM, 200);
//         if (dfplayerInitialized) {
//           playAudio(AUDIO_SYSTEM_READY);
//         }
//       }
//       break;
//     }
      
//     case LCD_READY: {
//       static unsigned long lastWifiUpdate = 0;
//       static bool lastWifiState = wifiConnected;
      
//       if (animationFrame == 0) {
//         lcd.clear();
//         drawDoubleSeparator(0);
//         centerText("READY TO SCAN", 1);
//         lcd.setCursor(0, 2);
//         lcd.print("WiFi:");
        
//         // Initial display
//         lcd.setCursor(6, 2);
//         if (wifiConnected) {
//           lcd.print("ONLINE  ");
//           lcd.write(223);
//         } else {
//           lcd.print("OFFLINE ");
//           lcd.print("X");
//         }
        
//         drawDoubleSeparator(3);
//         animationFrame = 1;
//         lastWifiState = wifiConnected;
//         lastWifiUpdate = millis();
//       }
      
//       // Update WiFi status regularly (every 500ms)
//       if (millis() - lastWifiUpdate > 500) {
//         if (wifiConnected != lastWifiState) {
//           lcd.setCursor(6, 2);
//           if (wifiConnected) {
//             lcd.print("ONLINE  ");
//             lcd.write(223);
//           } else {
//             lcd.print("OFFLINE ");
//             lcd.print("X");
//           }
//           lastWifiState = wifiConnected;
//         }
//         lastWifiUpdate = millis();
//       }
      
//       // Blinking "SCAN" text
//       if ((stateDuration / 500) % 2 == 0) {
//         lcd.setCursor(8, 1);
//         lcd.print("SCAN");
//       } else {
//         lcd.setCursor(8, 1);
//         lcd.print("    ");
//       }
//       break;
//     }
      
//     case LCD_CARD_DETECTED: {
//       if (animationFrame == 0) {
//         lcd.clear();
//         drawDoubleSeparator(0);
//         centerText("SCANNING CARD", 1);
//         lcd.setCursor(0, 2);
//         lcd.print("UID: ");
//         if (lastCardUID.length() > 12) {
//           lcd.print(lastCardUID.substring(0, 12));
//         } else {
//           lcd.print(lastCardUID);
//         }
//         drawDoubleSeparator(3);
//         beep(BEEP_SWIPE, 150);
//         if (dfplayerInitialized) {
//           playAudio(AUDIO_CARD_SCANNED);
//         }
//         animationFrame = 1;
//       }
      
//       int scanPos = (stateDuration / 200) % 16;
//       lcd.setCursor(2, 3);
//       for (int i = 0; i < 16; i++) {
//         if (i == scanPos) {
//           lcd.print(">");
//         } else {
//           lcd.print(" ");
//         }
//       }
      
//       if (stateDuration > 2000) {
//         setLCDState(LCD_BOOK_INFO_PAGE1);
//       }
//       break;
//     }
      
//     case LCD_BOOK_INFO_PAGE1: {
//       if (animationFrame == 0) {
//         lcd.clear();
//         drawDoubleSeparator(0);
//         centerText("BOOK DETAILS", 1);
//         lcd.setCursor(0, 2);
//         lcd.print("Page 1/3");
//         lcd.setCursor(12, 2);
//         lcd.print(">>");
//         drawDoubleSeparator(3);
//         animationFrame = 1;
//         beep(BEEP_PAGE_CHANGE, 100);
//       }
      
//       String displayTitle = cleanDisplayText(currentBookTitle);
//       centerText(displayTitle, 1);
      
//       if (stateDuration > 3000) {
//         setLCDState(LCD_BOOK_INFO_PAGE2);
//       }
//       break;
//     }
      
//     case LCD_BOOK_INFO_PAGE2: {
//       if (animationFrame == 0) {
//         lcd.clear();
//         drawDoubleSeparator(0);
//         centerText("AUTHOR & STATUS", 1);
//         lcd.setCursor(0, 2);
//         lcd.print("Page 2/3");
//         lcd.setCursor(12, 2);
//         lcd.print(">>");
//         drawDoubleSeparator(3);
//         animationFrame = 1;
//         beep(BEEP_PAGE_CHANGE, 100);
//       }
      
//       String displayAuthor = cleanDisplayText(currentBookAuthor);
//       centerText(displayAuthor, 1);
      
//       lcd.setCursor(0, 2);
//       lcd.print("Status: ");
//       if (currentBookStatus == "AVL") {
//         lcd.print("AVAILABLE ");
//         lcd.write(3);
//         if (animationFrame == 1) {
//           beep(BEEP_AVL, 300);
//           if (dfplayerInitialized) {
//             playAudio(AUDIO_BOOK_AVAILABLE);
//           }
//           animationFrame = 2;
//         }
//       } else if (currentBookStatus == "BRW") {
//         lcd.print("BORROWED  ");
//         lcd.write(1);
//         if (animationFrame == 1) {
//           beep(BEEP_BRW, 300);
//           if (dfplayerInitialized) {
//             playAudio(AUDIO_BOOK_BORROWED);
//           }
//           animationFrame = 2;
//         }
//       } else if (currentBookStatus == "MIS") {
//         lcd.print("MISPLACED ");
//         if (animationFrame == 1) {
//           beep(BEEP_ERROR, 300);
//           if (dfplayerInitialized) {
//             playAudio(AUDIO_BOOK_MISPLACED);
//           }
//           animationFrame = 2;
//         }
//       } else {
//         lcd.print(currentBookStatus);
//       }
      
//       if (stateDuration > 3000) {
//         setLCDState(LCD_BOOK_INFO_PAGE3);
//       }
//       break;
//     }
      
//     case LCD_BOOK_INFO_PAGE3: {
//       if (animationFrame == 0) {
//         lcd.clear();
//         drawDoubleSeparator(0);
//         centerText("LOCATION INFO", 1);
//         lcd.setCursor(0, 2);
//         lcd.print("Page 3/3");
//         lcd.setCursor(12, 2);
//         lcd.print(">>");
//         drawDoubleSeparator(3);
//         animationFrame = 1;
//         beep(BEEP_PAGE_CHANGE, 100);
//       }
      
//       String displayLoc = cleanDisplayText(currentBookLocation);
//       centerText(displayLoc, 1);
      
//       lcd.setCursor(0, 2);
//       lcd.print("Sensor: ");
//       if (currentSensorIndex >= 0 && currentSensorIndex < NUM_SENSORS) {
//         lcd.print(currentSensorIndex);
//         lcd.print(" (Pin ");
//         lcd.print(sensorPins[currentSensorIndex]);
//         lcd.print(")");
//       } else {
//         lcd.print("N/A");
//       }
      
//       if (stateDuration > 3000) {
//         setLCDState(LCD_READY);
//       }
//       break;
//     }
      
//     case LCD_OFFLINE: {
//       if (animationFrame == 0) {
//         lcd.clear();
//         drawDoubleSeparator(0);
//         centerText("OFFLINE MODE", 1);
//         lcd.setCursor(0, 2);
//         lcd.print("Data cached locally");
//         drawDoubleSeparator(3);
//         beep(BEEP_WIFI_DISCONNECT, 300);
//         animationFrame = 1;
//       }
      
//       if ((stateDuration / 400) % 2 == 0) {
//         lcd.setCursor(9, 1);
//         lcd.print("OFFLINE");
//       } else {
//         lcd.setCursor(9, 1);
//         lcd.print("       ");
//       }
//       break;
//     }
      
//     case LCD_NEW_BOOK: {
//       if (animationFrame == 0) {
//         lcd.clear();
//         drawDoubleSeparator(0);
//         centerText("NEW BOOK DETECTED", 1);
//         lcd.setCursor(0, 2);
//         lcd.print("UID: ");
//         if (lastCardUID.length() > 12) {
//           lcd.print(lastCardUID.substring(0, 12));
//         } else {
//           lcd.print(lastCardUID);
//         }
//         drawDoubleSeparator(3);
//         beep(BEEP_SWIPE, 100);
//         delay(50);
//         beep(BEEP_CONFIRM, 150);
//         if (dfplayerInitialized) {
//           playAudio(AUDIO_NEW_BOOK);
//         }
//         animationFrame = 1;
//       }
      
//       if ((stateDuration / 300) % 2 == 0) {
//         lcd.setCursor(2, 3);
//         lcd.print("*** REGISTER ME ***");
//       } else {
//         clearLine(3);
//       }
      
//       if (stateDuration > 4000) {
//         setLCDState(LCD_READY);
//       }
//       break;
//     }
      
//     case LCD_CLEARING_EEPROM: {
//       if (animationFrame == 0) {
//         lcd.clear();
//         drawDoubleSeparator(0);
//         centerText("EEPROM MAINTENANCE", 1);
//         lcd.setCursor(0, 2);
//         lcd.print("Cleaning storage...");
//         drawDoubleSeparator(3);
//         animationFrame = 1;
//       }
      
//       int progressBars = (stateDuration / 100) % 21;
//       lcd.setCursor(0, 3);
//       for (int i = 0; i < 20; i++) {
//         if (i < progressBars) {
//           lcd.print("#");
//         } else {
//           lcd.print(" ");
//         }
//       }
      
//       if (stateDuration > 2000) {
//         setLCDState(LCD_WELCOME);
//       }
//       break;
//     }
      
//     case LCD_CORRUPTED_DATA: {
//       if (animationFrame == 0) {
//         lcd.clear();
//         drawDoubleSeparator(0);
//         centerText("DATA INTEGRITY", 1);
//         lcd.setCursor(0, 2);
//         lcd.print("EEPROM Recovery...");
//         drawDoubleSeparator(3);
//         beep(BEEP_ERROR, 200);
//         delay(50);
//         beep(BEEP_ERROR, 200);
//         if (dfplayerInitialized) {
//           playAudio(AUDIO_ERROR);
//         }
//         animationFrame = 1;
//       }
      
//       if ((stateDuration / 200) % 2 == 0) {
//         lcd.setCursor(6, 1);
//         lcd.print("INTEGRITY");
//       } else {
//         lcd.setCursor(6, 1);
//         lcd.print("         ");
//       }
      
//       if (stateDuration > 3000) {
//         setLCDState(LCD_WELCOME);
//       }
//       break;
//     }
//   }
// }

// // ==================== EEPROM HELPERS ====================
// String eepromReadString(int addr, int maxLen) {
//   char buffer[maxLen + 1];
//   int i;
//   for (i = 0; i < maxLen; i++) {
//     byte c = EEPROM.read(addr + i);
//     if (c == 0x00) {
//       break;
//     }
//     if (c == 0x01) {
//       buffer[i] = (char)0x00;
//     } else {
//       buffer[i] = (char)c;
//     }
//   }
//   buffer[i] = '\0';
//   return String(buffer);
// }

// void eepromWriteString(int addr, const String &str, int maxLen) {
//   unsigned int len = min(str.length(), (unsigned int)(maxLen - 1));
  
//   for (unsigned int i = 0; i < len; i++) {
//     char c = str.charAt(i);
//     if (c == (char)0x00) {
//       EEPROM.write(addr + i, 0x01);
//     } else {
//       EEPROM.write(addr + i, (byte)c);
//     }
//   }
  
//   EEPROM.write(addr + len, 0x00);
  
//   for (unsigned int i = len + 1; i < (unsigned int)maxLen; i++) {
//     EEPROM.write(addr + i, 0xFF);
//   }
// }

// // ==================== JSON VALIDATION ====================
// bool isValidJSON(const String &json) {
//   if (json.length() < 10) return false;
//   if (!json.startsWith("{")) return false;
//   if (!json.endsWith("}")) return false;
  
//   int braceCount = 0;
//   bool inString = false;
  
//   for (unsigned int i = 0; i < json.length(); i++) {
//     char c = json.charAt(i);
    
//     if (c == '\\' && i + 1 < json.length()) {
//       i++;
//       continue;
//     }
    
//     if (c == '"') {
//       inString = !inString;
//     }
    
//     if (!inString) {
//       if (c == '{') braceCount++;
//       else if (c == '}') braceCount--;
      
//       if (braceCount < 0) return false;
//     }
//   }
  
//   return (braceCount == 0 && !inString);
// }

// // ==================== CORRUPTION DETECTION ====================
// bool isEEPROMCorrupted() {
//   bool allEmpty = true;
//   for (int i = 0; i < 20; i++) {
//     if (EEPROM.read(REGISTERED_BOOKS_START + i) != 0xFF) {
//       allEmpty = false;
//       break;
//     }
//   }
  
//   if (allEmpty) {
//     return false;
//   }
  
//   for (int i = 0; i < 5; i++) {
//     int addr = REGISTERED_BOOKS_START + i * (UID_LENGTH + 1);
    
//     bool slotEmpty = true;
//     for (int j = 0; j < UID_LENGTH; j++) {
//       byte c = EEPROM.read(addr + j);
//       if (c != 0xFF && c != 0x00) {
//         slotEmpty = false;
//         break;
//       }
//     }
    
//     if (slotEmpty) continue;
    
//     String uid = eepromReadString(addr, UID_LENGTH);
    
//     if (uid.length() == UID_LENGTH) {
//       bool validHex = true;
//       for (unsigned int j = 0; j < uid.length(); j++) {
//         char c = uid.charAt(j);
//         if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) {
//           validHex = false;
//           break;
//         }
//       }
      
//       if (!validHex) {
//         return true;
//       }
//     } else if (uid.length() > 0 && uid.length() < UID_LENGTH) {
//       return true;
//     }
//   }
  
//   return false;
// }

// void clearCorruptedEEPROM() {
//   setLCDState(LCD_CLEARING_EEPROM);
//   Serial.println("Clearing corrupted EEPROM data...");
  
//   for (int i = 0; i < MAX_REGISTERED_BOOKS * (UID_LENGTH + 1); i++) {
//     EEPROM.write(REGISTERED_BOOKS_START + i, 0xFF);
//   }
  
//   for (int i = 0; i < MAX_CACHED_BOOKS * BOOK_CACHE_SIZE; i++) {
//     EEPROM.write(BOOK_CACHE_START + i, 0xFF);
//   }
  
//   for (int i = 0; i < PENDING_MAX * PENDING_ENTRY_SIZE; i++) {
//     EEPROM.write(PENDING_START + i, 0xFF);
//   }
  
//   Serial.println("EEPROM cleared successfully");
//   delay(2000);
//   setLCDState(LCD_WELCOME);
// }

// // ==================== BOOK CACHE SYSTEM ====================
// int getBookCacheAddr(const String &uid) {
//   unsigned int hash = 0;
//   for (unsigned int i = 0; i < uid.length(); i++) {
//     hash = (hash * 31) + uid.charAt(i);
//   }
//   int slot = hash % MAX_CACHED_BOOKS;
//   return BOOK_CACHE_START + slot * BOOK_CACHE_SIZE;
// }

// void cacheBookData(const String &uid, const String &jsonData) {
//   if (uid.length() != 8 || jsonData.length() == 0) return;
  
//   if (!isValidJSON(jsonData)) {
//     return;
//   }
  
//   int addr = getBookCacheAddr(uid);
  
//   unsigned int len = jsonData.length();
//   len = min(len, (unsigned int)(BOOK_CACHE_SIZE - 3));
  
//   EEPROM.write(addr, (len >> 8) & 0xFF);
//   EEPROM.write(addr + 1, len & 0xFF);
  
//   for (unsigned int i = 0; i < len; i++) {
//     char c = jsonData.charAt(i);
//     if (c == (char)0x00) {
//       EEPROM.write(addr + 2 + i, 0x01);
//     } else {
//       EEPROM.write(addr + 2 + i, (byte)c);
//     }
//   }
  
//   EEPROM.write(addr + 2 + len, 0xFF);
// }

// String getCachedBookData(const String &uid) {
//   if (uid.length() != 8) return "";
  
//   int addr = getBookCacheAddr(uid);
  
//   int len = (EEPROM.read(addr) << 8) | EEPROM.read(addr + 1);
  
//   if (len > 0 && len <= (BOOK_CACHE_SIZE - 3)) {
//     String cached = "";
//     for (int i = 0; i < len; i++) {
//       byte c = EEPROM.read(addr + 2 + i);
//       if (c == 0x01) {
//         cached += (char)0x00;
//       } else {
//         cached += (char)c;
//       }
//     }
    
//     if (isValidJSON(cached)) {
//       return cached;
//     }
//   }
  
//   String cached = eepromReadString(addr, BOOK_CACHE_SIZE - 1);
//   if (isValidJSON(cached)) {
//     return cached;
//   }
  
//   return "";
// }

// // ==================== REGISTERED BOOKS MANAGEMENT ====================
// bool isBookRegistered(const String &uid) {
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     if (registeredUIDs[i] == uid) return true;
//   }
//   return false;
// }

// int getBookIndex(const String &uid) {
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     if (registeredUIDs[i] == uid) return i;
//   }
//   return -1;
// }

// void saveRegisteredBook(const String &uid, int sensorIndex = -1) {
//   if (isBookRegistered(uid)) return;
  
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     if (registeredUIDs[i].length() == 0) {
//       registeredUIDs[i] = uid;
      
//       if (sensorIndex >= 0 && sensorIndex < NUM_SENSORS) {
//         sensorAssignments[i] = sensorIndex;
//       } else {
//         for (int s = 0; s < NUM_SENSORS; s++) {
//           bool sensorUsed = false;
//           for (int j = 0; j < MAX_REGISTERED_BOOKS; j++) {
//             if (sensorAssignments[j] == s) {
//               sensorUsed = true;
//               break;
//             }
//           }
//           if (!sensorUsed) {
//             sensorAssignments[i] = s;
//             break;
//           }
//         }
//       }
      
//       int addr = REGISTERED_BOOKS_START + i * (UID_LENGTH + 1);
//       eepromWriteString(addr, uid, UID_LENGTH);
//       return;
//     }
//   }
// }

// void loadRegisteredBooks() {
//   Serial.println("Loading registered books from EEPROM...");
//   int bookCount = 0;
  
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     registeredUIDs[i] = "";
//     sensorAssignments[i] = 255;
//   }
  
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     int addr = REGISTERED_BOOKS_START + i * (UID_LENGTH + 1);
//     String uid = eepromReadString(addr, UID_LENGTH);
    
//     if (uid.length() == UID_LENGTH) {
//       bool validHex = true;
//       for (unsigned int j = 0; j < uid.length(); j++) {
//         char c = uid.charAt(j);
//         if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) {
//           validHex = false;
//           break;
//         }
//       }
      
//       if (validHex) {
//         registeredUIDs[bookCount] = uid;
//         sensorAssignments[bookCount] = 255;
//         bookCount++;
//       }
//     }
//   }
  
//   if (bookCount == 0) {
//     Serial.println("No registered books found in EEPROM");
//   } else {
//     Serial.print("Loaded ");
//     Serial.print(bookCount);
//     Serial.println(" registered books (sensors not assigned yet)");
//   }
// }

// // ==================== SENSOR FUNCTIONS ====================
// String readSensor(int pin) {
//   static String lastState[NUM_SENSORS];

//   int lowCount = 0;

//   for (int i = 0; i < 10; i++) {
//     if (digitalRead(pin) == LOW) lowCount++;
//     delayMicroseconds(100);
//   }

//   if (lowCount > 8) {
//     return "AVL";
//   } else if (lowCount < 2) {
//     return "BRW";
//   } else {
//     for (int i = 0; i < NUM_SENSORS; i++) {
//       if (sensorPins[i] == pin) {
//         return lastState[i];
//       }
//     }
//     return "BRW";
//   }
// }

// int getSensorIndexForBook(const String &uid) {
//   for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
//     if (registeredUIDs[i] == uid) {
//       if (sensorAssignments[i] >= 0 && sensorAssignments[i] < NUM_SENSORS) {
//         return sensorAssignments[i];
//       }
//       break;
//     }
//   }
//   return -1;
// }

// int findSensorForBook(const String &uid) {
//   int existingSensor = getSensorIndexForBook(uid);
//   if (existingSensor != -1) {
//     return existingSensor;
//   }
  
//   for (int sensorNum = 0; sensorNum < NUM_SENSORS; sensorNum++) {
//     bool sensorInUse = false;
    
//     for (int j = 0; j < MAX_REGISTERED_BOOKS; j++) {
//       if (sensorAssignments[j] == sensorNum) {
//         sensorInUse = true;
//         break;
//       }
//     }
    
//     if (!sensorInUse) {
//       for (int j = 0; j < MAX_REGISTERED_BOOKS; j++) {
//         if (registeredUIDs[j].length() == 0) {
//           registeredUIDs[j] = uid;
//           sensorAssignments[j] = sensorNum;
          
//           int addr = REGISTERED_BOOKS_START + j * (UID_LENGTH + 1);
//           eepromWriteString(addr, uid, UID_LENGTH);
          
//           Serial.print("ASSIGNED: Book ");
//           Serial.print(uid);
//           Serial.print(" → Sensor ");
//           Serial.print(sensorNum);
//           Serial.print(" (Pin ");
//           Serial.print(sensorPins[sensorNum]);
//           Serial.println(")");
//           return sensorNum;
//         }
//       }
//     }
//   }
  
//   Serial.println("WARNING: All sensors in use! Using sensor 0 as fallback");
//   return 0;
// }

// String getBookStatus(const String &uid) {
//   int sensorIndex = getSensorIndexForBook(uid);

//   if (sensorIndex == -1) {
//     return "AVL";
//   }

//   if (sensorIndex >= 0 && sensorIndex < NUM_SENSORS) {
//     return readSensor(sensorPins[sensorIndex]);
//   }

//   return "AVL";
// }

// // ==================== SERIAL COMMUNICATION ====================
// void sendToESP(const String &msg) {
//   while (Serial1.available()) Serial1.read();
//   delay(5);
  
//   Serial1.println(msg);
  
//   Serial.print("TO_ESP: ");
  
//   if (msg.startsWith("SEND:")) {
//     int pipePos = msg.indexOf('|');
//     if (pipePos != -1) {
//       Serial.print(msg.substring(0, pipePos + 1));
//       Serial.print("... [JSON length: ");
//       Serial.print(msg.length() - pipePos - 1);
//       Serial.println("]");
//     } else {
//       int showLen = min(msg.length(), 60);
//       Serial.println(msg.substring(0, showLen));
//     }
//   } else {
//     Serial.println(msg);
//   }
// }

// bool waitForResponse(String &response, unsigned long timeout) {
//   unsigned long start = millis();
//   response = "";
  
//   while (millis() - start < timeout) {
//     while (Serial1.available()) {
//       char c = Serial1.read();
//       if (c == '\n') {
//         response.trim();
//         if (response.length() > 0) {
//           if (!response.startsWith("DEBUG:") && !response.startsWith("WIFI:")) {
//             Serial.print("FROM_ESP: ");
//             Serial.println(response);
//           }
//           return true;
//         }
//       } else if (c != '\r' && response.length() < 2048) {
//         response += c;
//       }
//     }
//     delay(1);
//   }
//   return false;
// }

// // ==================== PENDING QUEUE ====================
// int pendingIndexAddr(int index) { 
//   return PENDING_START + index * PENDING_ENTRY_SIZE; 
// }

// bool addPendingEntry(const String &payload) {
//   if (!payload.startsWith("SEND:")) {
//     return false;
//   }
  
//   int pipePos = payload.indexOf('|');
//   if (pipePos != -1) {
//     String jsonPart = payload.substring(pipePos + 1);
//     if (!isValidJSON(jsonPart)) {
//       return false;
//     }
//   }
  
//   for (int i = 0; i < PENDING_MAX; i++) {
//     int addr = pendingIndexAddr(i);
//     if (EEPROM.read(addr) == 0xFF) {
//       eepromWriteString(addr, payload, PENDING_ENTRY_SIZE - 1);
//       Serial.print("QUEUED: ");
      
//       int showLen = min(payload.length(), 60);
//       Serial.println(payload.substring(0, showLen));
      
//       return true;
//     }
//   }
//   Serial.println("QUEUE FULL!");
//   return false;
// }

// bool popPendingEntry(String &outPayload) {
//   for (int i = 0; i < PENDING_MAX; i++) {
//     int addr = pendingIndexAddr(i);
//     if (EEPROM.read(addr) != 0xFF && EEPROM.read(addr) != 0x00) {
//       outPayload = eepromReadString(addr, PENDING_ENTRY_SIZE - 1);
      
//       for (int j = 0; j < PENDING_ENTRY_SIZE; j++) {
//         EEPROM.write(addr + j, 0xFF);
//       }
      
//       if (outPayload.length() > 10 && 
//           outPayload.startsWith("SEND:") && 
//           outPayload.indexOf('|') > 5) {
        
//         int pipePos = outPayload.indexOf('|');
//         String jsonPart = outPayload.substring(pipePos + 1);
//         if (isValidJSON(jsonPart)) {
//           return true;
//         }
//       }
//     }
//   }
//   return false;
// }

// void flushPendingQueue() {
//   if (!wifiConnected) return;
  
//   String payload;
//   int flushed = 0;
  
//   while (popPendingEntry(payload)) {
//     if (payload.length() > 10) {
//       if (!payload.startsWith("SEND:")) {
//         continue;
//       }
      
//       Serial.print("FLUSHING: ");
//       Serial.println(payload.substring(0, 60));
//       sendToESP(payload);
      
//       String response;
//       if (waitForResponse(response, REQUEST_TIMEOUT)) {
//         if (response.startsWith("SUCCESS:")) {
//           flushed++;
//           delay(50);
//         } else {
//           addPendingEntry(payload);
//           break;
//         }
//       } else {
//         addPendingEntry(payload);
//         break;
//       }
//     }
//   }
  
//   if (flushed > 0) {
//     Serial.print("Successfully flushed ");
//     Serial.print(flushed);
//     Serial.println(" queued entries");
//   }
// }

// // ==================== RFID ====================
// String readRFID() {
//   if (!mfrc522.PICC_IsNewCardPresent()) return "";
//   if (!mfrc522.PICC_ReadCardSerial()) return "";
  
//   String uid = "";
//   for (byte i = 0; i < mfrc522.uid.size; i++) {
//     if (mfrc522.uid.uidByte[i] < 0x10) uid += "0";
//     uid += String(mfrc522.uid.uidByte[i], HEX);
//   }
//   uid.toUpperCase();
//   mfrc522.PICC_HaltA();
//   return uid;
// }

// // ==================== MAIN PROCESSING ====================
// void processCard(const String &cardUID) {
//   if (!systemReady) return;
  
//   if (currentLCDState != LCD_READY) {
//     return;
//   }
  
//   if (cardUID == lastCardUID && millis() - lastCardTime < DEBOUNCE_TIME) {
//     return;
//   }
  
//   lastCardUID = cardUID;
//   lastCardTime = millis();
//   setLCDState(LCD_CARD_DETECTED);
  
//   int sensorIndex = findSensorForBook(cardUID);
//   String sensorStatus = "AVL";
//   if (sensorIndex >= 0 && sensorIndex < NUM_SENSORS) {
//     sensorStatus = readSensor(sensorPins[sensorIndex]);
//   }
  
//   Serial.println("=================================");
//   Serial.print("CARD: ");
//   Serial.println(cardUID);
//   Serial.print("SENSOR INDEX: ");
//   Serial.println(sensorIndex);
//   Serial.print("SENSOR STATUS: ");
//   Serial.println(sensorStatus);
//   Serial.println("=================================");
  
//   bool serverIsRegistered = false;
//   String serverTitle = "";
//   String serverAuthor = "";
//   String serverLocation = "";
//   String serverEspStatus = "AVL";
//   int serverSensor = 0;
//   bool usingCachedData = false;
  
//   if (wifiConnected) {
//     sendToESP("GET:" + cardUID);
//     String response;
//     if (waitForResponse(response, REQUEST_TIMEOUT)) {
//       if (response.startsWith("BOOKDATA:")) {
//         int p = response.indexOf('|');
//         if (p != -1) {
//           String serverData = response.substring(p + 1);
          
//           if (serverData != "null") {
//             JsonDocument doc;
//             DeserializationError err = deserializeJson(doc, serverData);
//             if (!err) {
//               serverIsRegistered = doc["isRegistered"] | false;
//               serverTitle = doc["title"] | "";
//               serverAuthor = doc["author"] | "";
//               serverLocation = doc["location"] | "";
//               serverSensor = doc["sensor"] | 0;
              
//               serverTitle.trim();
//               serverAuthor.trim(); 
//               serverLocation.trim();
              
//               if (doc["espStatus"]) {
//                 serverEspStatus = String(doc["espStatus"].as<const char*>());
//               }
              
//               Serial.print("FIREBASE: Registered=");
//               Serial.print(serverIsRegistered ? "YES" : "NO");
//               Serial.print(", Status=");
//               Serial.println(serverEspStatus);
              
//               cacheBookData(cardUID, serverData);
              
//               if (serverIsRegistered && !isBookRegistered(cardUID)) {
//                 saveRegisteredBook(cardUID, serverSensor);
//               }
//             }
//           } else {
//             Serial.println("Book not found in Firebase");
//           }
//         }
//       }
//     } else {
//       wifiConnected = false;
//       setLCDState(LCD_OFFLINE);
//     }
//   }
  
//   if (!wifiConnected || serverTitle.length() == 0) {
//     String cachedData = getCachedBookData(cardUID);
//     if (cachedData.length() > 0) {
//       JsonDocument doc;
//       DeserializationError err = deserializeJson(doc, cachedData);
//       if (!err) {
//         serverIsRegistered = doc["isRegistered"] | false;
//         serverTitle = doc["title"] | "";
//         serverAuthor = doc["author"] | "";
//         serverLocation = doc["location"] | "";
//         serverSensor = doc["sensor"] | 0;
        
//         if (doc["espStatus"]) {
//           serverEspStatus = String(doc["espStatus"].as<const char*>());
//         }
        
//         usingCachedData = true;
//         Serial.println("USING CACHED DATA");
//       }
//     }
//   }
  
//   if (serverTitle.length() == 0) {
//     if (isBookRegistered(cardUID)) {
//       serverTitle = "Book Title Unknown";
//       serverAuthor = "Author Unknown";
//       serverLocation = "Location Unknown";
//       serverIsRegistered = true;
//       Serial.println("Using local registration (no cache)");
//     } else {
//       serverTitle = "New Book";
//       serverAuthor = "Unknown";
//       serverLocation = "Unassigned";
//       serverIsRegistered = false;
//       setLCDState(LCD_NEW_BOOK);
//       Serial.println("New unregistered book");
//       return;
//     }
//   }
  
//   // Clean data before storing for LCD display
//   serverTitle = cleanDisplayText(serverTitle);
//   serverAuthor = cleanDisplayText(serverAuthor);
//   serverLocation = cleanDisplayText(serverLocation);
  
//   // Update global variables for LCD display
//   currentBookTitle = serverTitle;
//   currentBookAuthor = serverAuthor;
//   currentBookLocation = serverLocation;
//   currentBookStatus = sensorStatus;
//   currentSensorIndex = sensorIndex;
  
//   JsonDocument doc;
  
//   doc["title"] = serverTitle;
//   doc["author"] = serverAuthor;
//   doc["location"] = serverLocation;
//   doc["isRegistered"] = serverIsRegistered;
//   doc["lastSeen"] = String(millis() / 1000);
//   doc["sensor"] = sensorIndex;
//   doc["espStatus"] = sensorStatus;
  
//   if (sensorStatus == "AVL") doc["status"] = "Available";
//   else if (sensorStatus == "BRW") doc["status"] = "Borrowed";
//   else if (sensorStatus == "OVD") doc["status"] = "Overdue";
//   else if (sensorStatus == "MIS") doc["status"] = "Misplaced";
//   else doc["status"] = "Available";
  
//   doc["lastUpdated"] = String(millis() / 1000);
  
//   if (usingCachedData) {
//     doc["source"] = "cached";
//   }
  
//   String outJson;
//   serializeJson(doc, outJson);
  
//   Serial.print("JSON SIZE: ");
//   Serial.println(outJson.length());
  
//   if (!outJson.endsWith("}")) {
//     outJson += "}";
//   }
  
//   String sendCmd = "SEND:/books/" + cardUID + "|" + outJson;
  
//   if (wifiConnected) {
//     sendToESP(sendCmd);
    
//     String response;
//     if (waitForResponse(response, REQUEST_TIMEOUT)) {
//       if (response.startsWith("SUCCESS:")) {
//         cacheBookData(cardUID, outJson);
        
//         if (serverIsRegistered && !isBookRegistered(cardUID)) {
//           saveRegisteredBook(cardUID, sensorIndex);
//         }
//         Serial.println("Firebase update OK");
//       } else {
//         addPendingEntry(sendCmd);
//       }
//     } else {
//       addPendingEntry(sendCmd);
//     }
//   } else {
//     addPendingEntry(sendCmd);
//     Serial.println("WiFi offline - queued");
//   }
// }

// // ==================== SETUP ====================
// void setup() {
//   Serial.begin(115200);
//   Serial1.begin(115200);
  
//   pinMode(BUZZER_PIN, OUTPUT);
//   digitalWrite(BUZZER_PIN, LOW);
  
//   lcd.init();
//   lcd.backlight();
//   lcd.clear();
//   lcdInitialized = true;
  
//   lcd.setCursor(0, 0);
//   lcd.print("====================");
//   lcd.setCursor(4, 1);
//   lcd.print("LIBRARY SYSTEM");
//   lcd.setCursor(5, 2);
//   lcd.print("INITIALIZING");
//   lcd.setCursor(0, 3);
//   lcd.print("====================");
  
//   delay(100);
  
//   Serial.println();
//   Serial.println("========================================");
//   Serial.println("    LIBRARY SYSTEM v5.0 - MEGA 2560     ");
//   Serial.println("========================================");
  
//   for (int i = 0; i < NUM_SENSORS; i++) {
//     pinMode(sensorPins[i], INPUT_PULLUP);
//   }
  
//   // Initialize DFPlayer Mini
//   initDFPlayer();
  
//   if (isEEPROMCorrupted()) {
//     Serial.println("EEPROM corrupted - clearing");
//     clearCorruptedEEPROM();
//   } else {
//     loadRegisteredBooks();
//   }
  
//   SPI.begin();
//   mfrc522.PCD_Init();
  
//   byte v = mfrc522.PCD_ReadRegister(MFRC522::VersionReg);
//   if (v == 0x00 || v == 0xFF) {
//     Serial.println("RFID module not found!");
//     if (dfplayerInitialized) {
//       playAudio(AUDIO_ERROR);
//     }
//   } else {
//     Serial.print("RFID module v");
//     Serial.println(v, HEX);
//   }
  
//   Serial.println("Testing ESP connection...");
//   sendToESP("PING");
  
//   setLCDState(LCD_INIT);
  
//   Serial.println("Setup complete! System ready.");
// }

// // ==================== LOOP ====================
// void loop() {
//   updateLCD();
  
//   if (systemReady && currentLCDState == LCD_READY) {
//     String uid = readRFID();
//     if (uid.length() == 8) {
//       processCard(uid);
//     }
//   }
  
//   static String espBuffer = "";
//   while (Serial1.available()) {
//     char c = Serial1.read();
//     if (c == '\n') {
//       espBuffer.trim();
//       if (espBuffer.length() > 0) {
//         if (espBuffer.startsWith("WIFI:")) {
//           if (espBuffer == "WIFI:OK") {
//             if (!wifiConnected) {
//               wifiConnected = true;
//               Serial.println("WiFi connected");
//               beep(BEEP_WIFI_CONNECT, 300);
//               if (dfplayerInitialized) {
//                 playAudio(AUDIO_WIFI_CONNECTED);
//               }
//               flushPendingQueue();
//             }
//           } else if (espBuffer == "WIFI:OFFLINE") {
//             if (wifiConnected) {
//               wifiConnected = false;
//               Serial.println("WiFi offline");
//               beep(BEEP_WIFI_DISCONNECT, 300);
//               if (dfplayerInitialized) {
//                 playAudio(AUDIO_WIFI_DISCONNECTED);
//               }
//             }
//           }
//         } else if (espBuffer == "PONG") {
//           if (!wifiConnected) {
//             wifiConnected = true;
//             Serial.println("WiFi connected via PONG");
//           }
//         }
//       }
//       espBuffer = "";
//     } else if (c != '\r') {
//       espBuffer += c;
//     }
//   }
  
//   delay(10);
// }











//MIS CORRECT/ PLAYER PLAYS MIS
//=====================LCD PERFECT-CLOUD/ OFFLINE PERFECT/ DF-PLAYER INTEGRATION NOW ==================
//=====================LCD PERFECT-CLOUD/ OFFLINE PERFECT/ DF-PLAYER INTEGRATION NOW ==================
//=====================LCD PERFECT-CLOUD/ OFFLINE PERFECT/ DF-PLAYER INTEGRATION NOW ==================
//=====================LCD PERFECT-CLOUD/ OFFLINE PERFECT/ DF-PLAYER INTEGRATION NOW ==================
//=====================LCD PERFECT-CLOUD/ OFFLINE PERFECT/ DF-PLAYER INTEGRATION NOW ==================
//=====================LCD PERFECT-CLOUD/ OFFLINE PERFECT/ DF-PLAYER INTEGRATION NOW ==================
//=====================LCD PERFECT-CLOUD/ OFFLINE PERFECT/ DF-PLAYER INTEGRATION NOW ==================
//=====================LCD PERFECT-CLOUD/ OFFLINE PERFECT/ DF-PLAYER INTEGRATION NOW ==================
//=====================LCD PERFECT-CLOUD/ OFFLINE PERFECT/ DF-PLAYER INTEGRATION NOW ==================
//=====================LCD PERFECT-CLOUD/ OFFLINE PERFECT/ DF-PLAYER INTEGRATION NOW ==================
//=====================LCD PERFECT-CLOUD/ OFFLINE PERFECT/ DF-PLAYER INTEGRATION NOW ==================
//=====================LCD PERFECT-CLOUD/ OFFLINE PERFECT/ DF-PLAYER INTEGRATION NOW ==================
//=====================LCD PERFECT-CLOUD/ OFFLINE PERFECT/ DF-PLAYER INTEGRATION NOW ==================
//=====================LCD PERFECT-CLOUD/ OFFLINE PERFECT/ DF-PLAYER INTEGRATION NOW ==================
//=====================LCD PERFECT-CLOUD/ OFFLINE PERFECT/ DF-PLAYER INTEGRATION NOW ==================
//=====================LCD PERFECT-CLOUD/ OFFLINE PERFECT/ DF-PLAYER INTEGRATION NOW ==================

#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>  

// ==================== LCD CONFIGURATION ====================
#define LCD_ADDRESS 0x27
#define LCD_COLS 20
#define LCD_ROWS 4
LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLS, LCD_ROWS);

// ==================== BUZZER CONFIGURATION ====================
#define BUZZER_PIN 48

// ==================== DFPLAYER CONFIGURATION ====================
#define DFPLAYER_RX_PIN 10  // Connect to DFPlayer TX
#define DFPLAYER_TX_PIN 11  // Connect to DFPlayer RX
SoftwareSerial dfplayerSerial(DFPLAYER_RX_PIN, DFPLAYER_TX_PIN); // RX, TX
DFRobotDFPlayerMini dfplayer;
bool dfplayerInitialized = false;

// Audio track numbers (adjust based on your SD card files)
#define AUDIO_SYSTEM_READY 1
#define AUDIO_WIFI_CONNECTED 2
#define AUDIO_WIFI_DISCONNECTED 3
#define AUDIO_CARD_SCANNED 4
#define AUDIO_BOOK_AVAILABLE 5
#define AUDIO_BOOK_BORROWED 6
#define AUDIO_BOOK_MISPLACED 7
#define AUDIO_NEW_BOOK 8
#define AUDIO_ERROR 9

// ==================== GLOBAL VARIABLES ====================
bool wifiConnected = false;
bool systemReady = false;
bool lcdInitialized = false;
bool showingBookInfo = false;
String lastCardUID = "";
unsigned long lastCardTime = 0;
String registeredUIDs[50];
byte sensorAssignments[50];
unsigned long beepEndTime = 0;
int currentBeepFrequency = 0;



// ==================== NON-BLOCKING AUDIO VARIABLES ====================
int pendingAudioTrack = 0;
unsigned long audioStartTime = 0;
unsigned long audioDuration = 0;
bool isPlayingAudio = false;

// ==================== LCD STATES ====================
enum LCDState {
  LCD_INIT,
  LCD_WELCOME,
  LCD_READY,
  LCD_CARD_DETECTED,
  LCD_BOOK_INFO_PAGE1,
  LCD_BOOK_INFO_PAGE2,
  LCD_BOOK_INFO_PAGE3,
  LCD_OFFLINE,
  LCD_NEW_BOOK,
  LCD_CLEARING_EEPROM,
  LCD_CORRUPTED_DATA
};

LCDState currentLCDState = LCD_INIT;
unsigned long lcdStateStartTime = 0;
int animationFrame = 0;
String currentBookTitle = "";
String currentBookAuthor = "";
String currentBookLocation = "";
String currentBookStatus = "";
int currentSensorIndex = -1;

// ==================== BEEP TONES ====================
#define BEEP_AVL 2000
#define BEEP_BRW 1200
#define BEEP_SWIPE 800
#define BEEP_WIFI_CONNECT 2500
#define BEEP_WIFI_DISCONNECT 600
#define BEEP_ERROR 400
#define BEEP_CONFIRM 1800
#define BEEP_PAGE_CHANGE 1500

// ==================== ORIGINAL CONFIG ====================
#define RC522_SS_PIN 53
#define RC522_RST_PIN 49

#define REGISTERED_BOOKS_START 0
#define MAX_REGISTERED_BOOKS 50
#define UID_LENGTH 8

#define BOOK_CACHE_START (REGISTERED_BOOKS_START + MAX_REGISTERED_BOOKS * (UID_LENGTH + 1))
#define BOOK_CACHE_SIZE 512
#define MAX_CACHED_BOOKS 20

#define PENDING_START (BOOK_CACHE_START + MAX_CACHED_BOOKS * BOOK_CACHE_SIZE)
#define PENDING_ENTRY_SIZE 768
#define PENDING_MAX 8

#define DEBOUNCE_TIME 3000UL
#define REQUEST_TIMEOUT 8000UL
#define PING_INTERVAL 30000UL

#define NUM_SENSORS 15
const byte sensorPins[NUM_SENSORS] = {22,23,24,25,26,27,28,29,30,31,32,33,34,35,36};

// ==================== MFRC522 OBJECT ====================
MFRC522 mfrc522(RC522_SS_PIN, RC522_RST_PIN);

// ==================== LCD HELPER FUNCTIONS - FIXED ====================
String cleanDisplayText(String text) {
  text.trim();
  text.replace("\n", "");
  text.replace("\r", "");
  text.replace("\t", " ");
  
  // Remove multiple spaces
  while (text.indexOf("  ") != -1) {
    text.replace("  ", " ");
  }
  
  return text;
}

void clearLine(int row) {
  lcd.setCursor(0, row);
  for (int i = 0; i < LCD_COLS; i++) {
    lcd.print(" ");
  }
  lcd.setCursor(0, row);
}

void centerText(String text, int row) {
  text = cleanDisplayText(text);
  int textLen = text.length();
  
  if (textLen > LCD_COLS) {
    text = text.substring(0, LCD_COLS - 3) + "...";
    textLen = text.length();
  }
  
  int padding = max(0, (LCD_COLS - textLen) / 2);
  clearLine(row);
  lcd.setCursor(padding, row);
  lcd.print(text);
}

void drawSeparator(int row) {
  lcd.setCursor(0, row);
  for (int i = 0; i < LCD_COLS; i++) {
    lcd.print("-");
  }
}

void drawDoubleSeparator(int row) {
  lcd.setCursor(0, row);
  for (int i = 0; i < LCD_COLS; i++) {
    lcd.print("=");
  }
}

// ==================== NON-BLOCKING BUZZER FUNCTIONS ====================
void startBeep(int frequency, int duration) {
  if (frequency <= 0) return;
  tone(BUZZER_PIN, frequency, duration);
  beepEndTime = millis() + duration;
  currentBeepFrequency = frequency;
}

void stopBeep() {
  noTone(BUZZER_PIN);
  currentBeepFrequency = 0;
}

void updateBeep() {
  if (currentBeepFrequency > 0 && millis() > beepEndTime) {
    stopBeep();
  }
}

void beep(int frequency, int duration) {
  startBeep(frequency, duration);
}

// ==================== NON-BLOCKING AUDIO FUNCTIONS ====================
void startAudio(int track) {
  if (dfplayerInitialized && track >= 1 && track <= 9 && !isPlayingAudio) {
    dfplayer.play(track);
    pendingAudioTrack = 0;
    isPlayingAudio = true;
    audioStartTime = millis();
    
    // Set approximate durations for each track (adjust based on your audio)
    switch(track) {
      case 1: audioDuration = 3000; break;  // "Library system ready"
      case 2: audioDuration = 1500; break;  // "WiFi connected"
      case 3: audioDuration = 1500; break;  // "WiFi disconnected"
      case 4: audioDuration = 1000; break;  // "Card scanned"
      case 5: audioDuration = 1500; break;  // "Book available"
      case 6: audioDuration = 1500; break;  // "Book borrowed"
      case 7: audioDuration = 1500; break;  // "Book misplaced"
      case 8: audioDuration = 2000; break;  // "New book detected"
      case 9: audioDuration = 1500; break;  // "Error"
      default: audioDuration = 1000; break;
    }
  } else if (dfplayerInitialized && track >= 1 && track <= 9) {
    // Audio is already playing, queue this track
    pendingAudioTrack = track;
  }
}

void updateAudio() {
  if (isPlayingAudio && millis() - audioStartTime > audioDuration) {
    // Current audio finished
    isPlayingAudio = false;
    
    // Play pending audio if any
    if (pendingAudioTrack > 0) {
      startAudio(pendingAudioTrack);
    }
  }
}

// Keep this for backward compatibility but DON'T USE IT
void playAudio(int track) {
  startAudio(track);  // Just calls startAudio
}

// // ==================== DFPLAYER AUDIO FUNCTIONS ====================
// void playAudio(int track) {
//   if (dfplayerInitialized && track >= 1 && track <= 9) {
//     dfplayer.play(track);
//     delay(50); // Small delay to ensure command is sent
//   }
// }


void stopAudio() {
  if (dfplayerInitialized && isPlayingAudio) {
    dfplayer.stop();
    isPlayingAudio = false;
    pendingAudioTrack = 0;
  }
}
// void stopAudio() {
//   if (dfplayerInitialized) {
//     dfplayer.stop();
//     delay(20);
//   }
// }

void initDFPlayer() {
  dfplayerSerial.begin(9600);
  Serial.println("Initializing DFPlayer Mini...");
  
  // Try multiple initialization attempts
  bool dfplayerFound = false;
  for(int i = 0; i < 3; i++) {
    Serial.print("DFPlayer init attempt ");
    Serial.print(i+1);
    Serial.print("... ");
    
    if (dfplayer.begin(dfplayerSerial)) {
      dfplayerFound = true;
      Serial.println("SUCCESS!");
      break;
    } else {
      Serial.println("FAILED");
      delay(300);
    }
  }
  
  if (!dfplayerFound) {
    Serial.println("WARNING: DFPlayer Mini not found!");
    Serial.println("Audio announcements will be disabled.");
    dfplayerInitialized = false;
    return;
  }
  
  dfplayerInitialized = true;
  dfplayer.volume(30); // Set volume (0-30), 25 is about 83%
  delay(100);
  
  Serial.println("DFPlayer Mini initialized successfully");
}

// ==================== LCD STATES ====================
void setLCDState(LCDState newState) {
  if (currentLCDState != newState) {
    if (!((currentLCDState >= LCD_BOOK_INFO_PAGE1 && currentLCDState <= LCD_BOOK_INFO_PAGE3) &&
          (newState >= LCD_BOOK_INFO_PAGE1 && newState <= LCD_BOOK_INFO_PAGE3))) {
      if (lcdInitialized) {
        lcd.clear();
        delay(2);
      }
    }
    
    currentLCDState = newState;
    lcdStateStartTime = millis();
    animationFrame = 0;
    
    if (newState >= LCD_BOOK_INFO_PAGE1 && newState <= LCD_BOOK_INFO_PAGE3) {
      showingBookInfo = true;
    } else if (newState == LCD_READY) {
      showingBookInfo = false;
    }
  }
}

void updateLCD() {
  static unsigned long lastUpdate = 0;
  unsigned long currentMillis = millis();
  
  if (!lcdInitialized) return;
  
  if (currentMillis - lastUpdate < 150) return;
  lastUpdate = currentMillis;
  
  unsigned long stateDuration = currentMillis - lcdStateStartTime;
  
  updateBeep();
  
  switch (currentLCDState) {
    case LCD_INIT: {
      if (animationFrame == 0) {
        lcd.clear();
        drawDoubleSeparator(0);
        centerText("LIBRARY SYSTEM", 1);
        centerText("v5.0", 2);
        drawDoubleSeparator(3);
        animationFrame = 1;
      }
      if (stateDuration > 2000) {
        setLCDState(LCD_WELCOME);
      }
      break;
    }
      
    case LCD_WELCOME: {
      if (animationFrame == 0) {
        lcd.clear();
        centerText("LEEJINBOTICS", 0);
        drawSeparator(1);
        centerText("INITIALIZING", 2);
        animationFrame = 1;
      }
      
      lcd.setCursor(6, 3);
      int dotCount = (stateDuration / 300) % 8;
      for (int i = 0; i < 8; i++) {
        if (i < dotCount) {
          lcd.print(".");
        } else {
          lcd.print(" ");
        }
      }
      
      if (stateDuration > 3000) {
        setLCDState(LCD_READY);
        systemReady = true;
        beep(BEEP_CONFIRM, 200);
        if (dfplayerInitialized) {
          startAudio(AUDIO_SYSTEM_READY);
        }
      }
      break;
    }
      
    case LCD_READY: {
      static unsigned long lastWifiUpdate = 0;
      static bool lastWifiState = wifiConnected;
      
      if (animationFrame == 0) {
        lcd.clear();
        drawDoubleSeparator(0);
        centerText("READY TO SCAN", 1);
        lcd.setCursor(0, 2);
        lcd.print("WiFi:");
        
        // Initial display
        lcd.setCursor(6, 2);
        if (wifiConnected) {
          lcd.print("ONLINE  ");
          lcd.write(223);
        } else {
          lcd.print("OFFLINE ");
          lcd.print("X");
        }
        
        drawDoubleSeparator(3);
        animationFrame = 1;
        lastWifiState = wifiConnected;
        lastWifiUpdate = millis();
      }
      
      // Update WiFi status regularly (every 500ms)
      if (millis() - lastWifiUpdate > 500) {
        if (wifiConnected != lastWifiState) {
          lcd.setCursor(6, 2);
          if (wifiConnected) {
            lcd.print("ONLINE  ");
            lcd.write(223);
          } else {
            lcd.print("OFFLINE ");
            lcd.print("X");
          }
          lastWifiState = wifiConnected;
        }
        lastWifiUpdate = millis();
      }
      
      // Blinking "SCAN" text
      if ((stateDuration / 500) % 2 == 0) {
        lcd.setCursor(8, 1);
        lcd.print("SCAN");
      } else {
        lcd.setCursor(8, 1);
        lcd.print("    ");
      }
      break;
    }
      
    case LCD_CARD_DETECTED: {
      if (animationFrame == 0) {
        lcd.clear();
        drawDoubleSeparator(0);
        centerText("SCANNING CARD", 1);
        lcd.setCursor(0, 2);
        lcd.print("UID: ");
        if (lastCardUID.length() > 12) {
          lcd.print(lastCardUID.substring(0, 12));
        } else {
          lcd.print(lastCardUID);
        }
        drawDoubleSeparator(3);
        beep(BEEP_SWIPE, 150);
        if (dfplayerInitialized) {
          startAudio(AUDIO_CARD_SCANNED);
        }
        animationFrame = 1;
      }
      
      int scanPos = (stateDuration / 200) % 16;
      lcd.setCursor(2, 3);
      for (int i = 0; i < 16; i++) {
        if (i == scanPos) {
          lcd.print(">");
        } else {
          lcd.print(" ");
        }
      }
      
      if (stateDuration > 2000) {
        setLCDState(LCD_BOOK_INFO_PAGE1);
      }
      break;
    }
      
    case LCD_BOOK_INFO_PAGE1: {
      if (animationFrame == 0) {
        lcd.clear();
        drawDoubleSeparator(0);
        centerText("BOOK DETAILS", 1);
        lcd.setCursor(0, 2);
        lcd.print("Page 1/3");
        lcd.setCursor(12, 2);
        lcd.print(">>");
        drawDoubleSeparator(3);
        animationFrame = 1;
        beep(BEEP_PAGE_CHANGE, 100);
      }
      
      String displayTitle = cleanDisplayText(currentBookTitle);
      centerText(displayTitle, 1);
      
      if (stateDuration > 3000) {
        setLCDState(LCD_BOOK_INFO_PAGE2);
      }
      break;
    }
      
    case LCD_BOOK_INFO_PAGE2: {
      if (animationFrame == 0) {
        lcd.clear();
        drawDoubleSeparator(0);
        centerText("AUTHOR & STATUS", 1);
        lcd.setCursor(0, 2);
        lcd.print("Page 2/3");
        lcd.setCursor(12, 2);
        lcd.print(">>");
        drawDoubleSeparator(3);
        animationFrame = 1;
        beep(BEEP_PAGE_CHANGE, 100);
      }
      
      String displayAuthor = cleanDisplayText(currentBookAuthor);
      centerText(displayAuthor, 1);
      
      lcd.setCursor(0, 2);
      lcd.print("Status: ");
      if (currentBookStatus == "AVL") {
        lcd.print("AVAILABLE ");
        lcd.write(3);
        if (animationFrame == 1) {
          beep(BEEP_AVL, 300);
          if (dfplayerInitialized) {
            startAudio(AUDIO_BOOK_AVAILABLE);
          }
          animationFrame = 2;
        }
      } else if (currentBookStatus == "BRW") {
        lcd.print("BORROWED  ");
        lcd.write(1);
        if (animationFrame == 1) {
          beep(BEEP_BRW, 300);
          if (dfplayerInitialized) {
            startAudio(AUDIO_BOOK_BORROWED);
          }
          animationFrame = 2;
        }
      } else if (currentBookStatus == "MIS") {
        lcd.print("MISPLACED ");
        if (animationFrame == 1) {
          beep(BEEP_ERROR, 300);
          if (dfplayerInitialized) {
            startAudio(AUDIO_BOOK_MISPLACED);
          }
          animationFrame = 2;
        }
      } else {
        lcd.print(currentBookStatus);
      }
      
      if (stateDuration > 3000) {
        setLCDState(LCD_BOOK_INFO_PAGE3);
      }
      break;
    }
      
    case LCD_BOOK_INFO_PAGE3: {
      if (animationFrame == 0) {
        lcd.clear();
        drawDoubleSeparator(0);
        centerText("LOCATION INFO", 1);
        lcd.setCursor(0, 2);
        lcd.print("Page 3/3");
        lcd.setCursor(12, 2);
        lcd.print(">>");
        drawDoubleSeparator(3);
        animationFrame = 1;
        beep(BEEP_PAGE_CHANGE, 100);
      }
      
      String displayLoc = cleanDisplayText(currentBookLocation);
      centerText(displayLoc, 1);
      
      lcd.setCursor(0, 2);
      lcd.print("Sensor: ");
      if (currentSensorIndex >= 0 && currentSensorIndex < NUM_SENSORS) {
        lcd.print(currentSensorIndex);
        lcd.print(" (Pin ");
        lcd.print(sensorPins[currentSensorIndex]);
        lcd.print(")");
      } else {
        lcd.print("N/A");
      }
      
      if (stateDuration > 3000) {
        setLCDState(LCD_READY);
      }
      break;
    }
      
    case LCD_OFFLINE: {
      if (animationFrame == 0) {
        lcd.clear();
        drawDoubleSeparator(0);
        centerText("OFFLINE MODE", 1);
        lcd.setCursor(0, 2);
        lcd.print("Data cached locally");
        drawDoubleSeparator(3);
        beep(BEEP_WIFI_DISCONNECT, 300);
        animationFrame = 1;
      }
      
      if ((stateDuration / 400) % 2 == 0) {
        lcd.setCursor(9, 1);
        lcd.print("OFFLINE");
      } else {
        lcd.setCursor(9, 1);
        lcd.print("       ");
      }
      break;
    }
      
    case LCD_NEW_BOOK: {
      if (animationFrame == 0) {
        lcd.clear();
        drawDoubleSeparator(0);
        centerText("NEW BOOK DETECTED", 1);
        lcd.setCursor(0, 2);
        lcd.print("UID: ");
        if (lastCardUID.length() > 12) {
          lcd.print(lastCardUID.substring(0, 12));
        } else {
          lcd.print(lastCardUID);
        }
        drawDoubleSeparator(3);
        beep(BEEP_SWIPE, 100);
        delay(50);
        beep(BEEP_CONFIRM, 150);
        if (dfplayerInitialized) {
          startAudio(AUDIO_NEW_BOOK);
        }
        animationFrame = 1;
      }
      
      if ((stateDuration / 300) % 2 == 0) {
        lcd.setCursor(2, 3);
        lcd.print("*** REGISTER ME ***");
      } else {
        clearLine(3);
      }
      
      if (stateDuration > 4000) {
        setLCDState(LCD_READY);
      }
      break;
    }
      
    case LCD_CLEARING_EEPROM: {
      if (animationFrame == 0) {
        lcd.clear();
        drawDoubleSeparator(0);
        centerText("EEPROM MAINTENANCE", 1);
        lcd.setCursor(0, 2);
        lcd.print("Cleaning storage...");
        drawDoubleSeparator(3);
        animationFrame = 1;
      }
      
      int progressBars = (stateDuration / 100) % 21;
      lcd.setCursor(0, 3);
      for (int i = 0; i < 20; i++) {
        if (i < progressBars) {
          lcd.print("#");
        } else {
          lcd.print(" ");
        }
      }
      
      if (stateDuration > 2000) {
        setLCDState(LCD_WELCOME);
      }
      break;
    }
      
    case LCD_CORRUPTED_DATA: {
      if (animationFrame == 0) {
        lcd.clear();
        drawDoubleSeparator(0);
        centerText("DATA INTEGRITY", 1);
        lcd.setCursor(0, 2);
        lcd.print("EEPROM Recovery...");
        drawDoubleSeparator(3);
        beep(BEEP_ERROR, 200);
        delay(50);
        beep(BEEP_ERROR, 200);
        if (dfplayerInitialized) {
          startAudio(AUDIO_ERROR);
        }
        animationFrame = 1;
      }
      
      if ((stateDuration / 200) % 2 == 0) {
        lcd.setCursor(6, 1);
        lcd.print("INTEGRITY");
      } else {
        lcd.setCursor(6, 1);
        lcd.print("         ");
      }
      
      if (stateDuration > 3000) {
        setLCDState(LCD_WELCOME);
      }
      break;
    }
  }
}

// ==================== EEPROM HELPERS ====================
String eepromReadString(int addr, int maxLen) {
  char buffer[maxLen + 1];
  int i;
  for (i = 0; i < maxLen; i++) {
    byte c = EEPROM.read(addr + i);
    if (c == 0x00) {
      break;
    }
    if (c == 0x01) {
      buffer[i] = (char)0x00;
    } else {
      buffer[i] = (char)c;
    }
  }
  buffer[i] = '\0';
  return String(buffer);
}

void eepromWriteString(int addr, const String &str, int maxLen) {
  unsigned int len = min(str.length(), (unsigned int)(maxLen - 1));
  
  for (unsigned int i = 0; i < len; i++) {
    char c = str.charAt(i);
    if (c == (char)0x00) {
      EEPROM.write(addr + i, 0x01);
    } else {
      EEPROM.write(addr + i, (byte)c);
    }
  }
  
  EEPROM.write(addr + len, 0x00);
  
  for (unsigned int i = len + 1; i < (unsigned int)maxLen; i++) {
    EEPROM.write(addr + i, 0xFF);
  }
}

// ==================== JSON VALIDATION ====================
bool isValidJSON(const String &json) {
  if (json.length() < 10) return false;
  if (!json.startsWith("{")) return false;
  if (!json.endsWith("}")) return false;
  
  int braceCount = 0;
  bool inString = false;
  
  for (unsigned int i = 0; i < json.length(); i++) {
    char c = json.charAt(i);
    
    if (c == '\\' && i + 1 < json.length()) {
      i++;
      continue;
    }
    
    if (c == '"') {
      inString = !inString;
    }
    
    if (!inString) {
      if (c == '{') braceCount++;
      else if (c == '}') braceCount--;
      
      if (braceCount < 0) return false;
    }
  }
  
  return (braceCount == 0 && !inString);
}

// ==================== CORRUPTION DETECTION ====================
bool isEEPROMCorrupted() {
  bool allEmpty = true;
  for (int i = 0; i < 20; i++) {
    if (EEPROM.read(REGISTERED_BOOKS_START + i) != 0xFF) {
      allEmpty = false;
      break;
    }
  }
  
  if (allEmpty) {
    return false;
  }
  
  for (int i = 0; i < 5; i++) {
    int addr = REGISTERED_BOOKS_START + i * (UID_LENGTH + 1);
    
    bool slotEmpty = true;
    for (int j = 0; j < UID_LENGTH; j++) {
      byte c = EEPROM.read(addr + j);
      if (c != 0xFF && c != 0x00) {
        slotEmpty = false;
        break;
      }
    }
    
    if (slotEmpty) continue;
    
    String uid = eepromReadString(addr, UID_LENGTH);
    
    if (uid.length() == UID_LENGTH) {
      bool validHex = true;
      for (unsigned int j = 0; j < uid.length(); j++) {
        char c = uid.charAt(j);
        if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) {
          validHex = false;
          break;
        }
      }
      
      if (!validHex) {
        return true;
      }
    } else if (uid.length() > 0 && uid.length() < UID_LENGTH) {
      return true;
    }
  }
  
  return false;
}

void clearCorruptedEEPROM() {
  setLCDState(LCD_CLEARING_EEPROM);
  Serial.println("Clearing corrupted EEPROM data...");
  
  for (int i = 0; i < MAX_REGISTERED_BOOKS * (UID_LENGTH + 1); i++) {
    EEPROM.write(REGISTERED_BOOKS_START + i, 0xFF);
  }
  
  for (int i = 0; i < MAX_CACHED_BOOKS * BOOK_CACHE_SIZE; i++) {
    EEPROM.write(BOOK_CACHE_START + i, 0xFF);
  }
  
  for (int i = 0; i < PENDING_MAX * PENDING_ENTRY_SIZE; i++) {
    EEPROM.write(PENDING_START + i, 0xFF);
  }
  
  Serial.println("EEPROM cleared successfully");
  delay(2000);
  setLCDState(LCD_WELCOME);
}

// ==================== BOOK CACHE SYSTEM ====================
int getBookCacheAddr(const String &uid) {
  unsigned int hash = 0;
  for (unsigned int i = 0; i < uid.length(); i++) {
    hash = (hash * 31) + uid.charAt(i);
  }
  int slot = hash % MAX_CACHED_BOOKS;
  return BOOK_CACHE_START + slot * BOOK_CACHE_SIZE;
}

void cacheBookData(const String &uid, const String &jsonData) {
  if (uid.length() != 8 || jsonData.length() == 0) return;
  
  if (!isValidJSON(jsonData)) {
    return;
  }
  
  int addr = getBookCacheAddr(uid);
  
  unsigned int len = jsonData.length();
  len = min(len, (unsigned int)(BOOK_CACHE_SIZE - 3));
  
  EEPROM.write(addr, (len >> 8) & 0xFF);
  EEPROM.write(addr + 1, len & 0xFF);
  
  for (unsigned int i = 0; i < len; i++) {
    char c = jsonData.charAt(i);
    if (c == (char)0x00) {
      EEPROM.write(addr + 2 + i, 0x01);
    } else {
      EEPROM.write(addr + 2 + i, (byte)c);
    }
  }
  
  EEPROM.write(addr + 2 + len, 0xFF);
}

String getCachedBookData(const String &uid) {
  if (uid.length() != 8) return "";
  
  int addr = getBookCacheAddr(uid);
  
  int len = (EEPROM.read(addr) << 8) | EEPROM.read(addr + 1);
  
  if (len > 0 && len <= (BOOK_CACHE_SIZE - 3)) {
    String cached = "";
    for (int i = 0; i < len; i++) {
      byte c = EEPROM.read(addr + 2 + i);
      if (c == 0x01) {
        cached += (char)0x00;
      } else {
        cached += (char)c;
      }
    }
    
    if (isValidJSON(cached)) {
      return cached;
    }
  }
  
  String cached = eepromReadString(addr, BOOK_CACHE_SIZE - 1);
  if (isValidJSON(cached)) {
    return cached;
  }
  
  return "";
}

// ==================== REGISTERED BOOKS MANAGEMENT ====================
bool isBookRegistered(const String &uid) {
  for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
    if (registeredUIDs[i] == uid) return true;
  }
  return false;
}

int getBookIndex(const String &uid) {
  for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
    if (registeredUIDs[i] == uid) return i;
  }
  return -1;
}

void saveRegisteredBook(const String &uid, int sensorIndex = -1) {
  if (isBookRegistered(uid)) return;
  
  for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
    if (registeredUIDs[i].length() == 0) {
      registeredUIDs[i] = uid;
      
      if (sensorIndex >= 0 && sensorIndex < NUM_SENSORS) {
        sensorAssignments[i] = sensorIndex;
      } else {
        for (int s = 0; s < NUM_SENSORS; s++) {
          bool sensorUsed = false;
          for (int j = 0; j < MAX_REGISTERED_BOOKS; j++) {
            if (sensorAssignments[j] == s) {
              sensorUsed = true;
              break;
            }
          }
          if (!sensorUsed) {
            sensorAssignments[i] = s;
            break;
          }
        }
      }
      
      int addr = REGISTERED_BOOKS_START + i * (UID_LENGTH + 1);
      eepromWriteString(addr, uid, UID_LENGTH);
      return;
    }
  }
}

void loadRegisteredBooks() {
  Serial.println("Loading registered books from EEPROM...");
  int bookCount = 0;
  
  for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
    registeredUIDs[i] = "";
    sensorAssignments[i] = 255;
  }
  
  for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
    int addr = REGISTERED_BOOKS_START + i * (UID_LENGTH + 1);
    String uid = eepromReadString(addr, UID_LENGTH);
    
    if (uid.length() == UID_LENGTH) {
      bool validHex = true;
      for (unsigned int j = 0; j < uid.length(); j++) {
        char c = uid.charAt(j);
        if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))) {
          validHex = false;
          break;
        }
      }
      
      if (validHex) {
        registeredUIDs[bookCount] = uid;
        sensorAssignments[bookCount] = 255;
        bookCount++;
      }
    }
  }
  
  if (bookCount == 0) {
    Serial.println("No registered books found in EEPROM");
  } else {
    Serial.print("Loaded ");
    Serial.print(bookCount);
    Serial.println(" registered books (sensors not assigned yet)");
  }
}

// ==================== SENSOR FUNCTIONS ====================
String readSensor(int pin) {
  static String lastState[NUM_SENSORS];

  int lowCount = 0;

  for (int i = 0; i < 10; i++) {
    if (digitalRead(pin) == LOW) lowCount++;
    delayMicroseconds(100);
  }

  if (lowCount > 8) {
    return "AVL";
  } else if (lowCount < 2) {
    return "BRW";
  } else {
    for (int i = 0; i < NUM_SENSORS; i++) {
      if (sensorPins[i] == pin) {
        return lastState[i];
      }
    }
    return "BRW";
  }
}

int getSensorIndexForBook(const String &uid) {
  for (int i = 0; i < MAX_REGISTERED_BOOKS; i++) {
    if (registeredUIDs[i] == uid) {
      if (sensorAssignments[i] >= 0 && sensorAssignments[i] < NUM_SENSORS) {
        return sensorAssignments[i];
      }
      break;
    }
  }
  return -1;
}

/**
 * Finds or assigns a sensor for a book
 * 
 * @param uid The RFID card UID to find or register
 * @param shouldRegister If true (default), will automatically register new books.
 *                       If false, will only return sensor for already registered books.
 *                       This prevents new books from being auto-registered before detection.
 * @return The sensor index (0-14) if found/assigned, or -1 if not registered and shouldRegister=false
 */
int findSensorForBook(const String &uid, bool shouldRegister = true) {
  // ✅ STEP 1: Check if book already has a sensor assigned
  int existingSensor = getSensorIndexForBook(uid);
  if (existingSensor != -1) {
    return existingSensor; // Book already registered, return its sensor
  }
  
  // ⚠️ STEP 2: Book NOT found in local registry
  // If shouldRegister is FALSE, return -1 to indicate "book not registered"
  // This is the key fix for new book detection
  if (!shouldRegister) {
    return -1; // ⬅️ CRITICAL: This allows processCard() to detect new books
  }
  
  // ✅ STEP 3: AUTO-REGISTRATION (only runs if shouldRegister=true)
  // Find an available sensor for this new book
  for (int sensorNum = 0; sensorNum < NUM_SENSORS; sensorNum++) {
    bool sensorInUse = false;
    
    // Check if this sensor number is already assigned to another book
    for (int j = 0; j < MAX_REGISTERED_BOOKS; j++) {
      if (sensorAssignments[j] == sensorNum) {
        sensorInUse = true;
        break;
      }
    }
    
    // ✅ Found an available sensor!
    if (!sensorInUse) {
      // Find an empty slot in the registration array
      for (int j = 0; j < MAX_REGISTERED_BOOKS; j++) {
        if (registeredUIDs[j].length() == 0) {
          // Register the book with this sensor
          registeredUIDs[j] = uid;
          sensorAssignments[j] = sensorNum;
          
          // Save to EEPROM for permanent storage
          int addr = REGISTERED_BOOKS_START + j * (UID_LENGTH + 1);
          eepromWriteString(addr, uid, UID_LENGTH);
          
          Serial.print("✅ AUTO-REGISTERED: Book ");
          Serial.print(uid);
          Serial.print(" → Sensor ");
          Serial.print(sensorNum);
          Serial.print(" (Pin ");
          Serial.print(sensorPins[sensorNum]);
          Serial.println(")");
          return sensorNum; // Return the assigned sensor
        }
      }
    }
  }
  
  // ⚠️ STEP 4: ALL SENSORS ARE IN USE - emergency fallback
  Serial.println("⚠️ WARNING: All sensors occupied! Using sensor 0 as fallback");
  
  // Still try to register even if all sensors are in use
  for (int j = 0; j < MAX_REGISTERED_BOOKS; j++) {
    if (registeredUIDs[j].length() == 0) {
      registeredUIDs[j] = uid;
      sensorAssignments[j] = 0; // Force assign to sensor 0
      
      int addr = REGISTERED_BOOKS_START + j * (UID_LENGTH + 1);
      eepromWriteString(addr, uid, UID_LENGTH);
      break;
    }
  }
  
  return 0; // Return sensor 0 as fallback
}

// int findSensorForBook(const String &uid) {
//   int existingSensor = getSensorIndexForBook(uid);
//   if (existingSensor != -1) {
//     return existingSensor;
//   }
  
//   for (int sensorNum = 0; sensorNum < NUM_SENSORS; sensorNum++) {
//     bool sensorInUse = false;
    
//     for (int j = 0; j < MAX_REGISTERED_BOOKS; j++) {
//       if (sensorAssignments[j] == sensorNum) {
//         sensorInUse = true;
//         break;
//       }
//     }
    
//     if (!sensorInUse) {
//       for (int j = 0; j < MAX_REGISTERED_BOOKS; j++) {
//         if (registeredUIDs[j].length() == 0) {
//           registeredUIDs[j] = uid;
//           sensorAssignments[j] = sensorNum;
          
//           int addr = REGISTERED_BOOKS_START + j * (UID_LENGTH + 1);
//           eepromWriteString(addr, uid, UID_LENGTH);
          
//           Serial.print("ASSIGNED: Book ");
//           Serial.print(uid);
//           Serial.print(" → Sensor ");
//           Serial.print(sensorNum);
//           Serial.print(" (Pin ");
//           Serial.print(sensorPins[sensorNum]);
//           Serial.println(")");
//           return sensorNum;
//         }
//       }
//     }
//   }
  
//   Serial.println("WARNING: All sensors in use! Using sensor 0 as fallback");
//   return 0;
// }

String getBookStatus(const String &uid) {
  int sensorIndex = getSensorIndexForBook(uid);

  if (sensorIndex == -1) {
    return "AVL";
  }

  if (sensorIndex >= 0 && sensorIndex < NUM_SENSORS) {
    return readSensor(sensorPins[sensorIndex]);
  }

  return "AVL";
}

// ==================== SERIAL COMMUNICATION ====================
void sendToESP(const String &msg) {
  while (Serial1.available()) Serial1.read();
  delay(5);
  
  Serial1.println(msg);
  
  Serial.print("TO_ESP: ");
  
  if (msg.startsWith("SEND:")) {
    int pipePos = msg.indexOf('|');
    if (pipePos != -1) {
      Serial.print(msg.substring(0, pipePos + 1));
      Serial.print("... [JSON length: ");
      Serial.print(msg.length() - pipePos - 1);
      Serial.println("]");
    } else {
      int showLen = min(msg.length(), 60);
      Serial.println(msg.substring(0, showLen));
    }
  } else {
    Serial.println(msg);
  }
}

bool waitForResponse(String &response, unsigned long timeout) {
  unsigned long start = millis();
  response = "";
  
  while (millis() - start < timeout) {
    while (Serial1.available()) {
      char c = Serial1.read();
      if (c == '\n') {
        response.trim();
        if (response.length() > 0) {
          if (!response.startsWith("DEBUG:") && !response.startsWith("WIFI:")) {
            Serial.print("FROM_ESP: ");
            Serial.println(response);
          }
          return true;
        }
      } else if (c != '\r' && response.length() < 2048) {
        response += c;
      }
    }
    delay(1);
  }
  return false;
}

// ==================== PENDING QUEUE ====================
int pendingIndexAddr(int index) { 
  return PENDING_START + index * PENDING_ENTRY_SIZE; 
}

bool addPendingEntry(const String &payload) {
  if (!payload.startsWith("SEND:")) {
    return false;
  }
  
  int pipePos = payload.indexOf('|');
  if (pipePos != -1) {
    String jsonPart = payload.substring(pipePos + 1);
    if (!isValidJSON(jsonPart)) {
      return false;
    }
  }
  
  for (int i = 0; i < PENDING_MAX; i++) {
    int addr = pendingIndexAddr(i);
    if (EEPROM.read(addr) == 0xFF) {
      eepromWriteString(addr, payload, PENDING_ENTRY_SIZE - 1);
      Serial.print("QUEUED: ");
      
      int showLen = min(payload.length(), 60);
      Serial.println(payload.substring(0, showLen));
      
      return true;
    }
  }
  Serial.println("QUEUE FULL!");
  return false;
}

bool popPendingEntry(String &outPayload) {
  for (int i = 0; i < PENDING_MAX; i++) {
    int addr = pendingIndexAddr(i);
    if (EEPROM.read(addr) != 0xFF && EEPROM.read(addr) != 0x00) {
      outPayload = eepromReadString(addr, PENDING_ENTRY_SIZE - 1);
      
      for (int j = 0; j < PENDING_ENTRY_SIZE; j++) {
        EEPROM.write(addr + j, 0xFF);
      }
      
      if (outPayload.length() > 10 && 
          outPayload.startsWith("SEND:") && 
          outPayload.indexOf('|') > 5) {
        
        int pipePos = outPayload.indexOf('|');
        String jsonPart = outPayload.substring(pipePos + 1);
        if (isValidJSON(jsonPart)) {
          return true;
        }
      }
    }
  }
  return false;
}

void flushPendingQueue() {
  if (!wifiConnected) return;
  
  String payload;
  int flushed = 0;
  
  while (popPendingEntry(payload)) {
    if (payload.length() > 10) {
      if (!payload.startsWith("SEND:")) {
        continue;
      }
      
      Serial.print("FLUSHING: ");
      Serial.println(payload.substring(0, 60));
      sendToESP(payload);
      
      String response;
      if (waitForResponse(response, REQUEST_TIMEOUT)) {
        if (response.startsWith("SUCCESS:")) {
          flushed++;
          delay(50);
        } else {
          addPendingEntry(payload);
          break;
        }
      } else {
        addPendingEntry(payload);
        break;
      }
    }
  }
  
  if (flushed > 0) {
    Serial.print("Successfully flushed ");
    Serial.print(flushed);
    Serial.println(" queued entries");
  }
}

// ==================== RFID ====================


// ==================== RFID ====================
String readRFID() {
  // Look for new cards (non-blocking check)
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return "";
  }
  
  // Read the card
  if (!mfrc522.PICC_ReadCardSerial()) {
    return "";
  }
  
  // Get UID
  String uid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(mfrc522.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();
  
  // IMPORTANT: Clean up properly
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  
  return uid;
}
// String readRFID() {
//   if (!mfrc522.PICC_IsNewCardPresent()) return "";
//   if (!mfrc522.PICC_ReadCardSerial()) return "";
  
//   String uid = "";
//   for (byte i = 0; i < mfrc522.uid.size; i++) {
//     if (mfrc522.uid.uidByte[i] < 0x10) uid += "0";
//     uid += String(mfrc522.uid.uidByte[i], HEX);
//   }
//   uid.toUpperCase();
//   mfrc522.PICC_HaltA();
//   return uid;
// }







void processCard(const String &cardUID) {
  if (!systemReady) return;
  if (currentLCDState != LCD_READY) return;
  
  if (cardUID == lastCardUID && millis() - lastCardTime < DEBOUNCE_TIME) {
    return;
  }
  
  lastCardUID = cardUID;
  lastCardTime = millis();
  setLCDState(LCD_CARD_DETECTED);
  
  // ✅ STEP 1: Check WITHOUT auto-registering first
  int sensorIndex = findSensorForBook(cardUID, false); // false = don't auto-register
  bool locallyRegistered = (sensorIndex != -1);
  String sensorStatus = "AVL";
  
  // Only read sensor if book is already registered locally
  if (locallyRegistered && sensorIndex >= 0 && sensorIndex < NUM_SENSORS) {
    sensorStatus = readSensor(sensorPins[sensorIndex]);
  }
  
  Serial.println("=================================");
  Serial.print("CARD: ");
  Serial.println(cardUID);
  Serial.print("LOCAL REGISTRATION: ");
  Serial.println(locallyRegistered ? "YES" : "NO");
  if (locallyRegistered) {
    Serial.print("SENSOR INDEX: ");
    Serial.println(sensorIndex);
    Serial.print("SENSOR STATUS: ");
    Serial.println(sensorStatus);
  }
  Serial.println("=================================");
  
  bool serverIsRegistered = false;
  String serverTitle = "";
  String serverAuthor = "";
  String serverLocation = "";
  String serverEspStatus = "AVL";
  bool serverMisplaced = false;
  int serverSensor = 0;
  bool usingCachedData = false;
  String serverDataJson = "";
  
  // 🌐 STEP 2: Check Firebase
  if (wifiConnected) {
    sendToESP("GET:" + cardUID);
    String response;
    if (waitForResponse(response, REQUEST_TIMEOUT)) {
      if (response.startsWith("BOOKDATA:")) {
        int p = response.indexOf('|');
        if (p != -1) {
          serverDataJson = response.substring(p + 1);
          
          if (serverDataJson != "null") {
            JsonDocument doc;
            DeserializationError err = deserializeJson(doc, serverDataJson);
            if (!err) {
              serverIsRegistered = doc["isRegistered"] | false;
              serverTitle = doc["title"] | "";
              serverAuthor = doc["author"] | "";
              serverLocation = doc["location"] | "";
              serverSensor = doc["sensor"] | 0;
              serverMisplaced = doc["misplaced"] | false;
              
              serverTitle.trim();
              serverAuthor.trim(); 
              serverLocation.trim();
              
              if (doc["espStatus"]) {
                serverEspStatus = String(doc["espStatus"].as<const char*>());
              }
              
              Serial.print("FIREBASE: Registered=");
              Serial.print(serverIsRegistered ? "YES" : "NO");
              Serial.print(", Status=");
              Serial.print(serverEspStatus);
              Serial.print(", Misplaced=");
              Serial.println(serverMisplaced ? "YES" : "NO");
              
              cacheBookData(cardUID, serverDataJson);
              
              if (serverIsRegistered && !locallyRegistered) {
                // ⚠️ FIX: Only register locally AFTER Firebase confirms registration
                sensorIndex = findSensorForBook(cardUID, true); // NOW register
                saveRegisteredBook(cardUID, serverSensor);
                locallyRegistered = true;
                
                // Update sensor reading
                if (sensorIndex >= 0 && sensorIndex < NUM_SENSORS) {
                  sensorStatus = readSensor(sensorPins[sensorIndex]);
                }
              }
            }
          } else {
            Serial.println("Book not found in Firebase");
          }
        }
      }
    } else {
      wifiConnected = false;
      setLCDState(LCD_OFFLINE);
    }
  }
  
  // 📦 STEP 3: Check cache (offline mode)
  if (!wifiConnected || serverTitle.length() == 0) {
    String cachedData = getCachedBookData(cardUID);
    if (cachedData.length() > 0) {
      serverDataJson = cachedData;
      JsonDocument doc;
      DeserializationError err = deserializeJson(doc, cachedData);
      if (!err) {
        serverIsRegistered = doc["isRegistered"] | false;
        serverTitle = doc["title"] | "";
        serverAuthor = doc["author"] | "";
        serverLocation = doc["location"] | "";
        serverSensor = doc["sensor"] | 0;
        serverMisplaced = doc["misplaced"] | false;
        
        if (doc["espStatus"]) {
          serverEspStatus = String(doc["espStatus"].as<const char*>());
        }
        
        usingCachedData = true;
        Serial.println("USING CACHED DATA");
      }
    }
  }
  
  // 🆕 STEP 4: NEW BOOK DETECTION - FIXED
  if (serverTitle.length() == 0) {
    if (locallyRegistered) {
      // Case 1: In local memory but not in Firebase/cache
      serverTitle = "Book Title Unknown";
      serverAuthor = "Author Unknown";
      serverLocation = "Location Unknown";
      serverIsRegistered = true;
      Serial.println("Using local registration (no Firebase/cache data)");
    } else {
      // 🎉 Case 2: TRULY NEW BOOK - BOTH AUDIO AND FIREBASE UPDATE
      serverTitle = "New Book";
      serverAuthor = "Unknown";
      serverLocation = "Unassigned";
      serverIsRegistered = false;
      
      // ✅ FIX: Play track 008 for new books
      stopAudio(); // Stop any current audio
      setLCDState(LCD_NEW_BOOK);
      beep(BEEP_SWIPE, 100);
      delay(50);
      beep(BEEP_CONFIRM, 150);
      if (dfplayerInitialized) {
        startAudio(AUDIO_NEW_BOOK); // Track 008
      }
      
      Serial.println("🎉 NEW UNREGISTERED BOOK DETECTED - Playing track 008");
      
      // ✅ FIX: STILL SEND TO FIREBASE for editing (like before)
      // This creates a placeholder in the main books collection
      JsonDocument doc;
      doc["title"] = "Book Title Unknown";
      doc["author"] = "Author Unknown";
      doc["location"] = "Location Unknown";
      doc["isRegistered"] = true; // ⬅️ Mark as registered
      doc["lastSeen"] = String(millis() / 1000);
      
      // ⚠️ AUTO-ASSIGN A SENSOR for this new book (like your old code)
      sensorIndex = findSensorForBook(cardUID, true); // true = auto-register
      doc["sensor"] = sensorIndex;
      
      // Get sensor status for the new book
      if (sensorIndex >= 0 && sensorIndex < NUM_SENSORS) {
        sensorStatus = readSensor(sensorPins[sensorIndex]);
        doc["espStatus"] = sensorStatus;
        if (sensorStatus == "AVL") doc["status"] = "Available";
        else if (sensorStatus == "BRW") doc["status"] = "Borrowed";
        else doc["status"] = "Available";
      } else {
        doc["espStatus"] = "BRW";
        doc["status"] = "Borrowed";
      }
      
      doc["lastUpdated"] = String(millis() / 1000);
      
      String outJson;
      serializeJson(doc, outJson);
      
      // Send to main Firebase path (like before)
      String sendCmd = "SEND:/books/" + cardUID + "|" + outJson;
      
      if (wifiConnected) {
        sendToESP(sendCmd);
        String response;
        if (waitForResponse(response, REQUEST_TIMEOUT)) {
          if (response.startsWith("SUCCESS:")) {
            cacheBookData(cardUID, outJson);
            saveRegisteredBook(cardUID, sensorIndex);
            Serial.println("✅ New book registered in Firebase");
          } else {
            addPendingEntry(sendCmd);
          }
        } else {
          addPendingEntry(sendCmd);
        }
      } else {
        addPendingEntry(sendCmd);
        Serial.println("WiFi offline - queued");
      }
      
      return; // Exit after handling new book
    }
  }
  
  // ✨ STEP 5: Normal processing for existing books
  serverTitle = cleanDisplayText(serverTitle);
  serverAuthor = cleanDisplayText(serverAuthor);
  serverLocation = cleanDisplayText(serverLocation);
  
  currentBookTitle = serverTitle;
  currentBookAuthor = serverAuthor;
  currentBookLocation = serverLocation;
  currentSensorIndex = sensorIndex;
  
  if (serverEspStatus == "MIS" || serverMisplaced) {
    currentBookStatus = "MIS";
    Serial.println("OVERRIDE: Book marked as MISPLACED in Firebase");
  } else {
    currentBookStatus = sensorStatus;
  }
  
  JsonDocument doc;
  doc["title"] = serverTitle;
  doc["author"] = serverAuthor;
  doc["location"] = serverLocation;
  doc["isRegistered"] = serverIsRegistered;
  doc["lastSeen"] = String(millis() / 1000);
  doc["sensor"] = sensorIndex;
  
  if (serverEspStatus == "MIS" || serverMisplaced) {
    doc["espStatus"] = "MIS";
    doc["status"] = "Misplaced";
    doc["misplaced"] = true;
  } else {
    doc["espStatus"] = sensorStatus;
    
    if (sensorStatus == "AVL") doc["status"] = "Available";
    else if (sensorStatus == "BRW") doc["status"] = "Borrowed";
    else if (sensorStatus == "OVD") doc["status"] = "Overdue";
    else if (sensorStatus == "MIS") {
      doc["status"] = "Misplaced";
      doc["misplaced"] = true;
    } else doc["status"] = "Available";
  }
  
  doc["lastUpdated"] = String(millis() / 1000);
  
  if (usingCachedData) {
    doc["source"] = "cached";
  }
  
  String outJson;
  serializeJson(doc, outJson);
  
  Serial.print("JSON SIZE: ");
  Serial.println(outJson.length());
  
  if (!outJson.endsWith("}")) {
    outJson += "}";
  }
  
  String sendCmd = "SEND:/books/" + cardUID + "|" + outJson;
  
  if (wifiConnected) {
    sendToESP(sendCmd);
    String response;
    if (waitForResponse(response, REQUEST_TIMEOUT)) {
      if (response.startsWith("SUCCESS:")) {
        cacheBookData(cardUID, outJson);
        if (serverIsRegistered && !locallyRegistered) {
          saveRegisteredBook(cardUID, sensorIndex);
        }
        Serial.println("Firebase update OK");
      } else {
        addPendingEntry(sendCmd);
      }
    } else {
      addPendingEntry(sendCmd);
    }
  } else {
    addPendingEntry(sendCmd);
    Serial.println("WiFi offline - queued");
  }
}
//NEW BOOK DOES NOT PLAY TRACK 8
//NEW BOOK DOES NOT PLAY TRACK 8
// ==================== MAIN PROCESSING ====================
//NEW BOOK DOES NOT PLAY TRACK 8
//NEW BOOK DOES NOT PLAY TRACK 8
// void processCard(const String &cardUID) {
//   if (!systemReady) return;
  
//   if (currentLCDState != LCD_READY) {
//     return;
//   }
  
//   if (cardUID == lastCardUID && millis() - lastCardTime < DEBOUNCE_TIME) {
//     return;
//   }
  
//   lastCardUID = cardUID;
//   lastCardTime = millis();
//   setLCDState(LCD_CARD_DETECTED);
  
//   int sensorIndex = findSensorForBook(cardUID);
//   String sensorStatus = "AVL";
//   if (sensorIndex >= 0 && sensorIndex < NUM_SENSORS) {
//     sensorStatus = readSensor(sensorPins[sensorIndex]);
//   }
  
//   Serial.println("=================================");
//   Serial.print("CARD: ");
//   Serial.println(cardUID);
//   Serial.print("SENSOR INDEX: ");
//   Serial.println(sensorIndex);
//   Serial.print("SENSOR STATUS: ");
//   Serial.println(sensorStatus);
//   Serial.println("=================================");
  
//   bool serverIsRegistered = false;
//   String serverTitle = "";
//   String serverAuthor = "";
//   String serverLocation = "";
//   String serverEspStatus = "AVL";
//   bool serverMisplaced = false;
//   int serverSensor = 0;
//   bool usingCachedData = false;
//   String serverDataJson = "";
  
//   if (wifiConnected) {
//     sendToESP("GET:" + cardUID);
//     String response;
//     if (waitForResponse(response, REQUEST_TIMEOUT)) {
//       if (response.startsWith("BOOKDATA:")) {
//         int p = response.indexOf('|');
//         if (p != -1) {
//           serverDataJson = response.substring(p + 1);
          
//           if (serverDataJson != "null") {
//             JsonDocument doc;
//             DeserializationError err = deserializeJson(doc, serverDataJson);
//             if (!err) {
//               serverIsRegistered = doc["isRegistered"] | false;
//               serverTitle = doc["title"] | "";
//               serverAuthor = doc["author"] | "";
//               serverLocation = doc["location"] | "";
//               serverSensor = doc["sensor"] | 0;
//               serverMisplaced = doc["misplaced"] | false;
              
//               serverTitle.trim();
//               serverAuthor.trim(); 
//               serverLocation.trim();
              
//               if (doc["espStatus"]) {
//                 serverEspStatus = String(doc["espStatus"].as<const char*>());
//               }
              
//               Serial.print("FIREBASE: Registered=");
//               Serial.print(serverIsRegistered ? "YES" : "NO");
//               Serial.print(", Status=");
//               Serial.print(serverEspStatus);
//               Serial.print(", Misplaced=");
//               Serial.println(serverMisplaced ? "YES" : "NO");
              
//               cacheBookData(cardUID, serverDataJson);
              
//               if (serverIsRegistered && !isBookRegistered(cardUID)) {
//                 saveRegisteredBook(cardUID, serverSensor);
//               }
//             }
//           } else {
//             Serial.println("Book not found in Firebase");
//           }
//         }
//       }
//     } else {
//       wifiConnected = false;
//       setLCDState(LCD_OFFLINE);
//     }
//   }
  
//   if (!wifiConnected || serverTitle.length() == 0) {
//     String cachedData = getCachedBookData(cardUID);
//     if (cachedData.length() > 0) {
//       serverDataJson = cachedData;
//       JsonDocument doc;
//       DeserializationError err = deserializeJson(doc, cachedData);
//       if (!err) {
//         serverIsRegistered = doc["isRegistered"] | false;
//         serverTitle = doc["title"] | "";
//         serverAuthor = doc["author"] | "";
//         serverLocation = doc["location"] | "";
//         serverSensor = doc["sensor"] | 0;
//         serverMisplaced = doc["misplaced"] | false;
        
//         if (doc["espStatus"]) {
//           serverEspStatus = String(doc["espStatus"].as<const char*>());
//         }
        
//         usingCachedData = true;
//         Serial.println("USING CACHED DATA");
//       }
//     }
//   }
  
//   if (serverTitle.length() == 0) {
//     if (isBookRegistered(cardUID)) {
//       serverTitle = "Book Title Unknown";
//       serverAuthor = "Author Unknown";
//       serverLocation = "Location Unknown";
//       serverIsRegistered = true;
//       Serial.println("Using local registration (no cache)");
//     } else {
//       serverTitle = "New Book";
//       serverAuthor = "Unknown";
//       serverLocation = "Unassigned";
//       serverIsRegistered = false;
//       setLCDState(LCD_NEW_BOOK);
//       Serial.println("New unregistered book");
//       return;
//     }
//   }
  
//   // Clean data before storing for LCD display
//   serverTitle = cleanDisplayText(serverTitle);
//   serverAuthor = cleanDisplayText(serverAuthor);
//   serverLocation = cleanDisplayText(serverLocation);
  
//   // Update global variables for LCD display
//   currentBookTitle = serverTitle;
//   currentBookAuthor = serverAuthor;
//   currentBookLocation = serverLocation;
//   currentSensorIndex = sensorIndex;
  
//   // PRIORITY LOGIC: If book is marked as misplaced in Firebase, use MIS status
//   if (serverEspStatus == "MIS" || serverMisplaced) {
//     currentBookStatus = "MIS";
//     Serial.println("OVERRIDE: Book marked as MISPLACED in Firebase");
//   } else {
//     currentBookStatus = sensorStatus;  // Use sensor reading for other statuses
//   }
  
//   JsonDocument doc;
  
//   doc["title"] = serverTitle;
//   doc["author"] = serverAuthor;
//   doc["location"] = serverLocation;
//   doc["isRegistered"] = serverIsRegistered;
//   doc["lastSeen"] = String(millis() / 1000);
//   doc["sensor"] = sensorIndex;
  
//   // If book is marked as misplaced, keep it as MIS regardless of sensor
//   if (serverEspStatus == "MIS" || serverMisplaced) {
//     doc["espStatus"] = "MIS";
//     doc["status"] = "Misplaced";
//     doc["misplaced"] = true;
//   } else {
//     // Normal sensor-based logic
//     doc["espStatus"] = sensorStatus;
    
//     if (sensorStatus == "AVL") doc["status"] = "Available";
//     else if (sensorStatus == "BRW") doc["status"] = "Borrowed";
//     else if (sensorStatus == "OVD") doc["status"] = "Overdue";
//     else if (sensorStatus == "MIS") {
//       doc["status"] = "Misplaced";
//       doc["misplaced"] = true;
//     } else doc["status"] = "Available";
//   }
  
//   doc["lastUpdated"] = String(millis() / 1000);
  
//   if (usingCachedData) {
//     doc["source"] = "cached";
//   }
  
//   String outJson;
//   serializeJson(doc, outJson);
  
//   Serial.print("JSON SIZE: ");
//   Serial.println(outJson.length());
  
//   if (!outJson.endsWith("}")) {
//     outJson += "}";
//   }
  
//   String sendCmd = "SEND:/books/" + cardUID + "|" + outJson;
  
//   if (wifiConnected) {
//     sendToESP(sendCmd);
    
//     String response;
//     if (waitForResponse(response, REQUEST_TIMEOUT)) {
//       if (response.startsWith("SUCCESS:")) {
//         cacheBookData(cardUID, outJson);
        
//         if (serverIsRegistered && !isBookRegistered(cardUID)) {
//           saveRegisteredBook(cardUID, sensorIndex);
//         }
//         Serial.println("Firebase update OK");
//       } else {
//         addPendingEntry(sendCmd);
//       }
//     } else {
//       addPendingEntry(sendCmd);
//     }
//   } else {
//     addPendingEntry(sendCmd);
//     Serial.println("WiFi offline - queued");
//   }
// }
// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  Serial1.begin(115200);
  
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcdInitialized = true;
  
  lcd.setCursor(0, 0);
  lcd.print("====================");
  lcd.setCursor(4, 1);
  lcd.print("LIBRARY SYSTEM");
  lcd.setCursor(5, 2);
  lcd.print("INITIALIZING");
  lcd.setCursor(0, 3);
  lcd.print("====================");
  
  delay(100);
  
  Serial.println();
  Serial.println("========================================");
  Serial.println("    LIBRARY SYSTEM v5.0 - MEGA 2560     ");
  Serial.println("========================================");
  
  for (int i = 0; i < NUM_SENSORS; i++) {
    pinMode(sensorPins[i], INPUT_PULLUP);
  }
  
  // Initialize DFPlayer Mini
  initDFPlayer();
  
  if (isEEPROMCorrupted()) {
    Serial.println("EEPROM corrupted - clearing");
    clearCorruptedEEPROM();
  } else {
    loadRegisteredBooks();
  }
  
  SPI.begin();
  mfrc522.PCD_Init();
  
  // Test RFID
  byte v = mfrc522.PCD_ReadRegister(MFRC522::VersionReg);
  Serial.print("RFID Version: 0x");
  Serial.println(v, HEX);

  if (v == 0x00 || v == 0xFF) {
    Serial.println("RFID: Check external power connection");
  } else {
    Serial.println("RFID: Ready with external power!");
  }
  
  Serial.println("Testing ESP connection...");
  sendToESP("PING");
  
  setLCDState(LCD_INIT);
  
  Serial.println("Setup complete! System ready.");
}

// ==================== LOOP ====================
void loop() {

   // Update non-blocking audio (MUST BE FIRST!)
  updateAudio();


    
  // RFID health check (every 30 seconds)
  static unsigned long lastRFIDCheck = 0;
  if (millis() - lastRFIDCheck > 30000) {
    byte v = mfrc522.PCD_ReadRegister(MFRC522::VersionReg);
    if (v == 0x00 || v == 0xFF) {
      Serial.println("RFID: Lost communication! Check power.");
    }
    lastRFIDCheck = millis();
  }
  
  
  updateLCD();
  
  if (systemReady && currentLCDState == LCD_READY) {
    String uid = readRFID();
    if (uid.length() == 8) {
      processCard(uid);
    }
  }
  
  static String espBuffer = "";
  while (Serial1.available()) {
    char c = Serial1.read();
    if (c == '\n') {
      espBuffer.trim();
      if (espBuffer.length() > 0) {
        if (espBuffer.startsWith("WIFI:")) {
          if (espBuffer == "WIFI:OK") {
            if (!wifiConnected) {
              wifiConnected = true;
              Serial.println("WiFi connected");
              beep(BEEP_WIFI_CONNECT, 300);
              if (dfplayerInitialized) {
                startAudio(AUDIO_WIFI_CONNECTED);
              }
              flushPendingQueue();
            }
          } else if (espBuffer == "WIFI:OFFLINE") {
            if (wifiConnected) {
              wifiConnected = false;
              Serial.println("WiFi offline");
              beep(BEEP_WIFI_DISCONNECT, 300);
              if (dfplayerInitialized) {
                startAudio(AUDIO_WIFI_DISCONNECTED);
              }
            }
          }
        } else if (espBuffer == "PONG") {
          if (!wifiConnected) {
            wifiConnected = true;
            Serial.println("WiFi connected via PONG");
          }
        }
      }
      espBuffer = "";
    } else if (c != '\r') {
      espBuffer += c;
    }
  }
  
  delay(10);
}