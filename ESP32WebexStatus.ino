// #include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>


// Pin definitions for LEDs
const int greenLedPin = 12;
const int redLedPin = 13;
const int orangeLedPin = 14;

//flag counters
unsigned long oldTime = 0;
unsigned long newTime = 0;

// WiFi credentials
const char* ssid = "ENTER_YOUR_SSID";
const char* password = "ENTER_YOUR_PASSWORD";

// Webex API endpoint and bearer token
const String apiUrl = "https://webexapis.com/v1/people/***Enter your API URL***";
const String bearerToken = "Bearer ***Enter your Bearer Token***";


// Server to serve the webpage
WiFiServer server(80);

String webexStatus = "unknown";  // Default webexStatus
String status = ""; //default status
String displayName = ""; //Default displayName 
String avatarUrl = ""; //default avatar


void setup() {
  Serial.begin(115200);


    // Initialize the LED pins as outputs
  pinMode(greenLedPin, OUTPUT);
  pinMode(redLedPin, OUTPUT);
  pinMode(orangeLedPin, OUTPUT);

  digitalWrite(greenLedPin, HIGH);
  digitalWrite(redLedPin, HIGH);
  digitalWrite(orangeLedPin, HIGH);

  delay(500);

  digitalWrite(greenLedPin, LOW);
  digitalWrite(redLedPin, LOW);
  digitalWrite(orangeLedPin, LOW);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Start the server
  server.begin();
  Serial.println("Server started");

  // Show the IP address
  Serial.print("ESP32 IP address: ");
  Serial.println(WiFi.localIP());
  digitalWrite(greenLedPin, HIGH);
  delay(500);
  digitalWrite(greenLedPin, LOW);
}

void loop() {
  // Handle any incoming clients
  WiFiClient client = server.available();
    // Make API request to get Webex status
  webexStatus = getWebexStatus();
    
  updateLEDs(webexStatus);

    // Serve the webpage
  serveWebPage(client);

  delay(4000);  // Refresh every 4 seconds
}

// Function to get Webex status via API
String getWebexStatus() {

  HTTPClient http;
  http.begin(apiUrl);
  http.addHeader("Authorization", bearerToken);


  int httpResponseCode = http.GET();
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println(response);

    // Parse JSON
    DynamicJsonDocument doc(1024); 
    deserializeJson(doc, response);

    status = doc["status"].as<String>();  // Extract "status"
    displayName = doc["displayName"].as<String>(); // Extract "displayName"
    avatarUrl = doc["avatar"].as<String>();  // Extract "avatar" URL
    return String(status);
  } else {
    Serial.print("Error on HTTP request: ");
    Serial.println(httpResponseCode);
    return "unknown";  // Default status in case of error
  }

  http.end();

}

// Function to serve the webpage based on the Webex status
void serveWebPage(WiFiClient& client) {


  // Wait for the client to send data
  while (client.connected() && !client.available()) {
    delay(1);
  }

  // Read the HTTP request
  String request = client.readStringUntil('\r');
  Serial.println(request);
  client.flush();

  // Determine the color based on Webex status
  String color = "orange";  // Default for "unknown" or other statuses
  if (webexStatus == "active") {
    color = "green";
  } else if (webexStatus == "presenting" || webexStatus == "DoNotDisturb") {
    color = "red";
  } else if (webexStatus == "call" || webexStatus == "meeting" || webexStatus == "unknown") {
    color = "orange";
  }

  // Send the response (HTML webpage)
  String html = "<!DOCTYPE html><html><head><meta http-equiv='refresh' content='10'>";
  html += "<title>Webex Status</title></head><body>";
  html += "<h1 style='color:" + color + ";'>" + displayName +" status: " + webexStatus + "</h1>";
  html += "<img src='" + avatarUrl + "' alt='Avatar' width='100' height='100'>";
  html += "</body></html>";

  // Send the response headers and content
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println("Connection: close");
  client.println();
  client.println(html);
  client.println();


}

// Function to control the LEDs based on Webex status
void updateLEDs(String status) {
  // Turn off all LEDs initially
  digitalWrite(greenLedPin, LOW);
  digitalWrite(redLedPin, LOW);
  digitalWrite(orangeLedPin, LOW);

  if (status == "active") {
    digitalWrite(greenLedPin, HIGH);  // Green for Available
    Serial.println("Green LED");
  } else if (webexStatus == "Meeting" || webexStatus == "unknown") {
    digitalWrite(orangeLedPin, HIGH); // Orange for In a Meeting
    Serial.println("Orange LED");
  } else if (webexStatus == "Presenting" || webexStatus == "DoNotDisturb") {
    digitalWrite(redLedPin, HIGH);    // Red for Do Not Disturb
    Serial.println("Red LED");
  }
}
