#include <Wire.h>
#include <Servo.h>

const uint8_t MPU_ADDR = 0x68;
const uint8_t LCD_ADDR = 0x27;
const uint8_t SERVO_PIN = 9;

Servo servo;

// ---------- I2C (TWI) pe registre, pentru LCD ----------

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

// ---------- LCD 1602 (HD44780) prin PCF8574, mod 4 biti ----------

#define LCD_RS 0x01
#define LCD_EN 0x04
#define LCD_BL 0x08

static void lcdNibble(uint8_t bits) {
  twiStart(LCD_ADDR << 1);
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
  twiInit();
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

// ---------- MPU6050 ----------

bool mpuInit() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0);
  return Wire.endTransmission() == 0;
}

void mpuRead(int16_t &ay, int16_t &az) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3D);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, (uint8_t)4);
  ay = Wire.read() << 8;
  ay |= Wire.read();
  az = Wire.read() << 8;
  az |= Wire.read();
}

// ---------- Program ----------

void setup() {
  servo.attach(SERVO_PIN);
  servo.write(90);

  Wire.begin();
  lcdInit();

  lcdSetCursor(0, 0);
  lcdPrint(mpuInit() ? "MPU OK" : "MPU lipsa");
  delay(1000);
  lcdClear();
}

void loop() {
  static const uint8_t pos[] = { 0, 90, 180, 90 };
  static uint8_t i = 0;

  servo.write(pos[i]);
  i = (i + 1) % 4;

  int16_t ay, az;
  mpuRead(ay, az);
  float angle = atan2(ay, az) * 180.0 / PI;

  char buf[8];
  lcdSetCursor(0, 0);
  lcdPrint("Unghi: ");
  lcdPrint(dtostrf(angle, 6, 1, buf));
  lcdPrint("   ");
  lcdSetCursor(0, 1);
  lcdPrint("Servo: ");
  lcdPrint(itoa(servo.read(), buf, 10));
  lcdPrint(" grd   ");

  delay(1000);
}
