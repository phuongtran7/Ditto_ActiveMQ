# Ditto
<h4 align="center">An X-Plane plugin that can add/remove/swap datarefs and endpoints on-the-fly.</h4>

Ditto is an X-Plane plugin that allows the user to pause the simulator in a specific scenario to add/remove or swap the datarefs that Ditto is sending the values out. This version of Ditto will act as a MQTT publisher and publish a byte message that contains serialized flexbuffers data. Ditto also has the ability to write the data back into X-Plane dataref by subscribing to another topic if defined in the config file.

Ditto is mainly tested with [ActiveMQ Artemis](https://activemq.apache.org/components/artemis/), hence the name. However, any MQTT broker, for example: [Eclipse Mosquitto](https://mosquitto.org/) and [HiveMQ](https://github.com/hivemq/hivemq-community-edition), that supports MQTT v3.1.1 should work as well.

Ditto uses [Flexbuffers](https://google.github.io/flatbuffers/flexbuffers.html), [{fmt}](https://github.com/fmtlib/fmt), [cpptoml](https://github.com/skystrife/cpptoml), [X-Plane SDK](https://developer.x-plane.com/sdk/), [Eclipse Paho MQTT C++ Client Library](https://github.com/eclipse/paho.mqtt.cpp).

## Installation
### Windows
If you don't want to compile the plugin by yourself, you can head over the [releases](https://github.com/phuongtran7/Ditto_ActiveMQ/releases) tab a get a pre-compiled version.

1. Install Flatbuffers, X-Plane SDK, cpptoml and fmt, ActiveMQ-CPP with Microsoft's [vcpkg](https://github.com/Microsoft/vcpkg).
    * `vcpkg install flatbuffers`
    * `vcpkg install x-plane`
    * `vcpkg install cpptoml`
    * `vcpkg install fmt`
    * `vcpkg install paho-mqttpp3`
2. Clone the project: `git clone https://github.com/phuongtran7/Ditto_ActiveMQ`.
3. Build the project with Visual Studio.

## Usage
### Start up
1. Copy the compiled Ditto into aircraft plugin folder in X-Plane. For example, `X_Plane root/Aircraft/Laminar Research/Boeing B737-800/plugins/`.
2. Copy `Datarefs.toml` file into Ditto folder. For example, `X_Plane root/Aircraft/Laminar Research/Boeing B737-800/plugins/Ditto/`. 
3. Define the address of the MQTT broker and the topics that Ditto should publish under.
4. Define all the datarefs that the plugin should send the value out.
5. Start X-Plane.

### Modifying datarefs/endpoints
1. Disable Ditto by using Plugin Manager in X-Plane.
2. (Optional) Pause X-Plane.
3. Modify `Datarefs.toml`. Note: the config file can be named anything as long as the extension is `.toml`.
4. Re-enable Ditto and unpause X-Plane if necessary.

## Code samples
1. Example `Datarefs.toml` content:
```
# Setting address and topic
address = "tcp://192.168.72.249:1883"

# Define the name of the topic(s) that the plugin will send the data to
# Every datarefs define under the same topic name will be grouped and send out to that particular topic
publish_topic = ["Data", "Another"]

# Define the name of the topic(s) that the plugin will receive data from
# The plugin will expect to receive a Flexbuffers map that contain the data corresponding to the dataref
# name defined in the topic
subscribe_topic = ["InData"]

# Getting a float dataref
[[Data]] # Name of the topic that this dataref should be published under
name = "airspeed" # User specify name of the dataref, which will be used to access data later
string = "sim/flightmodel/position/indicated_airspeed" # Dataref
type = "float" # Type of the dataref. Can be either "int", "float" or "char"

# Getting a float array dataref
[[Data]]
name = "fuel_flow"
string = "sim/cockpit2/engine/indicators/fuel_flow_kg_sec"
type = "float"
# If "start_index" and "num_value" present in the table, that dataref will be treated as an array of value
start_index = 0 # The start index of the array that Ditto should start reading from
num_value = 2 # Number of value after the start_index Ditto should read.

# Getting a string dataref
[[Data]]
name = "legs_full"
string = "laminar/B738/fms/legs"
type = "string"

# Getting a string dataref with an offset
[[Data]]
name = "legs_offset"
string = "laminar/B738/fms/legs"
type = "string"
start_index = 2

# Getting part of string dataref
[[Data]]
name = "legs_part"
string = "laminar/B738/fms/legs"
type = "string"
start_index = 2
num_value = 10

# Getting a string dataref and send it to "Another" topic instead of "Data" topic
[[Another]]
name = "legs_full"
string = "laminar/B738/fms/legs"
type = "string"

# The plugin will write the received data to this dataref
[[InData]]
name = "nav1_freq_hz"
string = "sim/cockpit/radios/nav1_freq_hz"
type = "int"
```

2. Ditto expected data if a subscribing topic is defined
```cpp
flexbuffers::Builder fbb;
auto map_start = fbb.StartMap();
fbb.Int("nav1_freq_hz", 12345);
fbb.EndMap(map_start);
fbb.Finish();
```

## Notes
At the moment of this writing (February 11th, 2012), there is a bug in C++ Flexbuffers implementation. It will cause a crash when reading the buffer that was created by Ditto in C# using [FlexBuffers-CSharp](https://github.com/mzaks/FlexBuffers-CSharp). The bug is filled [here](https://github.com/mzaks/FlexBuffers-CSharp/issues/1), and already [fixed](https://github.com/google/flatbuffers/issues/5760) in newer version of Flexbuffers (newer than 1.10, which is currently used in vcpkg).