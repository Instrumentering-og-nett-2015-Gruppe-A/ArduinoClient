/*   Web client
  Circuit: Ethernet shield attached to pins 10, 11, 12, 13
*/
#include <SPI.h>
#include <Ethernet.h>

char id[]; // id received from server

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; // MAC: sticker on shield
IPAddress server(74,125,232,128);  // numeric IP for Google (no DNS)
IPAddress ip(192,168,0,177); // Set static IP address to use if the DHCP fails to assign

// Initialize the Ethernet client library (port 80 is default for HTTP):
EthernetClient client;

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);

  // start the Ethernet connection:
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // no point in carrying on, so do nothing forevermore:
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip);
  }
  // give the Ethernet shield a second to initialize:
  delay(1000);
  Serial.println("connecting...");

  // if you get a connection, report back via serial:
  if (client.connect(server, 80)) {
    Serial.println("connected");
    // Make a HTTP request:
//    client.println("GET /search?q=arduino HTTP/1.1");
//    client.println("Host: www.google.com");
//    client.println("Connection: close");
//    client.println();
  } 
  else {    // if you didn't get a connection to the server:
    Serial.println("connection failed");
  }
  
  // get id
  if (id == 0){ // if no id registred in memory...
    client.write(get_id); // annet navn?
    while(client.available() == 0) {
    }
    if (client.available()) {
      for(int i=0; i<client.available(); i++){
        char id[i] = client.read(); //saves next available byte in id
      }
    }
  }
}

void loop()
{
  // if there are incoming bytes available 
  // from the server, read them and print them:
  if (client.available()) {
    char c = client.read(); //reads next available byte 
    Serial.print(c);
  }

    
    
    
  // if the server's disconnected, stop the client:
  if (!client.connected()) {
    Serial.println();
    Serial.println("disconnecting.");
    client.stop();
  }
}
