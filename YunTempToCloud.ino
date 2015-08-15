/**
 * Gathers ambient measurements and sends them to the Parse.com cloud provider.
 */
#include <Bridge.h>
#include <Parse.h>
#include <FileIO.h>
#include <MemoryFree.h>
#include "CloudKeys.h"
#include "DHT.h"
#include "measures.h"

#define DHTPIN 5     // what pin we're connected to
#define DHTTYPE DHT11   // DHT 11
#define POLLDELAY 600000 // Sample every 10 minues


// Initialize DHT sensor.
// Note that older versions of this library took an optional third parameter to
// tweak the timings for faster processors.  This parameter is no longer needed
// as the current DHT reading algorithm adjusts itself to work on faster procs.
DHT dht(DHTPIN, DHTTYPE);

void setup() {
  
  // initialize sensor
  dht.begin();
  
  // Bridge startup
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
  Bridge.begin();
  digitalWrite(13, HIGH);
  
  // Initialize Parse - add a CloudKeys.h file with your own  cloud keys; these are an example:
  // #define APPLICATIONID  "WGDJynlEEf4OwVDjiDENtFh09OsBIXI5dkjfj4h2j"
  // #define CLIENTKEY  "dtp9OI5LExUwD52VOQlzB1uU5xqdpqLqfjejdjcjd"
  Parse.begin(APPLICATIONID, CLIENTKEY);
  
   // Setup File IO for logging
  FileSystem.begin();
  
  delay(2000);
}

void loop() {
  
  log("freeMemory()=");
  logln(freeMemoryString());
  
  MEASURES readings = readDhtMeasurements();
  if (readings.error != "") {
    delay(10000);
    log("Error: " + readings.error);
    return;
  }
  
  log(now() + "Temp ");
  logln(String(readings.tempF));
  
  publishToParse(readings); 
  
  delay(POLLDELAY); // Poll; minumum should be 2 seconds for DHT sensor to settle down/
}

/**
 * Read some measurements from the DHT sensor and stuff them into a MEASURES struct
 */
MEASURES readDhtMeasurements() {
  MEASURES readings = {  0.0, 0.0, "" };
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  readings.humidity = dht.readHumidity();

  // Read temperature as Fahrenheit (isFahrenheit = true)
  readings.tempF = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(readings.humidity) || isnan(readings.tempF)) {
    readings.error = "Failed to read from DHT sensor!";
    return readings;
  }

  // Compute heat index in Fahrenheit (the default)
  float heatIndex = dht.computeHeatIndex(readings.tempF, readings.humidity);
  return readings;
}

/**
 * Send what we got to parse.com
 */
void publishToParse(MEASURES readings) {
  
  ParseObjectCreate create = ParseObjectCreate();
  create.setClassName("AmbientReading");
  create.add("location", "MasterBedRoom");
  create.add("locationType", "Indoor");
  create.add("temperature", readings.tempF);
  create.add("humidity", readings.humidity);
  ParseResponse response = create.send();
  
  logln("\nResponse for saving object:");
  log(response.getJSONBody());
  if (response.getErrorCode() == 0) {
     String objectId = response.getString("objectId");
     log("object id:");
     logln(objectId);
  } else {
     logln("Failed to save the object:");
  }
  response.close(); // Do not forget to free the resource 
}

/**
 * Log stuff and issue a line feed.
 */
void logln(String message) {
  log(message + "\n");
}

/**
 * Log to file system on Linino
 * Log stuff 
 */
void log(String message) {
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  // The FileSystem card is mounted at the following "/mnt/FileSystema1"
  File dataFile = FileSystem.open("/mnt/sd/datalog.txt", FILE_APPEND);

  // if the file is available, write to it:
  if (dataFile) {
    dataFile.print(message);
    dataFile.close();
  }  
  // if the file isn't open, pop up an error:
  else {
    // error: sorry, no other log to write to (yet)!
  } 
}

String now() {
   // get the time from the server:
  Process time;
  time.runShellCommand("date");
  String timeString = "";
  while (time.available()) {
    char c = time.read();
    timeString += c;
  }
  return timeString;
}

String freeMemoryString() {
  String memory = "";
  int free = freeMemory();
  memory += free;
  return memory;
}
