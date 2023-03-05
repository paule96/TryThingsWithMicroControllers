#include <Arduino.h>

int Buzzer = 14;
int PWMChannel = 0;

/* This is interesting:
If you use sizeof it will return the bytes that is used by the object you pass in as a parameter.
Because objects can have different size, this is not a good method to count objects in an array.
To count objects in the array you must devide the size of the array,
by the size of a single entry in the array. There is no builtin way in C++ todo it better.
This will always work, because an array can only contains elements from the same type.
If you want to programm very efficently, define the length of the array in a const and
pass it in by the array initialisation, like so:
note_t song[7] = {........};

Then you know for sure your array will always be 7 items long.
*/
uint getLength(int array[])
{
    return sizeof(array) / sizeof(array[0]);
}

uint getLength(note_t array[])
{
    return sizeof(array) / sizeof(array[0]);
}

int leds[] = {2, 3, 4, 5, 6, 7, 8};
note_t song[] = {NOTE_C, NOTE_D, NOTE_E, NOTE_F, NOTE_G, NOTE_A, NOTE_B};

int songLength = getLength(song);

void setup()
{
    for (int i = 0; i < getLength(leds); i++)
    {
        printf("Active pin: %.1f", i);
        pinMode(leds[i], OUTPUT);
    }

    pinMode(Buzzer, OUTPUT);
    ledcAttachPin(Buzzer, PWMChannel);
}

void loop()
{
    for (int octave = 4; octave < 8; octave++)
    {
        for (int i = 0; i < songLength; i++)
        {
            printf("play note: %.1f from the song", i);
            switch (song[i])
            {
            case NOTE_C:
                digitalWrite(leds[0], HIGH);
                break;
            case NOTE_D:
                digitalWrite(leds[1], HIGH);
                break;
            case NOTE_E:
                digitalWrite(leds[2], HIGH);
                break;
            case NOTE_F:
                digitalWrite(leds[3], HIGH);
                break;
            case NOTE_G:
                digitalWrite(leds[4], HIGH);
                break;
            case NOTE_A:
                digitalWrite(leds[5], HIGH);
                break;
            case NOTE_B:
                digitalWrite(leds[6], HIGH);
                break;
            default:
                // TODO: mach fehler
                break;
            }
            ledcWriteNote(PWMChannel, song[i], octave);
            delay(300);
            for (size_t l = 0; l < getLength(leds); l++)
            {
                printf("Set pin %.1f to low.", l);
                digitalWrite(leds[l], LOW);
            }
        }
    }
}
