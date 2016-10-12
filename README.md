# sensorspace

sensorspace is a set of tools/applicaitons that provide integration between different sensors and platforms with a view to providing a complete "sensorspace".

sensorspace is a new project so current sensor support is limited, however, it provides the framework with which a complete sensorspace can be constructed.

Further information can be found at https://sensorspace.org.

## Current tools

### reading_mqtt
A server that allows the user to provide manual - or scripted - sensor readings via the command line, and send these readings directly to an MQTT broker as a PUBLISH packet.

### tty_mqtt
A server that fowards sensor data transmitted over a serial link, to an MQTT broker, as a PUBLISH message. The sensor data can be encapsulated in JSON, ini, format, or transmitted raw.

For the sensor data to be encapsulated in JSON, or ini, the raw sensor data from a given device needs to be decoded. With this in mind, some "glue" is required to enable this support, however, adding new devices should be trivial. Currently supported sensor devices:

* Current Cost energy monitors

It is possible to send RAW sensor data without adding additional support. In this instance, the sensor data may need to be decoded further down the line.

## sensorspace framework
The ultimate goal for the sensorspace project is to provide a complete set of tools that facilitate a complete sensorspace solution, such as:

* MQTT to RRDtool integration - to allowing sensor data to be graphed.
* MQTT to DB integration - to allow sensor data to be stored in a database, such as MySQL and SQLITE databases.
* MQTT based socket interface - to allow dumb MQTT clients to interact with the MQTT broker in much the same way that MQTT-sn works.
* MQTT based user dashboard - To display realtime sensor data to the user.
* MQTT alert application - To alert the users of changes to sensor data.

### Readings
A reading is simply a set of measurements from a given device or sensor, and have the following structure illistraited with JSON:
'''
{
  "date" : "2015-03-28 00:00:00",

    "device" :
    {
      "id"   : "9",
      "name" : "Temp recorder"
    },

    "sensors" : [
    {
      "id" : "2",
      "meas" : "5",
      "name" : "Outside Temp"
    },

    {
      "id" : "2",
      "meas" : "21",
      "name" : "Inside Temp"
    }
    ]
}
'''

The "device" object is optional, but the inclusion of such data facilitates better reporting in dumb clients. The same is true for the "name" and "id" fields.

### MQTT support
sensorspace users the uMQTT library to provide MQTT support, further information on uMQTT can be found on the github page: https://github/sjs205/uMQTT.

### Defined MQTT Topics
The sensorspace project defines the MQTT topics:

* $SENSORSPACE/error/device/<device_id>/<device_name>
 * Device based error reporting

* sensorspace/reading/[+location+]/<device_id>/<device_name>
 * Readings from a device

#### To be implemented

* sensorspace/device/<device_id>/<device_name>
* sensorspace/sensor/<sensor-id>/<sensor-name>
 * Device and sensor information.
 * Where possible, this information should be published by individual sensors, devices.

* sensorspace/<measurement-type>/[+location+]/<sensor-id>/<sensor-name>
 * Single sensor measurement

* sensorspace/calc/<calc-type>/<measurement-type>/<sensor-id>/<sensor-name>
 * Calculation for a sensor or device

* sensorspace/hist/
 * Historic data. Topic structure TBD
