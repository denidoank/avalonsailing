# To Do Items #

## Mechanics ##

| **Item** | **Who/when done** |
|:---------|:------------------|
| Check rudder and find reason why left rudder reacts to commands with delay | lvd/2012-04-12    |
| Change rudder axis design such that we get less slack and better adjustability of the cone wheels. The current version might be too soft (Nut/Feder connection had big slack) | not perfect, but good enough |
| Have 2 new rudder axis manufactured. Replace the old ones. | Esteemed to be not the weakest point, very expensive |
| Make IMU box fixation without magnetizable screws. |grundmann/2012-04-02 |
| Mount compass in IMU box | grundmann/2012-04-05 |
| Make watertight IMU box (currently 2 sealings leak). Test it. | grundmann/2012-03-19 |
| Fix boom end with black plastic (material purchased) | grundmann/2012-03-05 |
| Additional fixation for solar panels (each needs 2 bolts) |                   |
| One bolt in bow hatch is missing, would leak water | grundmann/2012-05-18  |
| Two bolts in starboard front hatch are missing, would leak water | grundmann/2012-05-18 |
| Pump up carriage tyres | grundmann/2012-06-15 |
| Charge accumulators | grundmann/2012-06-15 (were charged) |
| Make  2 4cm thick pads for boat trailer (that prevent rudder damage) | grundmann/2012-08-08 |

## Electric ##

| **Item** | **Who/when done** |
|:---------|:------------------|
| Remove items from watchdog box or make it functional |                   |
| Test maximum voltage and have a look with a scope at the supply voltage at full light, fully charged, drives accelerating and braking. | test done, slight push of boom gives extra 3V, need brake chopper on mains line, grundmann/2012-05-28 |
| Overvoltage discharge brake chopper 25W/50W peak. | 60W at 30.1V grundmann/2012-06-12 |
| Change or replace CPU box such that the processor gets cooling. Currently it reaches 65 degrees Celsius at the outside because the processor box and the cooling plate are separated by air. | bridged air gap with aluminium and heat conduction paste, grundmann/2012-05-28  |
| Test bilge pump (better outside than in the lobby) |  runs but does not pump, TODO: check polarity (2012-7-17) |
| Finally, remove mushroom switch (not seawater tight). | (after lake test) |
| Finally, mount new AIS antenna. (material purchased already) Test reception. | (after lake test) |
| Mount thermometers | lvd               |
| Mount external USB-stick | lvd               |
| Fix all internal wiring against motion (could damage cables |                   |


## Software ##
| **Item** | **Who/when done** |
|:---------|:------------------|
| Make helmsman less dependant on speed through water. Test in simulator. | grundmann/done and works. But the IMU speed is still worse than it could be. Checking alternative ways to calculate the speed from raw data. |
| Make compass info out of IMU data. | grundmann/compass code done 2012/03/08 |
| Make compass mix logic | grundmann/2012-05-12 |
| Set the drive current limits to reasonable values (drives are rated as 250 and 400W)  | lvd/2012-04-12    |
| Fix sail drive zero position | lvd/done          |
| The rudder angle ranges need adjustment. The values of 90 and 100 degress are just rough estimates.  | lvd/done, but the boat does not go straight, but slightly right. |
| implement and test Drive recovery from temporary outages (power dips, overtemperature) | lvd/ (should work out of the box, only needs testing) |
| Update start\_avalon.sh for compass\_cat. | grundmann/2012-05-12 |
| Fix big drive delays (currently about 360ms or 650ms for the sail drive) Should be around 200ms. | lvd/Done          |
| Fix odd sail drive behaviour. Leave brake released. Brake 10s after the last reference value change. | lvd/done          |
| Make imucfg reset the IMU if it produces invalid messages for half an hour | lvd/Done          |
| Check system power management options. Currently the computer gets rather hot when not loaded. | appears to be fixed with the cooling fin + heat paste arrangement |
| Make clean image with startup script | lvd/              |



## System and Test ##

| **Item** | **Who/when done** |
|:---------|:------------------|
| Drive tests, check response times with TestController | grundmann/2012-06-05 |
| Logging tests. Check that we can get complete logs from all processes easily and reliably. | done              |
| Wind sensor test. Check that wind sensor/ RS485 converter / driver / wind cat  work. Check that wind direction is calculated properly with fan. | grundmann/done, tested in reality |
| IMU magnetic output plus compass test. Check that the IMU box containing IMU and compass is producing reliable and correct bearing vectors. | grundmann/done, seems to work, test under extreme conditions to be done |
| Need a 2nd GPS (reason reliability, 30% noise reduction) | lvd               |