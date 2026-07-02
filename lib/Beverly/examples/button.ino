/*
    Using BEVERLY to handle push button events.
    -> Markusianito
*/


#include <Beverly.h>

#define button_pin 5
Button button(button_pin);

void setup()
{
    Serial.begin(9600);
    button.start();
}

void loop()
{
    BAction action = button.watch();

    if (action == BAction::Pressed)
        Serial.println("Pressed");

    if (action == BAction::Held)
        Serial.println("Held");

    if (action == BAction::Released)
        Serial.println("Released");
}