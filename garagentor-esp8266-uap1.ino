/*
  To upload through terminal you can use: curl -F "image=@firmware.bin" esp8266-webupdate.local/firmware
*/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <stdio.h>
#include <string.h>

// Uses Ticker to do a nonblocking loop every 1 second to check status
#include <Ticker.h>

unsigned long lastmillis;

const char* sVersion = "0.8.23";


#define CLOSED LOW
#define OPEN HIGH

#include <PubSubClient.h>
//Your MQTT Broker
const char* mqtt_server = "hwr-pi";

const char* mqtt_client_id = "hoermann-garage";

const char* mqtt_topic_cmd = "tuer/garagentor/cmd";
const char* mqtt_topic_status = "tuer/garagentor/status";
const char* mqtt_topic_action = "tuer/garagentor/action";
const char* mqtt_topic_version = "tuer/garagentor/version";
const char* mqtt_topic_wifi = "tuer/garagentor/wifi";



const char* host = "esp8266-garage";
//Your Wifi SSID
const char* ssid = "....";
//Your Wifi Key
const char* password = "...";

const char* update_path = "/firmware";
const char* update_username = "admin";
const char* update_password = "admin";

ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

WiFiClient espClient;
PubSubClient client(espClient);


int gpioLed = 2; // internal blue LED

int gpioO1TorOben = 5;  // D1
int gpioO2TorUnten = 0; // D3

int gpioS4CmdZu = 14;          // D5
int gpioS5CmdLampeToggle = 12; // D6
int gpioS2CmdAuf = 13;         // D7
int gpioS3CmdHalb = 15;        // D8


int FEHLER=0;
int ZU=1;
int AUF=2;
int HALBAUF=3;

const char* statusmeldung[] = { "Fehler", "Zu", "Auf", "Halb auf" };

int status = FEHLER;

bool SendUpdate = false;

Ticker ticker;




//check gpio (input of hoermann uap1)
void statusTor() 
{
  int myOldStatus = status;
 
  if ((digitalRead(gpioO1TorOben) == OPEN  ) && (digitalRead(gpioO2TorUnten) == OPEN  )) { status = HALBAUF; }
  if ((digitalRead(gpioO1TorOben) == CLOSED) && (digitalRead(gpioO2TorUnten) == OPEN  )) { status = AUF; }
  if ((digitalRead(gpioO1TorOben) == OPEN  ) && (digitalRead(gpioO2TorUnten) == CLOSED)) { status = ZU; }
  if ((digitalRead(gpioO1TorOben) == CLOSED) && (digitalRead(gpioO2TorUnten) == CLOSED)) { status = FEHLER; }

#if 0
  Serial.print("status ");
  Serial.println(statusmeldung[status]);
#endif

  if ( myOldStatus != status )
  {
    Serial.print("Status has changed: ");
    Serial.println(statusmeldung[status]);
    client.publish(mqtt_topic_status, statusmeldung[status]);
  }

  digitalWrite(gpioLed, (status != FEHLER) ); // LED on bei Fehler  
  delay(1000);
  digitalWrite(gpioLed, (status == FEHLER) ); // LED on bei Fehler
}



void oeffneTor() {
      Serial.println("oeffneTor start");
      client.publish(mqtt_topic_action, "oeffneTor start");

      digitalWrite(gpioS2CmdAuf, HIGH);
      delay(1000);
      digitalWrite(gpioS2CmdAuf, LOW);

      Serial.println("oeffneTor fertig");
      client.publish(mqtt_topic_action, "oeffneTor fertig");
}


void schliesseTor() {
      Serial.println("schliesseTor start");
      client.publish(mqtt_topic_action, "schliesseTor start");

      digitalWrite(gpioS4CmdZu, HIGH);
      delay(1000);
      digitalWrite(gpioS4CmdZu, LOW);

      Serial.println("schliesseTor fertig");
      client.publish(mqtt_topic_action, "schliesseTor fertig");
}



void MqttCallback(char* topic, byte* payload, unsigned int length) {
  payload[length] = 0;

  Serial.println("mqtt call back");
  Serial.println( topic );
  Serial.println(  (const char *) payload );
  
  if ( strstr( (const char *)payload, "zu") != NULL) {
    schliesseTor();
  }
  if ( strstr( (const char *)payload, "auf") != NULL) {
    oeffneTor();
  }
  if ( strstr( (const char *)payload, "version") != NULL) {
    client.publish(mqtt_topic_version, sVersion);
    long lRssi = WiFi.RSSI();
    char sBuffer[50];
    sprintf(sBuffer, "RSSI = %d dBm", lRssi);
    client.publish(mqtt_topic_wifi, sBuffer);
  }
  if ( strstr( (const char *)payload, "status") != NULL) {
    statusTor();
    client.publish(mqtt_topic_status, statusmeldung[status]);
  }
}



void MqttReconnect() {
  while (!client.connected()) {
    Serial.print("Connect to MQTT Broker "); Serial.println( mqtt_server );
    delay(1000);
    if (client.connect(mqtt_client_id)) {
      Serial.print("connected: topic ");  Serial.println( mqtt_topic_cmd );
      client.subscribe(mqtt_topic_cmd);
    } else {
      Serial.print("failed: ");
      Serial.print(client.state());
      Serial.println(" try again...");
      delay(5000);
    }
  }
  Serial.println(" ok...");

}



void CheckDoorStatus()
{
  int oldStatus = status;
  statusTor();
  if (oldStatus == status)
  {
    //new status is the same as the current status, return
    return;
  }
  else
  {
    Serial.print("Status has changed to: ");
    Serial.println(statusmeldung[status]);
    SendUpdate = true;
  }
}


void setup(void){

  Serial.begin(115200);
  delay(300);
  Serial.println("starte hoermann garage...");
  Serial.println("");
  Serial.println(sVersion);
  Serial.println();

  pinMode(gpioO1TorOben, INPUT_PULLUP);
  pinMode(gpioO2TorUnten, INPUT_PULLUP);

  pinMode(gpioLed, OUTPUT);

  pinMode(gpioS4CmdZu, OUTPUT);
  digitalWrite(gpioS4CmdZu, LOW);
  pinMode(gpioS5CmdLampeToggle, OUTPUT);
  digitalWrite(gpioS5CmdLampeToggle, LOW);
  pinMode(gpioS2CmdAuf, OUTPUT);
  digitalWrite(gpioS2CmdAuf, LOW);
  pinMode(gpioS3CmdHalb, OUTPUT);
  digitalWrite(gpioS3CmdHalb, LOW);



//  WiFi.persistent(false);
  WiFi.mode(WIFI_OFF);
  WiFi.mode(WIFI_STA);

  WiFi.begin(ssid, password);


  while(WiFi.waitForConnectResult() != WL_CONNECTED){
    WiFi.begin(ssid, password);
    Serial.println("WiFi failed, retrying.");
  }

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  digitalWrite(gpioLed, HIGH );
  delay(250);
  digitalWrite(gpioLed, LOW );
  delay(250);
  digitalWrite(gpioLed, HIGH );
  delay(250);
  digitalWrite(gpioLed, LOW );
  delay(250);
  digitalWrite(gpioLed, HIGH );
  delay(250);


  MDNS.begin(host);

  httpUpdater.setup(&httpServer, update_path, update_username, update_password);

  httpServer.begin();

  MDNS.addService("http", "tcp", 80);

  Serial.printf("HTTPUpdateServer ready! \n  Open http://%s.local%s in your browser and\n login with username '%s' and password '%s'\n", host, update_path, update_username, update_password);

  Serial.printf("\nsketch version: %s\n", sVersion);


  client.setServer(mqtt_server, 1883);
  client.setCallback(MqttCallback);
  MqttReconnect();
  client.publish(mqtt_topic_version, sVersion);
  statusTor();
  

  httpServer.on("/", []() {
    statusTor();

    if (status == ZU) {
      httpServer.send(200, "text/html", "garage ist aktuell zu.<p><a href=\"auf\">&oumlffnen</a></p>");
    }
    if (status == AUF ) {
      httpServer.send(200, "text/html", "garage ist aktuell auf.<p><a href=\"zu\">schliessen</a></p>");
    }
    if (status == HALBAUF ) {
      httpServer.send(200, "text/html", "garage ist aktuell HALB auf.<p><a href=\"zu\">schliessen</a></p>");
    }
    if (status == FEHLER ) {
      httpServer.send(200, "text/html", "garage ist aktuell FEHLER.<p>!!!</p>");
    }
 
    httpServer.send ( 302, "text/plain", "");
  });


  httpServer.on("/status", []() {
    long lRssi = WiFi.RSSI();
    char sBuffer[100];
    sprintf(sBuffer, "WiFi  RSSI = %d dBm\r\nVersion = %s\r\n", lRssi, sVersion);
    Serial.println( sBuffer );

    httpServer.send(200, "text/plain", sBuffer);
 
    httpServer.send ( 302, "text/plain", "");
  });


  httpServer.on("/toggle", []() {
    Serial.println("http toggle");
    statusTor();
    if (status == ZU) {
      oeffneTor();
    }
    if (status == AUF) {
      schliesseTor ();
    }
    httpServer.send ( 302, "text/plain", "");
  });

  // add redirect zu '/'
  httpServer.on("/zu", []() {
    schliesseTor();
    httpServer.send ( 302, "text/plain", "schliessen OK");
  });

  // add redirect zu '/'
  httpServer.on("/auf", []() {
    oeffneTor();
    httpServer.send ( 302, "text/plain", "oeffnen OK");
  });

  //Check the door status every 1 second
  ticker.attach(5, CheckDoorStatus);

}



void loop(void){
  httpServer.handleClient();

  if (!client.connected()) { MqttReconnect(); }
  client.loop();

  if (millis() - lastmillis >  60000) {
    Serial.print("still running since ");
    Serial.print(millis()/60000);
    Serial.println(" minutes");
    lastmillis = millis();
    SendUpdate = true;
  }

  if (SendUpdate)
  {
    Serial.print("mqtt status update: ");
    Serial.println(statusmeldung[status]);
    client.publish(mqtt_topic_status, statusmeldung[status]);
    SendUpdate = false;
  }

}


