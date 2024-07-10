#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <SoftwareSerial.h>
#include <WiFi.h> // Include the WiFi library

const byte ROWS = 4;
const byte COLS = 3;
char keys[ROWS][COLS] = {
  { '1', '2', '3' },
  { '4', '5', '6' },
  { '7', '8', '9' },
  { '*', '0', '#' }
};
byte rowPins[ROWS] = { 18, 0, 4, 17 };  // connect to the row pinouts of the keypad
byte colPins[COLS] = { 5, 19, 16 };     // connect to the column pinouts of the keypad

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
LiquidCrystal_I2C lcd(0x27, 20, 4);

String phoneNumber = "";
String savedPhoneNumber[10] = {};
String registeredPhoneNumber[10] = {};
int count = 0;

bool isRegistered = false;
bool toRegister = false;

const char* ssid = "GardenWhispers";     // Replace with your WiFi network SSID
const char* password = "blink2023?"; // Replace with your WiFi network password
const char* serverAddress = "127.0.0.1"; // Replace with your WampServer IP address

WiFiClient client;

void setup() {
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Enter Phone No:");
  Serial.begin(9600);
  Serial.println("Enter Phone No:\n");

  // Initialize WiFi connection
  connectWiFi();

  // Initialize keypad and SIM800L (if you're using it for SMS or other purposes)
  // InitializeSIM800L(); // Uncomment and implement if needed
}

void loop() {
  char key = keypad.getKey();
  if (key) {
    capturePhoneNo(key);
  }

  // Example HTTP GET request (triggered by a condition or periodically)
  if (toRegister) {
    sendToWampServer();
    toRegister = false; // Reset flag after making request
  }
}

void capturePhoneNo(char key) {
  if (key >= '0' && key <= '9' && phoneNumber.length() < 10) {
    phoneNumber += key;
    displayPhoneNumber();
  } else if (key == '*') {
    deleteDigit();
  } else if (key == '#') {
    if (phoneNumber.length() == 10) {
      saveNumber();
      verifyRegistration();
    } else {
      lcd.setCursor(0, 2);
      lcd.print("Invalid number");
      Serial.print("Invalid number\n");
      delay(2000);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Enter Phone No:");
      Serial.println("Enter Phone No:\n");
      phoneNumber = "";
    }
  }
}

void displayPhoneNumber() {
  lcd.setCursor(0, 1);
  lcd.print(phoneNumber);
  Serial.println(phoneNumber);
  lcd.print("          ");
}

void deleteDigit() {
  if (phoneNumber.length() > 0) {
    phoneNumber.remove(phoneNumber.length() - 1);
    displayPhoneNumber();
  }
}

void saveNumber() {
  lcd.setCursor(0, 2);
  lcd.print("Phone No saved");
  Serial.println("Phone No saved\n");
  delay(2000); 
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter Phone No:");
  Serial.print("Enter Phone No:\n");
  savedPhoneNumber[count] = phoneNumber;
  count++;
  Serial.println("Count: " + String(count));
}

void verifyRegistration() {
  Serial.println(phoneNumber);
  for (int i = 0; i <= 9; i++) { 
    if (phoneNumber == registeredPhoneNumber[i]) {
      isRegistered = true;
      break;
    } else {
      isRegistered = false;
    }
  }
  phoneNumber = "";
  if (isRegistered) {
    awardLoyaltyPoints();
    processTransaction();
  } else {
    promptRegistration();
    if (!toRegister) {
      processTransaction();
    } else {
      registerCustomer();
      processTransaction();
    }
  }
}

void awardLoyaltyPoints() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Awarding Points");
  Serial.print("Awarding Points\n");
  delay(2000); 
}

void processTransaction() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Processing...");
  Serial.print("Processing...\n");
  delay(3000);  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter Phone No:");
  Serial.print("Enter Phone No:\n");
}

void registerCustomer() {
  registeredPhoneNumber[count] = savedPhoneNumber[count - 1];
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Registration done");
  Serial.print("Registration done\n");
  delay(2000);

  // Display the registered phone number
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Registered Number:");
  lcd.setCursor(0, 1);
  lcd.print(registeredPhoneNumber[count]);
  Serial.print("Registered Number: ");
  Serial.println(registeredPhoneNumber[count]);
  delay(2000);

  // Send data to WampServer MySQL
  sendToWampServer();
}

void promptRegistration() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Not registered");
  Serial.print("Not registered\n");
  delay(2000);
  lcd.clear();
  lcd.print("1: Register");
  Serial.print("1: Register\n");
  lcd.setCursor(0, 1);
  lcd.print("0: Skip");
  Serial.print("0: Skip\n");

  while (true) {
    char key = keypad.getKey();
    if (key == '1') {
      toRegister = true;
      break;
    } else if (key == '0') {
      toRegister = false;
      break;
    }
  }
}

void connectWiFi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting...");
  }
  
  Serial.println("WiFi Connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void sendToWampServer() {
  // Prepare your HTTP request to WampServer MySQL
  String url = "/insert_numbers.php"; // Replace with your PHP script path on WampServer
  String postData = "registeredNumbers=";
  
  // Concatenate all registered phone numbers into postData
  for (int i = 0; i < count; i++) {
    postData += registeredPhoneNumber[i];
    if (i < count - 1) {
      postData += ",";
    }
  }

  // Send HTTP POST request to WampServer MySQL
  Serial.print("Sending data to server...\n");
  Serial.print("Data: ");
  Serial.println(postData);
  
  if (client.connect(serverAddress, 80)) {
    client.println("POST " + url + " HTTP/1.1");
    client.println("Host: " + String(serverAddress));
    client.println("Connection: close");
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.print("Content-Length: ");
    client.println(postData.length());
    client.println();
    client.print(postData);
    delay(500); // Allow time for response

    // Read server response
    while (client.available()) {
      String responseLine = client.readStringUntil('\r');
      Serial.print(responseLine);
    }
    client.stop();
    Serial.println("\nData sent to server.");
  } else {
    Serial.println("Failed to connect to server.");
  }
}
