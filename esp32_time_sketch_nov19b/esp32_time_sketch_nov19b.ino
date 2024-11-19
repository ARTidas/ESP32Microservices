#include <WiFi.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <WebServer.h>

// Replace with your network credentials
const char* ssid = "PTI";
const char* password = "bolcseszmernok1";

// Configuration for the microservice
const char* serviceName = "esp32-time";     // Unique hostname
const char* serviceType = "microservice";   // Custom mDNS service type
const char* apiUrl = "http://worldtimeapi.org/api/timezone/Europe/Budapest";

// Web server on port 80
WebServer server(80);

// Variable to store fetched API data
String apiData;

// Function to fetch data from the API
void fetchAPIData() {
  HTTPClient http;
  http.begin(apiUrl);

  int httpCode = http.GET();
  if (httpCode == 200) {  // Success
    apiData = http.getString();
    Serial.println("API Data fetched successfully:");
    Serial.println(apiData);
  } else {
    Serial.printf("Failed to fetch API data, error code: %d\n", httpCode);
    apiData = "Error fetching API data";
  }

  http.end();
}

// Handler for HTTP GET requests to "/data"
void handleDataRequest() {
  server.send(200, "application/json", apiData);
}

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Start mDNS responder with a unique hostname
  if (!MDNS.begin(serviceName)) {  // e.g., esp32-microservice.local
    Serial.println("Error setting up mDNS responder!");
    return;
  }
  Serial.println("mDNS responder started");

  // Advertise the microservice on port 80
  MDNS.addService(serviceType, "tcp", 80);

  // Fetch initial API data
  fetchAPIData();

  // Set up the API endpoint
  server.on("/data", handleDataRequest);
  server.begin();

  Serial.printf("%s service started: http://%s.local/data\n", serviceName, serviceName);
}

void loop() {
  server.handleClient();

  // Periodically fetch API data every 5 minutes
  static unsigned long lastFetchTime = 0;
  if (millis() - lastFetchTime > 300000) {  // 5 minutes in milliseconds
    fetchAPIData();
    lastFetchTime = millis();
  }
}
