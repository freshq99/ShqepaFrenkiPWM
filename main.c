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
#define UserTop 255
#define DInit 50 //Percento

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h> // contiene funzioni varie per gestire l'input/output (es. sprintf(), sscanf()...)
#include <string.h> // contiene funzioni varie per manipolare le stringhe (es. strlen(), strcmp()...)
#include <util/setbaud.h> // contiene l'utility per il calcolo di UBRR_VALUE a partire da F_CPU e BAUD
#include <math.h>

//Parto con l'interfaccia.
//Decido di cambiare la velocità del motore in due modi: 'up'->aumenta di 1%, 'down'->diminuisce di 1%.
//Parto con l'implementazione di queste funzioni


//----------------------PROTOTIPI FUNZIONI------------------------
void init(void);
void USART_init(void);
void USART_RX_string(char *, unsigned const int);
void USART_TX_string(char *);
void LedOn(void);
void LedOff(void);
char BinToDec(volatile char[], char);
char SwitchConcat(char, char, char);
void stato_dip_switch(void);
void timer_init(void);
void timer_off(void);
void timer_on(void);
void istruzioniSelettoreEsterno(void);
void istruzioniTerminale(void);
void benvenuto(void);
int dimensioneVettore(char *);

//----------------------VARIABILI GLOBALI------------------------

volatile unsigned char top = (unsigned char) UserTop;
volatile unsigned char valoreDC = (unsigned char) DInit; // variabile per memorizzazione percentuale duty cycle

//Flag di inserimento da terminale. Se '1', il terminale ha precedenza.
volatile char inserimentoDaTerminale;

volatile char flag_accensione = 0; //Se è a 1, vuol dire che c'è stato lo spegnimento del timer

//Le seguenti variabili globali vengono utilizzate per capire quale 'bit'
//del dip switch delle unità viene modificato.
volatile char units[4];
volatile char tens[4];
volatile char hundreds[1];

//Voglio creare una macchina a stati.
//Definisco allora la variabile di stato e i suoi possibili valori
volatile enum state {TerminaleAttivo, SelettoreEsternoAttivo, ModificaDCTerminale, ModificaDCSelettore} PresentState = TerminaleAttivo;
//Si inizia con l'inserimento da terminale


//----------------------MAIN PROGRAM------------------------
int main(void){
	
	init();
	USART_init();
	LedOn();
	//Iniziamo a far muovere il motore
	timer_init();
	
	char str[MAX_STR_LEN + 1]; // array per la stringa (una cella in più per ospitare il carattere terminatore di stringa)
	//unsigned char valoreDC = DInit; //Valore da inserire nel registro OC0B
	unsigned char PerDC;
	
	unsigned char u;
	unsigned char ts;
	unsigned char hs;
	
	//Messaggi di benvenuto
	benvenuto();
	
	stato_dip_switch();
	
	while(1){
		
		switch (PresentState){
			
			case TerminaleAttivo: //L'inserimento da terminale è attivo
				istruzioniTerminale();
				USART_RX_string(str, MAX_STR_LEN);
			
				if(!inserimentoDaTerminale){
					if((!strcmp(str, "up")) || (!strcmp(str, "down")))
						PresentState = ModificaDCTerminale;
					
					else
						USART_TX_string("\n-> Comando non riconosciuto");
				}
				
				else
					PresentState = SelettoreEsternoAttivo;
				
				break;
			
			case SelettoreEsternoAttivo:
				if(inserimentoDaTerminale){
					istruzioniSelettoreEsterno();
					
					USART_RX_string(str, MAX_STR_LEN);
					
					if(!strcmp(str, "inizio"))
					PresentState = ModificaDCSelettore;
					
					else {
						USART_TX_string("Devi scrivere \"inizio\" per iniziare la modifica del DC");
						PresentState = SelettoreEsternoAttivo;
					}
				}
				
				else
					PresentState = TerminaleAttivo;
				
				
				break;
			
			case ModificaDCTerminale:
				if(!strcmp(str, "up")){
				
					if(flag_accensione == 1){
						timer_on();
					
					}
				
					else if((valoreDC+1) > 100){
						USART_TX_string("\n-> Duty Cycle massimo raggiunto");
					}
				
					else{
						valoreDC++;
						OCR0B = ceil(valoreDC*top/100);
						PerDC = valoreDC;
						sprintf(str, "\n-> Duty Cycle aumentato a %d %%", PerDC);
						USART_TX_string(str);
					}
				}
			
				//Nella seguente condizione faccio a meno di fare un compare con str
				//perchè in questo stato ci si entra solo nella condizione in cui
				//str sia o "up" o "down"
				else{
					if(valoreDC <= 1){
						flag_accensione = 1;
						USART_TX_string("\n-> Motore Spento !");
						timer_off();
					}
				
					else{
						valoreDC--;
						OCR0B = ceil(valoreDC*top/100);
						PerDC = valoreDC;
						sprintf(str, "\n-> Duty Cycle decrementato a %d %%", PerDC);
						USART_TX_string(str);
					}
				}
				PresentState = TerminaleAttivo;
			
				break;
			
			case ModificaDCSelettore:
				USART_TX_string("\nScrivi \"fine\" quando hai finito la modifica del DC");
				
				USART_RX_string(str, MAX_STR_LEN);
				
				if(!strcmp(str, "fine")){
					u = BinToDec(units, 4);
					ts = BinToDec(tens, 4);
					hs = BinToDec(hundreds, 1);
					
					valoreDC = SwitchConcat(hs, ts, u);
					
					if(valoreDC > 100 || valoreDC > 9 || valoreDC > 9)
						USART_TX_string("Devi usare la codifica BCD. Numero inserito non ammesso.");
						
					else{
						OCR0B = ceil(valoreDC*top/100);
						PerDC = valoreDC;
						sprintf(str, "\n-> Duty Cycle impostato a %d %%", PerDC);
						USART_TX_string(str);
					}
						
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
	PCMSK0 = (129<<PCINT7);

	//Abilito gli interrupt Pin Change 0, 1 e 2
	PCICR = (7<<PCIE0);
	
	
	//Rimuovo le maschere agli interrupt sui pin
	//8, 9, 10, 11
	//18, 19, 20, 22, 23
	PCMSK1 =  15<<PCINT8;
	PCMSK2 = 39<<PCINT18;
	
	//PORTCD E PORTC
	PORTD = ~( 39<<PORTD2 );
	PORTC = ~( 15<<PORTC0 );
	
	inserimentoDaTerminale = 0;
	
	sei();
	
}

void timer_init(void){
	// impostazione del pin 5 del portD (OC0B) come uscita (gli altri pin sono ingressi di default)
	DDRD = (1<<DDD5); // equivale a DDRD = 0b00100000;
	
	// Impostazione timer T0 in modalità Fast PWM su OCOA (PD6) con TOP=UserTop e prescaler 256
	OCR0A = (char) UserTop;
	OCR0B = ceil(valoreDC*top/100);
	TCCR0A = ((1<<COM0B1)|(1<<WGM01)|(1<<WGM00)); // equivale a TCCR0A = 0b00100011;
	TCCR0B = ((1<<WGM02)|(1<<CS02)); // equivale a TCCR0B = 0b00001101;
}

void timer_off(void){
	TCCR0B = 0x00; // spegnimento timer
	
	// reset Timer T1; l'operazione deve essere "atomica"!
	// essendo eseguita all'interno di una ISR, gli interrupt sono già disabilitati
	TCNT1H = 0x00; // reset del counter T1 (parte alta)
	TCNT1L = 0x00; // reset del counter T1 (parte bassa)

	// azzeramento flag di un eventuale output compare appena occorso (l'azzeramento è ottenuto scrivendo '1' nel flag)
	TIFR1 = (1<<OCF1A); // equivale a TCCR1B = 0b00000010
}

void timer_on(void){
	// Impostazione timer T0 in modalità Fast PWM su OCOA (PD6) con TOP=UserTop e prescaler 256
	OCR0A = (char) UserTop;
	OCR0B = 0;
	TCCR0A = ((1<<COM0B1)|(1<<WGM01)|(1<<WGM00)); // equivale a TCCR0A = 0b00100011;
	TCCR0B = ((1<<WGM02)|(1<<CS02)); // equivale a TCCR0B = 0b00001101;
	flag_accensione = 0;
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

void stato_dip_switch(){
	
	units[3] = !((PINC & (1<<PINC0)) == 0);
	units[2] = !((PINC & (1<<PINC1)) == 0);
	units[1] = !((PINC & (1<<PINC2)) == 0);
	units[0] = !((PINC & (1<<PINC3)) == 0);


	tens[3] = !((PIND & (1<<PIND2)) == 0);
	tens[2] = !((PIND & (1<<PIND3)) == 0);
	tens[1] = !((PIND & (1<<PIND4)) == 0);
	tens[0] = !((PIND & (1<<PIND7)) == 0);

	hundreds[0] = !((PINB & (1<<PINB2)) == 0);
	
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

void benvenuto(){
	USART_TX_string("Comando motore mediante PWM - Frenki Shqepa");
	USART_TX_string("~~~~~~~~~~~~~~~~~Benvenuto!~~~~~~~~~~~~~~~~");
	USART_TX_string("\nPuoi scegliere due modalità di inserimento del Duty Cycle:");
	USART_TX_string("-Scrivi \"up\" per aumentare il Duty Cycle di 1%");
	USART_TX_string(" Scrivi \"down\" per decrementare il Duty Cycle di 1%");
	USART_TX_string("-Se clicchi sul pulsante presente sulla scheda Xplained Mini,");
	USART_TX_string(" devi utilizzare i Dip Switch sulla Breadboard (uno per ogni cifra)");
}

void istruzioniSelettoreEsterno(){
	USART_TX_string("~~~~~~~Istruzioni per l'utilizzo del Selettore Esterno~~~~~~");
	USART_TX_string("L'inserimento da selettore esterno è ora attivo.");
	USART_TX_string("Orienta la breadboard in modo da avere");
	USART_TX_string("il Dip Switch singolo all'estrema SX.");
	USART_TX_string("In questo modo vedrai, da SX a DX");
	USART_TX_string("i Dip Switch delle centinaia, decine, unità.");
	USART_TX_string("\nScrivi nel terminale i seguenti comandi:");
	USART_TX_string("-Scrivi \"inizio\" prima di modificare il DC.");
	USART_TX_string("-Scrivi \"fine\" quando hai modificato il DC.");
	USART_TX_string("Il Duty Cycle va da 0% a 100%, con risoluzione 1%");
	USART_TX_string("Devi usare i Dip Switch con la codifica BCD per ogni cifra");
}

void istruzioniTerminale(){
	USART_TX_string("\nScrivi \"up\" per aumentare il Duty Cycle di 1%");
	USART_TX_string("Scrivi \"down\" per decrementare il Duty Cycle di 1%");
}

//Le due ISR seguenti hanno il compito di modificare i valori delle variabili assegnate ad ogni switch dei dip switch esterni.
//Le variabili uPC0..uPC3, daPD1..daPD4, h, sono diciarate come variabili globali, in quanto devono essere modificate dalle ISR.
ISR(PCINT1_vect){
	
	units[3] = !((PINC & (1<<PINC0)) == 0);
	units[2] = !((PINC & (1<<PINC1)) == 0);
	units[1] = !((PINC & (1<<PINC2)) == 0);
	units[0] = !((PINC & (1<<PINC3)) == 0);

}

ISR(PCINT2_vect){
	tens[3] = !((PIND & (1<<PIND2)) == 0);
	tens[2] = !((PIND & (1<<PIND3)) == 0);
	tens[1] = !((PIND & (1<<PIND4)) == 0);
	tens[0] = !((PIND & (1<<PIND7)) == 0);

	hundreds[0] = !((PINB & (1<<PINB2)) == 0);
}

//returns the size of a character array using a pointer to the first element of the character array
int dimensioneVettore(char *ptr)
{
	size_t dimensione = sizeof(ptr) / sizeof(ptr[0]);
	return (int)dimensione;
}

//La seguente funzione permette di convertire un numero binario memorizzato in un array in decimale.
//Il numero nella posizione 0 dell'array è il Most Significant Bit.
//Significa quindi che il calcolo parte da quel valore.
char BinToDec(volatile char cifreBin[], char dimensione){
	
	//Nella seguente variabile salvo il numero decimale
	char dec = 0;

	for(int i=0; i<dimensione; i++){
		dec += (cifreBin[dimensione-1-i] * pow(2, i));
	}
	
	//Questa è il numero corrispondente al numero inserito con il Dip Switch
	return dec;
}

//La seguente funzione concatena le unità con le decine e le centinaia.
//Essendo BCD la codifica utilizzata, posso essere sicuro che l'utente debba mettere un valore tra 0 e 9 per ogni cifra
//del Numero decimale da inserire. Ottengo il numero decimale:
//moltiplicando per 100 la cifra delle centinaia
//moltiplicando per 10 la cifra delle decine
//moltiplicando per 1 la cifra delle unità
char SwitchConcat(char centinaia, char decine, char units){
	char num = 100*centinaia + 10*decine + 1*units;
	
	return num;
}
