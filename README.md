# competition-register-lap-timer

![Co-funded by the European Union · ROBO STEAM ACADEMY · partner logos](images/logos_strip.png)

**Erasmus+ project ROBO STEAM ACADEMY** (KA220-VET-7CF4F308) — co-funded by the European Union — <https://robosteam.eu/>

Part of the **PicoBot Teachers' Toolkit** (lesson plans, student materials, slides and guides in five languages):
<https://github.com/robosteamdev/robo-steam-academy-teachers-toolkit>

Connect a **NodeMCU ESP8266 + E18-D80NK infrared sensor** to the
[RoboSTEAM competition website](https://competitions.robosteam.eu/), copy its MAC
address, and test the sensor before installing the real timer firmware.

This sketch registers through a heartbeat every 30 seconds. Passing your hand
in front of the sensor prints `BEAM #1 crossed` messages with a counter and the
interval since the previous crossing, and blinks the onboard LED. **It sends no
lap events, records no competition times, and handles no START/STOP commands.**
The counter and intervals are local sensor diagnostics and reset on restart.

## Hardware

| Part | Details |
|---|---|
| Controller | NodeMCU ESP8266; PlatformIO board `nodemcuv2` |
| Sensor | E18-D80NK infrared obstacle sensor, powered from **3.3 V** |
| Connection | USB data cable and computer |
| Network | 2.4 GHz Wi-Fi with access to the competition broker |

### Wiring (3.3 V, the normal way)

Disconnect the USB cable before changing wires.

| Sensor wire | Connect to | Purpose |
|---|---|---|
| Brown | **3.3 V** pin of the NodeMCU, or the 3.3 V rail of the baseboard | Sensor power |
| Blue | **GND** | Ground |
| Black | **D1 (GPIO5)** | Signal (OUT) |

The sensor works from 3.3 V. Because it is powered from 3.3 V, its signal also stays at 3.3 V, which is the level the ESP8266 pin needs. No extra parts are needed.

The signal behaves as in the [competition firmware](https://github.com/robosteamdev/robosteam-lap-timer): LOW means the beam is blocked, the internal pull-up is enabled, and the debounce is 30 ms.

The wire colours above are the usual ones. If your sensor has different colours, check its label or data sheet before connecting.

### If your sensor does not work at 3.3 V

Some sensors need more than 3.3 V. In that case:

1. Power the sensor from the **5 V** pin of the baseboard, not from 3.3 V.
2. Protect pin D1, because the signal can now go up to 5 V and the ESP8266 pin only takes 3.3 V. Put a **level shifter** or a **voltage divider** between the black wire and D1. For example, 10 kΩ from the black wire to D1 and 20 kΩ from D1 to GND.
3. Keep the grounds joined.

Never connect a 5 V signal directly to D1. Ask your teacher to check the wiring if you are unsure.

### Testing the sensor

Hold your hand or an object in front of the sensor. A reflective sensor reacts to an object in its sensing area. If it reacts too early or too late, adjust the range screw on the sensor. Every time the beam is blocked, the serial monitor prints a `BEAM` line.

## Files

| File | Purpose |
|---|---|
| `platformio/platformio.ini` | NodeMCU board, speeds and PubSubClient dependency |
| `platformio/src/main.cpp` | PlatformIO entry point; includes the shared implementation |
| `platformio/include/credentials.h.example` | Dummy connection settings for PlatformIO |
| `arduino/register_my_lap_timer/register_my_lap_timer.ino` | Arduino IDE entry point |
| `arduino/register_my_lap_timer/registration.h` | Shared registration and sensor-test implementation |
| `arduino/register_my_lap_timer/credentials.h.example` | Dummy connection settings for Arduino IDE |
| `README.md`, `LICENSE`, `.gitignore`, `images/logos_strip.png` | Documentation, licence and repository files |

## How to use

### 1. Get your connection settings

Ask the competition administrator for the MQTT **TCP host, port, username and
password**. The website address is not the MQTT broker address. The program
uses plain MQTT TCP, not HTTPS, WebSockets or TLS; supply the matching port.
Use a trusted network suitable for these administrator-provided settings.

Use **2.4 GHz Wi-Fi** without a browser login page. At a new location, update
the Wi-Fi name and password and upload again. MQTT settings can stay the same
if that location can reach the same broker.

Choose one of the following installation methods. In its `credentials.h.example`,
replace **all** `your-...` values and change `MQTT_PORT` from `0` to the
administrator's port. The example deliberately cannot connect unchanged.

| Setting | What to enter |
|---|---|
| `WIFI_SSID` | Exact Wi-Fi network name |
| `WIFI_PASSWORD` | Wi-Fi password |
| `MQTT_BROKER` | Host/IP only, without a URL scheme or path |
| `MQTT_PORT` | Plain MQTT TCP port, 1–65535 |
| `MQTT_USER` | MQTT username |
| `MQTT_PASSWORD` | MQTT password |

Keep the quotes around text values; leave the port as a number. Escape a literal
quote as `\"` and a backslash as `\\` inside a C++ string. Never share
`credentials.h`, screenshots of it, or firmware binaries containing passwords.
Both credentials files are gitignored. No competition/category ID is needed.

### 2A. Upload with PlatformIO / VS Code

1. Install VS Code and the PlatformIO IDE extension.
2. Download this entire repository and keep its folders together.
3. In PlatformIO, **Open Project** and select the `platformio/` folder.
4. Copy `platformio/include/credentials.h.example` to
   `platformio/include/credentials.h`. Edit the six settings described above.
5. Connect the NodeMCU with a USB **data** cable. Select its serial port if
   PlatformIO does not detect it automatically.
6. Click **Build**, then **Upload**. Board: `nodemcuv2`. PlatformIO installs
   **PubSubClient** (`^2.8`). Wi-Fi and SNTP come with the ESP8266
   Arduino framework; no ArduinoJson or NTPClient library is needed.
7. Open **Serial Monitor at 115200 baud**, then press the board's reset button.

Terminal equivalents, from this repository root:

```powershell
pio run -d platformio
pio run -d platformio -t upload
pio device monitor -b 115200
```

The upload speed is 921600, matching the real timer project. If uploads fail,
try `upload_speed = 115200` in `platformio.ini`.

### 2B. Upload with Arduino IDE

1. Install Arduino IDE. In **Preferences → Additional Boards Manager URLs**, add
   `https://arduino.esp8266.com/stable/package_esp8266com_index.json`.
2. In **Boards Manager**, install **esp8266 by ESP8266 Community** (ESP8266 core).
3. In **Library Manager**, install **PubSubClient**, version 2.8.
   No other extra libraries are required.
4. Open `arduino/register_my_lap_timer/register_my_lap_timer.ino`.
   Keep `registration.h` alongside the sketch.
5. Copy the adjacent `credentials.h.example` to `credentials.h` in that same
   sketch folder. Edit the six settings described above. Reopen the sketch if
   necessary to see the new tab.
6. Select **Tools → Board → esp8266 → NodeMCU 1.0 (ESP-12E Module)**, and select
   the NodeMCU's **Port**. Keep the other board defaults; upload speed 115200 is
   a useful starting point.
7. Click the **checkmark button**, then **Upload**. Open **Serial Monitor**, set **115200 baud**,
   and press reset.

The `.ino` and PlatformIO entry point both use `registration.h` in the Arduino
sketch folder. This is one implementation, so both versions have the same
functions and serial messages. PlatformIO needs the complete repository;
the Arduino sketch folder can be copied independently.

### 3. Read the messages and test the sensor

Example output (the MAC and IP below are examples):

```text
=== RoboSTEAM Register My Lap Timer ===
Firmware: register-laptimer/1.0.0
+--------------------------+
| MAC: AA:BB:CC:DD:EE:FF   |
+--------------------------+
Sensor: D1/GPIO5, active-low, 30 ms debounce.
Sensor starts CLEAR. Cross the beam to test.
Wi-Fi: connecting....
Wi-Fi OK. IP: 192.0.2.10
MQTT: connecting with MAC as client ID...
Heartbeat sent (every 30 s).
Heartbeat written to broker connection (MAC AA:BB:CC:DD:EE:FF)
Confirm ONLINE on the website; claiming approves the timer.
--- Next ---
Website: https://competitions.robosteam.eu/
Sign in with a Judge or Organiser account.
Judge: Select Category -> Add my lap timer
or Organiser: Devices -> Add my lap timer
Claim lap timer by MAC: AA:BB:CC:DD:EE:FF
Test the sensor, then flash competition timer firmware:
https://github.com/robosteamdev/robosteam-lap-timer
This sketch sends no lap events or competition commands.
BEAM #1 crossed  (first crossing)
BEAM #2 crossed  (+1.84 s since the last crossing)
BEAM #3 crossed  (+2.10 s since the last crossing)
Heartbeat sent (every 30 s).
```

Move your hand into and out of the sensing area several times. Each new clear
to blocked transition should print one line and blink the onboard LED for about
120 ms. Holding an object still should not keep counting. A sensor already
blocked at boot must first clear before it can count a crossing. Both blocked
and clear states must settle for 30 ms. Sensor testing works while Wi-Fi is
connecting or retrying. MQTT connection attempts are synchronous and may
briefly pause polling; very short crossings during those attempts can be missed.
The diagnostic intervals are not competition timing measurements.

### 4. Claim it on the website, then install the real firmware

Keep the NodeMCU powered on. Sign in at
[competitions.robosteam.eu](https://competitions.robosteam.eu/) with a **Judge
or Organiser** account:

- **Judge:** open [Select Category](https://competitions.robosteam.eu/contest/judge/).
- **Organiser:** open [Devices](https://competitions.robosteam.eu/contest/organise/devices/).

On either page, find **Add my lap timer**, paste the MAC, optionally enter a
friendly name, and press **Claim lap timer**. Claiming approves the timer for
timing; ask the organiser if it is already owned, in use, or you lack access.
Check that the website shows **ONLINE**. A successful MQTT publish has no
application acknowledgement: the serial registration message means the
heartbeat was written to the network connection; the website confirms server processing.

After checking the sensor, flash the full competition firmware, repo
[`robosteam-lap-timer`](https://github.com/robosteamdev/robosteam-lap-timer), following its README.
Use the same physical NodeMCU, which keeps its
MAC address. The organiser can then assign the timer for the competition.
This registration sketch cannot run a competition or answer the website's
PING/PONG connection test; it only supplies heartbeat freshness.

## Troubleshooting

| What you see | What to check |
|---|---|
| No serial output | Correct USB data cable, serial port, monitor open at **115200**; press reset. Close other programs using that port. |
| Garbled characters only at reset | ESP8266 ROM boot messages use **74880 baud**; this brief boot garbage is normal. Sketch messages use **115200**. |
| `Edit credentials.h: #define ...` and fast LED blink | Edit each named setting, save and upload again. The program stops before networking and sensor testing. |
| Build says `credentials.h: No such file` | Copy the example into the correct folder and remove `.example` from the copy's name. Check Windows did not add `.txt`. |
| Build cannot find PubSubClient | Install the library in Arduino IDE, or build the `platformio/` project with PlatformIO. |
| PlatformIO cannot find `registration.h` | Keep the repository's `arduino/` and `platformio/` folders together. |
| Wi-Fi times out / network not found | Exact SSID and password, signal strength, **2.4 GHz**, no captive portal. The sketch retries after 5 seconds. |
| MQTT `-2` unreachable / `-4` timeout | Correct TCP host and port, network route/firewall, broker running. A website URL or TLS/WebSocket port will not work. |
| MQTT `4` bad credentials / `5` not authorised | Recheck MQTT username/password with the administrator; permission to publish the timer status topic is required. |
| Heartbeats sent but no ONLINE timer | Ask the administrator to check the server MQTT bridge, broker selection and topic access. MQTT publish success cannot confirm the database update. |
| Sensor always blocked/on | Check wiring, common ground, signal voltage/polarity and distance screw. LOW means blocked for the specified E18 model. Clear the sensing area. |
| No beam messages | D1 is **GPIO5**, not GPIO1; check the sensor power (3.3 V) and the wiring (brown 3.3 V, blue GND, black D1), adjust range, then block and clear for longer than 30 ms each. |
| Multiple messages per pass | Adjust range/alignment and check loose wires, power and reflections; each stable clear-to-blocked transition counts. |
| Website Test connection has no PONG | Expected with this sketch; check fresh ONLINE heartbeat. Full competition firmware handles PING/PONG. |
| Upload fails | Correct board/port, data cable and USB driver (CH340 or CP210x depending on board); close Serial Monitor and try slower upload speed. |

## Settings

| Setting | Meaning | Example default |
|---|---|---|
| `WIFI_SSID` | Your 2.4 GHz network name | `your-wifi-name` |
| `WIFI_PASSWORD` | Your network password | `your-wifi-password` |
| `MQTT_BROKER` | Plain TCP broker host | `your-broker-address` |
| `MQTT_PORT` | Organiser-supplied TCP port, 1–65535 | `0` — must be changed |
| `MQTT_USER` | Broker login | `your-mqtt-username` |
| `MQTT_PASSWORD` | Broker password | `your-mqtt-password` |
| `SENSOR_PIN` | Sensor input in `registration.h` | `D1` / GPIO5 |
| `SENSOR_ACTIVE_LOW` | LOW means blocked | `true` |
| `DEBOUNCE_MS` | Stable sensor state before accepting a transition | `30` ms |
| `HEARTBEAT_MS` | Registration/status interval | `30000` ms |

**Phone hotspot tip:** use 2.4 GHz or the phone's compatibility option, enable
mobile data, and keep the hotspot on. Ask the organiser for a broker reachable
from that network.

## Licence and credits

Code: MIT Licence (see `LICENSE`).
Please credit "ROBO STEAM ACADEMY, Erasmus+ project KA220-VET-7CF4F308" — <https://robosteam.eu/>.

ESP8266 Arduino core and PubSubClient are external dependencies installed by the selected toolchain; they retain their own licences.

Funded by the European Union. Views and opinions expressed are however those of the author(s) only and do not
necessarily reflect those of the European Union or the Human Resource Development Centre (HRDC). Neither the European
Union nor the granting authority can be held responsible for them.
