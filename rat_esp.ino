#include <FS.h>                   //this needs to be first, or it all crashes and burns...

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>


#define DEBUG 0
#if (DEBUG==1)
  #define PRINTDEBUGLN(STR) Serial.println(STR)
  #define PRINTDEBUG(STR) Serial.print(STR)
#else
  #define PRINTDEBUGLN(STR) /*NOTHING*/
  #define PRINTDEBUG(STR) /*NOTHING*/
#endif

// Dont use D3,D4,D8 GPIO0,2,15 ,, RX,TX GPIO3,1 ,, ADC,TOUT

#define esp_ack     5       //D1 Done operation and ready to sleep
#define esp_ok      4       //D2 ESP last operaion status
#define esp_motion  14      //D5 Motion detected 
#define esp_config  12      //D6 Config button
#define esp_update  13      //D7 Update button


#define API "?name="
// WIFI Configuration
char update_server[30] = "7elol.com";
char update_server_page[30] = "/update/config.php";
char server[30] = "7elol.com";
char server_page[30] = "/update/rfid.php";
char server_port[6] = "80";
char PLACE[20] = "Nest1";
//default custom static IP
char static_ip[16] = "192.168.0.10";
char static_gw[16] = "192.168.0.1";
char static_sn[16] = "255.255.255.0";

char SSID[24] = "airlive";
char SSID_pwd[24] = "";
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
          strcpy(SSID, json["ssid"]);
          strcpy(SSID_pwd, json["ssid_pwd"]);
          PRINTDEBUGLN(SSID);
          PRINTDEBUGLN(SSID_pwd);
          if(json["update_server"]) 
          strcpy(update_server, json["update_server"]);          
          PRINTDEBUGLN(update_server);
          if(json["update_server_page"]) 
            strcpy(update_server_page, json["update_server_page"]);          
          PRINTDEBUGLN(update_server_page);
          //if(json["server"]) 
          strcpy(server, json["server"]);
          PRINTDEBUGLN(server);
          if(json["server_page"])
            strcpy(server_page, json["server_page"]);
          PRINTDEBUGLN(server_page);
          //if(json["server_port"]) 
          //  strcpy(server_port, json["server_port"]);
          //if(json["place"])
          strcpy(PLACE, json["place"]);
          PRINTDEBUGLN(PLACE);
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
            return false;
          }
        } else {
          PRINTDEBUGLN("failed to load json config");
          return false;
        }
      }
    }else{
      PRINTDEBUGLN("Files Doesn't exist");
    }
  } else {
    PRINTDEBUGLN("failed to mount FS");
    return false;
  }
  //end read
  PRINTDEBUGLN(static_ip);
  PRINTDEBUGLN(static_gw);
  PRINTDEBUGLN(static_sn);
  return true;
}

/*************************************************************************************
 *  Pinging the server
 *************************************************************************************/
//Pingging main
bool msg (String msg="alarm"){
   // Use WiFiClientSecure class to create TLS connection
  WiFiClient client;
  PRINTDEBUGLN("Connecting to:");
  PRINTDEBUGLN(server);
  if(!client.connect(server, 80))
  {
    PRINTDEBUGLN("Connection failed");
    return false;
  }
  // URL request

  String url = String(server_page) + String(API) + String(PLACE) + String("&msg=");
  url += msg;
  url += String("&v=") + (ESP.getVcc());
  url += String("&mac=") + String(WiFi.macAddress());
  PRINTDEBUGLN("Requesting URL:");
  PRINTDEBUGLN(url);
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + server + "\r\n" +
               "User-Agent: ratnest\r\n" +
               "Connection: close\r\n\r\n");
  PRINTDEBUGLN("Request sent");
  String line = client.readStringUntil('\n');
  PRINTDEBUGLN(line);

  return true;
}

/*************************************************************************************
 *  Requesting Config fromserver
 *************************************************************************************/
//Pingging main
bool config (String msg="config"){
   // Use WiFiClientSecure class to create TLS connection
  WiFiClient client;
  PRINTDEBUGLN("Connecting to:");
  PRINTDEBUGLN(update_server);
  if(!client.connect(update_server, 80))
  {
    PRINTDEBUGLN("Connection failed");
    return false;
  }
  // URL request

  String url = String(update_server_page) + String(API) + String(PLACE) + String("&msg=");
  url += msg;
  url += String("&v=") + (ESP.getVcc());
  url += String("&mac=") + String(WiFi.macAddress());
  PRINTDEBUGLN("Requesting URL:");
  PRINTDEBUGLN(url);
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + update_server + "\r\n" +
               "User-Agent: ratnest\r\n" +
               "Connection: close\r\n\r\n");
  PRINTDEBUGLN("Request sent");
  PRINTDEBUGLN("Headers:");
  PRINTDEBUGLN("========");
  String line;
  while(client.connected())
  {
    line = client.readStringUntil('\n');
    PRINTDEBUGLN(line);
    if(line == "\r")
    {
      PRINTDEBUGLN("========");      
      PRINTDEBUGLN("Headers received");
      //break;
    }
    if(line.startsWith("{")){
      break;
    }

  }
  //String line = client.readStringUntil('\n');
  PRINTDEBUGLN("Reply was:");
  PRINTDEBUGLN("==========");
  PRINTDEBUGLN(line);
  PRINTDEBUGLN("==========");
  PRINTDEBUGLN("Closing connection");
  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[600]);
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(line);
  #if (DEBUG==1)  
    json.printTo(Serial);
  #endif
  if (json.success()) {
    PRINTDEBUGLN("\nparsed json");
    strcpy(SSID, json["ssid"]);
    strcpy(SSID_pwd, json["ssid_pwd"]);
    PRINTDEBUGLN(SSID);
    PRINTDEBUGLN(SSID_pwd);
    if(json["update_server"]) 
    strcpy(update_server, json["update_server"]);          
    PRINTDEBUGLN(update_server);
    if(json["update_server_page"]) 
      strcpy(update_server_page, json["update_server_page"]);          
    PRINTDEBUGLN(update_server_page);
    //if(json["server"]) 
    strcpy(server, json["server"]);
    PRINTDEBUGLN(server);
    if(json["server_page"])
      strcpy(server_page, json["server_page"]);
    PRINTDEBUGLN(server_page);
    //if(json["server_port"]) 
    //  strcpy(server_port, json["server_port"]);
    //if(json["place"])
    strcpy(PLACE, json["place"]);
    PRINTDEBUGLN(PLACE);
    if(json["ip"]) {
      PRINTDEBUGLN("setting custom ip from config");
      strcpy(static_ip, json["ip"]);
      strcpy(static_gw, json["gateway"]);
      strcpy(static_sn, json["subnet"]);
      PRINTDEBUGLN(static_ip);
/*            PRINTDEBUGLN("converting ip");
      IPAddress ip = ipFromCharArray(static_ip);
      PRINTDEBUGLN(ip);*/
      return true;
    } else {
      PRINTDEBUGLN("no custom ip in config");
      return false;
    }
  } else {
    PRINTDEBUGLN("failed to load json config");
    return false;
  }
      
  return false;
}

/*************************************************************************************
 *  ////////////////////SETUP ////////////////
 *************************************************************************************/
ADC_MODE(ADC_VDD);

void setup() {
  // put your setup code here, to run once:
  pinMode(esp_ok,OUTPUT);
  pinMode(esp_ack,OUTPUT);
  digitalWrite(esp_ok, LOW);
  digitalWrite(esp_ack, LOW);
  pinMode(esp_motion, INPUT);
  pinMode(esp_update, INPUT);
  pinMode(esp_config, INPUT);

  #if (DEBUG==1)
    Serial.begin(115200);     // Initialize serial communications
    delay(100);
    Serial.setTimeout(2000);
    Serial.setDebugOutput(true);
    Serial.println();
    Serial.println("Author:: Mahmoud Shokry");
    Serial.println("version:: 0.6");
  #endif
  load_wifi();
  WiFi.mode(WIFI_STA);
}

void loop() {
  //ADC_MODE(ADC_VCC);
  PRINTDEBUGLN(String("Voltage = ")+ (ESP.getVcc()) +String(" V"));

  if(digitalRead(esp_motion)){
      // Wifi Start
    PRINTDEBUGLN("entering Motion detected");
    WiFi.forceSleepWake();
    delay(1);
    WiFi.mode(WIFI_STA);
    //WiFi.setOutputPower(6);
    IPAddress _ip,_gw,_sn;
    _ip.fromString(static_ip);
    _gw.fromString(static_gw);
    _sn.fromString(static_sn);
    delay( 1 );
    WiFi.persistent( false );
    WiFi.config( _ip, _gw, _gw, _sn );
    //WiFi.begin( "Ebni.Eitesal", "eitesalngo" );
    WiFi.begin( SSID, SSID_pwd );

    while (WiFi.status() != WL_CONNECTED)
    {
      delay(100);
      PRINTDEBUG(".");
    }
    PRINTDEBUGLN(WiFi.localIP());
    //if Motion flag
    bool status = false;
    if(!digitalRead(esp_update)){
      status = msg();
    }else{
      status = msg("test");
    }
    if(status)
      digitalWrite(esp_ok, HIGH);
    else
      digitalWrite(esp_ok, LOW);
    
    digitalWrite(esp_ack,HIGH);

   // Trun Off WIFI force all close 
    //WiFi.mode( WIFI_OFF );
    //WiFi.disconnect(true);
    //WiFi.forceSleepBegin();
    //delay( 1 );
    // Going To Sleep
  }else  if(digitalRead(esp_update)){
    PRINTDEBUGLN("entering Update");
    WiFi.forceSleepWake();
    delay(1);
    WiFi.mode(WIFI_STA);
    //WiFi.setOutputPower(6);
    IPAddress _ip,_gw,_sn;
    _ip.fromString(static_ip);
    _gw.fromString(static_gw);
    _sn.fromString(static_sn);
    delay( 1 );
    WiFi.persistent( false );
    WiFi.config( _ip, _gw, _gw, _sn );
    //WiFi.begin( "Ebni.Eitesal", "eitesalngo" );
    WiFi.begin( SSID, SSID_pwd );

    while (WiFi.status() != WL_CONNECTED)
    {
      delay(100);
      PRINTDEBUG(".");
    }
    PRINTDEBUGLN(WiFi.localIP());

    if (config()) {
      PRINTDEBUGLN("saving config");
      digitalWrite(esp_ok, HIGH);
      DynamicJsonBuffer jsonBuffer;
      JsonObject& json = jsonBuffer.createObject();
      json["server"] = server;
      json["server_page"] = server_page;
      json["update_server"] = update_server;
      json["update_server_page"] = update_server_page;
      json["place"] = PLACE;
      json["ssid"] =  SSID;
      json["ssid_pwd"] =  SSID_pwd;
      json["ip"] = static_ip;
      json["gateway"] = static_gw;
      json["subnet"] = static_sn;
      File configFile = SPIFFS.open("/config.json", "w");
      if (!configFile) {
        PRINTDEBUGLN("failed to open config file for writing");
      }
      #if (DEBUG==1)
        json.prettyPrintTo(Serial);
      #endif
      json.printTo(configFile);
      configFile.close();
      //end save
    }else{
      digitalWrite(esp_ok, LOW);
    }
    PRINTDEBUGLN("All Done configuration");
    WiFi.mode(WIFI_STA);
    digitalWrite(esp_ack,HIGH);
  }else if(digitalRead(esp_config)){
    PRINTDEBUGLN("entering configuration");
    WiFi.forceSleepWake();
    delay(1);
    WiFiManagerParameter custom_update("update_server", "Update Server", update_server, 34);
    WiFiManagerParameter custom_update_page("update_server_page", "Update Server route", update_server_page, 34);
    WiFiManagerParameter custom_server("server", "Server name", server, 34);
    WiFiManagerParameter custom_server_page("server_page", "Server route", server_page, 34);
    WiFiManagerParameter custom_place("place", "Location Name", PLACE, 20);
    WiFiManager wifiManager;
    #if (DEBUG==1)
    wifiManager.setDebugOutput(true);
    #endif
    #if (DEBUG==0)
    wifiManager.setDebugOutput(false);
    #endif
    //set config save notify callback
    wifiManager.setSaveConfigCallback(saveConfigCallback);

    //add all your parameters here
    wifiManager.addParameter(&custom_update);
    wifiManager.addParameter(&custom_update_page);
    wifiManager.addParameter(&custom_server);
    wifiManager.addParameter(&custom_server_page);
    wifiManager.addParameter(&custom_place);
    //set minimu quality of signal so it ignores AP's under that quality
    //defaults to 8% to reduce the power next
    //wifiManager.setMinimumSignalQuality();
    
    wifiManager.setTimeout(280); // 14.7 min in seconds
    wifiManager.startConfigPortal("Rat_nest","password");
    
    strcpy(update_server, custom_update.getValue());
    strcpy(update_server_page, custom_update_page.getValue());
    strcpy(server, custom_server.getValue());
    strcpy(server_page, custom_server_page.getValue());
    strcpy(PLACE, custom_place.getValue());
    //strcpy(SSID, wifiManager.getSSID());
    //strcpy(SSID_pwd, wifiManager.getSSIDpwd());
    //PRINTDEBUGLN(WiFiManager.getConfigPortalSSID())
    if (shouldSaveConfig) {
      PRINTDEBUGLN("saving config");
      digitalWrite(esp_ok, HIGH);
      DynamicJsonBuffer jsonBuffer;
      JsonObject& json = jsonBuffer.createObject();
      json["server"] = server;
      json["server_page"] = server_page;
      json["update_server"] = update_server;
      json["update_server_page"] = update_server_page;
      json["place"] = PLACE;
      json["ssid"] =  WiFi.SSID();
      json["ssid_pwd"] =  WiFi.psk();
      json["ip"] = WiFi.localIP().toString();
      json["gateway"] = WiFi.gatewayIP().toString();
      json["subnet"] = WiFi.subnetMask().toString();
      File configFile = SPIFFS.open("/config.json", "w");
      if (!configFile) {
        PRINTDEBUGLN("failed to open config file for writing");
      }
      #if (DEBUG==1)
        json.prettyPrintTo(Serial);
      #endif
      json.printTo(configFile);
      configFile.close();
      //end save
    }else{
      digitalWrite(esp_ok, LOW);
    }
    PRINTDEBUGLN("All Done configuration");
    WiFi.mode(WIFI_STA);
    digitalWrite(esp_ack,HIGH);
  }

  wifi_set_sleep_type(LIGHT_SLEEP_T);
  wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);
  WiFi.disconnect(true);
  WiFi.mode( WIFI_OFF );
  WiFi.forceSleepBegin();
  delay(1);
  wifi_fpm_open();
  wifi_fpm_do_sleep(26843455);
  long endMs = millis() + 20000;
  while (millis() < endMs) {
     yield();
  }
  //delay(10000); //Wain untill the nano close the system
}
