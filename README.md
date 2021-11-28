 <h1>Comando motore mediante PWM </h1> 

 <h2>Requisiti di progetto</h2> 
 
Programma che genera un segnale PWM per il pilotaggio di in motore a velocità variabile.
La velocità del motore è proporzionale al duty-cycle del segnale PWM e può essere variata da 0 a
100% a passi di 1%. La velocità può essere impostata in due modi: 
1. Localmente, mediante tre selettori esterni a dip switch (uno per ogni cifra decimale); 
2. Da remoto, mediante comando inviato tramite terminale RS232. Un apposito selettore esterno permette di fissare la priorità tra
velocità impostata da locale e velocità impostata da remoto.

N.B.: i selettori possono essere sostituiti con semplici fili verso massa o alimentazione.
Il progetto necessita di scheda Xplained Mini e di oscilloscopio/frequenzimetro per misurare la
frequenza del segnale PWM. Una versione “light”, senza la parte RS232, può essere realizzata
con il solo simulatore.

 <h2>Funzionalità del programma</h2> 
 
Tramite terminale:
* Se viene scritto il comando "up", ho un incremento del Duty Cycle pari al 1%
* Se viene scritto il comando "down", ho un decremento del Duty Cycle pari al 1%

Tramite Dip Switch:
* Ho uno switch per ogni cifra decimale (tre Dip Switch)

Devo permettere di selezionare i due metodi, che sono mutualmente esclusivi. Per garantire l'esclusività,
utilizzo il pulsante sul pin 7 del port B: in questo modo scelgo di abilitare la modifica di duty cycle
attraverso terminale. Per via software si esclude successivamente, in questo caso, l'inserimento del Duty Cycle
via selettore esterno.
