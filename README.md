# Ditto
<h4 align="center">An X-Plane plugin that can add/remove/swap datarefs and endpoints on-the-fly.</h4>

Ditto is an X-Plane plugin that allows the user to pause the simulator in a specific scenario to add/remove or swap the datarefs that Ditto is sending the values out. This version of Ditto will act as an ActiveMQ publisher and publish a byte message that contains serialized flexbuffers data.

Ditto uses <a href="https://google.github.io/flatbuffers/flexbuffers.html">Flexbuffers</a>, <a href="https://github.com/skystrife/cpptoml">cpptoml</a>, <a href="https://developer.x-plane.com/sdk/">X-Plane SDK</a>, <a href="https://github.com/fmtlib/fmt">{fmt}</a> and <a href="http://activemq.apache.org/components/cms/">ActiveMQ-CPP</a>.

## Installation
### Windows
If you don't want to compile the plugin by yourself, you can head over the <a href="https://github.com/phuongtran7/Ditto_ActiveMQ/releases">releases</a> tab a get a pre-compiled version.

1. Install Flatbuffers, X-Plane SDK, cpptoml and fmt, ActiveMQ-CPP with Microsoft's <a href="https://github.com/Microsoft/vcpkg">vcpkg</a>.
    * `vcpkg install flatbuffers`
    * `vcpkg install x-plane`
    * `vcpkg install cpptoml`
    * `vcpkg install fmt`
    * `vcpkg install activemq-cpp`
2. Clone the project: `git clone https://github.com/phuongtran7/Ditto_ActiveMQ`.
3. Build the project with Visual Studio.

## Usage
### Start up
1. Copy the compiled Ditto into aircraft plugin folder in X-Plane. For example, `X_Plane root/Aircraft/Laminar Research/Boeing B737-800/plugins/`.
2. Copy `Datarefs.toml` file into Ditto folder. For example, `X_Plane root/Aircraft/Laminar Research/Boeing B737-800/plugins/Ditto/`. 
3. Define the address of the ActiveMQ server and the topics that Ditto should publish under.
4. Define all the datarefs that the plugin should send the value out.
5. Start X-Plane.

### Modifying datarefs/endpoints
1. Disable Ditto by using Plugin Manager in X-Plane.
2. (Optional) Pause X-Plane.
3. Modify `Datarefs.toml`.
4. Re-enable Ditto and unpause X-Plane if necessary.

## Code samples
1. Example `Datarefs.toml` content:
```
# Setting address and topic
address = "failover:(tcp://192.168.72.249:61616)"

# Define the name of the topic(s) that the plugin will send the data to
# Every datarefs define under the same topic name will be grouped and send out to that particular topic
topic = ["Data", "Another"]

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
```