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
3. Define the address of the ActiveMQ server and the topic that Ditto should publish under.
4. Define all the datarefs that the plugin should send the value out. Ditto has the ability to retry finding the dataref if that dataref is created by another plugin that loaded after Ditto. However, looking for dataref is a rather exenpsive task, so Ditto only retrying after every 5 seconds. One way to work around that is making Ditto load last by renaming the Ditto plugin folder, for example `zDitto` and copy it into aircraft folder. This way Ditto will be loaded last, after other plugins finish publishing datarefs.
5. Start X-Plane.

### Modifying datarefs/endpoints
1. Disable Ditto by using Plugin Manager in X-Plane.
2. (Optional) Pause X-Plane.
3. Modify `Datarefs.toml`.
4. Re-enable Ditto and unpause X-Plane if necessary.

## Limitations
1. Even though Ditto supports multiple clients connecting at the same time, there is currently no way to specify which client should receive which values. All clients will be receiving the same data that is output by Ditto.

## Code samples
1. Example `Datarefs.toml` content:
```
# Setting address and topic
address = "failover:(tcp://192.168.72.249:61616)"
topic = "XP-Data"

# Setting the maximum number of retry. Zero to disable retry.
retry_limit = 5 // Retry 5 times. Each time after 5 seconds

# Getting a float dataref
[[Data]] # Each dataref is an Data table
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
type = "char"

# Getting a string dataref with an offset
[[Data]]
name = "legs_offset"
string = "laminar/B738/fms/legs"
type = "char"
start_index = 2

# Getting part of string dataref
[[Data]]
name = "legs_part"
string = "laminar/B738/fms/legs"
type = "char"
start_index = 2
num_value = 10
```