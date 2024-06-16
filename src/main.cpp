#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// Update these with values suitable for your network.
const char *ssid = "xxx";  // ชื่อ WiFi
const char *password = "x";         // รหัสผ่าน WiFi
const char *mqtt_server = "xxx.xxx.xxx.xxx"; // IP ของเซิร์ฟเวอร์ MQTT
const char *username_device = "xxx";  // ชื่อผู้ใช้สำหรับอุปกรณ์ MQTT
const char *password_device = "xxxx";  // รหัสผ่านสำหรับอุปกรณ์ MQTT
const char *led = NULL;
WiFiClient espClient;           // สร้าง client สำหรับการเชื่อมต่อ WiFi
PubSubClient client(espClient); // สร้าง client สำหรับการเชื่อมต่อ MQTT
JsonDocument doc;               // สร้างเอกสาร JSON สำหรับการจัดการ JSON
StaticJsonDocument<256> docCallBack;
unsigned long lastMsg = 0;      // เก็บเวลาของข้อความสุดท้ายที่ส่ง
#define MSG_BUFFER_SIZE (50)    // ขนาดของบัฟเฟอร์ข้อความ
char msg[MSG_BUFFER_SIZE];      // บัฟเฟอร์สำหรับเก็บข้อความที่จะส่ง
char buffer[256];               // บัฟเฟอร์สำหรับเก็บข้อมูล JSON

void setup_wifi()
{
  delay(10); // หน่วงเวลา 10 มิลลิวินาที
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);        // ตั้งค่าโหมด WiFi เป็นสถานี (STA)
  WiFi.begin(ssid, password); // เริ่มการเชื่อมต่อ WiFi

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500); // หน่วงเวลา 500 มิลลิวินาที
    Serial.print(".");
  }

  randomSeed(micros()); // สร้าง seed สำหรับ random number generator

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP()); // แสดง IP address ของอุปกรณ์
}

void callback(char *topic, byte *payload, unsigned int length)
{
  deserializeJson(docCallBack, payload, length);
  led = docCallBack["led"];

  Serial.print("Message arrived [");
  Serial.print(topic); // แสดงหัวข้อ (topic) ของข้อความที่ได้รับ
  Serial.print("] ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]); // แสดงเนื้อหาของข้อความที่ได้รับ
  }
  Serial.println();

  // เปิดหรือปิด LED ขึ้นอยู่กับค่าของข้อความที่ได้รับ
  if (led == "0")
  {
    Serial.println("LED OFF");
    digitalWrite(BUILTIN_LED, LOW); // เปิด LED
  }
  else
  {
    Serial.println("LED ON");
    digitalWrite(BUILTIN_LED, HIGH); // ปิด LED
  }
}

void reconnect()
{
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX); // สร้าง client ID แบบสุ่ม
    if (client.connect(clientId.c_str(), username_device, password_device))
    {
      doc["hello"] = "World!";    // สร้างข้อมูล JSON
      serializeJson(doc, buffer); // แปลงข้อมูล JSON เป็นสตริงและเก็บใน buffer
      Serial.println("connected");
      client.publish("xxx", buffer); // ส่งข้อความไปยัง MQTT
      client.subscribe("xxx");     // สมัครรับข้อความจากหัวข้อ (topic) นี้
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state()); // แสดงสถานะการเชื่อมต่อที่ล้มเหลว
      Serial.println(" try again in 5 seconds");
      delay(5000); // รอ 5 วินาทีก่อนลองเชื่อมต่อใหม่
    }
  }
}

void setup()
{
  pinMode(BUILTIN_LED, OUTPUT);        // กำหนดให้ PIN ของ LED เป็น OUTPUT
  Serial.begin(115200);                // เริ่มการสื่อสาร Serial ที่ baud rate 115200
  setup_wifi();                        // เรียกฟังก์ชันเพื่อเชื่อมต่อ WiFi
  client.setServer(mqtt_server, 1883); // ตั้งค่าเซิร์ฟเวอร์ MQTT
  client.setCallback(callback);        // ตั้งค่าฟังก์ชัน callback สำหรับจัดการข้อความที่ได้รับจาก MQTT
}

void loop()
{
  if (!client.connected())
  {
    reconnect(); // ถ้าไม่ได้เชื่อมต่อกับ MQTT ให้เรียกฟังก์ชัน reconnect เพื่อเชื่อมต่อใหม่
  }
  client.loop(); // รักษาการเชื่อมต่อกับ MQTT

  unsigned long now = millis();
  if (now - lastMsg > 2000) // ถ้าผ่านไป 2 วินาทีแล้วตั้งแต่ส่งข้อความล่าสุด
  {
    lastMsg = now;
    long r = random(23, 48);                // สร้างตัวเลขสุ่มระหว่าง 23 ถึง 48
    doc["hello"] = r;                       // ใส่ตัวเลขสุ่มใน JSON
    serializeJson(doc, buffer);             // แปลง JSON เป็นสตริงและเก็บใน buffer
    snprintf(msg, MSG_BUFFER_SIZE, buffer); // ใส่ข้อความลงในบัฟเฟอร์ msg
    Serial.print("Publish message: ");
    Serial.println(msg);                                              // แสดงข้อความที่จะส่ง
    client.publish("xxx", msg); // ส่งข้อความไปยัง MQTT
  }
}
