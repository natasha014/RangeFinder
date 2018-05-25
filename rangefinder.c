/********************************************
 *
 *  Name:Natasha Thakur
 *  Section:Wednesday 12:30
 *  Assignment: Project
 *
 ********************************************/

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <string.h>
#include <avr/eeprom.h>
#include "lcd.h"


// Serial communications functions and variables
void serial_init(unsigned short);
void serial_stringout(char *);
void serial_txchar(char);
char* createString(unsigned long md);

#define FOSC 16000000           // Clock frequency
#define BAUD 9600               // Baud rate used
#define MYUBRR FOSC/16/BAUD-1   // Value for UBRR0 register
#define ADC_CHAN 0              // Buttons use ADC channel 0




//volatile variables
void variable_delay_us(int delay);
volatile unsigned char stopTimer= 0b11111000;
volatile unsigned char startTimer= 0b00000010;
volatile unsigned int pulsecount;
volatile int rfState= 0;
volatile int flag = 0;
volatile int minDistance; //should this be eprom.
volatile int old_state=0;
unsigned char check;
unsigned char a, b;
int new_state;
void changeMinDistance();
volatile int sendingDataFlag=0;
volatile int changeDistanceFlag=0;
volatile char buffer[5];
volatile char startedRecievingData=0;
volatile char buffercount=0;
volatile char finishedRecievingData=0;
volatile char validData=0;


int main(void) {

    // Initialize the modules and LCD
    lcd_init();
    //enable pull-up for button inputs 
    //PORTC1 for mode PORTC2 for accquire;
    PORTC |= ((1<<PC1)|(1<<PC2));
    //PORTC3 for pulse output
    DDRC |=(1<<PC3);
    DDRB|=(1<<PB4);
    //set for LED outputs
    DDRC |= (1<<PC5);
    DDRB |= (1<<PB5);
    PORTC |=(1<<PC5);
    PORTB |=(1<<PB5);
    //enable pull-up for rotary incoder
    PORTD |=((1<<PD2)|(1<<PD3));
    unsigned char buttonPress;
    //0=single, 1=repeat
    int state =0;
    unsigned long displaycount;
    char timestring[30];       // time to display
    char outputString[30];

    //Timer Initailization
    //Counter Initialization
    //max range is 400cm. 400*58=23200us 
    //Prescalar = 8.
    //(16000000*0.0232)= 371200
    //371200/8=46400
    TCCR1B |=(1<<WGM12);
    TIMSK1 |= (1 << OCIE1A);
    OCR1A = 46400; 
    //Initially Timer is stopped.
    TCCR1B &= 0b11111000;
   
    
    //initialize the modules
    serial_init(MYUBRR);

    //Initialize interrupts to get the response
    //Initailize interrups for rotary encoders
    PCICR |= ((1<<PCIE1)|(1<<PCIE2));
    PCMSK1 |= (1 << PCINT12);
    PCMSK2 |= ((1<<PCINT18)|(1<<PCINT19));

    //enable global interrupts
    sei();

    //set min distance
    minDistance=eeprom_read_word((void *)100);
    //Splash Screen 
    lcd_writecommand(1);
    lcd_stringout("Welcome To");
    lcd_moveto(1,0);
    lcd_stringout("RangeFinder");
    _delay_ms(2000);
    lcd_writecommand(1);
    lcd_moveto(0,0);
    //display min distance
    snprintf(timestring, 30, "Min D:%9d", minDistance);
    lcd_stringout(timestring);
    _delay_ms(2000);
    lcd_writecommand(1);



     // Loop forever
    while (1) {
    buttonPress=PINC;
    
        //single mode
        if(state ==0)
        {
        lcd_moveto(0,0);
        lcd_stringout("Single Mode");
            //if change mode button is pressed
            if(!(buttonPress & (1<<PC1) ))
            {
                while(!(buttonPress & (1<<PC1) ))
                {
                    buttonPress=PINC;
                    //debouncing
                }
                state=1;
                lcd_moveto(0,0);
                lcd_stringout("                ");

                
            }
            //if accquire button is pressed
            else if(!(buttonPress & (1<<PC2) ))
            {
                while(!(buttonPress & (1<<PC2)  ))
                {

                    buttonPress=PINC;
                    //debouncing
                }
                lcd_moveto(1,0);
                PORTC |=(1<< PC3);
                _delay_us(11); //11.7 us. 
                PORTC &= ~(1<<PC3);
                lcd_moveto(1,0);
               


            }
        }
        //repeat mode
        else if(state==1)
        {
            lcd_moveto(0,0);
            lcd_stringout("repeat Mode");
            if(!(buttonPress & (1<<PC1)))
            {
                while(!(buttonPress & (1<<PC1) ))
                {

                    buttonPress=PINC;
                    //debouncing
                }
                state=0;
                lcd_moveto(0,0);
                lcd_stringout("                ");

            }
            PORTC |=(1<< PC3);
            _delay_us(11); //11.7 us. 
            PORTC &= ~(1<<PC3);
            lcd_moveto(1,0);

        }

        //if the range finder is done accquiring a reading, print it out
        if (flag) 
        {
            flag = 0;
            lcd_moveto(1,0);
            lcd_stringout("               ");
            lcd_moveto(1,0);    
            displaycount=(pulsecount/2);
            displaycount=(displaycount*10/58);
            displaycount=(unsigned int) (pulsecount*0.086);
            int decimal=displaycount%10;
            int displayDistance=displaycount/10;
            snprintf(timestring, 30, "Dist:%2d.%d", displayDistance, decimal);
            lcd_stringout(timestring);
            _delay_ms(100);
            createString(displaycount);
               
        }
        //if the minimum distance is changed, print it out
        if(changeDistanceFlag==1)
        {
            lcd_moveto(1,0);
            lcd_stringout("                ");
            lcd_moveto(1,0);
            snprintf(timestring, 30, "M:%9d", minDistance);
            lcd_stringout(timestring);
            changeDistanceFlag=0;

        }
        //if its done recieveing data print it out
        if(finishedRecievingData==1 && validData==1)
        {
            char start;
            char end;
            unsigned int recievedLength;
            sscanf(buffer, "%c %u %c", &start, &recievedLength, &end);
            char print[30];
            lcd_moveto(1,0);
            lcd_stringout("                ");
            lcd_moveto(1,0);
            snprintf(print,30 , "DR:%c%u%c",start,recievedLength,  end);
            lcd_stringout(print);
            _delay_ms(100);
            if(recievedLength<(minDistance*10))
                 play_note(349);
            if(recievedLength<displaycount)
            {
                //turn on green LED
                PORTC &= ~(1<<PC5);
            }
            else if (recievedLength> displaycount)
            {
                //turn on gree LED
                PORTB &= ~(1<<PB5);

            }
            else if(recievedLength==displaycount)
            {
                //turn on both
                PORTC &= ~(1<<PC5);
                PORTB &= ~(1<<PB5);

            }
            finishedRecievingData=0;
            startedRecievingData=0;
            validData=0;
            buffercount=0;




        }


    }
    return 0;
    
}



//ISR for interrupts form the rangefinder
ISR(PCINT1_vect) 
{
    
    
    //when ther is a rising edge, enable timer, 
    if(rfState==0)
    {
        TCNT1 = 0;
        TCCR1B |= startTimer;
        rfState=1;

    }
    //when ther is a falling edge, stop timer,
    else 
    {
        rfState=0;
        TCCR1B &=stopTimer;
        pulsecount= TCNT1;
        flag = 1;
    }
   




}


//ISC for when a falling edge was not detected in sufficient time
ISR(TIMER1_COMPA_vect){
// Range was too far
    lcd_moveto(1,0);
    lcd_stringout("                 ");
    lcd_moveto(1,0);
    lcd_stringout("Out of Range");
    _delay_ms(2000);
    lcd_moveto(1,0);
    lcd_stringout("                 ");
    TCCR1B &=stopTimer;
    TCNT1 = 0;
    rfState=0;
    pulsecount=0;

    
}



//rotatry encoder interrupt
ISR(PCINT2_vect)
{
   changeMinDistance();
   changeDistanceFlag=1;
   eeprom_update_word((void *)100, minDistance);

}



//code to control the behaviour of the rotary encoder
void changeMinDistance()
{
    check= PIND & (0b000001100);
    a= check &(1<<2);
    b= check &(1<<3);
    
    if (old_state == 0) {

        // Handle A and B inputs for state 0
        if(a && !b)
        {
            if(minDistance<400)
            minDistance++;
            else 
            minDistance=0;
            new_state=1;
        }
        if(b && !a)
        {
            if(minDistance>0)
                minDistance--;
            else
                minDistance=400;
            new_state=2;
        }

    }
    else if (old_state == 1) {

        // Handle A and B inputs for state 1
        if(a && b)
        {
            if(minDistance<400)
            minDistance++;
            else 
            minDistance=0;
            new_state=3;
        }
        if(!b && !a)
        {
            if(minDistance>0)
                minDistance--;
            else
                minDistance=400;
            new_state=0;
        }

    }
    else if (old_state == 2) {

        // Handle A and B inputs for state 2
        if(a && b)
        {
            if(minDistance>0)
                minDistance--;
            else
                minDistance=400;
            new_state=3;
        }
        if(!b && !a)
        {
            if(minDistance<400)
            minDistance++;
            else 
            minDistance=0;
            new_state=0;
        }

    }
    else {   // old_state = 3

        // Handle A and B inputs for state 3
        if(a && !b)
        {
            new_state=1;
            if(minDistance>0)
                minDistance--;
            else
                minDistance=400;
            new_state=1;
        }
        if(b && !a)
        {
            new_state=2;
            if(minDistance<400)
            minDistance++;
            else 
            minDistance=0;
                new_state=2;
        }

    }
    old_state=new_state;
    

}



//code for buzzer
void play_note(unsigned short freq)
{
    unsigned long period;
    period = 1000000 / freq;      // Period of note in microseconds

    while (freq--) {
    // Make PB4 high
     PORTB |= (1 << PB4);
     // Use variable_delay_us to delay for half the period
     variable_delay_us(period/2);
     // Make PB4 low
     PORTB &= ~(1 << PB4);
     // Delay for half the period again
     variable_delay_us(period/2);
    }
    
}


//code for buzzer
void variable_delay_us(int delay)
{
    int i = (delay + 5) / 10;

    while (i--)
        _delay_us(10);
}





//Sending and recieveing data
void serial_init(unsigned short ubrr_value)
{

    // Set up USART0 registers
      DDRB|=(1<<3);

    // Enable tri-state
      PORTB &= ~(1<<3);
    //set baud rate
    UBRR0 = MYUBRR;
    UCSR0B = (1<<RXCIE0);
    UCSR0C = (3 << UCSZ00);               // Async., no parity,
                                          // 1 stop bit, 8 data bits
    UCSR0B |= (1 << TXEN0 | 1 << RXEN0);  // Enable RX and TX

  
}

void serial_txchar(char ch)
{
    while ((UCSR0A & (1<<UDRE0)) == 0)
        { }
    UDR0 = ch;
}

void serial_stringout(char *s)
{
    int i=0;
    // Call serial_txchar in loop to send a string
    for(i=0; i<strlen(s); i++)
        serial_txchar(s[i]);

}




//code to process the recieved data
ISR(USART_RX_vect)
{
    char ch;
    
    // Get the received charater
    ch = UDR0; 
    char test[20];                
    if(ch=='@')
    {
        startedRecievingData=1;
        buffercount=0;
        validData=0;
        buffer[buffercount]=ch;
        buffercount++;

    }
    else if( ch=='$')
    {
        if(buffercount==1)
            validData=0;
        else 
        {
            validData=1;
            finishedRecievingData=1;
            buffer[buffercount]=ch;
            buffercount++;
        }
    }
    else if(buffercount>0 && (!isdigit(ch)))
    {
        startedRecievingData=0;
        buffercount=0;
        validData=0;

    }
    else 
    {
        buffer[buffercount]=ch;
        buffercount++;
    }

}



//format the string to send to the serial device and send it.
char* createString(unsigned long md)
{
   

    char add[30];
    lcd_moveto(1,0);
    snprintf(add, 40, "%s%lu%s","@", md,"$");
    serial_stringout(add);
    return add;
   
}
