
#define INPUT_DATA_PIN 12
#define INPUT_MODE_PIN 10
#define CLOCK_PIN 9
#define OUTPUT_DATA_PIN 13
#define OUTPUT_LATCH_PIN 11

enum TubeStatus
{
    Empty = 0,
    Torpedo = 1,
    Mine = 2
};

// These structs' format must be kept in sync with the corresponding structs in the
// code that runs on the computer
struct Control {
  uint8_t tubeArmed[5];
  uint8_t tubeLoadTorpedo[5];
  uint8_t tubeLoadMine[5];
  uint32_t debugValue;
} __attribute__((packed));

struct Display {
  uint8_t tubeOccupancy[5];
} __attribute__((packed));

Control cont;
Display disp;

void setup() {
  Serial.begin(115200);
  pinMode(INPUT_DATA_PIN, INPUT);
  pinMode(INPUT_MODE_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(OUTPUT_DATA_PIN, OUTPUT);
  pinMode(OUTPUT_LATCH_PIN, OUTPUT);
}

void loop() {
  disp.tubeOccupancy[0] = Mine;
  disp.tubeOccupancy[1] = Torpedo;
  disp.tubeOccupancy[2] = Mine;
  disp.tubeOccupancy[3] = Torpedo;
  disp.tubeOccupancy[4] = Mine;

  uint16_t switchValues;
  uint16_t combinedSwitchValues = 0x0000;
  shiftRegisters(prepareShiftDisplay(Torpedo), &switchValues);
  combinedSwitchValues |= switchValues;
  delay(5);
  shiftRegisters(prepareShiftDisplay(Mine), &switchValues);
  combinedSwitchValues |= switchValues;
  delay(5);
  shiftRegisters(0x0000, &switchValues);
  combinedSwitchValues |= switchValues;
  interpretShiftControl(combinedSwitchValues);
  cont.debugValue = switchValues;
  
  sendOutput();
}

void shiftRegisters(uint16_t output, uint16_t *input) {
  *input = 0;
  digitalWrite(INPUT_MODE_PIN, LOW);
  digitalWrite(INPUT_MODE_PIN, HIGH);
  for (int i = 0; i < 16; i++) {
    digitalWrite(OUTPUT_DATA_PIN, (output & 0x0001) ? HIGH : LOW);
    if (digitalRead(INPUT_DATA_PIN) == LOW) {
      *input |= 0x0001;
    }
    digitalWrite(CLOCK_PIN, HIGH);
    digitalWrite(CLOCK_PIN, LOW);
    output >>= 1;
    *input <<= 1;
  }
  digitalWrite(OUTPUT_LATCH_PIN, HIGH);
  digitalWrite(OUTPUT_LATCH_PIN, LOW);
}

uint16_t prepareShiftDisplay(TubeStatus type) {
  uint16_t s = 0;

  /* High side */
  if (disp.tubeOccupancy[0] != type) s |= (1 << 15);
  if (disp.tubeOccupancy[1] != type) s |= (1 << 13);
  if (disp.tubeOccupancy[2] != type) s |= (1 << 14);
  if (disp.tubeOccupancy[3] != type) s |= (1 << 12);
  if (disp.tubeOccupancy[4] != type) s |= (1 << 8);

  /* Low side */
  if (type == Torpedo) s |= (1 << 10);
  if (type == Mine) s |= (1 << 11);

  return s;
}

void interpretShiftControl(uint16_t switchValues) {
  cont.tubeArmed[0]       = !!(switchValues & 0x0040);
  cont.tubeArmed[1]       = !!(switchValues & 0x0080);
  cont.tubeArmed[2]       = !!(switchValues & 0x0100);
  cont.tubeArmed[3]       = !!(switchValues & 0x4000);
  cont.tubeArmed[4]       = !!(switchValues & 0x8000);

  cont.tubeLoadTorpedo[0] = !!(switchValues & 0x0200);
  cont.tubeLoadTorpedo[1] = !!(switchValues & 0x0400);
  cont.tubeLoadTorpedo[2] = !!(switchValues & 0x0800);
  cont.tubeLoadTorpedo[3] = !!(switchValues & 0x1000);
  cont.tubeLoadTorpedo[4] = !!(switchValues & 0x2000);

  cont.tubeLoadMine[0]    = !!(switchValues & 0x0002);
  cont.tubeLoadMine[1]    = !!(switchValues & 0x0004);
  cont.tubeLoadMine[2]    = !!(switchValues & 0x0008);
  cont.tubeLoadMine[3]    = !!(switchValues & 0x0010);
  cont.tubeLoadMine[4]    = !!(switchValues & 0x0020);
}

/* I/O code for talking to the computer */

static const int inputBufSize = sizeof(Display) + 1;
uint8_t inputBuf[inputBufSize];
int inputPos = 0; // index into inputBuf measured in *nibbles*!
uint8_t inputChecksum = 0;

int parseHex(int character) {
  if (character >= '0' && character <= '9') {
    return character - '0';
  } else if (character >= 'a' && character <= 'f') {
    return 10 + character - 'a';
  } else if (character >= 'A' && character <= 'F') {
    return 10 + character - 'A';
  } else {
    return -1;
  }
}

void receiveInput() {
  while (true) {
    int c = Serial.read();
    if (c == -1) {
      return;
    } else if (c == '[') {
      inputPos = 0;
      inputChecksum = 0;
    } else if (c == ']') {
      if (inputPos == inputBufSize * 2 && inputChecksum == 0) {
        memcpy(&disp, inputBuf, sizeof(Display));
      }
      inputPos = inputBufSize * 2;
    } else {
      int h = parseHex(c);
      if (h == -1 || inputPos == inputBufSize * 2) {
        inputPos = inputBufSize * 2;
        continue;
      }
      int bytePos = inputPos >> 1;
      int nibblePos = inputPos & 0x01;
      if (nibblePos == 0) {
        inputBuf[bytePos] = h << 4;
      } else {
        inputBuf[bytePos] |= h;
        inputChecksum ^= inputBuf[bytePos];
      }
      inputPos += 1;
    }
  }
}

int generateHex(int value) {
  if (value < 10) {
    return '0' + value;
  } else {
    return 'A' + (value - 10);
  }
}

void sendOutput() {
  Serial.write('[');
  uint8_t checksum = 0;
  const uint8_t *outputBuf = (const uint8_t *)&cont;
  for (int i = 0; i < sizeof(Control); ++i) {
    Serial.write(generateHex((outputBuf[i] >> 4) & 0x0F));
    Serial.write(generateHex(outputBuf[i] & 0x0F));
    checksum ^= outputBuf[i];
  }
  Serial.write(generateHex((checksum >> 4) & 0x0F));
  Serial.write(generateHex(checksum & 0x0F));
  Serial.write(']');
}
