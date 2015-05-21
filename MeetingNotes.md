# Meeting Notes #


## 2011-02-21 ##

  * Students from ETH visit the Zurich office
  * General discussion as to how things could progress

## 2011-02-28 ##

  * Lots of new people show up
  * Define license detail -> Freedom to open-sourcing granted
  * Keep software separate from mars-rover, internal stuff, etc.
  * Write code from scratch, except where already opensourced
  * Responsibilities:
    * alexeyn, wolfb: Planner
    * emma: Skipper
    * grundmann: Helmsmann (Support: wolfb with sailing, marius)
    * code repository: uqs
    * watchdog & system reliability: bernhard
    * radio: Marius
    * sponsoring and HW
    * electronic navigational charts (wolf)
    * sensorics: Richard
    * Get a bigger room (with VC) for next meeting

## 2011-03-07 ##

  * introduced a pdb avalon
  * boat in the lobby Tuesday night
  * Planner-Skipper interface not documented (sep. meeting setup by grundmann)
  * Skipper-Helmsman interface defined, to be documented by grundmann
  * Agreed on Alexey's proposal about units: all interfaces in knots, nautical miles, degrees. All angles in [0, 360)
  * Status updates:
    * Marius: Sat receiver can give rough (eps<12nm) position
    * Steffen: Polardiagram code ready
    * Wolf: ENC available (US for free, EU at reasonable cost)
    * Emma: Interface definitions (see above)
    * Bernhard: Power System Setup, meeting at ETH. The plan is to replace the main switch
    * all sensors deliver healthy flag, all process check in at different frequencies and report life status, Luuk proposes syslog lib.
    * Richard: NMEA libraries
  * Open items (push over to next meeting):
    * shall we switch on the AIS to improve collision avoidance?
    * Who gets the permissions to operat in the start and arrival countries?
    * Who is responsible for the insurance of damages of the boat to others?
    * Warning plate/flag
  * Source code discussions:
    * On the boat: only C/C++
    * Off the boat: whatever floats your boat
    * Proposed src tree layout
      * docs/
      * planner/
      * skipper/
      * helmsman/lib/`*`.{c,h,a}
      * io/rudder/
      * io/ais/
      * io/gps/, etc.
      * radio/ (perhaps under of io?)
    * No circular dependencies, skipper may depend on helmsman or io, but not the other way round
  * Please put docs either in doc/ or on the Wiki page (which will  then end up in SVN under the wiki/ directory)
  * We decided on using Rietveld for review, people should download 'upload.py' from http://codereview.appspot.com (and create an account)
  * AI(uqs): put checkout/build instructions on the Wiki
  * AI(lvd): put up the shopping list for discussion: https://spreadsheets.google.com/a/google.com/ccc?key=0AnVYj5ZguanKdDZ4VDhydjFwR3A3T3pjTzNzbHY3R2c&hl=en#gid=0
  * AI(lvd): ask professional legal input on the broadcast AIS issue.  Sent out mail to legal@.
  * AI(all): request commit access from uqs

## 2011-03-14 ##

Status update

  * Marius: Modem connection hardware needed (25pin RS232), help by Luuk
  * Alexej, Luuk, Wolf: Update on maps and planning algorithm, code in review,
  * Bernhard: Relais controller
  * Steffen: send polar diagram code for review, added normalization and conversion routines (in experimental/user/grundmann/avalon/common)

Action items:
  * Make a demo compilation (uqs and lvd)
  * Helmsman-Skipper interface (grundmann)

## 2011-03-28 ##
  * Rough test planning proposal
    * June 1: Hardware complete
    * June 14: All software components complete
    * June 16-29: Everything put together
    * July 1-7: TestSeries1 Zurich lake
    * August 1-7: TestSeries2 on Zurich lake (or Lake Lucerne, depending on the wind)
    * End of August: Transport to Brest + test on ocean + Launch
    * September 1-4: Sail!

  * AI: get Iridium SIM card, open item, 900CHF
  * AI: follow the idea of replacing the expensive iridium phone by something else for testing (lvd, marius)
  * Addendum: test commit from SVN done on 29.3.

## 2011-04-04 ##
  * new team members Hafez, William
  * grundmann, simulation code, many bugs in reports
  * marius: position (20 nm precision) from satellite phone
    * SIM card defunct, new costs 900CHF, marius takes care of that now.
    * Marius send encode SMS code for review
  * wind, compass
    * Wolf: Is the compass calibrated? (interference with on board electronics possible)
  * Wolf started download of 90m grid data
    * buy navigational charts 100 Euro for British channel charts (IHO format)
    * we have weather and current maps
    * plus US coastal maps
    * Task: check in the maps in a known format (Wolf)
    * Task: buy charts (Wolf)
    * Get 100 Euro (Piet)
  * Logistics: William offered help
  * Richard: Was on military leave,
    * sent code for NMEA processor
    * can start on the non-NMEA sensors, access from tomorrow
  * Map code in Panner review
    * basically ok, but some doubt about proof of correctness.
    * Alexej and lvd continue on improvements
  * Modem cable and connection
  * Deadlines precision beta version of helmsman at the end of April
  * Bernhard: Reviews
  * Adam switch bought, LVD (cut-off at 20V) ordered timing delay relais
    * Financing by Piet
  * lvd expenses: (250 for power supply, 50 cable, 8.90 gender changer)

## 2011-04-11 ##
  * lvd: boat connected, ssh works (rowyourboat) search valentine for that,
    * 2 accounts: rower pw then ssh to boat: 2 logins: root and castor
    * No Go on the boat.
    * lvd will be gone for 3 weeks, spending authority delegated to Pete
    * some money (Sim card ok)
  * liam building images
  * Richard: submitted NeMEA code, almost works, get no dat for now. tried wind sensor, IMU should work
    * Bernhard : something happening during start-up, Edge port?  Richard will clarify,
    * Kendy looks into Epos library for drive interface. (The protocol is documented)
  * Wolf: no big updates, made a script, less than 3m depth is forbidden,
    * will be away for 6 weeks after next week
  * Steffen: Simulation code, aerodynamics and boat geometry done
  * Bernhard: sent some code for review, lab work, ordered a RS323-485 converter,
    * code of the process manager
    * Marius: Play with the sim card, send SMS code, in C
      * ound documentation page describing the function, will send an SMS soon.
    * Wolf: would be good to have a map application, fusion table -> map with current SMS info.
      * share position in Latitude? Just reuse the message format. XMPP?
      * Or sent everything to twitter.
    * Wojciech:
      * optimal\_angle = f(position) is the world for skipper
      * small obstacles (buoys, drilling platforms) generalize to a 10-50 mile island
    * AI: Arriving point? lvd talks to ETH
    * Wojciech: started drafting a design
      * loop woken by alarm or fixed time
    * AI: Steffen: current true wind input format average over last 15-60 minutes. Plus boat speed
    * Averaging the wind in the sensor task
    * Get raw 3D position parallel to filtered sensor-fusioned data.

## 2011-04-18 ##
  * Marius
    * Did some testing of the system by sending a text message

  * Alexey
    * finally done with source data, has depth map, has wind and tides
    * tides. best to do in the collision avoider, no best to do in the global planner
    * what about just lowering the ocean by 3M such that any points that we could run into look like islands
    * global planner is digital now

  * Testing
    * Would be good to put it in the garden for a couple of days and monitor battery life and wear
    * Lake Geneva might be good
    * Would be good to do an Atlantic test with some amount of time
    * Need to bear in mind the budget
    * Come back to this when Luke is here and need to discuss with the ETH guys

  * Emma
    * Will schedule a monthly social with ETH
    * Poster, need a team photo. Emma will schedule some time to take the photo this week.

AI(Emma): Schedule some time to take picture on Wednesday

  * Wolf
    * working on the map data
    * will make shipping lanes no go zones

  * Richard
    * sensor cable
    * looked at IMU protocol

  * Bernhard
    * bunch of useful utilities, communication
    * wrote a buffered I/O (because the standard way didn't seem to be safe)

  * Steffen
    * Finished the first model of the boat with sails and rudder and everything
    * Doing it in octave (Open Source version of matlab)
    * Going to test and debug it now

## 2011-04-27 ##

  * Bernhard
    * done with lowest level of system
    * needs config file, alarms, recovery
    * duplicate suppression in logging to avoid overloading

  * Alexey
    * digital planner needs to be put together
    * meet to discuss the collision avoider

  * Steffen
    * simulation of straight sailing
    * need

  * Marius
    * Reviews
    * patched Bernhard's client issue with the serial port to connect to the modem
    * send init commands etc.

  * Richard
    * mid May initial Garden test
    * state of the CAN interfaces, Kendy?
    * AI Richard: ask Kendy
    * finished
    * working on IMU interface
    * wind sensor cannot be read maybe it has its own power?
    * wind sensor can measure the system voltage level

Variants for interprocess communication:
  * fifo
  * UNIX named pipes
  * atomic hard links to files on RAM-disk

  * Liam OS?
    * net boot of the controller?

Juerg comes to Google this Friday.
Check with Juerg: need for more power, radio modem, hardware work

## 2011-05-09 ##

  * Bernhard
    * try to boot on Friday or today together with Liam
    * NFS mount from rowyourboat
    * core RAM disk image
    * image + ssh
    * sshd
    * multiple image
    * test image and prod image
    * How to persist?
    * Have never booted
    * Need USB drivers, hopefully this week.

  * conclusion Logging without WiFi, but to USB
  * Because we have the GSM modem working
  * sending works! (Marius)

  * Marius
    * garden tests with Satellite modem
    * Decode SMS code to be done.
    * Financing of a SIM card is solved
    * priority of dev work: 1. send and receive SMS, 2. modem

  * Luuk
    * FakeSkipper
    * FakeHelmsman
    * How arbitrate?

  * Emma
    * Poster finished!
    * every second Thursday, pub time Thursday, Rat house
    * Should have a second light as a backup

  * Alexey
    * map of Lake Zurich
    * global digital Planner ready for review

  * Richard
    * talk to SMS people
    * broadcast mechanism for sensor information
    * have wind, IMU,
    * Rudder, sail no progress, need Epos library
    * AIS no connection
    * will work some time 100% on the boat

  * Luuk
    * money expenses
    * spends a week on the boat next week

  * Ulrich
    * make/build system
    * test didn't include the lib

  * Open Problems
    * temperature in box, metal box
    * rudder slack, AI Steffen/Juerg
    * When Garden test? end of this week
    * and where?
    * AI Steffen: Send heat sticker order URL to Bernhard

## 2011-05-23 ##

  * AI for Liam:
    * move the rudder on to one com bus
    * try wind sensor with Bernhard's converter
    * write a script to detect serial ports
  * Time schedule
    * May 30: Garden test
    * June 13: Software components complete
    * June 27: All put together
    * July 7: Test test on the lake
    * August 1: Second test on lake
    * August 22: Atlantic test
  * Components:
    * Base image (owner Liam):
      * need to check it it
      * make sure we run from the flash card and not the network
      * build process for computer
      * logging config, make persistent logging
    * Rudder and sail (owner Luuk, Emmanuel):
      * check them in
      * wrap daemon to talk to tty
      * ready for garden test
    * Helmsman (owner Grundmann)
      * sail controller done
      * integration testing with the daemon
      * simple controller for stop/brake
      * simple controller for remote by sms commands
      * tack/jibe controller will not be ready for the garden test
    * Skipper (lake): ?
    * Skipper (ocean): ?
    * Joystick skipper, a.k.a. remote control?:
    * Global planner (owner Alexey):
      * have something
      * read/write maps in geotiff format
      * need more testing
      * sanity checking
    * Maps library (owner Wolf):
      * not essential for Atlantic test
      * maps for Atlantic and coastal dep/arrival
    * System management (owner Bernhard):
      * close for garden test
      * need decide what should be done for garden test
    * Communication (owner Marius):
      * send/receive SMS ready
      * simple API/impl to do
      * need to buy Iridium SIM
    * Sensors IMU, wind, AIS (owner Richard):
      * IMU/wind done
      * need AIS faking for testing
      * ready for the garden test
      * AIS not done, need to be ready for Atlantic test
    * Wiring (owner Luuk):
      * wiring changes: not essential for garden test
    * Garden test (owner Luuk):
      * simple few hours test to check everything is moving
      * multi-day test to check autonomous systems working and sustainable power
      * figure out how to get into the garden
      * getting on the lake

## 2011-05-30 ##

  * Richard
    * NMEA ready to be submitted
    * finalized can talk to IMU
    * parametrized
    * Wind data
    * need integration with system manager (handle sensor errors, corruption etc.)
    * Wind sensor outputs something, but reports invalid data.
    * Try to get rid of one converter (Bernhard)
    * Initial version of the IMU code, Ready by end of the week

  * Marius
    * SIM card valid for 1 year/500min
    * sending SMS did not succeed over mobile
    * redirect
    * Need to send email to Iridium#number to send commands
    * 3 weeks off, leaving on Thursday
    * will make message queue today
    * Mount it back on Wednesday into the boat
    * Problem Antenna does not work in the office

  * Emma
    * MTV

  * Bernhard
    * 2 CLs in review
    * can monitor process entities,
    * next device entity, hierarchy model, power zone, USB port etc.
    * escalation policy
    * Alarms
    * Messy part and tuning to be done.
    * away from June 8th-July 12th

  * Liam
    * not much
    * figure out
    * no final hardware modification
    * HW kind of ok.
    * Need build system on the boat, ETA his week
    * How to checkout Avalon on the gateway machine?
    * aim: build with uc/libc
    * Compact Flash cards arrived

  * Steffen
    * State control CL

## 2011-06-06 ##

  * demons:
    * eposcom, eposd, rudderd
    * windd
    * IMUd
    * SMSd?

  * Liam:
    * missing binaries
    * build script puts them into
    * /usr/bin
    * make install
    * config file free
    * no scripts for startup
    * config for the fm

  * 10/6 test in carpark
  * logging to USB-stick
  * Wednesday morning for helmsman-driven interface.


## 2011-06-20 ##

lvd:
  * Epos communication problems
  * lagging on schedule
  * Spot in the Berlin Hackathon end of July
  * Idea, 3-4 people to work on skipper ((grundmann) and maybe Planner)

Steffen
  * Normal controller
  * Collision avoidance algorithm
  * Away next week, wants to finish Helmsman
  * Command Interpreter for SMS heading commands for lake tests
  * Compass state, Doubt in the function of the IMU, got a new compass, has RS232 NME interface

Luuk: SMS interface into the Drive demon and to the helmsman

Critical is that we are lagging behind our schedule.
Richard intends to work more during July. Steffen will work for a week in July on Avalon.


## 2011-08-15 ##
  * POWER\_SHUTDOWN, helmsman exits, system reboots needs to kick watchdog.
  * Marius: fuel cell monitor, remote control by SMS
  * Steffen: remote control execution
  * Don't panic
    * We need to keep what we've got so far, not loosing functionality with changes (e.g. drive valid flag functions)
    * Do not repeat mistakes
      * Resolution problem with GPS latitude/longitude coordinates having 3 digits after the decimal point reappeared with the rudder angle having 1 digit after the point only.           This is an artificial stair function, an artificial non-linearity which costs us energy because the rudder cannot get to its equilibrical point and speed.
      * Rudder must not be sticky
> > > > We talked about the effect of the slack in the rudder mechanism.
> > > > Exactly the same applies the the rudder control.
> > > > The rudder control value must be passed through as it is, unchanged and
> > > > immediately.
> > > > (Currently it doesn't change, if the reference value deviates from the actual by less than 1.5 degrees,
> > > > which is about 8% of our control range. This drives the bearing controller mad
> > > > and costs a lot of energy and speed.)
      * Sail angle can be sticky (not in a closed loop, but ideally that logic should go into the sail controller where it is tested and documented.
      * Write tests
      * Use what is there, tested and tuned (e.g. boat\_model.cc)
      * The communication pattern Send-on-change-only is broken because the participants have an independant start-up and live time. Send at a fixed period.
  * This is a real-time system (at least drives, sensorics and helmsman)

> > Our software is part of the real world, has to be live at all times and the worst time latency of any operation is an essential performance factor.
> > In the closed loop control the differential equations are replaced by difference equations
> > under the assumption that the sampling period is constant.
> > This has the advantage that all coefficients of the difference equations
> > are constant, otherwise we would have to calculate them anew
> > everytime we run a the ship controller using exponential functions.
> > This means the ship control has to run at a constant (+-10%) frequency.
  * Luuk: AIS wrapper
  * Steffen: Get ETH support for today
  * Richard: AIS antenna
  * Start week: Friday 16.9. plus following week, 4 to 5 days sea testing and release
  * Emma: Fax to the police, email GlobalTag again



## 2011-09-05 ##

  * Participants: Cedric (via Skype), Jürg, Luuk, Pete, Bernhard, Marius, Richard, Emma, Julien, Steffen,
  * Status of the boat after collission on August 28th:
    * Some delamination in the boom, possibly reduced strength
    * For details seee Patricks mail from August 31st
    * The rudders have been repaired and the boat is in a sailable condition for lake tests
    * New masttop mount was finished by Patrick, Mechanical mounting of the wind sensor was tested on last Friday
    * ZSG was very friendly and will handle the scratches during normal maintanance
  * Wind sensor
    * Tested by Steffen, definitively broken

  * Original goal of participating in the MicroTransat 2011 not feasible, AI: Cedric informs the organizers
  * Both ETH-ASL and Google intend to continue with the project, the goal is to get the boat sailing and participate in (or organize ourselves) competitions and races.
  * Jürg will leave the project (starts a new job in November). He will be available for know-how-transfer for a limited time. Thanks for the huge contribution, Jürg!
  * Student support will be necessary, Cedric checks options of thesises or just student work support based on a prioritized list (from Patrick damage list): AI Steffen: Send list of important topics to Cedric



## 2011-09-12 ##

  * Participants: Luuk, Pete, Bernhard, Marius, Richard, Emma, Julien, Steffen,
  * Steffen
    * needs to test the Moxa 1130I communication (store /var/log message to know which driver was loaded)
    * Liam AI: discusses general scheme of the power supply system
    * Julien AI: secondary wind sensor mounting (we have an extra pair of wires for data, and 24V supply)