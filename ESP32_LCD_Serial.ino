/**
 * @file ESP32_LCD_Serial.ino
 * @brief ESP32 bridge: Serial Monitor input displayed on a 16x2 I2C LCD.
 *
 * Hardware:
 *   - ESP32 (any variant with default I2C-capable GPIOs)
 *   - 16x2 LCD with PCF8574 I2C backpack
 *
 * Wiring (I2C):
 *   LCD SDA  ->  ESP32 GPIO 21
 *   LCD SCL  ->  ESP32 GPIO 22
 *   LCD VCC  ->  5V (or 3.3V per module datasheet)
 *   LCD GND  ->  GND
 *
 * Dependencies (Arduino Library Manager):
 *   - "LiquidCrystal I2C" by Frank de Brabander
 *
 * Usage:
 *   Open Serial Monitor at 115200 baud, line ending "Newline" or "Both NL & CR".
 *   Type a message and press Enter; the LCD clears and shows the new text.
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// -----------------------------------------------------------------------------
// Pin and bus configuration
// -----------------------------------------------------------------------------

/** I2C data line on ESP32 (default SDA). */
static const uint8_t I2C_SDA_PIN = 21;

/** I2C clock line on ESP32 (default SCL). */
static const uint8_t I2C_SCL_PIN = 22;

/** Host serial baud rate for Serial Monitor. */
static const uint32_t SERIAL_BAUD = 115200;

// -----------------------------------------------------------------------------
// LCD geometry and I2C address
// -----------------------------------------------------------------------------

static const uint8_t LCD_COLUMNS = 16;
static const uint8_t LCD_ROWS    = 2;

/**
 * Typical PCF8574 backpack addresses: 0x27 or 0x3F.
 * If the display stays blank after upload, change this constant and reflash.
 */
static const uint8_t LCD_I2C_ADDRESS = 0x27;

/** Maximum characters stored on the LCD (both rows). */
static const size_t LCD_CHAR_CAPACITY = LCD_COLUMNS * LCD_ROWS;

// -----------------------------------------------------------------------------
// Global objects
// -----------------------------------------------------------------------------

LiquidCrystal_I2C lcd(LCD_I2C_ADDRESS, LCD_COLUMNS, LCD_ROWS);

/** Accumulates characters from Serial until a line terminator is received. */
String incomingLine;

// -----------------------------------------------------------------------------
// Forward declarations
// -----------------------------------------------------------------------------

void initializeI2cBus();
void initializeLcd();
void showWelcomeScreen();
void processSerialInput();
void displayTextOnLcd(const String& text);
String sanitizeForDisplay(const String& raw);

// -----------------------------------------------------------------------------
// Arduino entry points
// -----------------------------------------------------------------------------

void setup() {
  Serial.begin(SERIAL_BAUD);
  while (!Serial) {
    // Wait for USB serial on boards that need it (harmless on ESP32).
  }

  initializeI2cBus();
  initializeLcd();
  showWelcomeScreen();

  Serial.println(F("ESP32 LCD ready. Type a message and press Enter."));
}

void loop() {
  processSerialInput();
}

// -----------------------------------------------------------------------------
// Initialization
// -----------------------------------------------------------------------------

/**
 * Starts the I2C master on the ESP32 using the specified SDA/SCL pins.
 * Must be called before any LCD or other I2C device access.
 */
void initializeI2cBus() {
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
}

/**
 * Initializes the HD44780-compatible LCD over I2C and enables the backlight.
 */
void initializeLcd() {
  lcd.init();
  lcd.backlight();
  lcd.clear();
}

/**
 * Shows a short startup message on both LCD rows.
 */
void showWelcomeScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("ESP32 Ready"));
  lcd.setCursor(0, 1);
  lcd.print(F("Serial->LCD"));
}

// -----------------------------------------------------------------------------
// Serial handling
// -----------------------------------------------------------------------------

/**
 * Reads available Serial bytes, builds a line buffer, and refreshes the LCD
 * when the user sends a complete line (Enter / newline).
 */
void processSerialInput() {
  while (Serial.available() > 0) {
    const char incomingChar = static_cast<char>(Serial.read());

    if (incomingChar == '\n' || incomingChar == '\r') {
      if (incomingLine.length() > 0) {
        const String displayText = sanitizeForDisplay(incomingLine);
        displayTextOnLcd(displayText);
        Serial.print(F("LCD: "));
        Serial.println(displayText);
        incomingLine = "";
      }
      continue;
    }

    incomingLine += incomingChar;
  }
}

/**
 * Removes control characters and limits length to the physical LCD size.
 *
 * @param raw  Raw string from Serial Monitor.
 * @return     Text safe to print on a 16x2 display (max 32 characters).
 */
String sanitizeForDisplay(const String& raw) {
  String cleaned;
  cleaned.reserve(LCD_CHAR_CAPACITY);

  for (size_t i = 0; i < raw.length() && cleaned.length() < LCD_CHAR_CAPACITY; ++i) {
    const char c = raw.charAt(i);
    if (c >= 32 && c <= 126) {
      cleaned += c;
    }
  }

  return cleaned;
}

// -----------------------------------------------------------------------------
// LCD output
// -----------------------------------------------------------------------------

/**
 * Clears the LCD and prints up to 32 characters across two rows.
 * Row 0: characters 0..15, Row 1: characters 16..31.
 * Longer input is truncated to fit the display.
 *
 * @param text  Message to show (already sanitized).
 */
void displayTextOnLcd(const String& text) {
  lcd.clear();

  if (text.length() == 0) {
    lcd.setCursor(0, 0);
    lcd.print(F("(empty)"));
    return;
  }

  lcd.setCursor(0, 0);
  if (text.length() <= LCD_COLUMNS) {
    lcd.print(text);
    return;
  }

  // First row: first 16 characters.
  lcd.print(text.substring(0, LCD_COLUMNS));

  // Second row: next 16 characters (if any).
  lcd.setCursor(0, 1);
  if (text.length() > LCD_COLUMNS) {
    const uint8_t secondRowLen =
        static_cast<uint8_t>(min(text.length() - LCD_COLUMNS, static_cast<size_t>(LCD_COLUMNS)));
    lcd.print(text.substring(LCD_COLUMNS, LCD_COLUMNS + secondRowLen));
  }
}
