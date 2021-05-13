#include <WiFiManager.h>      // Using the Arduino Library Manager, install "WifiManager by tzapu" - lib at https://github.com/tzapu/WiFiManager

#include <WiFiUdp.h>


// Create wifi, and networking interface objects
WiFiUDP Udp;


// Create a buffer for UDP communications
int lengthRead;
int const incomingBufferSize = 135;    // was 128 -JM
char incomingPacket[incomingBufferSize];

// Specify the request packets
// Kudos to http://www.rane.com/note161.html and http://www.net-snmp.org/ for helping understand the SNMP packet
// Code adapted from Alistair MacDonald 2018's ElectroMaker project https://www.electromaker.io/project/view/the-internet-speed-o-meter#code

int downloadMeter = D2;           // the PWM pin the download meter is attached to
int uploadMeter = D1;             // the PWM pin the upload meter is attached to

int maxDownloadSpeed = 10;             //Max download speed (in Mbps) on the scale
int maxUploadSpeed = 10;               //Max upload speed (in Mbps) on the scale

double downloadSpeed = 0;
double uploadSpeed = 0;

int const requestSize = 42;

byte downloadRequestPacket[requestSize] = { 0x30, // Sequence Complex Data Type         DONE
                                         0x28, // Request Size (less this 2 byte header)
                                         0x02, // Version Type Integer
                                         0x01, // Version Length 1
                                         0x00, // Version Number 0
                                         0x04, // Type Octet String
                                         0x06, // Length 7
                                         0x70, // p
                                         0x75, // u
                                         0x62, // b
                                         0x6C, // l
                                         0x69, // i
                                         0x63, // c
                                         0xA0, // SNMP PDU Type GetRequest
                                         0x1B, // PDU Length
                                         0x02, // Request ID Type Integer
                                         0x01, // Request ID Length
                                         0x10, // Request ID Value LSB (using 16 to id download)
                                         0x02, // Error Tyne Integer
                                         0x01, // Error Length 1
                                         0x00, // Error Value 0
                                         0x02, // Error Index Type Integer
                                         0x01, // Error Index Type Length 1
                                         0x00, // Error Index Type Value 0
                                         0x30, // Varbind List Type Sequence
                                         0x10, // Varbind List Type Length
                                         0x30, // Varbind Type Sequence
                                         0x0E, // Varbind Type Length
                                         0x06, // Object Identifier Type Object Identifier
                                         0x0A, // Object Identifier Length
                                         0x2B, // ISO.3
                                         0x06, // 6
                                         0x01, // 1
                                         0x02, // 2
                                         0x01, // 1
                                         0x02, // 2
                                         0x02, // 2
                                         0x01, // 1
                                         0x10, // 16 (Download)
                                         0x01, //                      // was 0x07 (eth0/lan on RV024G)
                                         0x05, // Value Type Null
                                         0x00 }; // Value Length 0

byte uploadRequestPacket[requestSize] = { 0x30, // Sequence Complex Data Type         DONE
                                         0x28, // Request Size (less this 2 byte header)
                                         0x02, // Version Type Integer
                                         0x01, // Version Length 1
                                         0x00, // Version Number 0
                                         0x04, // Type Octet String
                                         0x06, // Length 7
                                         0x70, // p
                                         0x75, // u
                                         0x62, // b
                                         0x6C, // l
                                         0x69, // i
                                         0x63, // c
                                         0xA0, // SNMP PDU Type GetRequest
                                         0x1B, // PDU Length
                                         0x02, // Request ID Type Integer
                                         0x01, // Request ID Length
                                         0x0A, // Request ID Value LSB (using 10 to id upload)
                                         0x02, // Error Tyne Integer
                                         0x01, // Error Length 1
                                         0x00, // Error Value 0
                                         0x02, // Error Index Type Integer
                                         0x01, // Error Index Type Length 1
                                         0x00, // Error Index Type Value 0
                                         0x30, // Varbind List Type Sequence
                                         0x10, // Varbind List Type Length
                                         0x30, // Varbind Type Sequence
                                         0x0E, // Varbind Type Length
                                         0x06, // Object Identifier Type Object Identifier
                                         0x0A, // Object Identifier Length
                                         0x2B, // ISO.3
                                         0x06, // 6
                                         0x01, // 1
                                         0x02, // 2
                                         0x01, // 1
                                         0x02, // 2
                                         0x02, // 2
                                         0x01, // 1
                                         0x0A, // 10 (Upload)
                                         0x01, // 3                      // was 0x07 (eth0/lan on RV024G)
                                         0x05, // Value Type Null
                                         0x00 }; // Value Length 0


// Variables to store the last read values and times
unsigned long downloadValueLast = 0;
unsigned long uploadValueLast = 0;
unsigned long downloadValueNow = 0;
unsigned long uploadValueNow = 0;

unsigned long downloadTimeNow = 0;
unsigned long uploadTimeNow = 0;
unsigned long downloadTimeLast = 0;
unsigned long uploadTimeLast = 0;

// Variables for storing the derived values
double downloadRate = 0;  // Result in Mbps
double uploadRate = 0;    // Result in Mbps


// Keep track of when we last request data
unsigned long millisLast = 0;
unsigned long millisNow = 0;

// Setup routine                  DONE
void setup() {

  // Connect to the WiFi
  Serial.begin(74880);

  WiFiManager wifiManager;             // Start a Wifi link
  wifiManager.autoConnect("snmpSpeedometer","password");         //   (on first use: will set up as a Wifi name; set up by selecting new Wifi network on iPhone for instance)
  
  Serial.println(" ");
  Serial.print("Connecting..");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    }
  
  Serial.println("Connected!");

  Serial.print("WiFi.gatewayIP() = ");
  Serial.println(WiFi.gatewayIP());
  
  // Start listening for SNMP messges
  Udp.begin(161);        // was 162 -JM

  pinMode(downloadMeter, OUTPUT);
  pinMode(uploadMeter, OUTPUT);

}


// The main loop
void loop() {

  millisNow = millis();
  /// Check if we need to send another SNMP request - no more than once every 2 seconds
  if ((millisNow-millisLast)>500) {               /// (was 5000)


  sendSnmpRrequest("Down");
  checkForReply("Down", millisNow);      /// Check for a new SNMP message

  sendSnmpRrequest("Up");
  checkForReply("Up", millisNow);      /// Check for a new SNMP message



 // double downloadRate = downloadSpeed * 100;
 // double uploadRate = downloadSpeed * 100;

 // int downloadSpeedInt = (int)downloadRate;
 // int uploadSpeedInt = (int)uploadRate;

 // downloadRate = map(downloadSpeedInt, 0, maxDownloadSpeed*100, 1, 1023);
 // uploadRate   = map(uploadSpeedInt,   0, maxUploadSpeed*100,   1, 1023);
  /// syntax: map(value, fromLow, fromHigh, toLow, toHigh)
  
  
  Serial.print(" [XX] downloadRate = ");
  Serial.print(downloadRate);
  Serial.print(" - uploadRate = ");
  Serial.print(uploadRate);
  Serial.print(" [] ");

  Serial.println(" [EOL]");



    updateMeter();
    millisLast = millisNow;      //Remember when we did this to wait another 5 seconds

  }

}


// Function to request upload or download data from the gateway         DONE
String sendSnmpRrequest(String traffic) {

  Udp.beginPacket(WiFi.gatewayIP(), 161);
  if (traffic == "Down") {
    Udp.write(downloadRequestPacket, requestSize);
  }
  else if (traffic == "Up") {
    Udp.write(uploadRequestPacket, requestSize);
  }
  Udp.endPacket();
  
}



// Calculate the Meter's new position and set it                     DONE
void updateMeter() {
  
  analogWrite(downloadMeter, downloadRate);
  analogWrite(uploadMeter, uploadRate);

}




int checkForReply(String traffic, unsigned long inNow) {
  if (Udp.parsePacket()>0) {
    lengthRead = Udp.read(incomingPacket, incomingBufferSize);
    Udp.read(incomingPacket, incomingBufferSize);
    // printResponse();                            //Uncomment to see the response
    int pointer = 49;                             //49th byte tells the length of data 

    int dataLength = incomingPacket[pointer];
    pointer += 1;
    unsigned long value = 0;

    for (int i = 0; i < dataLength; i++) {
      value = (value<<8) | incomingPacket[pointer + i];
    }
   
  // Check what to do with the data we received based on the ID
    if (traffic=="Down") {
      downloadValueLast = downloadValueNow;
      downloadValueNow = value;
      downloadTimeLast = downloadTimeNow;
      downloadTimeNow = inNow;
      
//      downloadRate = (double) 8/1000 * (downloadValueNow-downloadValueLast) / (downloadTimeNow-downloadTimeLast); // Result in Mbps
      downloadRate = 1000 * (downloadValueNow-downloadValueLast) / (downloadTimeNow-downloadTimeLast);

      Serial.print( " [XX] downloadRate = ");
      Serial.print(downloadRate);
    return true;
    }
   else if (traffic=="Up") {
      uploadValueLast = uploadValueNow;
      uploadValueNow = value;
      uploadTimeLast = uploadTimeNow;
      uploadTimeNow = inNow;
      
 //     uploadRate = (double) 8/1000 * (uploadValueNow-uploadValueLast) / (uploadTimeNow-uploadTimeLast); // Result in Mbps
      uploadRate = 1000 * (uploadValueNow-uploadValueLast) / (uploadTimeNow-uploadTimeLast);

      Serial.print( " - uploadRate = ");
      Serial.print(uploadRate);
      
      return true;
    }

  }
  return false; 
}
