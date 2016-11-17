#include <SoftwareSerial.h>
#define WIFI_SSID        "oksbwn-world"
#define WIFI_PASS        "omisoksbwn" // My luggage has the same combination!
#define SERVER_IP     "192.168.0.1"
#define TIMEOUT     5000 // mS
#define CONTINUE    false
#define HALT        true
#define NO_OF_PARAMETERS 1

// #define ECHO_COMMANDS // Un-comment to echo AT+ commands to serial monitor

SoftwareSerial esp8266SerialPort(10, 11); // RX, TX

void exception(String msg){ //Exception occured. Print msg and stops.
  Serial.println(msg);
  Serial.println("HALT");
  while(true){
   readResponseData("");
   delay(60000);
  };
}
boolean readResponseData(String keyword){ //Receive data from the serial port. Returns true if keyword matches.
  String response;
  long deadline = millis() + TIMEOUT;
  while(millis() < deadline){
    if (esp8266SerialPort.available()){
      char ch = esp8266SerialPort.read();
      response+=ch;
      if(keyword!=""){
        if(response.indexOf(keyword)>0){
          Serial.println(response);
          return true;
        }
      }
    }
  }
  Serial.println(response);
  return false;
}

boolean sendCommand(String command, String acknowledgement, boolean stopOnError)
{
  esp8266SerialPort.println(command);
  #ifdef ECHO_COMMANDS
  Serial.print("--"); 
  Serial.println(command);
  #endif

  if (!readResponseData(acknowledgement)) 
    if (stopOnError)
      exception(command+" Failed to execute.");
    else
      return false;            // Let the caller handle it.
  return true;                   // ack blank or ack found
}

boolean initializeESP8266Module(){
  esp8266SerialPort.begin(9600);  
  esp8266SerialPort.setTimeout(TIMEOUT);
  delay(2000);

  //sendCommand("AT+RST", "ready", HALT);    // Reset & test if the module is ready  
  sendCommand("AT+GMR", "OK", CONTINUE);   // Retrieves the firmware ID (version number) of the module. 
  sendCommand("AT+CWMODE?","OK", CONTINUE);// Get module access mode. 
  sendCommand("AT+CWMODE=1","OK", HALT);    // Station mode
  sendCommand("AT+CIPMUX=1","OK", HALT);    // Allow multiple connections (we'll only use the first).

  String cmd = "AT+CWJAP=\""; cmd += WIFI_SSID; cmd += "\",\""; cmd += WIFI_PASS; cmd += "\"";
  for(int counter=0;counter<5;counter++){
    if (sendCommand(cmd, "OK", CONTINUE)){ // Join Access Point
      Serial.println("Connected to WiFi.");
      break;
    }else if(counter==4)
      exception("Connection to Wi-Fi failed. Please Check");
  }
  
  delay(5000);

  sendCommand("AT+CWSAP=?", "OK", CONTINUE); // Test connection
  sendCommand("AT+CIFSR","OK", HALT);         // Echo IP address. (Firmware bug - should return "OK".)
}

void setup()  
{
  Serial.begin(9600);
  Serial.println("ESP8266 Demo");
  initializeESP8266Module();
  Serial.println("Module is ready.");
}

void loop() 
{
  String cmd = "AT+CIPSTART=0,\"TCP\",\""; cmd += SERVER_IP; cmd += "\",80";
  if (!sendCommand(cmd, "OK", CONTINUE)) 
    return;
  delay(2000);

  if (!sendCommand("AT+CIPSTATUS", "OK", CONTINUE))
    return;
    
  String  tag[NO_OF_PARAMETERS]={"NAME"};
  String data[NO_OF_PARAMETERS]={"my Name"};
  
  cmd = "GET /ESP8266/index.php?";
  for(int count=0;count<NO_OF_PARAMETERS;count++){
    if(count!=0)
      cmd+="&";
    cmd+=tag[count];
    cmd+="="+data[count];
  }
  Serial.println(cmd);
  cmd+=" HTTP/1.1\r\nHost: "; cmd += SERVER_IP; cmd += ":80\r\n\r\n";
  if (!sendCommand("AT+CIPSEND=0,"+String(cmd.length()), ">", CONTINUE)){
    sendCommand("AT+CIPCLOSE", "", CONTINUE);
    Serial.println("Connection timeout.");
    return;
  }

  sendCommand(cmd, "OK", CONTINUE);  // GET
  
  readResponseData("");
  exception("ONCE ONLY");
}
