//DDS with LGT8F328P
//Aldo Buson
#include <avr/power.h>
#define F_CPU 16000000L

//LGT8F328P
//Wemos TTGO XI 8F328P
//DAC output on D4 pin  
//
//
//




#define BUFFERBITS 9                          //2^9=512 byte in the buffer
#define BUFFERBYTES (1<<BUFFERBITS)           //number of bytes in the buffer
#define COUNTERBITS 16                        //bits used for phase_accumulator and phase_step
#define DECIMALBITS (COUNTERBITS-BUFFERBITS)  //7 in this case
#define INTFREQUENCY 222222                   //interrupt frequency
volatile uint8_t DDS_OUT[BUFFERBYTES];        //buffer for a full period of the signal
//contains a full period of the wave
volatile uint16_t phase_accumulator = 0,
                  phase_step = 0;//(1<<(16-BUFFERBITS));
//most significative bits of  phase_accumulator are the index in the DDS_OUT buffer. in this case 9 bits (BUFFERBITS),
// so the remaining 7 are the "decimal part" of the accumulator. at every tick of the interrupt the index
// is incremented by phase_step
//if phase_step=(1<<7) then at every step the phase_accumulator jump from one sample in DDS_OUT to the next
//if phase_step<(1<<7) then more then tick of the interrupt is required to move the
// most significative bits of phase_accumulator

//target performance is a 100Khz signal so the timer tick at 222Khz (Ftick) to provide at least 2 sample per period

//example:
//phase_accumulator=xxxxxxxxx0000000
//phase_step       =0000000010000000=(1<<7)
//with 512 tick all the period is swept so the output period is Tout=bufferzise*1/Ftick=512*1/222Khz=0,0023S -->Fout=1/0,002=434Hz
//changing phase_step the Fout change also.
//doubling phase_step Fout doubles also so freq and phase_step are proportional
//Fout=(222Khz/512)*(phase_step/(1<<7))

//Fout=(222Khz/BUFFERBYTES)*(phase_step/(1<<DECIMALBITS))
//phase_step=(Fout*(1<<DECIMALBITS))/(INTFREQUENCY/BUFFERBYTES)
//esamples:
//512*100Hz*128/222222=29,49-->rounded to 29-->Fout=(222222/512)*(29/128)=98,33



volatile unsigned int x = 0;
ISR(TIMER2_COMPA_vect)                            //isr on count=top
{
//output to DAC
  DALR =  DDS_OUT[phase_accumulator >> (COUNTERBITS - BUFFERBITS)]; //use most significative bits of the accumulator
  phase_accumulator += phase_step ;
}


//enable interrupt for INTFREQUENCY frequency
void init_interrupt() {
  cli();


  ASSR &= ~(1 << AS2);                              //prescaler source clock set to internal clock

  //1 clk t2s
  //2 clk t2s / 8, from prescaler
  //3 clk t2s / 32, from prescaler
  //4 clk t2s / 64, from prescaler
  //5 clk t2s / 128, from prescaler
  //6 clk t2s / 256, from prescaler
  //7 clk t2s / 1024, from prescaler

  TCCR2A = (1 << WGM21);                            // WGM22=0 + WGM21=1 + WGM20=0 = Mode2 (CTC)
  TCCR2B = (0 << CS22) | (1 << CS21) | (0 << CS20); // set prescaler to 16/8-->2Mhz
  TCNT2 = 0;                                        //clear counter
  OCR2A = (9) - 1;                                  // set top--> count up to 10 -->222222hz
  sei();
  TIMSK2 |= (1 << OCIE2A);                          //start interrupt
}



//create a sinewave
void DDS_sin()
{
  for (int i = 0; i < BUFFERBYTES; i++)
  {
    DDS_OUT[i] = 128 + 127.0 * sin(i * 2.0 * 3.1415927 / (float) BUFFERBYTES )  ;
  }
}

//compute phasestep according to frequency
void DDS_freq(long freq)
{
  phase_step =   ((double)BUFFERBYTES *  (double)freq * (double)(1 << DECIMALBITS) ) / (double)INTFREQUENCY;

}



void setup() {
  //clock_prescale_set(clock_div_1);                //32mhz cpu clock
  clock_prescale_set(clock_div_2);                  //16mhz cpu clock DEFAULT
  
  // DACEN: enable DAC
  // DAOE: let the DAC output reach DAO port (PD4)
  DACON = (1 << DACEN) | (1 << DAOE);

  DDS_sin();        //create waveform. 1 period
  
  DDS_freq(55000);  //set frequency
   
 

  init_interrupt();


}




void loop() {

}
