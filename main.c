/*************************************************************************************************************
-----------------------------------PWM GENERAZIONE DI SEGNALI TRAMITE PWM-------------------------------------
----------------------------------------Comando motore mediante PWM-------------------------------------------

++++++++++++++++++++++++++++++++++++++++++++REQUISITI DI PROGETTO+++++++++++++++++++++++++++++++++++++++++++++
Programma che genera un segnale PWM per il pilotaggio di in motore a velocità variabile.
La velocità del motore è proporzionale al duty-cycle del segnale PWM e può essere variata da 0 a
100% a passi di 1%. La velocità può essere impostata in due modi: 1) localmente, mediante tre
selettori esterni a dip switch (uno per ogni cifra decimale); 2) da remoto, mediante comando
inviato tramite terminale RS232. Un apposito selettore esterno permette di fissare la priorità tra
velocità impostata da locale e velocità impostata da remoto.

N.B.: i selettori possono essere sostituiti con semplici fili verso massa o alimentazione.
Il progetto necessita di scheda Xplained Mini e di oscilloscopio/frequenzimetro per misurare la
frequenza del segnale PWM. Una versione “light”, senza la parte RS232, può essere realizzata
con il solo simulatore.

+++++++++++++++++++++++++++++++++++++++++FUNZIONALITA' DEL PROGRAMMA+++++++++++++++++++++++++++++++++++++++++
Tramite terminale:
-Se viene scritto il comando "up", ho un incremento del Duty Cycle pari al 1%
-Se viene scritto il comando "down", ho un decremento del Duty Cycle pari al 1%

Tramite Dip Switch:
-Ho uno switch per ogni cifra decimale (tre Dip Switch)

Devo permettere di selezionare i due metodi, che sono mutualmente esclusivi. Per garantire l'esclusività,
utilizzo il pulsante sul pin 7 del port B: in questo modo scelgo di abilitare la modifica di duty cycle
attraverso terminale. Per via software si esclude successivamente, in questo caso, l'inserimento del Duty Cycle
via selettore esterno.

*************************************************************************************************************/

#define F_CPU 16000000UL
#define BAUD 9600
#define MAX_STR_LEN 60
#define Ttoggle 100

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h> // contiene funzioni varie per gestire l'input/output (es. sprintf(), sscanf()...)
#include <string.h> // contiene funzioni varie per manipolare le stringhe (es. strlen(), strcmp()...)
#include <util/setbaud.h> // contiene l'utility per il calcolo di UBRR_VALUE a partire da F_CPU e BAUD
#include <util/delay.h>
#include <math.h>

//Parto con l'interfaccia.
//Decido di cambiare la velocità del motore in due modi: 'up'->aumenta di 1%, 'down'->diminuisce di 1%.
//Parto con l'implementazione di queste funzioni


//Prototipi funzioni
void init(void);
void USART_init(void);
void USART_RX_string(char *, unsigned const int);
void USART_TX_string(char *);
void LedOn(void);
void LedOff(void);
int BinToDec(int[]);
int SwitchConcat(int, int, int);

//Variabili globali
//Flag di inserimento da terminale. Se '1', il terminale ha precedenza.
volatile char inserimentoDaTerminale;

//Le seguenti variabili globali vengono utilizzate per capire quale 'bit'
//del dip switch delle unità viene modificato.
volatile int units[4];
volatile int tens[4];
volatile int hundreds[1];

//Voglio creare una macchina a stati.
//Definisco allora la variabile di stato e i suoi possibili valori
volatile enum state {TerminaleAttivo, SelettoreEsternoAttivo, ModificaDCTerminale, ModificaDCSelettore} PresentState = TerminaleAttivo;
//Si inizia con l'inserimento da terminale

int main(void){
	
	init();
	USART_init();
	LedOn();
	
	char str[MAX_STR_LEN + 1]; // array per la stringa (una cella in più per ospitare il carattere terminatore di stringa)

	int u = 0;
	int ts = 0;
	int hs = 0;
	int DC = 0;
 	
	//Messaggi di benvenuto
	USART_TX_string("Comando motore mediante PWM - Frenki Shqepa");
	USART_TX_string("~~~~~~~~~~~~~~~~~Benvenuto!~~~~~~~~~~~~~~~~");
	USART_TX_string("\nPuoi scegliere due modalità di inserimento del Duty Cycle:");
	USART_TX_string("-Scrivi \"up\" per aumentare il Duty Cycle di 1%");
	USART_TX_string(" Scrivi \"down\" per decrementare il Duty Cycle di 1%");
	USART_TX_string("-Se clicchi sul pulsante presente sulla scheda Xplained Mini,");
	USART_TX_string(" devi utilizzare i Dip Switch sulla Breadboard (uno per ogni cifra)");
	
	
	while(1){
				
		switch (PresentState){
			
			case TerminaleAttivo: //L'inserimento da terminale è attivo
			USART_TX_string("\nScrivi \"up\" per aumentare il Duty Cycle di 1%");
			USART_TX_string("Scrivi \"down\" per decrementare il Duty Cycle di 1%");
			USART_RX_string(str, MAX_STR_LEN);
			
			if(!inserimentoDaTerminale){
				if(!strcmp(str, "up"))
				PresentState = ModificaDCTerminale;
				
				else if (!strcmp(str, "down"))
				PresentState = ModificaDCTerminale;
				
				else
				USART_TX_string("\n-> Comando non riconosciuto");
			}
			
			else
			PresentState = SelettoreEsternoAttivo;
			
			break;
			
			case SelettoreEsternoAttivo:
			USART_TX_string("~~~~~~~Istruzioni per l'utilizzo del Selettore Esterno~~~~~~");
			USART_TX_string("L'inserimento da selettore esterno è ora attivo.");
			USART_TX_string("Sono presenti tre Dip Switch.");
			USART_TX_string("Orienta la breadboard in modo da avere");
			USART_TX_string("il Dip Switch singolo all'estrema SX.");
			USART_TX_string("In questo modo vedrai, da SX a DX");
			USART_TX_string("i Dip Switch delle centinaia, decine, unità.");
			USART_TX_string("\nScrivi nel terminale i seguenti comandi:");
			USART_TX_string("-Scrivi \"inizio\" prima di modificare il DC.");
			USART_TX_string("-Scrivi \"fine\" quando hai modificato il DC.");
			USART_TX_string("Il Duty Cycle va da 0% a 100%, con risoluzione 1%");
			USART_TX_string("Devi usare i Dip Switch con la codifica BCD per ogni cifra");
			
			USART_RX_string(str, MAX_STR_LEN);
			
			if(!strcmp(str, "inizio"))
			PresentState = ModificaDCSelettore;
			
			else {
				USART_TX_string("Devi scrivere \"inizio\" per iniziare la modifica del DC");
				PresentState = SelettoreEsternoAttivo;
			}
			
			break;
			
			case ModificaDCTerminale:
			USART_TX_string("\n-> Duty Cycle incrementato con successo");
			USART_TX_string("\n-> Duty Cycle decrementato con successo");
			PresentState = TerminaleAttivo;
			break;
			
			case ModificaDCSelettore:
			USART_TX_string("\nScrivi \"fine\" quando hai finito la modifica del DC");
			
			USART_RX_string(str, MAX_STR_LEN);
			
			if(!strcmp(str, "fine")){
				u = BinToDec(units);
				ts = BinToDec(tens);
				hs = BinToDec(hundreds);
				
				DC = SwitchConcat(hs, ts, u);
				
				USART_TX_string("\nNuovo DC -> %DC");
				PresentState = SelettoreEsternoAttivo;
			}
			
			else{
				USART_TX_string("Il Duty Cycle non è stato modificato con successo");
				PresentState = SelettoreEsternoAttivo;
			}
			break;
		}
	}
}//Fine main

//----------------------FUNZIONI UTENTE------------------------

//Usiamo il led per capire quando avviene una corretta TX/RX
void init(void){
	//Devo impostare come uscita il pin 5 del PORTB.
	DDRB = (1<<DDB5);
	
	//Non inserisco la configurazione dei DDRC e DDRD perchè andrebbero configurati a '0'.
	
	//Devo fare il clear del bit 5 del portB, quindi usctia a zero.
	PORTB &= ~(1<<PORTB5);
	
	//Rimozione maschera all'interrupt sul pin 7
	PCMSK0 = (1<<PCINT7);

	//Abilito gli interrupt Pin Change 0, 1 e 2
	PCICR = (7<<PCIE0);
	
	
	//Rimuovo le maschere agli interrupt sui pin
	//8, 9, 10, 11
	//18, 19, 20, 21, 22
	PCMSK1 =  15<<PCINT8;
	PCMSK2 = 31<<PCINT18;
	
	//PORTCD E PORTC
	PORTD = ~( 31<<PORTD2 );
	PORTC = ~( 15<<PORTC0 );
	
	inserimentoDaTerminale = 0;
	
	sei();
	
}

//Devo inizializzare la periferica USART.
//Scelgo per il frame il formato 8N1.
void USART_init(void){
	//Imposto per prima cosa il baud rate
	UBRR0 = UBRR_VALUE;
	
	//Ora attivo la periferica di TX
	UCSR0B = (1<<TXEN0);
	
	//Imposto la modalità asincrona con 8 bit di dati, nessuna parità, 1 bit di stop
	UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);
	
	//Ci sarebbero anche altri bit da configurare, ma sono '0'.
}

//Le seguenti funzioni USART_TX_string e USART_RX_string permettono di trasmettere e ricevere caratteri attraverso RS-EIA-232.
void USART_TX_string(char *strPtr){
	//Devo trasmettere un carattere alla volta.
	//Trasmetto quindi fino al terminatore di stringa.
	
	while(*strPtr != '\0'){
		//Devo verificare che il buffer di Trasmissione non sia pieno.
		//In questo modo non sovrascrivo dati ancora da trasmettere.
		//Rimango alla linea seguente fino a che il buffer di TX si svuota.
		while (!(UCSR0A & (1<<UDRE0)));
		
		//Se si arriva a questo punto, si può proseguire con la trasmissione della stringa,
		//un carattere per volta.
		//Con la seguente linea salvo il dato da trasmettere nel registro di trasmissione.
		UDR0 = *strPtr++;
	}
	
	//Sono uscito dal ciclo, significa che sono arrivato al terminatore di linea.
	//Inserisco il "line feed (LF)", per terminare la stringa con un ritorno a capo.
	//Anche adesso però devo verificare che il buffer di trasmissione sia libero.
	while (!(UCSR0A & (1<<UDRE0)));
	
	//Posso ora trasmettere il ritorno a capo
	UDR0 = '\n';
}

void USART_RX_string(char *strPtr, unsigned const int max_char){
	//La seguente variabile serve come indice del puntatore
	unsigned int n_char = 0;
	
	//Attivo l'unità di Ricezione
	UCSR0B |= (1<<RXEN0);
	
	//Ora procedo con la lettura. Il procedimento è analogo alla Tx, ma in senso oppost.
	do{
		//Con la seguente linea verifico che il buffer di ricezione non sia pieno.
		//Nel caso lo sia, mi fermo e aspetto che si liberi spazio.
		while (!(UCSR0A & (1<<RXC0)));
		
		//Ora posso leggere il dato dal buffer
		strPtr[n_char] = UDR0;
		
		//Ora voglio filtrare caratteri stampabili da quelli non stampabili.
		
		//Se il carattere è stampabile, quindi nella tabella ASCII,
		//dopo il carattere ' space '.
		if(strPtr[n_char] >= ' ')
		n_char++;
		else
		//Se arrivo al terminatore, esco dal ciclo,
		//in modo da poter inserire il terminatore di stringa.
		if(strPtr[n_char] == '\n')
		break;
		
	}while(n_char < max_char);
	
	//Inserimento terminatore di stringa nella prima cella libera dell'array
	strPtr[n_char] = '\0';
	
	//Disattivazione unità USART di ricezione (lasciando attiva l'unità di trasmissione)
	//Svuotamento del buffer FIFO di ricezione in caso di overflow per evitare che i caratteri residui siano acquisiti al ciclo successivo
	UCSR0B &= ~(1<<RXEN0);
}

//Le seguenti funzioni LedOn e LedOff permettono all'utente di avere un feedback visivo per quanto riguarda la modalità di inserimento attiva.
//Se il led è acceso, si è nella modalità di inserimento da Terminale
//Se il led è spento, si è nella modalità di inserimento da Selettore Esterno
void LedOn(void){
	PORTB &= ~(1<<PORTB5);
	// accensione led
	PINB |= (1<<PINB5);
}

void LedOff(void){
	//Spegnimento led
	PORTB &= ~(1<<PORTB5);
}

//La seguente funzione permette di convertire un numero binario memorizzato in un array in decimale.
//Il numero nella posizione 0 dell'array è il Most Significant Bit.
//Significa quindi che il calcolo parte da quel valore.
int BinToDec(int cifreBin[]){
	
	//Calcolo la lunghezza dell'array di numeri 
	int len = sizeof(cifreBin) / sizeof(cifreBin[0]);
	
	//Nella seguente variabile salvo il numero decimale
	int dec = 0;
	
	for(int i=0; i<len; i++)
		dec += (cifreBin[i] * pow(2, len-i-1));
	
	//Questa è il numero corrispondente al numero inserito con il Dip Switch
	return dec;
}

//La seguente funzione concatena le unità con le decine e le centinaia. 
//Essendo BCD la codifica utilizzata, posso essere sicuro che l'utente debba mettere un valore tra 0 e 9 per ogni cifra
//del Numero decimale da inserire. Ottengo il numero decimale:
//moltiplicando per 100 la cifra delle centinaia
//moltiplicando per 10 la cifra delle decine
//moltiplicando per 1 la cifra delle unità
int SwitchConcat(int centinaia, int decine, int units){
	int num = 100*centinaia + 10*decine + 1*units;
	
	if(num > 100 || centinaia > 9 || units > 9){
		USART_TX_string("Devi usare la codifica BCD. Numero inserito non ammesso.");
		PresentState = SelettoreEsternoAttivo;
	}
	
	else
		USART_TX_string(("-> Duty cycle impostato al valore desiderato"));
		
	return num;
}

//La seguente ISR ha il compito di gestire la modalità di inserimento. 
//Quando il pulsante, sul pin 7 del portB, viene premuto, si passa da una modalità all'altra.
ISR(PCINT0_vect){

	//Leggo lo stato del bit 7 del portB,
	if((PINB & (1<<PINB7)) == 0){
		
		if(!inserimentoDaTerminale){ //Se il terminale è attualmente attivo
			LedOff();
			USART_TX_string("Ora sei passato all'inserimento tramite Selettore Esterno");
			PresentState = SelettoreEsternoAttivo;
		}
		else{ //Se il selettore esterno è attualmente attivo
			PresentState = TerminaleAttivo;
			USART_TX_string("\nModalità di inserimento tramite Terminale avvenuta con successo");
			LedOn();
		}
		
		inserimentoDaTerminale = !inserimentoDaTerminale;
	}
	
}

//Le due ISR seguenti hanno il compito di modificare i valori delle variabili assegnate ad ogni switch dei dip switch esterni.
//Le variabili uPC0..uPC3, daPD1..daPD4, h, sono diciarate come variabili globali, in quanto devono essere modificate dalle ISR.
ISR(PCINT1_vect){
	
	units[3] = (PINC & (1<<PINC0)) == 1;
	units[2] = (PINC & (1<<PINC1)) == 1;
	units[1] = (PINC & (1<<PINC2)) == 1;
	units[0] = (PINC & (1<<PINC3)) == 1;

}

ISR(PCINT2_vect){
	tens[3] = (PIND & (1<<PIND2)) == 1;
	tens[2] = (PIND & (1<<PIND3)) == 1;
	tens[1] = (PIND & (1<<PIND4)) == 1;
	tens[0] = (PIND & (1<<PIND5)) == 1;

	hundreds[0] = (PIND & PIND6) == 1;		
}



