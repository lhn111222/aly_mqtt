/*************************************/
/*Mission14：NodeMCU+DHT11+LED接入aliyunIoT

/*************************************/
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
//#include <Ticker.h>
#include <Adafruit_SSD1306.h> 
#include <DHT.h>
#include <AliyunIoTSDK.h>
 
// 设置wifi接入信息(请根据您的WiFi信息进行修改)
const char* WiFi_SSID = "Redmi K70 Pro";
const char* WiFi_PASSWD = "20010922";
const char* mqttServer = "k1own7VPzJY.iot-06z00e9cmifyqci.mqtt.iothub.aliyuncs.com";  //输入你所创建设备的MQTT连接参数mqttHostUrl
 
//Ticker ticker;
WiFiClient wifiClient;  
PubSubClient mqttClient(wifiClient);
 
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
#define DHTPIN D3     //DHT 输出引脚接至D3
#define LedGreenPIN D5 // 空调制冷输出引脚
#define LedRedPIN D6 //空调制热输出引脚
#define DHTTYPE  DHT11     // 定义传感器类型为DHT11 
DHT dht(DHTPIN, DHTTYPE);
float t = 0.0;//初始化温度变量
int h = 0;//初始化湿度变量
//int count;    // Ticker计数用变量

unsigned long lastMsMain = 0;
// 设置产品和设备的信息，从阿里云设备信息里查看
#define PRODUCT_KEY "k1own7VPzJY"
#define DEVICE_NAME "THcontrollor"
#define DEVICE_SECRET "d987ca29050f46d67083ad10acef62e2"
#define REGION_ID "cn-shanghai"//该参数不变

WiFiServer server(80);
// Variable to store the HTTP request
String header;

// Auxiliar variables to store the current output state
String LedGreenState = "off";
String LedRedState = "off";

unsigned long currentTime = millis();// Current time
unsigned long previousTime = 0; // Previous time
const long timeoutTime = 2000;// Define timeout time in milliseconds (example: 2000ms = 2s)

void setup() {
  Serial.begin(115200);
  pinMode(DHTPIN,INPUT);
  pinMode(LedRedPIN,OUTPUT);
  pinMode(LedGreenPIN,OUTPUT);
  digitalWrite(LedRedPIN, LOW);
  digitalWrite(LedGreenPIN, LOW);   
 //初始化OLED    
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); }
  delay(2000);//等待OLED初始化完成
  display.clearDisplay();//清屏
  display.setTextColor(WHITE);   
  wifiInit();// 连接WiFi
  mqttClient.setServer(mqttServer, 1883); // 设置MQTT服务器和端口号
  connectMQTTServer();// 连接MQTT服务器
  dht.begin();  
  server.begin();    
  
//初始化 iot，需传入 wifi 的 client，和设备产品信息
  AliyunIoTSDK::begin(wifiClient, PRODUCT_KEY, DEVICE_NAME, DEVICE_SECRET, REGION_ID);
  // 绑定一个设备属性回调，当远程修改此属性，会触发 AircleanerCallback
  AliyunIoTSDK::bindData("AC", ACcallback);
  AliyunIoTSDK::send("AC", 0);
  Serial.println("mission17 Start...");  
  //ticker.attach(1, tickerCount); // Ticker定时对象
  }  
 
 void loop() { 
  AliyunIoTSDK::loop(); 
  h = dht.readHumidity();
  t = dht.readTemperature(); 
  if (isnan(h) || isnan(t)) {
  Serial.println("Failed to read from DHT sensor!"); }
  Serial.print("temp: "); Serial.print(t); Serial.println("°C");
  Serial.print("humi: "); Serial.print(h);  Serial.println("%"); 
  Serial.print("Aircleaner:"); //串口显示空气净化器状态，0表示开，1表示关                           
  Serial.print("AC-cool"); //串口显示空调制冷状态，0表示开，1表示关                           
  Serial.println(digitalRead(LedGreenPIN)); 
  Serial.print("AC-heat"); //串口显示空调制热状态，0表示开，1表示关 
  Serial.println(digitalRead(LedRedPIN));  
  display.clearDisplay(); // 清屏
// display temp
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("temp: ");
  display.setTextSize(2);
  display.setCursor(0,10);
  display.print(t);
  display.print(" ");
  display.setTextSize(1);
  display.cp437(true);
  display.write(167);
  display.setTextSize(2);
  display.print("C");
  // display humi
  display.setTextSize(1);
  display.setCursor(0, 35);
  display.print("humi: ");
  display.setTextSize(2);
  display.setCursor(0, 45);
  display.print(h);
  display.print(" %"); 
  display.display(); 
  
  //if (mqttClient.connected()) { // 如果NodeMCU成功连接mqtt服务器
  //  if (count >= 10){// 每隔10秒钟发布一次信息
      AliyunIoTSDK::send("temp",t);//发送当前温度值到云平台
      AliyunIoTSDK::send("humi",h);//发送当前湿度值到云平台
      AliyunIoTSDK::send("AC",AC_State(digitalRead(LedGreenPIN),digitalRead(LedRedPIN)));//发送当前空气净化器状态到云平台
  //     count = 0; }    
 //     mqttClient.loop();} // 保持心跳
 //  else {                  // 如果NodeMCU未能成功连接mqtt服务器
 //   connectMQTTServer(); }   
   WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    currentTime = millis();
    previousTime = currentTime;
    while (client.connected() && currentTime - previousTime <= timeoutTime) { // loop while the client's connected
      currentTime = millis();         
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
           if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // turns the GPIOs on and off
            if (header.indexOf("GET /5/on") >= 0) {
              Serial.println("LedGreen on");
              LedGreenState = "on";
              digitalWrite(LedGreenPIN, HIGH);
            } else if (header.indexOf("GET /5/off") >= 0) {
              Serial.println("LedGreen off");
              LedGreenState = "off";
              digitalWrite(LedGreenPIN, LOW);
            } else if (header.indexOf("GET /4/on") >= 0) {
              Serial.println("LedRed on");
              LedRedState = "on";
              digitalWrite(LedRedPIN, HIGH);
            } else if (header.indexOf("GET /4/off") >= 0) {
              Serial.println("LedRed off");
              LedRedState = "off";
              digitalWrite(LedRedPIN, LOW);
            }
            
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #195B6A; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #77878A;}</style></head>");
            client.println("<body><h1>Smart Home Controller</h1>");// Web Page Heading
            // Display current state, and ON/OFF buttons for LedGreenPIN  
            client.println("<p>AC cooling- state " + LedGreenState + "</p>");
             if (LedGreenState=="off") {
              client.println("<p><a href=\"/5/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/5/off\"><button class=\"button button2\">OFF</button></a></p>");
            } 
             // Display current state, and ON/OFF buttons for LedRedPIN 
            client.println("<p>AC heating - State " + LedRedState + "</p>");
             if (LedRedState=="off") {
              client.println("<p><a href=\"/4/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/4/off\"><button class=\"button button2\">OFF</button></a></p>");
            }
            client.print("<h2>temp: ");
            client.print(t); 
            client.print("</h2><br>\r\n");
            client.print("<h2>humi: ");
            client.print(h);
            client.print("</h2><br>\r\n");
            client.println("</body></html>");
            client.println();// The HTTP response ends with another blank line
             break;// Break out of the while loop
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    header = ""; // Clear the header variable
    client.stop();// Close the connection
    Serial.println("Client disconnected.");
    Serial.println("");
  }
 delay(1000) ;
}  

void connectMQTTServer(){  //连接mqtt服务器
  //根据设备信息中的MQTT连接参数，替换下面字符串内容
  String clientId = "k1own7VPzJY.THcontrollor|securemode=2,signmethod=hmacsha256,timestamp=1725597761426|";   
  String user = "THcontrollor&k1own7VPzJY";
  String password = "d9d8027b8234713482b2e16d497fcb335964100f5173f6701b006c7cd1612872";
 
  if (mqttClient.connect(clientId.c_str(),user.c_str(),password.c_str())) { 
    Serial.println("MQTT Server Connected.");
    Serial.println("Server Address: ");
    Serial.println(mqttServer);
    Serial.println("ClientId:");
    Serial.println(clientId); } 
    else {
    Serial.print("MQTT Server Connect Failed. Client State:");
    Serial.println(mqttClient.state());
    delay(3000); }   
}

// NodeMCU连接wifi
void wifiInit(){
    WiFi.mode(WIFI_STA); //设置ESP8266工作模式为无线终端模式
    WiFi.begin(WiFi_SSID, WiFi_PASSWD);   
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("WiFi not Connect"); }
    Serial.print("NodeMCU Connected to ");
    Serial.println(WiFi.SSID());  
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP()); }

void ACcallback(JsonVariant P){
   int AC= P["AC"];
    if  (AC== 1) {
      digitalWrite(LedGreenPIN, HIGH);digitalWrite(LedRedPIN, LOW);}//启动空调制冷
    else if  (AC == 2) {
    digitalWrite(LedRedPIN, HIGH);digitalWrite(LedGreenPIN, LOW);}//启动空调制热
    else{
      digitalWrite(LedRedPIN, LOW);digitalWrite(LedGreenPIN, LOW); }// 关闭空调
}

bool AP_State(bool s){
  if(s == HIGH)
   { return 1;}//Led状态为HIGH表示开机，返回1
  else{
    return 0;}//Led状态为LOW表示关机，返回0
}

int AC_State(bool s1,bool s2){
  if(s1 == HIGH)
   { return 1;}//Led状态为HIGH表示制冷，返回1
  else if
    (s2 == HIGH){return 2;}//Led状态为HIGH表示制热，返回2
    else {return 0;}
}
