 <h1>Motor control via PWM</h1>. 

 <h2>Project Requirements</h2> 
 
Program that generates a PWM signal to drive a variable speed motor.
The speed of the motor is proportional to the duty-cycle of the PWM signal and can be varied from 0 to
100% in steps of 1%. The speed can be set in two ways: 
1. Locally, using three external dip switches (one for each decimal place); 
2. Remotely, by means of a command sent via RS232 terminal. A special external selector allows you to set the priority between
speed set locally and speed set remotely.

N.B.: the selectors can be replaced with simple wires to ground or power supply.
The project requires Xplained Mini board and oscilloscope/frequency meter to measure the frequency of the
frequency of the PWM signal.

 <h2>Program Functionality</h2> 
 
Via terminal:
* If the command "up" is written, I have an increase of the Duty Cycle equal to 1%.
* If the command "down" is written, I have a decrease of the Duty Cycle equal to 1%.

Through Dip Switch:
* I have a switch for each decimal digit (three Dip Switches).

I must allow to select the two methods, which are mutually exclusive. To guarantee exclusivity
I use the button on pin 7 of port B: in this way I choose to enable the duty cycle modification
through terminal. By software, in this case, the insertion of Duty Cycle is subsequently excluded
via external selector.


Translated with www.DeepL.com/Translator (free version)
