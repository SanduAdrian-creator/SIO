#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const uint8_t MPU_ADDR = 0x68;

LiquidCrystal_I2C lcd(0x27, 16, 2);

void servoInit() {
  DDRB |= (1 << PB1);
  TCCR1A = (1 << COM1A1) | (1 << WGM11);
  TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11);
  ICR1 = 39999;
  OCR1A = 3000;
}

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
  uint8_t b[4];
  for (int i = 0; i < 4; i++) b[i] = Wire.read();
  ay = (b[0] << 8) | b[1];
  az = (b[2] << 8) | b[3];
}

void setup() {
  servoInit();
  Wire.begin();
  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print(mpuInit() ? "MPU OK" : "MPU lipsa");
  delay(1000);
  lcd.clear();
}

void loop() {
  static const uint16_t pos[] = { 2000, 3000, 4000, 3000 };
  static uint8_t i = 0;

  OCR1A = pos[i];
  i = (i + 1) % 4;

  int16_t ay, az;
  mpuRead(ay, az);
  float angle = atan2(ay, az) * 180.0 / PI;

  lcd.setCursor(0, 0);
  lcd.print("Unghi: ");
  lcd.print(angle, 1);
  lcd.print("   ");
  lcd.setCursor(0, 1);
  lcd.print("Servo: ");
  lcd.print(OCR1A / 2000.0, 2);
  lcd.print("ms   ");

  delay(1000);
}
