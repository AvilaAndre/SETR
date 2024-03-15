


#define d1 13
#define d2 12
#define d3 11
#define d4 10


/* Define shift register pins used for seven segment display */
#define LATCH_DIO 4
#define CLK_DIO 7
#define DATA_DIO 8
 
/* Segment byte maps for numbers 0 to 9 */
const byte SEGMENT_MAP[] = {0xC0,0xF9,0xA4,0xB0,0x99,0x92,0x82,0xF8,0X80,0X90};
/* Byte maps to select digit 1 to 4 */
const byte SEGMENT_SELECT[] = {0xF1,0xF2,0xF4,0xF8};


int Count; // Stores the value that will be displayed
int A1Pressed; // Stores if the button was pressed the last interruption
int A2Pressed;
int A3Pressed;
int A4Pressed;

void WriteNumber(int Number)
{
  WriteNumberToSegment(0 , Number / 1000);
  WriteNumberToSegment(1 , (Number / 100) % 10);
  WriteNumberToSegment(2 , (Number / 10) % 10);
  WriteNumberToSegment(3 , Number % 10);
}

/* Wite a ecimal number between 0 and 9 to one of the 4 digits of the display */
void WriteNumberToSegment(byte Segment, byte Value)
{
  digitalWrite(LATCH_DIO,LOW);
  int val = SEGMENT_MAP[Value];
  if (Segment == 0 || Segment == 2) {
    val = val & 0x7F;
  }
  
  shiftOut(DATA_DIO, CLK_DIO, MSBFIRST, val);
  shiftOut(DATA_DIO, CLK_DIO, MSBFIRST, SEGMENT_SELECT[Segment] );
  digitalWrite(LATCH_DIO,HIGH);
}

void SetCount(int newCount) {
  Count = newCount;

  int minutes = Count / 1000;
  int seconds = (Count / 10) % 100;
  int milliseconds = Count % 10;

  if (seconds > 59) {
    seconds = 0;
    minutes++;
  }

  Count = minutes * 1000 + seconds * 10 + milliseconds;
}

/* 4 Tasks:
 *     T1 - T = 100ms   -> Led d1 toggle
 *     T2 - T = 200ms   -> Led d2 toggle
 *     T3 - T = 600ms   -> Led d3 toggle
 *     T4 - T = 100ms   -> Led d4 copied from button A1
 */

void t1(void) {
  if (!digitalRead(A1) && digitalRead(A1) != A1Pressed) {
    SetCount(Count+1);;
  }
  digitalWrite(d1,  digitalRead(A1));

  A1Pressed = digitalRead(A1);
}
void t2(void) {
  if (!digitalRead(A2) && digitalRead(A2) != A2Pressed) {
    SetCount(Count+10);
  }
   digitalWrite(d2,  digitalRead(A2));

  A2Pressed = digitalRead(A2);

}
void t3(void) {
  if (!digitalRead(A3) && digitalRead(A3) != A3Pressed) {
    SetCount(Count+100);
  }
   digitalWrite(d3,  digitalRead(A3));

  A3Pressed = digitalRead(A3);  
}
void t4(void) {
  SetCount(++Count);
};

void digits_display(void) {
  WriteNumber(Count%10000);
}




typedef struct {
  /* period in ticks */
  int period;
  /* ticks until next activation */
  int delay;
  /* function pointer */
  void (*func)(void);
  /* activation counter */
  int exec;
} Sched_Task_t;

#define NT 20
Sched_Task_t Tasks[NT];


int Sched_Init(void){
  for(int x=0; x<NT; x++)
    Tasks[x].func = 0;
  /* Also configures interrupt that periodically calls Sched_Schedule(). */
  noInterrupts(); // disable all interrupts
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
 
//OCR1A = 6250; // compare match register 16MHz/256/10Hz
//OCR1A = 31250; // compare match register 16MHz/256/2Hz
  OCR1A = 31;    // compare match register 16MHz/256/2kHz
  TCCR1B |= (1 << WGM12); // CTC mode
  TCCR1B |= (1 << CS12); // 256 prescaler
  TIMSK1 |= (1 << OCIE1A); // enable timer compare interrupt
  interrupts(); // enable all interrupts

  /* Set DIO pins to outputs */
  pinMode(LATCH_DIO,OUTPUT);
  pinMode(CLK_DIO,OUTPUT);
  pinMode(DATA_DIO,OUTPUT);

  Count =0;
  A1Pressed = 0;
  A2Pressed = 0;
  A3Pressed = 0;
  A4Pressed = 0;
}

int Sched_AddT(void (*f)(void), int d, int p){
  for(int x=0; x<NT; x++)
    if (!Tasks[x].func) {
      Tasks[x].period = p;
      Tasks[x].delay = d;
      Tasks[x].exec = 0;
      Tasks[x].func = f;
      return x;
    }
  return -1;
}


void Sched_Schedule(void){
  for(int x=0; x<NT; x++) {
    if(Tasks[x].func){
      if(Tasks[x].delay){
        Tasks[x].delay--;
      } else {
        /* Schedule Task */
        Tasks[x].exec++;
        Tasks[x].delay = Tasks[x].period-1;
      }
    }
  }
}


void Sched_Dispatch(void){
  for(int x=0; x<NT; x++) {
    if((Tasks[x].func)&& (Tasks[x].exec)) {
      Tasks[x].exec=0;
      Tasks[x].func();
      /* Delete task if one-shot */
      if(!Tasks[x].period) Tasks[x].func = 0;
      return; // fixed priority version!
    }
  }
}


// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(d4, OUTPUT);
  pinMode(d3, OUTPUT);
  pinMode(d2, OUTPUT);
  pinMode(d1, OUTPUT);
  Sched_Init();

  Sched_AddT(t4, 1 /* delay */,   200 /* period */);
  Sched_AddT(t3, 1 /* delay */,  10 /* period */);
  Sched_AddT(t2, 1 /* delay */,  10 /* period */);
  Sched_AddT(t1, 1 /* delay */, 10 /* period */);
  Sched_AddT(digits_display, 1, 10);
}



ISR(TIMER1_COMPA_vect){//timer1 interrupt
  Sched_Schedule();
}




// the loop function runs over and over again forever
void loop() {
  Sched_Dispatch();
}
