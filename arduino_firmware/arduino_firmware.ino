struct Control {
  uint32_t value;
} cont;

struct Display {
  uint32_t value;
} disp;

void setup() {
  Serial.begin(9600);
}

void loop() {
  receiveInput();
  cont.value = disp.value * disp.value;
  sendOutput();
  delay(10);
}

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
