#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Update.h>

// ==================== CONFIG ====================
const char* ssid = "secroom";
const char* password = "$1231234";
const char* firebaseHost = "libtest-33d26-default-rtdb.firebaseio.com";
const char* apiKey = "AIzaSyAuN-gyzwFj7FeLUMfsFyo5FmWnzjHnyMM";

// ==================== GLOBALS ====================
bool wifiConnected = false;
TaskHandle_t wifiTaskHandle = NULL;

// ==================== SAFE WIFI CONNECT FUNCTION ====================
void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);              // FIX 1: No modem-sleep (stops random drops)
  WiFi.setAutoReconnect(true);       // FIX 2: Enable auto reconnect
  WiFi.persistent(true);

  WiFi.begin(ssid, password);

  Serial.print("Connecting WiFi");

  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) {
    delay(500);
    Serial.print(".");
    retry++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println("\nWIFI:OK");
  } else {
    wifiConnected = false;
    Serial.println("\nWIFI:OFFLINE");
  }
}

// ==================== WIFI TASK (RUNS ON CORE 0) ====================
void wifiTask(void * parameter) {
  while (true) {

    if (WiFi.status() != WL_CONNECTED) {
      if (wifiConnected == true) {
        // It was connected before → now disconnected
        Serial.println("WIFI:OFFLINE");
      }

      wifiConnected = false;

      Serial.println("WIFI:RECONNECTING...");
      connectWiFi();                    // REAL reconnect, not soft reconnect
    }
    else {
      if (!wifiConnected) {
        Serial.println("WIFI:OK");
      }
      wifiConnected = true;
    }

    delay(3000);
  }
}

// ==================== FIREBASE FUNCTIONS ====================
bool firebasePatch(const String &path, const String &jsonData) {
  if (!wifiConnected) return false;

  String url = "https://" + String(firebaseHost) + path + ".json?auth=" + String(apiKey);
  HTTPClient http;

  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(8000);  // Increased timeout for better reliability

  int code = http.PATCH(jsonData);
  String response = http.getString();
  http.end();

  // Log the response for debugging
  if (code != 200) {
    Serial.print("Firebase PATCH failed: ");
    Serial.print(code);
    Serial.print(" - ");
    Serial.println(response);
  }

  return (code == 200);
}

String firebaseGet(const String &uid) {
  if (!wifiConnected) return "";

  String url = "https://" + String(firebaseHost) + "/books/" + uid + ".json?auth=" + String(apiKey);
  HTTPClient http;

  http.begin(url);
  http.setTimeout(5000);

  int code = http.GET();
  String response = http.getString();
  http.end();

  if (code == 200) {
    response.trim();
    if (response == "null") return "";
    return response;
  }

  Serial.print("Firebase GET failed: ");
  Serial.print(code);
  Serial.print(" - ");
  Serial.println(response);
  return "";
}

// ==================== JSON VALIDATION HELPERS ====================
bool isValidBookTitle(const String &title) {
  if (title.length() == 0) return false;
  if (title == "New Book") return false;
  if (title == "Book Title Unknown") return false;
  if (title.indexOf("undefined") != -1) return false;
  if (title.indexOf("null") != -1) return false;
  return true;
}

bool isValidBookAuthor(const String &author) {
  if (author.length() == 0) return false;
  if (author == "Unknown") return false;
  if (author.indexOf("undefined") != -1) return false;
  if (author.indexOf("null") != -1) return false;
  return true;
}

bool isValidBookLocation(const String &location) {
  if (location.length() == 0) return false;
  if (location == "Unassigned") return false;
  if (location.indexOf("undefined") != -1) return false;
  if (location.indexOf("null") != -1) return false;
  return true;
}
void processCommand(const String &cmd) {
  String cleanCmd = cmd;
  cleanCmd.trim();

  if (cleanCmd == "PING") {
    Serial.println("PONG");
    return;
  }

  if (cleanCmd.startsWith("GET:")) {
    String uid = cleanCmd.substring(4);
    uid.trim();

    if (uid.length() != 8) {
      Serial.println("ERROR:InvalidUID");
      return;
    }

    String data = firebaseGet(uid);
    if (data.length() == 0) {
      Serial.println("BOOKDATA:" + uid + "|null");
    } else {
      Serial.println("BOOKDATA:" + uid + "|" + data);
    }
    return;
  }

  if (cleanCmd.startsWith("SEND:")) {
    int p = cleanCmd.indexOf('|');
    if (p == -1) {
      Serial.println("ERROR:BadFormat");
      return;
    }

    String path = cleanCmd.substring(5, p);
    String json = cleanCmd.substring(p + 1);

    path.trim();
    json.trim();

    if (json.length() < 10 || !json.startsWith("{") || !json.endsWith("}")) {
      Serial.println("ERROR:InvalidJSON");
      return;
    }

    DynamicJsonDocument doc(4096);
    DeserializationError err = deserializeJson(doc, json);

    if (err) {
      Serial.print("ERROR:InvalidJSON - ");
      Serial.println(err.c_str());
      return;
    }

    // Get current Firebase data silently
    String uid = "";
    if (path.startsWith("/books/")) {
      uid = path.substring(7);
    }
    
    String currentFirebaseData = firebaseGet(uid);
    String currentTitle = "";
    String currentAuthor = "";
    String currentLocation = "";
    String currentEspStatus = "";
    bool currentMisplaced = false;
    
    if (currentFirebaseData.length() > 0) {
      DynamicJsonDocument existingDoc(1024);
      if (!deserializeJson(existingDoc, currentFirebaseData)) {
        currentTitle = existingDoc["title"] | "";
        currentAuthor = existingDoc["author"] | "";
        currentLocation = existingDoc["location"] | "";
        currentEspStatus = existingDoc["espStatus"] | "";
        currentMisplaced = existingDoc["misplaced"] | false;
      }
    }
    
    // Get incoming data
    String incomingTitle = doc["title"] | "";
    String incomingAuthor = doc["author"] | "";
    String incomingLocation = doc["location"] | "";
    bool isRegistered = doc["isRegistered"] | false;
    String incomingEspStatus = doc["espStatus"] | "";
    
    // Preserve real Firebase data over Mega defaults
    bool titleWasDefault = false;
    bool authorWasDefault = false;
    bool locationWasDefault = false;
    
    if ((incomingTitle == "Book Title Unknown" || incomingTitle == "New Book" || 
         incomingTitle == "undefined" || incomingTitle == "null" || incomingTitle.length() == 0) &&
        (currentTitle != "Book Title Unknown" && currentTitle != "New Book" && 
         currentTitle.length() > 0 && !currentTitle.equals("undefined"))) {
      doc["title"] = currentTitle;
      titleWasDefault = true;
    }
    
    if ((incomingAuthor == "Author Unknown" || incomingAuthor == "Unknown" || 
         incomingAuthor == "undefined" || incomingAuthor == "null" || incomingAuthor.length() == 0) &&
        (currentAuthor != "Author Unknown" && currentAuthor != "Unknown" && 
         currentAuthor.length() > 0 && !currentAuthor.equals("undefined"))) {
      doc["author"] = currentAuthor;
      authorWasDefault = true;
    }
    
    if ((incomingLocation == "Location Unknown" || incomingLocation == "Unassigned" || 
         incomingLocation == "undefined" || incomingLocation == "null" || incomingLocation.length() == 0) &&
        (currentLocation != "Location Unknown" && currentLocation != "Unassigned" && 
         currentLocation.length() > 0 && !currentLocation.equals("undefined"))) {
      doc["location"] = currentLocation;
      locationWasDefault = true;
    }
    
    // ==================== MIS STATUS PRESERVATION FIX ====================
    // Preserve MIS status if already set in Firebase or incoming data
    if (currentEspStatus == "MIS" || incomingEspStatus == "MIS" || currentMisplaced) {
      doc["espStatus"] = "MIS";
      doc["misplaced"] = true;
      
      // Also ensure status field matches
      if (doc.containsKey("status")) {
        doc["status"] = "Misplaced";
      }
      
      Serial.println("INFO: MIS status preserved from Firebase");
    }
    // ==================== END FIX ====================
    
    // Auto-registration logic
    String title = doc["title"] | "";
    String author = doc["author"] | "";
    String location = doc["location"] | "";
    
    bool hasRealTitle = isValidBookTitle(title);
    bool hasRealAuthor = isValidBookAuthor(author);
    bool hasRealLocation = isValidBookLocation(location);
    
    int realFieldCount = 0;
    if (hasRealTitle) realFieldCount++;
    if (hasRealAuthor) realFieldCount++;
    if (hasRealLocation) realFieldCount++;
    
    bool shouldBeRegistered = (realFieldCount >= 2);
    
    if (doc.containsKey("sensor")) {
      int sensor = doc["sensor"] | -1;
      if (sensor >= 0) {
        shouldBeRegistered = true;
      }
    }
    
    bool isEditedBook = (title != "New Book" && author != "Unknown" && location != "Unassigned");
    
    bool registrationUpdated = false;
    if ((shouldBeRegistered || isEditedBook) && !isRegistered) {
      doc["isRegistered"] = true;
      registrationUpdated = true;
      
      // Only set espStatus if not already set (and not MIS)
      if (!doc.containsKey("espStatus") || String(doc["espStatus"] | "").length() == 0) {
        if (doc.containsKey("status")) {
          String status = doc["status"] | "";
          if (status == "Available") doc["espStatus"] = "AVL";
          else if (status == "Borrowed") doc["espStatus"] = "BRW";
          else if (status == "Overdue") doc["espStatus"] = "OVD";
          else if (status == "Misplaced") doc["espStatus"] = "MIS";
          else doc["espStatus"] = "AVL";
        } else {
          doc["espStatus"] = "AVL";
        }
      }
      
      doc["lastUpdated"] = String(millis() / 1000);
    }
    
    if ((titleWasDefault || authorWasDefault || locationWasDefault) && 
        currentFirebaseData.length() > 0 && !isRegistered) {
      doc["isRegistered"] = true;
    }

    json = "";
    serializeJson(doc, json);
    
    if (json.length() < 10 || !json.startsWith("{") || !json.endsWith("}")) {
      Serial.println("ERROR:InvalidJSONAfterFix");
      return;
    }

    // Send to Firebase
    if (firebasePatch(path, json)) {
      if (registrationUpdated || titleWasDefault || authorWasDefault || locationWasDefault) {
        String statusMsg = "SUCCESS:" + path;
        if (registrationUpdated) statusMsg += "|REGISTERED";
        if (titleWasDefault) statusMsg += "|TITLE_KEPT";
        if (authorWasDefault) statusMsg += "|AUTHOR_KEPT";
        if (locationWasDefault) statusMsg += "|LOCATION_KEPT";
        Serial.println(statusMsg);
      } else {
        Serial.println("SUCCESS:" + path);
      }
    } else {
      Serial.println("ERROR:SendFailed");
    }
    return;
  }

  Serial.println("ERROR:UnknownCommand");
}
// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  delay(800);

  while (Serial.available()) Serial.read();

  Serial.println("ESP32 Ready - Library System with Registration Awareness");

  connectWiFi();

  // Create WiFi monitoring task on Core 0
  xTaskCreatePinnedToCore(
    wifiTask,
    "WiFiTask",
    4096,
    NULL,
    1,
    &wifiTaskHandle,
    0
  );
}

// ==================== LOOP (RUNS ON CORE 1) ====================
void loop() {
  static String buffer = "";

  while (Serial.available()) {
    char c = Serial.read();

    if (c == '\n') {
      if (buffer.length() > 0) processCommand(buffer);
      buffer = "";
    }
    else if (c != '\r' && buffer.length() < 8192) {
      buffer += c;
    }
  }

  delay(1);
}