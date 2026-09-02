#define SEND_PWM_BY_TIMER
#include <IRremote.hpp>

#include "worldIrCodes.h"
#include "tvbGone.h"
#include "../../libs/gui/gui.h"

namespace {

// ---- Параметры тайминга и региона -----------------------------------

constexpr uint16_t kDelayBetweenCodesMs = 205;   // пауза между кодами
constexpr uint16_t kDelayBeforeBlinkMs  = 1300;  // пауза перед сигналом "готово"
constexpr uint8_t  kDoneBlinkCount      = 8;
constexpr uint16_t kDoneBlinkOnMs       = 60;
constexpr uint16_t kDoneBlinkOffMs      = 60;

constexpr int kRegionNA = 0;
constexpr int kRegionEU = 1;
// TODO: сделать регион переключаемым пользователем (кнопкой/меню),
constexpr int kSelectedRegion = kRegionEU;

IRsend irSender(IR_TX_PIN);

// ---- Разбор сжатого формата IrCode ------------------------------------

// Читает биты последовательно из code->codes, начиная с начала.
class BitReader {
public:
  void reset(const IrCode* code) {
    code_ = code;
    bytePos_ = 0;
    bitsLeft_ = 0;
  }

  uint8_t read(uint8_t count) {
    uint8_t result = 0;
    for (uint8_t i = 0; i < count; i++) {
      if (bitsLeft_ == 0) {
        currentByte_ = code_->codes[bytePos_++];
        bitsLeft_ = 8;
      }
      bitsLeft_--;
      result |= (((currentByte_ >> bitsLeft_) & 1) << (count - 1 - i));
    }
    return result;
  }

private:
  const IrCode* code_ = nullptr;
  uint8_t bytePos_ = 0;
  uint8_t bitsLeft_ = 0;
  uint8_t currentByte_ = 0;
};

// Разворачивает сжатую запись IrCode в буфер длительностей (в микросекундах),
// готовый для передачи через IRsend::sendRaw(). Возвращает число элементов,
// реально записанных в outBuffer (numPairs * 2).
size_t decodeToRawBuffer(const IrCode* code, uint16_t* outBuffer, size_t bufferCapacity) {
  BitReader reader;
  reader.reset(code);

  const size_t maxPairs = bufferCapacity / 2;
  const uint8_t numPairs = (code->numpairs < maxPairs) ? code->numpairs : maxPairs;

  for (uint8_t k = 0; k < numPairs; k++) {
    const uint16_t idx     = reader.read(code->bitcompression) * 2;
    const uint16_t onTime  = code->times[idx];
    const uint16_t offTime = code->times[idx + 1];
    outBuffer[k * 2]     = onTime * 10;
    outBuffer[k * 2 + 1] = offTime * 10;
    yield();
  }
  return numPairs * 2;
}

// ---- Передача одного кода ---------------------------------------------

void sendSingleCode(const IrCode* code) {
  static uint16_t rawBuffer[300];
  const size_t rawLength = decodeToRawBuffer(code, rawBuffer, sizeof(rawBuffer) / sizeof(rawBuffer[0]));

  irSender.sendRaw(rawBuffer, rawLength, code->timer_val); // timer_val хранит частоту несущей в кГц
  yield();

  delay(kDelayBetweenCodesMs);
}

// Индикация "передача завершена" — серия мигков в конце всего процесса.
void blinkDoneIndicator() {
  delay(kDelayBeforeBlinkMs);
  for (uint8_t i = 0; i < kDoneBlinkCount; i++) {
    digitalWrite(IR_TX_PIN, HIGH);
    delay(kDoneBlinkOnMs);
    digitalWrite(IR_TX_PIN, LOW);
    delay(kDoneBlinkOffMs);
  }
}

// Короткая вспышка после КАЖДОГО отправленного кода — просто визуальный
// "тик", что сигнал ушёл. К моменту вызова ШИМ на IR_TX_PIN уже остановлен
// (sendSingleCode() внутри дожидается delay(kDelayBetweenCodesMs)), поэтому
// пином можно так же безопасно управлять как обычным GPIO, как и в
// blinkDoneIndicator() выше.
constexpr uint16_t kStepBlinkOnMs = 20;

void blinkStepIndicator() {
  digitalWrite(IR_TX_PIN, HIGH);
  delay(kStepBlinkOnMs);
  digitalWrite(IR_TX_PIN, LOW);
}

void ensureIrInitialized() {
  static bool initialized = false;
  if (initialized) return;

  // DISABLE_LED_FEEDBACK: свою индикацию делаем вручную в blinkDoneIndicator()/
  // blinkStepIndicator() — встроенный feedback-LED библиотеки не подходит
  // (LED_BUILTIN не определён для этой платы, да и физически это тот же
  // пин, что и ИК-передатчик).
  irSender.begin(IR_TX_PIN, DISABLE_LED_FEEDBACK);
  initialized = true;
}

} // namespace

void TvbGoneApp::displayProgress() const {
    char buf[24];
    if (finished) {
      snprintf(buf, sizeof(buf), "Complete: %u", totalCodes);
    } else {
      snprintf(buf, sizeof(buf), "Done: %u/%u", currentIndex, totalCodes);
    }
    displayBigText(buf);
    displayProgressBar((float)currentIndex / totalCodes, 10);
}

void TvbGoneApp::Setup() {
    ensureIrInitialized();

    codesList = (kSelectedRegion == kRegionEU) ? EUpowerCodes : NApowerCodes;
    const uint8_t numNACodes = NUM_ELEM(NApowerCodes);
    const uint8_t numEUCodes = NUM_ELEM(EUpowerCodes);
    totalCodes = (kSelectedRegion == kRegionEU) ? numEUCodes : numNACodes;
    currentIndex = 0;
    finished = false;

    displayProgress();
}

bool TvbGoneApp::Loop() {
    if (StickCP2.BtnPWR.wasPressed()) {
      displayClear();
      displayBigText("CANCELED!");
      delay(300);
      finished = true;
      return false;
    }

    sendSingleCode(codesList[currentIndex]);
    blinkStepIndicator();
    currentIndex++;

    if (currentIndex >= totalCodes) {
      finished = true;
      displayProgress();
      blinkDoneIndicator();
      return false;
    }

    displayProgress();
    return true;
}