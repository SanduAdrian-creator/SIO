uint8_t lcdAddr = 0;   // adresa I2C a LCD-ului (0 = negasita)

// ---------- I2C (TWI) pe registre ----------

void twiInit() {
  PORTC |= (1 << PC4) | (1 << PC5);   // pull-up pe SDA (A4) si SCL (A5)
  TWSR = 0;                           // prescaler = 1
  TWBR = 72;                          // viteza 100 kHz
  TWCR = (1 << TWEN);                 // porneste modulul TWI
}

static uint8_t twiWait() {
  while (!(TWCR & (1 << TWINT)));     // asteapta sa se termine operatia
  return TWSR & 0xF8;                 // returneaza statusul
}

bool twiStart(uint8_t adrW) {
  TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);   // trimite START
  uint8_t st = twiWait();
  if (st != 0x08 && st != 0x10) return false;         // START esuat

  TWDR = adrW;                        // pune adresa (+ bit de scriere)
  TWCR = (1 << TWINT) | (1 << TWEN);  // trimite adresa
  return twiWait() == 0x18;           // true daca a primit ACK
}

bool twiWrite(uint8_t data) {
  TWDR = data;                        // pune octetul de trimis
  TWCR = (1 << TWINT) | (1 << TWEN);  // trimite
  return twiWait() == 0x28;           // true daca a primit ACK
}

void twiStop() {
  TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);   // trimite STOP
  while (TWCR & (1 << TWSTO));        // asteapta sa se termine STOP
}

uint8_t twiScan() {
  uint8_t found = 0;                  // prima adresa gasita
  for (uint8_t a = 0x08; a < 0x78; a++) {   // adrese valide I2C
    bool ack = twiStart(a << 1);      // incearca adresa
    twiStop();
    if (ack) {                        // a raspuns un dispozitiv
      Serial.print("Dispozitiv I2C la 0x");
      Serial.println(a, HEX);
      if (!found) found = a;          // retine doar prima
    }
  }
  return found;
}

// ---------- LCD 1602 (HD44780) prin PCF8574, mod 4 biti ----------

#define LCD_RS 0x01   // 0 = comanda, 1 = caracter
#define LCD_EN 0x04   // enable (LCD-ul citeste cand scade)
#define LCD_BL 0x08   // lumina de fundal

static void lcdNibble(uint8_t bits) {
  twiStart(lcdAddr << 1);
  twiWrite(bits | LCD_BL | LCD_EN);   // EN = 1
  twiWrite(bits | LCD_BL);            // EN = 0 -> LCD-ul preia datele
  twiStop();
}

static void lcdSend(uint8_t value, uint8_t rs) {
  lcdNibble((value & 0xF0) | rs);     // jumatatea de sus
  lcdNibble((value << 4) | rs);       // jumatatea de jos
}

void lcdCmd(uint8_t cmd) { lcdSend(cmd, 0); }        // trimite comanda
void lcdChar(char c) { lcdSend(c, LCD_RS); }         // trimite caracter

void lcdPrint(const char *s) {
  while (*s) lcdChar(*s++);           // afiseaza pana la '\0'
}

void lcdClear() {
  lcdCmd(0x01);                       // sterge ecranul
  delay(2);
}

void lcdSetCursor(uint8_t col, uint8_t row) {
  lcdCmd(0x80 | ((row ? 0x40 : 0x00) + col));   // rand 1 incepe la 0x40
}

void lcdInit() {
  delay(50);
  lcdNibble(0x30); delay(5);          // secventa de reset (3 x 0x30)
  lcdNibble(0x30); delayMicroseconds(150);
  lcdNibble(0x30);
  lcdNibble(0x20);                    // trece in mod 4 biti
  lcdCmd(0x28);                       // 4 biti, 2 randuri
  lcdCmd(0x0C);                       // afisaj pornit, fara cursor
  lcdCmd(0x06);                       // cursorul merge spre dreapta
  lcdClear();
}

// ---------- Program ----------

void setup() {
  Serial.begin(9600);
  twiInit();

  lcdAddr = twiScan();                // cauta LCD-ul
  if (!lcdAddr) {                     // nu s-a gasit nimic
    Serial.println("Niciun dispozitiv I2C. Verifica firele SDA/SCL/VCC/GND.");
    while (true);                     // opreste programul
  }
  Serial.print("Folosesc LCD la 0x");
  Serial.println(lcdAddr, HEX);

  lcdInit();
  lcdSetCursor(0, 0);                 // stanga-sus
  lcdPrint("Test LCD - TWI");
}

void loop() {
  static uint16_t sec = 0;            // isi pastreaza valoarea
  char buf[6];                        // loc pentru numar ca text

  lcdSetCursor(0, 1);                 // randul al doilea
  lcdPrint("Secunde: ");
  lcdPrint(itoa(sec++, buf, 10));     // numar -> text, apoi sec + 1

  delay(1000);                        // asteapta 1 secunda
}
