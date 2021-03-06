/*
 * Implementation of a class for the management of the 16 bit Timer1 on the ATmega328P mounted on an Arduino Uno.
 * Authors: Matteo Castellucci, Giorgia Rondinini
 * Date: 17-10-2019
 */

#include "MiniTimerOne.h"

#define TIMER1_MAX_VALUE 65536UL
#define CYCLES_PER_MICROSEC 16

// A function that does nothing, useful for initializing and resetting the routine called with a timer interrupt
void emptyCallback(void) {}

MiniTimerOne *MiniTimerOne::SINGLETON {};

MiniTimerOne *MiniTimerOne::getInstance(void) {
    return SINGLETON;
}

// Default constructor
MiniTimerOne::MiniTimerOne(void) {
    this->_clockSelectBits = 0;
    this->_isrCallback = emptyCallback;
}

void MiniTimerOne::init(void) {
    TCCR1B = _BV(WGM12); // Setting the timer mode, CTC, and disabling the timer clock source (stopping it)
    TCCR1A = 0; // No flag in this register is needed
    TIMSK1 = 0; // Disabling interrupts capture
    cli(); // Disabling interrupts, because TCNT1 setting is not an atomic operation
    TCNT1 = 0; // Resetting the timer count
    sei(); // Re-enabling interrupts
}

void MiniTimerOne::setPeriod(unsigned long int period) {
    const unsigned long int cycles = CYCLES_PER_MICROSEC * period;
    const int prescalerValues[] = { 1, 8, 64, 256, 1024 };
    const unsigned int csBitsValues[] = { _BV(CS10), _BV(CS11), _BV(CS11) | _BV(CS10), _BV(CS12), _BV(CS12) | _BV(CS10) };
    bool timerSet = false;

    for (int i = 0; i < 5 && !timerSet; i++) {
        if (cycles < TIMER1_MAX_VALUE * prescalerValues[i]) {
            /*
             * To start the timer three bits will need to be set in the TCCR1B register for the prescaler value.
             * How those bits needs to be set depends on how long the period of the timer is, because the value
             * of the period must fit the "compare match" register (OCR1A) and the prescaler is needed to downscale it.
             * Setting those bits will start the timer, so their value is temporarily stored in _clockSelectBits.
             */
            this->_clockSelectBits = csBitsValues[i]; 
            cli(); // Disabling interrupts, because OCR1A setting is not an atomic operation
            OCR1A = cycles / prescalerValues[i]; // Setting the value with which the counter (TCNT1) will be compared
            sei(); // Re-enabling interrupts
            timerSet = true;
        }
    }
    /*
     * If value exceeds the maximum value the register can hold, even with the maximum prescaler value, it sets the register
     * value to the maximum value.
     */
    if (!timerSet) {
        this->_clockSelectBits = _BV(CS12) | _BV(CS10);
        cli(); // Disabling interrupts, because OCR1A setting is not an atomic operation
        OCR1A = TIMER1_MAX_VALUE - 1; 
        sei(); // Re-enabling interrupts
    }
}

void MiniTimerOne::attachInterrupt(void (*isr)(void)) {
    this->_isrCallback = isr; // Setting the function to be called on the interrupt capture
    TIMSK1 |= _BV(OCIE1A); // Enabling interrupts capture
}

void MiniTimerOne::detachInterrupt(void) {
    TIMSK1 = 0; // Disabling interrupts capture
    this->_isrCallback = emptyCallback; // Resetting function called on interrupt to an empty one
}

void MiniTimerOne::start(void) {
    TCCR1B |= this->_clockSelectBits; // Setting the clock source, and consequently starting the timer
}

void MiniTimerOne::stop(void) {
    TCCR1B = _BV(WGM12); // Disabling the timer clock source (stopping it), and setting the timer mode (CTC)
}

void MiniTimerOne::reset(void) {
    cli(); // Disabling interrupts, because TCNT1 setting is not an atomic operation
    TCNT1 = 0; // Resetting the timer counter, to start a new period
    sei(); // Re-enabling interrupts
}

void (*MiniTimerOne::getCallback(void))(void) {
    return this->_isrCallback;
}

// Creating an "alias" to the singleton instance of this class so as to easily access it from the Arduino code
MiniTimerOne MiniTimer1 = *MiniTimerOne::getInstance();

/*
 * Setting that the function to be called during an interrupt generated by the timer in CTC mode is the one
 * previously set by attachInterrupt and saved into the object.
 */
ISR(TIMER1_COMPA_vect) {
    (MiniTimer1.getCallback())();
}
