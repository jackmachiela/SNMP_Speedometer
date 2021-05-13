# SNMP_Speedometer
### Speedometer for local LAN using SNMP

This project uses the SNMP protocol to interrogate the local router's bandwidth throughput on a specific port, and displays the results on two analog meters - one for uploads and one for downloads. I used an old RF Power Meter that I picked up second-hand for $5, and a WeMos Mini-D1 arduino-compatible board with the ESP8266 module on it for the Wi-Fi.

It looks something like this:

![](https://github.com/jackmachiela/SNMP_Speedometer/blob/master/Graphics/SNMP-Speedometer.gif)

Specifically, this is a stand-alone unit that doesn't require a physical connection to the PC or Router, and doesn't require a separate MQTT server. You can put it anywhere as long it has power and Wi-Fi.


My unfinished code shows that it can work, but currently it's not yet finished - it's not actually accurate in any way yet. Hit the "Watch" button if you want to be kept up to date on it.

