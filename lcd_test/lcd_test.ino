uint8_t lcdAddr = 0;

// ---------- I2C (TWI) pe registre ----------

void twiInit() {
  PORTC |= (1 << PC4) | (1 << PC5);
  TWSR = 0;
  TWBR = 72;
  TWCR = (1 << TWEN);
}

static uint8_t twiWait() {
  while (!(TWCR & (1 << TWINT)));
  return TWSR & 0xF8;
}

bool twiStart(uint8_t adrW) {
  TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
  uint8_t st = twiWait();
  if (st != 0x08 && st != 0x10) return false;

  TWDR = adrW;
  TWCR = (1 << TWINT) | (1 << TWEN);
  return twiWait() == 0x18;
}

bool twiWrite(uint8_t data) {
  TWDR = data;
  TWCR = (1 << TWINT) | (1 << TWEN);
  return twiWait() == 0x28;
}

void twiStop() {
  TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
  while (TWCR & (1 << TWSTO));
}

uint8_t twiScan() {
  uint8_t found = 0;
  for (uint8_t a = 0x08; a < 0x78; a++) {
    bool ack = twiStart(a << 1);
    twiStop();
    if (ack) {
      Serial.print("Dispozitiv I2C la 0x");
      Serial.println(a, HEX);
      if (!found) found = a;
    }
  }
  return found;
}

// ---------- LCD 1602 (HD44780) prin PCF8574, mod 4 biti ----------

#define LCD_RS 0x01
#define LCD_EN 0x04
#define LCD_BL 0x08

static void lcdNibble(uint8_t bits) {
  twiStart(lcdAddr << 1);
  twiWrite(bits | LCD_BL | LCD_EN);
  twiWrite(bits | LCD_BL);
  twiStop();
}

static void lcdSend(uint8_t value, uint8_t rs) {
  lcdNibble((value & 0xF0) | rs);
  lcdNibble((value << 4) | rs);
}

void lcdCmd(uint8_t cmd) { lcdSend(cmd, 0); }
void lcdChar(char c) { lcdSend(c, LCD_RS); }

void lcdPrint(const char *s) {
  while (*s) lcdChar(*s++);
}

void lcdClear() {
  lcdCmd(0x01);
  delay(2);
}

void lcdSetCursor(uint8_t col, uint8_t row) {
  lcdCmd(0x80 | ((row ? 0x40 : 0x00) + col));
}

void lcdInit() {
  delay(50);
  lcdNibble(0x30); delay(5);
  lcdNibble(0x30); delayMicroseconds(150);
  lcdNibble(0x30);
  lcdNibble(0x20);
  lcdCmd(0x28);
  lcdCmd(0x0C);
  lcdCmd(0x06);
  lcdClear();
}

// ---------- Program ----------

void setup() {
  Serial.begin(9600);
  twiInit();

  lcdAddr = twiScan();
  if (!lcdAddr) {
    Serial.println("Niciun dispozitiv I2C. Verifica firele SDA/SCL/VCC/GND.");
    while (true);
  }
  Serial.print("Folosesc LCD la 0x");
  Serial.println(lcdAddr, HEX);

  lcdInit();
  lcdSetCursor(0, 0);
  lcdPrint("Test LCD - TWI");
}

void loop() {
  static uint16_t sec = 0;
  char buf[6];

  lcdSetCursor(0, 1);
  lcdPrint("Secunde: ");
  lcdPrint(itoa(sec++, buf, 10));

  delay(1000);
}
