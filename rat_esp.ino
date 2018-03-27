#include <FS.h>                   //this needs to be first, or it all crashes and burns...

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

#define DEBUG 1
#if (DEBUG==1)
  #define PRINTDEBUGLN(STR) Serial.println(STR)
  #define PRINTDEBUG(STR) Serial.print(STR)
#else
  #define PRINTDEBUGLN(STR) /*NOTHING*/
  #define PRINTDEBUG(STR) /*NOTHING*/
#endif


//#define esp_wake  D4           //Reset and EN
#define esp_motion  D5         // Motion detected 
#define esp_config  D6         // Config button
#define esp_update  D7         // Update button
#define esp_ack  D8            // Done operation and ready to sleep
#define esp_ok D9

#define API "/lock.php?place="
char update_server[40] = "192.168.1.50";
char server[33] = "192.168.1.50";
char server_port[6] = "80";
char PLACE[20] = "Nest1";

//default custom static IP
char static_ip[16] = "192.168.1.10";
char static_gw[16] = "192.168.1.1";
char static_sn[16] = "255.255.255.0";
//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  PRINTDEBUGLN("Should save config");
  shouldSaveConfig = true;
}

bool load_wifi(){
  if (SPIFFS.begin()) {
    PRINTDEBUGLN("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      PRINTDEBUGLN("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        PRINTDEBUGLN("opened config file");
        size_t size = configFile.size();
        if (size > 1024) {
          PRINTDEBUGLN("Config file size is too large");
          return false;
        }
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        #if (DEBUG==1)  
          json.printTo(Serial);
        #endif
        if (json.success()) {
          PRINTDEBUGLN("\nparsed json");
          if(json["update_server"]) 
            strcpy(update_server, json["update_server"]);
          if(json["server"]) 
            strcpy(server, json["server"]);
          if(json["server_port"]) 
            strcpy(server_port, json["server_port"]);

          if(json["ip"]) {
            PRINTDEBUGLN("setting custom ip from config");
            strcpy(static_ip, json["ip"]);
            strcpy(static_gw, json["gateway"]);
            strcpy(static_sn, json["subnet"]);
            PRINTDEBUGLN(static_ip);
/*            PRINTDEBUGLN("converting ip");
            IPAddress ip = ipFromCharArray(static_ip);
            PRINTDEBUGLN(ip);*/
          } else {
            PRINTDEBUGLN("no custom ip in config");
            return FALSE;
          }
        } else {
          PRINTDEBUGLN("failed to load json config");
          return FALSE;
        }
      }
    }
  } else {
    PRINTDEBUGLN("failed to mount FS");
    return FALSE;
  }
  //end read
  PRINTDEBUGLN(static_ip);
  PRINTDEBUGLN(blynk_token);
  PRINTDEBUGLN(mqtt_server);
  return TRUE;
}

/*************************************************************************************
 *  Pinging the server
 *************************************************************************************/
//Pingging main
bool ping (String msg="alarm"){
   // Use WiFiClientSecure class to create TLS connection
  WiFiClient client;
  PRINTDEBUGLN("Connecting to:");
  PRINTDEBUGLN(host);
  if(!client.connect(host, httpsPort))
  {
    PRINTDEBUGLN("Connection failed");
    return false;
  }
  // URL request

  String url = String(API) + String(PLACE) + String("&msg=");
  url += msg;
  url += String("&v=") + ESP.getVcc();
  PRINTDEBUGLN("Requesting URL:");
  PRINTDEBUGLN(url);
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: ratnest\r\n" +
               "Connection: close\r\n\r\n");
  PRINTDEBUGLN("Request sent");
  String line = client.readStringUntil('\n');
  PRINTDEBUGLN(line);

  return true;
}

/*************************************************************************************
 *  ////////////////////SETUP ////////////////
 *************************************************************************************/
void setup() {
  // put your setup code here, to run once:
  pinMode(esp_wake,OUTPUT);
  digitalWrite(esp_wake, LOW);
  pinMode(esp_motion, INPUT);

  #if (DEBUG==1)
    Serial.begin(115200);     // Initialize serial communications
    delay(100);
    Serial.setTimeout(2000);
    Serial.setDebugOutput(true);
    Serial.println();
    Serial.println("Author:: Mahmoud Shokry");
  #endif
  /*
  bool config = load_wifi();
  
  if(not config){
  WiFiManagerParameter custom_update("update_server", "Update Server", update_server, 40);
  WiFiManagerParameter custom_blynk_token("server", "Server", server, 34);
  }
  */

  // Wifi Start
  WiFi.forceSleepWake();
  WiFi.mode(WIFI_STA);
  //WiFi.setOutputPower(6);
  IPAddress ip( 192, 168, 1, 29 );
  IPAddress gateway( 192, 168, 1, 1 );
  IPAddress subnet( 255, 255, 255, 0 );
  delay( 1 );
  WiFi.persistent( false );
  WiFi.config( ip, gateway, subnet );
  WiFi.begin( "Ebni.Eitesal", "eitesalngo" );
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    PRINTDEBUG(".");
  }
  PRINTDEBUGLN(WiFi.localIP());
 
}

void loop() {
  ADC_MODE(ADC_VCC);
  PRINTDEBUGLN(String("Voltage = ")+ ESP.getVcc() +String(" V"));

  //if Motion flag
  bool status = msg();
  if(status)
    digitalWrite(esp_ok, HIGH);
  else
    digitalWrite(esp_ok, LOW);
  
  digitalWrite(esp_ack,HIGH);

 // Trun Off WIFI force all close 
  WiFi.mode( WIFI_OFF );
  WiFi.forceSleepBegin();
  delay( 1 );
  // Going To Sleep
  WiFi.disconnect( true );
  delay( 10000 ); //Wain untill the nano close the system
}
