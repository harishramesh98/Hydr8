## Inspiration
Water is an extremely essential part of life. In our daily lives most of us use some sort of water filtration units for consuming water. Most commercially available water filters require the internal tank to be manually refilled, which can be tedious and a bit of a hassle. The filters also have varying life spans that are usage-dependent and when the filter nears the end of its life span we get sub-standard water with little to no warning. Humans need to consume anywhere between 2-3 Liters of water each day for leading a healthy lifestyle, however, based on climate and general forgetfulness most of us lose track of how much water we drink and fall short. Our product Hydr8 was designed to combat these issues

## What it does
Hydr8 consists of 2 sensors and an actuator. Together, they do the following:
1.  The Total Dissolved Solids(TDS) sensor measures the purity of water by measuring the TDS value of the water.
2. The water level sensor senses the level of water in the tank in real time.
3. If the tank is empty(i.e. below 20%) the water will start refilling with a peristaltic pump till it reaches 80%.
4. If the TDS sensor detects high level of impurities then it will abort the process of refilling until the impure water has been flushed out and the filter is replaced.
5. The system also computes the volume of water consumed by a user based on the water level.
6. All this data is then sent over the Cloud and displayed on a dashboard in real time which can be accessed remotely.
7. The Dashboard also contains a master switch that can disable the entire system remotely in case of emergencies.

## How we built it
1. Ideation and Part Selection
    The following Processing Unit was selected:
     Atmel SAMD21G
    We chose the following sensors:
     - Grove Water Level Sensor(i2c based).
     - Gravity: Analog TDS Sensor (ADC Based)
     - 6V Peristaltic Pump(with food grade piping).
     
2. Hardware Design:
    The PCB for the system was designed using Altium PCB Design Software.
    Power Architecture was modelled using TI WEBENCH.
    The following Power architecture was used:
    - The input was provided using a 3.7V lithium-Polymer battery.
    - An onboard Battery charger unit was embedded onto the PCB that would charge the battery using 
    USB.
    - A 3.3V buck converter was implemented to power the MCU and Sensors.
    - A 6V boost converter was implemented to power the pump.
    We also implemented the analog circuitry for the  TDS sensors.
    The various peripherals for the MCU such as crystal oscillators, buttons with hardware debouncing, 
    various bypass capacitors, SD card etc. was implemented.
    We also came up with the various connectors needed based on voltage and current ratings and 
    availability.
    A final ERC and DRC Check was run, the GERBER files were generated and the Bill of Materials was 
   generated. The PCB was sent for manufacturing and assembly.
3. Software Prototyping on Development boards:
    Software prototyping of the expected firmware was done using the ATSAMW25 development kit. We first implemented and tested the functionalities of the sensors using their development boards. The TDS sensor was the first to be interfaced with the SAMW25 board, following which the pump and the water-level sensor was brought into the equation. 
4. Final Prototyping
After receiving our PCBA boards, we had minimal debugging needed as our power architecture and most of the circuitry worked well. We need slight debugging for our pump driver circuit. After the board started functioning the way we needed it to, we ported the code from the SAMW25 to the board, and made minor modifications. Initially we tested out the functionality by running the program through the command line, and we later decided against this and created a separate task to handle the basic functionality of the Hydr8 program. The next step was communicating with the cloud. We used Node-red to communicate with our device over the internet and also added OTAFU using a server hosted on Azure.

## Images
<img src="https://github.com/ese5160/a13-final-submission-group-hydr8/assets/38978733/ea161ba6-e521-4fd0-982b-00dd4043a53e" width="500" height="550">
<img src="https://github.com/ese5160/a13-final-submission-group-hydr8/assets/38978733/b31415e8-6728-44b5-8c62-1ad17aa57428" width="500" height="550">
<img src="https://github.com/ese5160/a13-final-submission-group-hydr8/assets/38978733/1b3fdb6d-55d4-401d-b6c8-82b0b99d93ea" width="500" height="550">
<img src="https://github.com/ese5160/a13-final-submission-group-hydr8/assets/38978733/5feb1c9f-af59-402d-8303-3d5e3bf3277b" width="500" height="550">
<img src="https://github.com/ese5160/a13-final-submission-group-hydr8/assets/38978733/19b39558-5463-4580-a2ba-8003a5af8d9a" width="500" height="550">

## Link to Video:
[Hydr8 - Small Explanation and Demo](https://youtu.be/GcatJEDIT-s)

## Challenges we ran into
We ran into various challenges at each step of the way.
1. Due to chip shortage we had to change the ICs being used for our power architecture multiple times.
2. Some of the sensors we had, had documentation that left more to be desired.
3. Power MOSFET selected for our on-chip pump driver circuit could not be driven by the Micro-controller well enough.
4. Writing the drivers for the sensors needed some trial-and-error in order for us to figure out the functionality.
5. There was a unique bug in our water level sensor, where if it detected moisture in two disjointed locations, its measurements will be skewed.

## Accomplishments that we're proud of
1. Every component of our system worked as expected both in software and hardware.
2. In Hardware:
    Power Architecture functioned well providing accurate voltages and sufficient current.
    The onboard TDS sensor circuitry worked identical to the development board .
    The I2C sensor worked according to our expectations.
     Very minimal amount of hardware debugging was required.
3. In software:
    It was the first time that we developed, programmed and debugged a complex Embedded System 
    that not only performed complex functions but also communicated to the cloud in real time.
    We achieved concurrency using FreeRTOS.
    This was the first time we wrote a complex embedded programs that employed sensors, actuators, 
    a command line interface, Over the air Firmware Updates and Cloud Communication.
But what we are proud of the most is that we worked flawlessly as a team, achieved our set goals and got a fully functional working prototype using an actual Water Filter.

## What we learned
1. Effective embedded systems programming using FreeRTOS APIs.
2. Schematic Capture for hardware implementation for embedded systems
3.. Printed Circuit Board Design using Altium Designer Software.
4.  We learned about the importance of mutual exclusion in concurrent systems and how to prevent any race conditions when there are logical dependencies between different functionalities in the same system.
5. We were introduced to Node Red and learned how to design and implement a dashboard, create nodes and establish communications between the nodes and the embedded systems.

## What's next for Hydr8
1. Create a fully workable commercially scalable water monitoring system that is market ready.
2. Reduce the from factor of the hardware,
3. Implementing more reliable and accurate industrial level sensors.
4. Remove any flaws in the embedded code.
5.  Improve the Dashboard and User Interface and implement Machine learning models on the data collected using the system.
5.  Create Test conditions, do a complete failure analysis and verification,
6. Meet various industry standards.
7. Secure seed funding to launch the legacy that will be Hydr8
