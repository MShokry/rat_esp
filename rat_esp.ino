#include <ESP8266WiFi.h>

void setup() {
  // put your setup code here, to run once:
pinMode(D0,WAKEUP_PULLUP);
// Trun Off WIFI force all close 
WiFi.mode( WIFI_OFF );
WiFi.forceSleepBegin();
delay( 1 );

 pinMode(D3,OUTPUT);
 digitalWrite(D3,LOW);
 //ADC_MODE(ADC_VCC);
 Serial.begin(115200);
  Serial.setTimeout(2000);

  // Wait for serial to initialize.
  while(!Serial) { }
  WiFi.setOutputPower(6);
  // Deep sleep mode for 30 seconds, the ESP8266 wakes up by itself when GPIO 16 (D0 in NodeMCU board) is connected to the RESET pin
  Serial.println("I'm awake, but I'm going into deep sleep mode for 30 seconds");
  Serial.println(String("Voltage = ")+ ESP.getVcc() +String(" V"));
  //digitalWrite(D3,HIGH);
  
  // Going To Sleep
  WiFi.disconnect( true );
  delay( 1 );
  ESP.deepSleep(5e6,WAKE_RF_DISABLED); 
  //30e6
  // Deep sleep mode until RESET pin is connected to a LOW signal (for example pushbutton or magnetic reed switch)
  //Serial.println("I'm awake, but I'm going into deep sleep mode until RESET pin is connected to a LOW signal");
  //ESP.deepSleep(0); 

}

void loop() {
  // put your main code here, to run repeatedly:
digitalWrite(D3,LOW);
WiFi.forceSleepWake();
IPAddress ip( 192, 168, 0, 1 );
IPAddress gateway( 192, 168, 0, 254 );
IPAddress subnet( 255, 255, 255, 0 );
delay( 1 );
WiFi.persistent( false );
WiFi.mode( WIFI_STA );
WiFi.config( ip, gateway, subnet );
WiFi.begin( "airlive", "" );

}
