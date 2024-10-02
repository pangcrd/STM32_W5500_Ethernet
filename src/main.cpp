/*
 * W5500 Ethernet Module NTP Client example.
 * 
 * Get the time from a Network Time Protocol (NTP) time server
 * Demonstrates use of UDP sendPacket and ReceivePacket 
 * For more on NTP time servers and the messages needed to communicate with them, 
 * see http://en.wikipedia.org/wiki/Network_Time_Protocol
 */

#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <Dns.h>
#include <TimeLib.h> 
#include <Adafruit_GFX.h>    // Core graphics library by Adafruit
#include <ST7789v_arduino.h>


#define TFT_CS   PA4
#define TFT_RST  PB12// Or set to -1 and connect to Arduino RESET pin
#define TFT_DC   PA3
#define TFT_MOSI PA7  // Data out
#define TFT_SCLK PA5  // Clock out
#define LED PA8

ST7789v_arduino tft = ST7789v_arduino(TFT_DC, TFT_RST, TFT_MOSI, TFT_SCLK, TFT_CS); //for display with CS pin
// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = {
  0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02
};

// Your NTP Server
#define NTP_SERVER     "pool.ntp.org"

// Your local time zone
#define TIME_ZONE      7            

// local port to listen for UDP packets
#define LOCAL_PORT      8888

// NTP time stamp is in the first 48 bytes of the message
#define NTP_PACKET_SIZE 48

//buffer to hold incoming and outgoing packets 
byte packetBuffer[ NTP_PACKET_SIZE ];

// A UDP instance to let us send and receive packets over UDP
EthernetUDP Udp;

// Time of last packet transmission(ms)
unsigned long lastSendPacketTime = 0;

// Get Day of the week string [Sun,Mon....]
char * dow_char_EN(byte days) {
  char *dayOfWeek[] = {"CN","T2","T3","T4","T5","T6","T7"};
  return dayOfWeek[days];
}

// Get Day of the week [0-Sunday, 1-Monday etc.]
uint8_t dow(unsigned long t) {
    return ((t / 86400) + 4) % 7;
}


void showTime(char * title, time_t timet, char * dow) {

  tft.setTextSize(2);
  tft.setTextColor(0x57EA);
  tft.setTextWrap(false);
  tft.setCursor(96, 149);
  tft.print(String(day(timet)) + "/" + String(month(timet)) + "/" + String(year(timet)));
  tft.setCursor(97, 207);
  tft.print(String(hour(timet)) + ":" + String(minute(timet)) + ":" + String(second(timet)));

  //  Serial.print("[");
  //  Serial.print(dow);
  //  Serial.print("]");
  //  Serial.print(day(timet));
  //  Serial.print("/");
  //  Serial.print(month(timet));
  //  Serial.print("/");
  //  Serial.print(year(timet));
  //  Serial.println(" ");
  //  Serial.print(hour(timet));
  //  Serial.print(":");
  //  Serial.print(minute(timet));
  //  Serial.print(":");
  //  Serial.print(second(timet));
}

// send an NTP request to the time server at the given address 
void sendNTPpacket()
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE); 

  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49; 
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:        
  Udp.beginPacket(NTP_SERVER, 123); //NTP requests are to port 123
  Udp.write(packetBuffer,NTP_PACKET_SIZE);
  Udp.endPacket();

  return;
}
void GetTimeNTP(){
   
    // wait to see if a reply is available
  if ( Udp.parsePacket() ) {  
    //Serial.println("\nNTP Client");

    // We've received a packet, read the data from it
    Udp.read(packetBuffer,NTP_PACKET_SIZE);  // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);  
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;  
    // Serial.print("Seconds since Jan 1 1900 = " );
    // Serial.println(secsSince1900);               

    // now convert NTP time into everyday time:
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;     
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;  
    // print Unix time:
    // Serial.print("Unix time = ");
    // Serial.println(epoch);                               

    // Greenwich Mean Time
    uint8_t DayOfWeek = dow(epoch); 
    showTime("", epoch, dow_char_EN(DayOfWeek));

    if (TIME_ZONE != 0) {
      // Local Time
      DayOfWeek = dow(epoch + (TIME_ZONE * 60 * 60));
      showTime("",epoch + (TIME_ZONE * 60 * 60), dow_char_EN(DayOfWeek));
    }    
  }

}

void setup() {
  // Open serial communications and wait for port to open:
  delay(1000);
  Serial.begin(115200);
  //SPI.beginTransaction(SPISettings(55000000, MSBFIRST, SPI_MODE1)); 
  tft.init(320,240);  
  tft.setRotation(0);  
  tft.invertDisplay(false);  // Đảo ngược màu nếu cần
  tft.fillScreen(BLACK);

  tft.setTextColor(0x57EA);
  tft.setTextSize(2.5);
  tft.setTextWrap(false);
  tft.setCursor(33, 27);
  tft.print("W5500 STM32 NTP");

  // tft.setTextColor(0xF800);
  // tft.setCursor(60, 95);
  // tft.print("LOCAL TIME");

  tft.fillRoundRect(43, 140, 158, 26, 2, 0x6474);
  tft.fillRoundRect(57, 198, 134, 24, 2, 0x6474);


  pinMode(LED,OUTPUT);
  digitalWrite(LED, HIGH);

  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // You can use Ethernet.init(pin) to configure the CS pin
// #if defined GPIO_CS
//   Serial.print("GPIO_CS=");
//   Serial.println(GPIO_CS);
//   Ethernet.init(GPIO_CS);
// #else
  Ethernet.init(PA15);
//#endif


  // start the Ethernet connection:
  Ethernet.begin(mac) == 0;
  Ethernet.hardwareStatus() == EthernetW5500;
  
  Serial.println(Ethernet.localIP());
  Serial.println(Ethernet.subnetMask());
  Serial.println(Ethernet.gatewayIP());
  Serial.println(Ethernet.dnsServerIP());

  

  // Resolving host names
  DNSClient dns;
  IPAddress ServerIP;
  dns.begin(Ethernet.dnsServerIP());
  if(dns.getHostByName(NTP_SERVER,ServerIP) == 1) {
    //Serial.println(F("dns lookup success"));
  } else {
    //Serial.println(F("dns lookup failed"));
    while(1) { }
  }
    
  Udp.begin(LOCAL_PORT);
  //Serial.println("Started listening for response.");
}

void loop() {
  long now = millis();
  if (now - lastSendPacketTime > 8000) { // 8 seconds passed
    lastSendPacketTime = now;
    sendNTPpacket();
  }
  GetTimeNTP();
  delay(1000);
  // wait to see if a reply is available
 
}