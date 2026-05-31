/*
 * Morse Code Encoder — Arduino Uno / Nano
 *
 * Hardware:
 *   - 4x4 matrix keypad (rows → D9–D6, cols → D5–D2)
 *   - Active buzzer or piezo speaker → D11 (via 100 Ω resistor if needed)
 *
 * Morse timing (ITU-R M.1677-1):
 *   Dot  = 1 unit
 *   Dash = 3 units
 *   Gap within character = 1 unit
 *   Gap between characters = 3 units
 *   Gap between words      = 7 units
 *
 *   unit (ms) = 1200 / WPM   (PARIS standard word length)
 *
 * Keypad layout (typical 4x4):
 *   [ 1 ] [ 2 ] [ 3 ] [ A ]
 *   [ 4 ] [ 5 ] [ 6 ] [ B ]
 *   [ 7 ] [ 8 ] [ 9 ] [ C ]
 *   [ * ] [ 0 ] [ # ] [ D ]
 *
 * Input model:
 *   0–9, A–D  → direct characters
 *   2–9       → also support phone-style letter cycling (ABC on 2, DEF on 3, …)
 *   *         → transmit buffered text as Morse
 *   #         → backspace one character
 *   Hold #    → clear entire buffer
 */

// ─── Pin assignments ───────────────────────────────────────────────────────────
const uint8_t ROW_PINS[4] = {9, 8, 7, 6};
const uint8_t COL_PINS[4] = {5, 4, 3, 2};
const uint8_t BUZZER_PIN  = 11;

// ─── Morse speed ───────────────────────────────────────────────────────────────
uint8_t morseWpm = 20;  // Words per minute (adjustable at compile time)

// ─── Keypad map (row-major) ──────────────────────────────────────────────────
const char KEY_MAP[4][4] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

// ─── Letter groups for phone-style multi-tap on numeric keys ───────────────────
const char* const PHONE_LETTERS[10] = {
  " ",     // 0 — space on multi-tap cycle
  ".,?",   // 1 — punctuation cycle
  "ABC",   // 2
  "DEF",   // 3
  "GHI",   // 4
  "JKL",   // 5
  "MNO",   // 6
  "PQRS",  // 7
  "TUV",   // 8
  "WXYZ"   // 9
};

// ─── Input buffer ──────────────────────────────────────────────────────────────
const uint8_t MAX_MESSAGE_LEN = 64;
char messageBuffer[MAX_MESSAGE_LEN + 1];
uint8_t bufferLength = 0;

// ─── Multi-tap state ───────────────────────────────────────────────────────────
char   lastTapKey      = '\0';
uint8_t lastTapIndex    = 0;
unsigned long lastTapMs = 0;
const unsigned long MULTI_TAP_TIMEOUT_MS = 800;

// ─── Debounce / hold timing ────────────────────────────────────────────────────
const unsigned long DEBOUNCE_MS    = 30;
const unsigned long HOLD_CLEAR_MS  = 1200;

// ─── Morse lookup table (ASCII → pattern; '.' = dot, '-' = dash) ─────────────
struct MorseSymbol {
  char character;
  const char* pattern;
};

const MorseSymbol MORSE_TABLE[] = {
  {'A', ".-"},     {'B', "-..."},   {'C', "-.-."},   {'D', "-.."},
  {'E', "."},      {'F', "..-."},   {'G', "--."},    {'H', "...."},
  {'I', ".."},     {'J', ".---"},   {'K', "-.-"},    {'L', ".-.."},
  {'M', "--"},     {'N', "-."},     {'O', "---"},    {'P', ".--."},
  {'Q', "--.-"},   {'R', ".-."},    {'S', "..."},    {'T', "-"},
  {'U', "..-"},    {'V', "...-"},   {'W', ".--"},    {'X', "-..-"},
  {'Y', "-.--"},   {'Z', "--.."},
  {'0', "-----"},  {'1', ".----"},  {'2', "..---"},  {'3', "...--"},
  {'4', "....-"},  {'5', "....."},  {'6', "-...."},  {'7', "--..."},
  {'8', "---.."},  {'9', "----."},
  {'.', ".-.-.-"}, {',', "--..--"}, {'?', "..--.."}, {'-', "-....-"},
  {'/', "-..-."},  {'(', "-.--."},  {')', "-.--.-"}, {' ', " "}
};

const uint8_t MORSE_TABLE_SIZE = sizeof(MORSE_TABLE) / sizeof(MORSE_TABLE[0]);

// ─── Timing helpers ────────────────────────────────────────────────────────────
inline unsigned long dotDurationMs() {
  return 1200UL / morseWpm;
}

inline unsigned long dashDurationMs() {
  return dotDurationMs() * 3UL;
}

inline unsigned long intraCharGapMs() {
  return dotDurationMs();
}

inline unsigned long letterGapMs() {
  return dotDurationMs() * 3UL;
}

inline unsigned long wordGapMs() {
  return dotDurationMs() * 7UL;
}

// ─── Keypad initialization ─────────────────────────────────────────────────────
void initKeypad() {
  for (uint8_t r = 0; r < 4; r++) {
    pinMode(ROW_PINS[r], OUTPUT);
    digitalWrite(ROW_PINS[r], HIGH);  // idle: rows inactive (HIGH)
  }
  for (uint8_t c = 0; c < 4; c++) {
    pinMode(COL_PINS[c], INPUT_PULLUP);
  }
}

/*
 * scanKeypad()
 * Efficient row-column scan:
 *   1. Drive one row LOW at a time.
 *   2. Read all columns; a LOW column means key press at (row, col).
 *   3. Restore row HIGH before next row.
 * Returns '\0' if no key is pressed.
 */
char scanKeypad() {
  for (uint8_t r = 0; r < 4; r++) {
    digitalWrite(ROW_PINS[r], LOW);

    for (uint8_t c = 0; c < 4; c++) {
      if (digitalRead(COL_PINS[c]) == LOW) {
        digitalWrite(ROW_PINS[r], HIGH);
        return KEY_MAP[r][c];
      }
    }

    digitalWrite(ROW_PINS[r], HIGH);
  }
  return '\0';
}

/*
 * waitForKeyRelease()
 * Blocks until the currently pressed key is released, with debounce.
 */
void waitForKeyRelease() {
  delay(DEBOUNCE_MS);
  while (scanKeypad() != '\0') {
    delay(5);
  }
  delay(DEBOUNCE_MS);
}

// ─── Buffer management ─────────────────────────────────────────────────────────
void appendToBuffer(char c) {
  if (bufferLength >= MAX_MESSAGE_LEN) {
    return;
  }
  messageBuffer[bufferLength++] = c;
  messageBuffer[bufferLength] = '\0';
}

void backspaceBuffer() {
  if (bufferLength > 0) {
    bufferLength--;
    messageBuffer[bufferLength] = '\0';
  }
}

void clearBuffer() {
  bufferLength = 0;
  messageBuffer[0] = '\0';
}

// ─── Multi-tap letter entry (2–9 and 0/1) ─────────────────────────────────────
char resolveMultiTap(char key) {
  if (key < '0' || key > '9') {
    return key;  // Not a numeric key — use as-is (A–D, etc.)
  }

  unsigned long now = millis();
  const char* group = PHONE_LETTERS[key - '0'];
  uint8_t groupLen = strlen(group);

  // First press or timeout → start new cycle
  if (key != lastTapKey || (now - lastTapMs) > MULTI_TAP_TIMEOUT_MS) {
    lastTapKey = key;
    lastTapIndex = 0;
  } else {
    lastTapIndex = (lastTapIndex + 1) % groupLen;
  }

  lastTapMs = now;
  return group[lastTapIndex];
}

void finalizeMultiTapIfPending() {
  if (lastTapKey != '\0' && (millis() - lastTapMs) > MULTI_TAP_TIMEOUT_MS) {
    lastTapKey = '\0';
    lastTapIndex = 0;
  }
}

// ─── Morse output ──────────────────────────────────────────────────────────────
void toneOn() {
  digitalWrite(BUZZER_PIN, HIGH);
}

void toneOff() {
  digitalWrite(BUZZER_PIN, LOW);
}

void playDot() {
  toneOn();
  delay(dotDurationMs());
  toneOff();
}

void playDash() {
  toneOn();
  delay(dashDurationMs());
  toneOff();
}

void playGap(unsigned long durationMs) {
  delay(durationMs);
}

/*
 * lookupMorsePattern()
 * Returns Morse pattern string for a character, or nullptr if unsupported.
 */
const char* lookupMorsePattern(char c) {
  if (c >= 'a' && c <= 'z') {
    c = c - 'a' + 'A';
  }

  for (uint8_t i = 0; i < MORSE_TABLE_SIZE; i++) {
    if (MORSE_TABLE[i].character == c) {
      return MORSE_TABLE[i].pattern;
    }
  }
  return nullptr;
}

/*
 * playMorseCharacter()
 * Plays one character using ITU element and intra-character spacing.
 */
void playMorseCharacter(char c) {
  const char* pattern = lookupMorsePattern(c);
  if (pattern == nullptr) {
    return;
  }

  if (pattern[0] == ' ' && pattern[1] == '\0') {
    return;  // Space handled at word level
  }

  for (uint8_t i = 0; pattern[i] != '\0'; i++) {
    if (pattern[i] == '.') {
      playDot();
    } else if (pattern[i] == '-') {
      playDash();
    }

    // Gap between elements within the same character (not after last element)
    if (pattern[i + 1] != '\0') {
      playGap(intraCharGapMs());
    }
  }
}

/*
 * transmitMessage()
 * Encodes and plays the full buffered message with correct letter/word spacing.
 */
void transmitMessage() {
  if (bufferLength == 0) {
    return;
  }

  for (uint8_t i = 0; i < bufferLength; i++) {
    char c = messageBuffer[i];

    if (c == ' ') {
      playGap(wordGapMs());
      continue;
    }

    playMorseCharacter(c);

    // Inter-character gap (skip after last character or before a space)
    if (i + 1 < bufferLength && messageBuffer[i + 1] != ' ') {
      playGap(letterGapMs());
    }
  }

  // Brief end-of-transmission silence
  playGap(wordGapMs());
}

// ─── Startup feedback ──────────────────────────────────────────────────────────
void playReadyBeeps() {
  playDot();
  playGap(intraCharGapMs());
  playDot();
}

// ─── Key handling ──────────────────────────────────────────────────────────────
void handleKeyPress(char key, unsigned long holdDurationMs) {
  if (key == '*') {
    // Transmit buffered text
    transmitMessage();
    return;
  }

  if (key == '#') {
    if (holdDurationMs >= HOLD_CLEAR_MS) {
      clearBuffer();
    } else {
      backspaceBuffer();
    }
    return;
  }

  // Numeric keys: multi-tap letters; A–D and direct digits still work
  char resolved = resolveMultiTap(key);
  appendToBuffer(resolved);
}

/*
 * readKeyWithHold()
 * Waits for a key, measures hold time, debounces, and waits for release.
 */
bool readKeyWithHold(char& keyOut, unsigned long& holdMsOut) {
  // Wait for initial press
  char key = '\0';
  while ((key = scanKeypad()) == '\0') {
    finalizeMultiTapIfPending();
    delay(5);
  }

  unsigned long pressStart = millis();

  // Measure hold duration until release
  while (scanKeypad() != '\0') {
    delay(5);
  }

  holdMsOut = millis() - pressStart;
  keyOut = key;

  delay(DEBOUNCE_MS);
  return true;
}

// ─── Arduino setup / loop ──────────────────────────────────────────────────────
void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
  toneOff();

  initKeypad();
  clearBuffer();

  playReadyBeeps();  // Audible "ready" indicator
}

void loop() {
  char key;
  unsigned long holdMs;

  if (readKeyWithHold(key, holdMs)) {
    handleKeyPress(key, holdMs);
  }
}
