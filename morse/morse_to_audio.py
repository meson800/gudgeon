import math
import time
import numpy as np


import pyaudio


p = pyaudio.PyAudio()
def sine_tone(frequency, duration, volume=1, sample_rate=22050):
    # generate samples, note conversion to float32 array
    samples = (np.sin(2*np.pi*np.arange(sample_rate*duration)*frequency/sample_rate)).astype(np.float32)

    # for paFloat32 sample values must be in range [-1.0, 1.0]
    stream = p.open(format=pyaudio.paFloat32,
                    channels=1,
                    rate=sample_rate,
                    output=True)

    # play. May repeat with different volume values (if done interactively) 
    stream.write(volume*samples)

    stream.stop_stream()
    stream.close()

    

morseAlphabet = {
    "A": ".-",
	"B": "-...",
    "C": "-.-.",
    "D": "-..",
    "E": ".",
    "F": "..-.",
    "G": "--.",
    "H": "....",
    "I": "..",
    "J": ".---",
    "K": "-.-",
    "L": ".-..",
    "M": "--",
    "N": "-.",
	"O": "---",
	"P": ".--.",
	"Q": "--.-",
	"R": ".-.",
	"S": "...",
	"T": "-",
	"U": "..-",
	"V": "...-",
	"W": ".--",
	"X": "-..-",
	"Y": "-.--",
	"Z": "--..",
	" ": "/",
	"1": ".----",
	"2": "..---",
	"3": "...--",
	"4": "....-",
	"5": ".....",
	"6": "-....",
	"7": "--...",
	"8": "---..",
	"9": "----.",
	"0": "-----"
}

inverseMorseAlphabet=dict((v,k) for (k,v) in morseAlphabet.items())


#encode a message in morse code, spaces between words are represented by '/'
def encodeToMorse(message):
    encodedMessage = ""
    for char in message[:]:
        encodedMessage += morseAlphabet[char.upper()] + " "   
    return encodedMessage
        
#converts encoded morse code to tones
def playMorseCode(message):
    for char in message[:]:
        if char == '.':
            sine_tone(400,.3)
            time.sleep(.1)
        if char == '-':
            sine_tone(400, .6)
            time.sleep(.1)
        if char == "/":
            time.sleep(.7)
        if char == ' ':
            time.sleep(.6);

output = encodeToMorse("AC8TY test message")
playMorseCode(output)

p.terminate()