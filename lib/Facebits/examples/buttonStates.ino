/*
    facebits basic example

    Stores the digital state of a button using a bitmask.
*/

#include <Arduino.h>
#include "Facebits.h"

constexpr uint8_t BUTTON_MASK = 0b00000001;
uint8_t state = 0;

void setup()
{
    pinMode(5, INPUT);
    Serial.begin(9600);
}

void loop()
{
    // Write button state into bitmask
    writeData(state, BUTTON_MASK, digitalRead(5));

    // Read button state from bitmask
    if (readData(state, BUTTON_MASK))
    {
        Serial.println("Button is HIGH");

        // Clear bit
        writeData(state, BUTTON_MASK, 0);
    }
}