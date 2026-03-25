#define BLYNK_TEMPLATE_ID   "TMPL54X1DIguQ"
#define BLYNK_TEMPLATE_NAME "rc car"
#define BLYNK_AUTH_TOKEN    "icFxZjNLX4Rh3KBywFLHB-D_zd53c4Q3"

#include <WiFi.h>
#include <HardwareSerial.h>
#include <WebServer.h>
#include <BlynkSimpleEsp32.h>

const char* ssid     = "embed";
const char* password = "weareincontrol";

WebServer server(80);

HardwareSerial GPS(1);
String gpsData  = "";
float latitude  = 0.0;
float longitude = 0.0;

#define AL_IN1 4
#define AL_IN2 5
#define AR_IN1 6
#define AR_IN2 7
#define VL_IN1 8
#define VL_IN2 9
#define VR_IN1 10
#define VR_IN2 11

#define LED1 20
#define LED2 21
#define LED3 22
#define LED4 23

// EXTRA LEDS
#define LED5 0
#define LED6 1
#define LED7 12
#define LED8 13

char auth[] = BLYNK_AUTH_TOKEN;

void stopAlles() {
  digitalWrite(AL_IN1, LOW); digitalWrite(AL_IN2, LOW);
  digitalWrite(AR_IN1, LOW); digitalWrite(AR_IN2, LOW);
  digitalWrite(VL_IN1, LOW); digitalWrite(VL_IN2, LOW);
  digitalWrite(VR_IN1, LOW); digitalWrite(VR_IN2, LOW);
}

void vooruit() {
  digitalWrite(AL_IN1, HIGH); digitalWrite(AL_IN2, LOW);
  digitalWrite(AR_IN1, HIGH); digitalWrite(AR_IN2, LOW);
  digitalWrite(VL_IN1, HIGH); digitalWrite(VL_IN2, LOW);
  digitalWrite(VR_IN1, HIGH); digitalWrite(VR_IN2, LOW);
}

void achteruit() {
  digitalWrite(AL_IN1, LOW); digitalWrite(AL_IN2, HIGH);
  digitalWrite(AR_IN1, LOW); digitalWrite(AR_IN2, HIGH);
  digitalWrite(VL_IN1, LOW); digitalWrite(VL_IN2, HIGH);
  digitalWrite(VR_IN1, LOW); digitalWrite(VR_IN2, HIGH);
}

void draaiRechts() {
  digitalWrite(AL_IN1, HIGH); digitalWrite(AL_IN2, LOW);
  digitalWrite(AR_IN1, LOW);  digitalWrite(AR_IN2, LOW);
  digitalWrite(VL_IN1, HIGH); digitalWrite(VL_IN2, LOW);
  digitalWrite(VR_IN1, LOW);  digitalWrite(VR_IN2, LOW);
}

void draaiLinks() {
  digitalWrite(AL_IN1, LOW);  digitalWrite(AL_IN2, LOW);
  digitalWrite(AR_IN1, HIGH); digitalWrite(AR_IN2, LOW);
  digitalWrite(VL_IN1, LOW);  digitalWrite(VL_IN2, LOW);
  digitalWrite(VR_IN1, HIGH); digitalWrite(VR_IN2, LOW);
}

BLYNK_WRITE(V0) { if (param.asInt()) vooruit();   else stopAlles(); }
BLYNK_WRITE(V1) { if (param.asInt()) achteruit(); else stopAlles(); }
BLYNK_WRITE(V2) { if (param.asInt()) draaiRechts(); else stopAlles(); }
BLYNK_WRITE(V3) { if (param.asInt()) draaiLinks();  else stopAlles(); }

String getField(String data, int index) {
  int found = 0;
  int start = 0;
  for (int i = 0; i <= (int)data.length(); i++) {
    if (i == (int)data.length() || data[i] == ',') {
      if (found == index) return data.substring(start, i);
      found++;
      start = i + 1;
    }
  }
  return "";
}

void parseGPS(String nmea) {
  nmea.trim();
  if (!nmea.startsWith("$GPRMC")) return;

  String status = getField(nmea, 2);
  if (status != "A") return;

  String latStr = getField(nmea, 3);
  String latDir = getField(nmea, 4);
  String lonStr = getField(nmea, 5);
  String lonDir = getField(nmea, 6);

  if (latStr.length() < 4 || lonStr.length() < 4) return;

  float rawLat = latStr.toFloat();
  float rawLon = lonStr.toFloat();

  float lat = (int)(rawLat / 100.0) + fmod(rawLat, 100.0) / 60.0;
  float lon = (int)(rawLon / 100.0) + fmod(rawLon, 100.0) / 60.0;

  if (latDir == "S") lat = -lat;
  if (lonDir == "W") lon = -lon;

  latitude  = lat;
  longitude = lon;
}

String htmlPage() {
  String page = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>RC Car GPS</title>
  <style>html, body { height: 100%; margin: 0; } #map { width: 100%; height: 100%; }</style>
  <link rel="stylesheet" href="https://unpkg.com/leaflet/dist/leaflet.css" />
  <script src="https://unpkg.com/leaflet/dist/leaflet.js"></script>
</head>
<body>
<div id="map"></div>
<script>
  var map = L.map('map').setView([51.05, 4.35], 16);
  L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', { maxZoom: 19 }).addTo(map);
  var marker = L.marker([51.05, 4.35]).addTo(map);
  var first = false;

  function updateGPS() {
    fetch('/gps').then(r => r.json()).then(data => {
      marker.setLatLng([data.lat, data.lon]);
      if (!first) { map.setView([data.lat, data.lon], 18); first = true; }
      else map.setView([data.lat, data.lon]);
    });
  }

  setInterval(updateGPS, 1000);
</script>
</body>
</html>
)rawliteral";
  return page;
}

void handleGPS() {
  String json = "{\"lat\":" + String(latitude, 6) + ",\"lon\":" + String(longitude, 6) + "}";
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);

  int motorPins[] = {AL_IN1, AL_IN2, AR_IN1, AR_IN2, VL_IN1, VL_IN2, VR_IN1, VR_IN2};
  for (int p : motorPins) pinMode(p, OUTPUT);

  // LEDS
  pinMode(LED1, OUTPUT); pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT); pinMode(LED4, OUTPUT);
  pinMode(LED5, OUTPUT); pinMode(LED6, OUTPUT);
  pinMode(LED7, OUTPUT); pinMode(LED8, OUTPUT);

  digitalWrite(LED1, HIGH); digitalWrite(LED2, HIGH);
  digitalWrite(LED3, HIGH); digitalWrite(LED4, HIGH);
  digitalWrite(LED5, HIGH); digitalWrite(LED6, HIGH);
  digitalWrite(LED7, HIGH); digitalWrite(LED8, HIGH);

  GPS.begin(9600, SERIAL_8N1, 2, 3);

  Serial.print("Verbinden met WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nVerbonden!");
  Serial.print("IP-adres: ");
  Serial.println(WiFi.localIP());

  server.on("/", []() { server.send(200, "text/html", htmlPage()); });
  server.on("/gps", handleGPS);
  server.begin();

  Blynk.config(auth);
  Blynk.connect(3000);
}

void loop() {
  server.handleClient();

  if (!Blynk.connected()) {
    Blynk.connect(100);
  }
  Blynk.run();

  while (GPS.available()) {
    char c = GPS.read();
    gpsData += c;
    if (c == '\n') {
      parseGPS(gpsData);
      gpsData = "";
    }
  }

  if (Serial.available()) {
    if (Serial.read() == 'i') {
      Serial.print("Huidig IP-adres: ");
      Serial.println(WiFi.localIP());
    }
  }
}
