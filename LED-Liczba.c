#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>

#define LED_Direction DDRC		/* define LED Direction */
#define LED_PORT PORTC			/* define LED port */
#define LED_DIGIT_Direction DDRD		/* definicja rejestru DDR dla portu wyboru segmentu LED */
#define LED_DIGIT_PORT PORTD			/* definicja portu sterujécego wyborem aktywnego segmentu LED */
#define Switch_Port PIND

#define BAUD_PRESCALE (((F_CPU / (USART_BAUDRATE * 16UL))) - 1)

#define LedT 5

char array[]={0b10000001, 0b11001111, 0b10010010, 0b10000110, 0b11001100, 0b10100100, 0b10100000, 0b10001111, 0b10000000, 0b10000100, 0b11111110, 0b01111111};  // Znaki kolejno: 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -, kropka,

char Carray[]={0b01111111,0b10111110,0b11011100,0b11101000, 0b00000000}; // sterowanie uøywanym segmentem wyúwietlacza LED

int przedzial[]={1000,1500,2000,2500,3000,3500,4000,4500,5000};

int seriek[9]; //wprowadzone kanaly wykorzystywane do pomiarow

int i,p, prz,cz1, cz2, pom, wy1, wy2, wy3, wy4, cyf1, cyf2, liczba, czas, czas2, czas3, stan, uruchomienie, lpow, s,p,l, UwyD;

uint8_t kanal, UwyC; 

uint32_t stanADC, Nap1, Nap2;

char String[20];



void UART_init(long USART_BAUDRATE)
{
	UCSRB |= (1 << RXEN) | (1 << TXEN);
	UCSRC |= (1 << URSEL) | (1 << UCSZ0) | (1 << UCSZ1);
	UBRRL = BAUD_PRESCALE;		
	UBRRH = (BAUD_PRESCALE >> 8);
}


void UART_TxChar(char ch)
{
	while (! (UCSRA & (1<<UDRE)));
	UDR = ch ;
}

void UART_SendString(char *str)
{
	unsigned char j=0;
	
	while (str[j]!=0)		
	{
		UART_TxChar(str[j]);	
		j++;
	}
}

unsigned char UART_RxChar()
{
	while ((UCSRA & (1 << RXC)) == 0);
	return(UDR);			
}


void ADC_Init()
{
	DDRA=0x0;			
	ADCSRA = 0x87;		
	ADMUX = 0x40;		
	
}

int ADC_Read(char channel)
{
	ADMUX = 0x40;
	
	ADMUX=ADMUX|(channel & 0x0f);

	ADCSRA |= (1<<ADSC);		
	while((ADCSRA&(1<<ADIF))==0);
	
	_delay_us(10);
	return ADCW;
}

void T0_8ms() // 8 ms
{
	int internal=0;
	TCCR0=0x04;
	while(internal<11)
	{
		TCNT0=0xCE; 
		while ((TIFR&0x01)==0);
		internal++;
		TIFR=0x1;
	}
	internal=0;
	TCCR0=0;
}



void delaydisp(int czas)
{
    int d;
    for (d = 0; d < czas; d++) 
	{
		LED_PORT =array[wy4];			//Pierwszy znak od prawej - numer pomiaru w danej serii
		LED_DIGIT_PORT = Carray[0];
		T0_8ms();

		LED_PORT =array[wy3];			//Drugi znak od prawej - myúlnik
		LED_DIGIT_PORT = Carray[1];
		T0_8ms();

		LED_PORT =array[wy2];			//Trzeci znak od prawej - numer serii
		LED_DIGIT_PORT = Carray[2];
		T0_8ms();

		LED_PORT =array[wy1];			//Czwarty znak od prawej - numer serii
		LED_DIGIT_PORT = Carray[3];
		T0_8ms();

       
    }
}

void Uruchomienie_rej()
{
		LED_DIGIT_PORT = Carray[4];
		LED_PORT =array[11];
		UART_SendString("Zdalny rejestrator napiecia");	UART_TxChar(10); UART_TxChar(13);
		UART_SendString("---------------------------");	UART_TxChar(10); UART_TxChar(13);
		UART_SendString("Uruchomienie - S");	UART_TxChar(10); UART_TxChar(13); UART_TxChar(10); UART_TxChar(13);
		uruchomienie=UART_RxChar();
			
		if (uruchomienie==83)
		{
			if (ACSR&(1<<ACO))
			{
				UART_SendString("Napiecie prawidlowe");	UART_TxChar(10); UART_TxChar(13);
				stan=2;
			}
			else
			{
			UART_SendString("Napiecie za niskie, doreguluj napiecie");	UART_TxChar(10); UART_TxChar(13);
			
			}
		}
		else
		{
			UART_SendString("Nieprawidlowa komenda");	UART_TxChar(10); UART_TxChar(13);
		}
		
	
		UART_TxChar(10); UART_TxChar(13);
}

void Konfiguracja()
{
	UART_SendString("Konfiguracja rejestratora");	UART_TxChar(10); UART_TxChar(13);
	UART_SendString("---------------------------");	UART_TxChar(10); UART_TxChar(13);
	UART_SendString("Podaj liczbe powtorzen serii pomiarow(dozwolone 2-16):");	UART_TxChar(10); UART_TxChar(13);
	LED_DIGIT_PORT = Carray[0];		//Wyúwietlenie kropek na wyúwietlaczu 7 segmentowym
	LED_PORT =array[2];
	cyf1=UART_RxChar()-48;		//Wprowadzenie pierwszej cyfry liczby
	itoa(cyf1,String,10);
	UART_SendString(String);	//Wyúwietlenie wprowadzonej pierwszej cyfry
	cyf2=UART_RxChar()-48;		//Wprowadzenie drugiej cyfry liczby
	if (cyf2==-35)				//Jeøeli wciúniÍto ENTER druga cyfra to 0
	{
		cyf2=0;
	}
	else
	{
		itoa(cyf2,String,10);		//Wyúwietlenie wprowadzonej drugiej cyfry
		UART_SendString(String);
		UART_TxChar(10); UART_TxChar(13);
		cyf1=cyf1*10;
	}
	lpow=cyf1+cyf2;
	UART_TxChar(10); UART_TxChar(13);
	if (lpow>=2 && lpow<=16)
	{
		itoa(lpow,String,10);
		UART_SendString("Wpisano liczbe: ");
		UART_SendString(String); UART_TxChar(10); UART_TxChar(13);
		stan=3;
	}
	else
	{
		UART_SendString("Nieprawidlowa liczba");
		UART_SendString(String); UART_TxChar(10); UART_TxChar(13);
	}
	UART_TxChar(10); UART_TxChar(13);
}

void Wprowadzanie_kanalow()
{
	UART_SendString("dozwolone kanaly 0, 1, 3, 7; maksymalnie 9 kanalow:");	UART_TxChar(10); UART_TxChar(13);
	UART_SendString("k - zakonczenie");	UART_TxChar(10); UART_TxChar(13);
	LED_PORT =array[3];
				
	for (i=0; i<9; i++)
	{
		s=UART_RxChar()-48;
		if (s==0 || s==1 || s==3 || s==7)
		{
			seriek[i]=s;
			itoa(s,String,10);
			UART_SendString("Wpisano liczbe: ");
			UART_SendString(String); UART_TxChar(10); UART_TxChar(13);
			l=i;
		}
		else
		{	
		
			if (s==59) 
			{
				UART_SendString("Koniec "); UART_TxChar(10); UART_TxChar(13);
				stan=3;
				l=i;
				i=9;
			}
			else
			{
				UART_SendString("Wpisano nieprawidlowy kanal");	UART_TxChar(10); UART_TxChar(13);
				i--;
			}
		}

	}
	itoa(l,String,10);
	UART_SendString("Wpisano serie ");	
	UART_SendString(String);
	UART_SendString(" kanalow");	UART_TxChar(10); UART_TxChar(13);
	for (i=0; i<l; i++)
	{
		s=seriek[i];
		itoa(s,String,10);
		UART_SendString(String); 
		UART_SendString(" ");
		
	}
	UART_TxChar(10); UART_TxChar(13);
	UART_TxChar(10); UART_TxChar(13);
	stan=4;
}

void Wprowadzenie_przedzialu()
{
	LED_PORT =array[4];
	UART_SendString("Wybierz przedzial czasu pomiedzy pomiarami");	UART_TxChar(10); UART_TxChar(13);
	UART_SendString("1-1s   2-1,5s 3-2s   4-2,5s 5-3s");	UART_TxChar(10); UART_TxChar(13);
	UART_SendString("6-3,5s 7-4s   8-4,5s 9-5s");	UART_TxChar(10); UART_TxChar(13);
	p=UART_RxChar()-48;
	if (p>=1 && p<=9)
	{
		prz=p;
		itoa(p,String,10);
		UART_SendString("Wybrano przedzial: ");
		UART_SendString(String); UART_TxChar(10); UART_TxChar(13);
	}
	else
	{
		UART_SendString("Nieprawidlowy przedzial");
		UART_SendString(String); UART_TxChar(10); UART_TxChar(13);
	}
	stan=5;
	UART_TxChar(10); UART_TxChar(13);
}

void Wyswietlenie_konfiguracji()
{
	UART_SendString("Ustalono konfiguracje:");	UART_TxChar(10); UART_TxChar(13);
	UART_SendString("---------------------------");	UART_TxChar(10); UART_TxChar(13);
	UART_SendString("Liczba powtorzen serii: ");
	itoa(lpow,String,10);
	UART_SendString(String); UART_TxChar(10); UART_TxChar(13);
	UART_SendString("Seria kanalow");	UART_TxChar(10); UART_TxChar(13);
	for (i=0; i<l; i++)
		{
			s=seriek[i];
			itoa(s,String,10);
			UART_SendString(String); 
			UART_SendString(" ");
			
		}
	UART_TxChar(10); UART_TxChar(13);
	UART_SendString("Przedzial czasu miedzy pomiarami:");	
	prz=prz-1;
	cz1=przedzial[prz];
	cz1=cz1/1000;
	cz2=przedzial[prz];
	cz2=cz2%1000;
	itoa(cz1,String,10);
	UART_SendString(String);
	UART_SendString(",");
	itoa(cz2,String,10);
	UART_SendString(String);
	UART_SendString("s");
	UART_TxChar(10); UART_TxChar(13);
	UART_TxChar(10); UART_TxChar(13);
	stan=6;
}

void Pomiary()
{
	for (p=0; p<lpow; p++)	//PÍtla serii pomiarÛw
	{
		for (i=0; i<l; i++) //PÍtla pomiarÛw na kana≥ach 
		{	
			kanal=seriek[i];
			pom=pom+1;
			UART_SendString("Pomiar: ");
			itoa(pom,String,10);
			UART_SendString(String);
			UART_SendString(", ");
			UART_SendString("Kanal: ");
			itoa(kanal,String,10);
			UART_SendString(String);
			UART_SendString(", ");

			stanADC=ADC_Read(kanal);	
	
			UART_SendString("ADC: ");
			itoa(stanADC,String,10);
			UART_SendString(String);

			UART_SendString(". U= ");

			UwyC=stanADC*5/1024;
			itoa(UwyC,String,10);
			UART_SendString(String);

			UART_SendString(",");
			stanADC=5*stanADC-(UwyC*1024);

			UwyD=(stanADC*1000)/1024;
						
			itoa(UwyD,String,10);
			UART_SendString(String);

			UART_SendString(" V, ");
			czas=przedzial[prz];
			czas2=czas/100;
			czas3=czas3+czas2;
					
			cz1=czas3;
			cz1=cz1/10;
			cz2=czas3;
			cz2=cz2%10;
				
			UART_SendString("t= ");
			itoa(cz1,String,10);
			UART_SendString(String);
			UART_SendString(",");
			itoa(cz2,String,10);
			UART_SendString(String);
			UART_SendString("s");
			UART_TxChar(10); UART_TxChar(13);
			liczba=p+1;
			wy2=liczba%10;
			wy1=liczba/10;
			wy3=10;
			wy4=i+1;
			czas=czas/32;
			delaydisp(czas);				
		}
		wy1=0;		
		wy3=0;
		wy4=0;
	}
}


int main()
{
	ADC_Init();
	UART_init(9600);

	LED_Direction |= 0xff;		// define LED port direction is output 
	LED_PORT = 0xff;

	LED_DIGIT_Direction = 0b11110000; //ustawienie najstarszych linii portu D jako wyj£cia, m-odsze jako wej£cia
	LED_DIGIT_PORT = 0xff;
	
	SFIOR&=(0<<ACME);
	ACSR&=0x00;
	
	cz1=0;
	cz1=0;
	liczba=0;
	cyf1=0;
	cyf2=0;
	l=0;
	prz=0;
	czas=0;
	czas2=0;
	czas3=0;
	pom=0;
	stan=1;
	wy1=0;		
	wy2=0;
	wy3=0;
	wy4=0;
	
	
    while (1)
	{		
		
		while (stan==1) //Uruchomienie programu
		{
			Uruchomienie_rej();				
		}
		while (stan==2) //Konfiguracja liczby powtorzen w serii pomiarow
		{
			Konfiguracja();		
		}
		while (stan==3) //Wprowadzanie kanalow pomiarowych
		{
			Wprowadzanie_kanalow();	
		}
		while (stan==4) //Wprowadzenie przedzialu czasu pomiedzy pomiarami
		{
			Wprowadzenie_przedzialu();		
		}
		while (stan==5) //Wyswietlenie wprowadzonej konfiguracji programu
		{
			Wyswietlenie_konfiguracji();		
		}
		while (stan==6)
		{
			Pomiary();
			
			wy2=0;
			stan=1;	
			pom=0;
			czas=0;
			czas2=0;
			czas3=0;		
			UART_TxChar(10); UART_TxChar(13);
		}
		
	}
}	

