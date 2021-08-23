SYSTEM_MODE(MANUAL);
SYSTEM_THREAD(ENABLED);
// This #include statement was automatically added by the Particle IDE.
#include <MQTT.h>

void callback(char* topic, byte* payload, unsigned int length);

// set server and port her fore mqtt
byte server[] = { 111,100,1,00 };  // your ip of your mqtt server here
MQTT client(server, 1883, callback);

//
// initialize my variables
//
//sensor pins
int tap1 = D1;
int tap2 = D2;
int tap3 = D3;

// delay in milliseconds before sending pour messsges to detect end of pour
unsigned long pourMsgDelay = 600;

//pulse count total variables
volatile int PulseCount1 = 0;
volatile int PulseCount2 = 0;
volatile int PulseCount3 = 0;
int LPulseCount1 = 0;
int LPulseCount2 = 0;
int LPulseCount3 = 0;

// pouring detection bool variables
bool pouring = false;
volatile bool pouring1 = false;
volatile bool pouring2 = false;
volatile bool pouring3 = false;

// timer variables for soft timer
unsigned long lastpouringtime = 0; //timer for check intervals
float pourLengthTimer = 0;
float pourStopTimer = 0;
bool firstboot = true;

float secPouring;
float pourRate;
char pathcreation [100];
//int loopcounter = 0;

// recieve mqtt messages here and call appropriate funtion or do appropriate tasks does nothing now but i left for future upgrades
void callback(char* topic, byte* payload, unsigned int length) {
    char p[length + 1];
    memcpy(p, payload, length);
    p[length] = NULL;
}

// Tap 1 interrupt function
void PulseCounter1(void){
	PulseCount1++;
	pouring1 = true;
}
// Tap 2 interrupt function
void PulseCounter2(void){
	PulseCount2++;
	pouring2 = true;
}
// Tap 3 interrupt function
void PulseCounter3(void){
	PulseCount3++;
	pouring3 = true;
}

//
// Setup Section
//
void setup() {
  RGB.control(true);  // take control of the LED
  RGB.color(0, 0, 0);  // the following sets the RGB LED off:
  wificonnect();
  mqttconnect();

  //set pinmodes and attach interrupts to count pusles from flow meters
  pinMode(tap1, INPUT);
  attachInterrupt(tap1, PulseCounter1, FALLING);
  pinMode(tap2, INPUT);
  attachInterrupt(tap2, PulseCounter2, FALLING);
  pinMode(tap3, INPUT);
  attachInterrupt(tap3, PulseCounter3, FALLING);
}

//
// Loop section main code
//
void loop() {
    if (!WiFi.ready()){
       RGB.color(0,0,0);
       WiFi.off();
       delay(100);
       wificonnect();
    }
    if (client.isConnected()){
      client.loop();
    }
    else{
      mqttconnect();
    }  
// calculate time now
unsigned long nowTimem = millis();
// check if state changed to pouring on any of the taps from the interrupt code and if it did mark the time so we can get a soft timer going
if (pouring == false){
    if (pouring1 == true || pouring2 == true || pouring3 == true){
        pouring = true;
        lastpouringtime = nowTimem; // establish a time that pour started use it to create a soft delay in checking for changes
        pourLengthTimer = nowTimem;
        //RGB.control(true);
        RGB.color(0, 0, 255);  // the following sets the RGB LED blue:
        } 
    }    

if (pouring == true){
    unsigned long tsince = (nowTimem - lastpouringtime);
    if (tsince > pourMsgDelay){ // only check for changes and stagnations if enough time has gone by soft timer to avoid interfering with interrupts
        int difTap1 = (PulseCount1 - LPulseCount1);
        int difTap2 = (PulseCount2 - LPulseCount2);
        int difTap3 = (PulseCount3 - LPulseCount3);
        
        if ((difTap1 == 0) && (difTap2 == 0) && (difTap3 == 0)){ // if its done pulsing send pours function otherwise move on to reset timer to check again
        //    Serial.println("triggered send pours");
            RGB.color(0, 0, 0); // white 
            pourStopTimer = millis();
            sendpours();
        }
        lastpouringtime = nowTimem; // reset soft timer
        LPulseCount1 = PulseCount1; // update these after delay as well to avoid machine gun messages
        LPulseCount2 = PulseCount2;
        LPulseCount3 = PulseCount3;
        }
    }
if (client.isConnected()){
        client.loop();
     }
else{
    mqttconnect();
    int i = 0;
    while(!client.isConnected() && i < 20) {
          delay(100);
          i++;
          }
    }
}    
    
void sendpours(){
    float millisPouring = (pourStopTimer-pourLengthTimer);
    secPouring = (millisPouring / 1000);
    
    if(!client.isConnected()){
        mqttconnect();
        int i = 0;
        while(!client.isConnected() && i < 20) {
              delay(100);
              i++;
        }
    }
    
    if (pouring1 == true){
        pourRate = (PulseCount1 / secPouring);
        if (PulseCount1 > 500 && PulseCount1 < 15000 && secPouring < 60 && secPouring > 2 && pourRate > 100 && pourRate < 1500){ // only if its not a super tiny pour or super large pour or longer than 25 seconds pour
            sprintf(pathcreation, "P;-1;1;%d", PulseCount1);
            delay(500);
            client.publish("rpints/pours", pathcreation); // publish an mqtt messag
            PulseCount1 = 0;
            pouring1 = false;
        }
        else{              // if the pour was too small reset the counter
            sprintf(pathcreation, "{\"tap\": 1,\"pulses\": %d,\"rate\": %f,\"secPouring\": %f}", PulseCount1, pourRate, secPouring);
            client.publish("falsepours", pathcreation, TRUE); // publish an mqtt message for debug
            RGB.color(255, 0, 0);
            PulseCount1 = 0;
            pouring1 = false;
        }
    }
    if (pouring2 == true){
        pourRate = (PulseCount2 / secPouring);
        if (PulseCount2 > 500 && PulseCount2 < 15000 && secPouring < 60 && secPouring > 2 && pourRate > 100 && pourRate < 1500){ // only if its not a super tiny pour
            sprintf(pathcreation, "P;-1;2;%d", PulseCount2);
            client.publish("rpints/pours", pathcreation); // publish an mqtt messag
            PulseCount2 = 0;
            pouring2 = false;

            }
            else{              // if the pour was too small reset the counter
                sprintf(pathcreation, "{\"tap\": 2,\"pulses\": %d,\"rate\": %f,\"secPouring\": %f}", PulseCount2, pourRate, secPouring);
                client.publish("falsepours", pathcreation, TRUE); // publish an mqtt message for debug
                RGB.color(255, 0, 0);
                PulseCount2 = 0;
                pouring2 = false;
            }
        }
    if (pouring3 == true){
        pourRate = (PulseCount3 / secPouring);
        if (PulseCount3 > 500 && PulseCount3 < 15000 && secPouring < 60 && secPouring > 2 && pourRate > 100 && pourRate < 1500){ // only if its not a super tiny pour
            sprintf(pathcreation, "P;-1;3;%d", PulseCount3);
            client.publish("rpints/pours", pathcreation); // publish an mqtt messag
            PulseCount3 = 0;
            pouring3 = false;

            }
        else{              // if the pour was too small reset the counter
                sprintf(pathcreation, "{\"tap\": 3,\"pulses\": %d,\"rate\": %f,\"secPouring\": %f}", PulseCount3, pourRate, secPouring);
                client.publish("falsepours", pathcreation, TRUE); // publish an mqtt message for debug
                RGB.color(255, 0, 0);
                PulseCount3 = 0;
                pouring3 = false;
            }
        }
    pouring = false;
    RGB.color(0, 255, 0);
    //System.reset(); // just reset the damn thing to avoid problems with strings and memory leak hopefully will emiminate weird ghost pour behavior?
}


void wificonnect(){
    WiFi.on();
    WiFi.setCredentials("ssid", "pass");
    WiFi.connect();
    waitUntil(WiFi.ready);
    while (WiFi.localIP() == IPAddress()) {
    	delay(10);
    }
    RGB.color(0,255,0);
}


void mqttconnect(){
// connect to the server
  client.connect("your_desired_client_name_anything", "Your_Username", "Your_Password");

// publish/subscribe
  if (client.isConnected()) {
    }
}
