#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <vector>  // To store discovered devices

// Replace with your network credentials
//const char* ssid = "FeldlercheBurg";
//const char* password = "ThisIsMyPassword";
const char* ssid = "PTI";
const char* password = "bolcseszmernok1";

// Web server on port 80
WebServer server(80);

// Structure to hold discovered device information
struct Device {
  String hostname;
  IPAddress ip;
};
std::vector<Device> discoveredDevices;  // Vector to store discovered devices

// Generate HTML content for displaying discovered devices
String generateHTML() {
  String html = "<html><head>";
  html += "<title>ESP32 Network Devices</title>";
  html += "<script src=\"https://ajax.googleapis.com/ajax/libs/jquery/3.7.1/jquery.min.js\"></script>";
  html += "<script>";
  html += "function refreshDevices() {";
  html += "  $.get('/devices', function(data) { $('#deviceList').html(data); });";
  html += "}";
  html += "setInterval(refreshDevices, 10000);";  // Refresh device list every 10 seconds
  html += "</script>";
  html += "</head><body>";
  html += "<h1>Discovered ESP32 Devices on Network</h1>";
  html += "<ul id='deviceList'>";

  // List discovered devices
  for (const auto& device : discoveredDevices) {
    html += "<li><a href=\"http://" + device.hostname + ".local/data\" target=\"_blank\">Hostname: " + device.hostname + ".local</a></li>";
  }

  html += "</ul>";
  html += "<button onclick='refreshDevices()'>Refresh Now</button>";
  html += "</body></html>";
  return html;
}

// Handler to serve the main page
void handleRoot() {
  Serial.println("\nHandling request: ROOT");
  server.send(200, "text/html", generateHTML());
}

// Handler to serve device list as HTML for AJAX requests
void handleDeviceList() {
  Serial.println("\nHandling request: /devices");
  String deviceListHTML;
  for (const auto& device : discoveredDevices) {
    deviceListHTML += "<li><a href=\"http://" + device.hostname + ".local/data\" target=\"_blank\">Hostname: " + device.hostname + ".local</a></li>";
  }
  server.send(200, "text/html", deviceListHTML);
}

// Discover ESP32 devices on the network using mDNS
void discoverESP32Devices() {
  int n = MDNS.queryService("microservice", "tcp");  // Query for "http" service

  discoveredDevices.clear();  // Clear previous discovery results
  Serial.printf("Found %d services:\n", n);
  for (int i = 0; i < n; i++) {
    Device device;
    device.hostname = MDNS.hostname(i);
    //device.ip = MDNS.IP(i);  // Retrieve IP address of the discovered device
    discoveredDevices.push_back(device);
    Serial.println("Discovered device:");
    Serial.println(" - Hostname: " + device.hostname);
    //Serial.println(" - IP: " + device.ip.toString());
  }
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

  // Start mDNS responder with a unique hostname for the discoverer
  if (!MDNS.begin("esp32-main")) {  // The hostname for this main ESP32
    Serial.println("Error setting up mDNS responder!");
    return;
  }
  Serial.println("mDNS responder started");

  // Advertise the HTTP service on port 80
  MDNS.addService("http", "tcp", 80);

  // Optional: Add a secondary "discoverer" service type
  MDNS.addService("discoverer", "tcp", 80);

  // Set up web server routes
  server.on("/", handleRoot);
  server.on("/devices", handleDeviceList);  // Route for AJAX to fetch device list
  server.begin();

  Serial.println("HTTP server and mDNS discovery service started");
}

void loop() {
  server.handleClient();

  // Perform mDNS discovery every 10 seconds
  static unsigned long lastDiscoveryTime = 0;
  if (millis() - lastDiscoveryTime > 10000) {
    discoverESP32Devices();
    lastDiscoveryTime = millis();
  }
}
