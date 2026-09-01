#include "M5StickCPlus2.h"

#define SEND_PWM_BY_TIMER
#include <IRremote.hpp>

#include "worldIrCodes.h"
#include "tvbGone.h"
#include "../../libs/gui/gui.h"
#include "../../libs/gui/activityCheck.h"

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

// Индикация "передача завершена". К моменту вызова ШИМ на IR_TX_PIN уже
// остановлен библиотекой, поэтому пином можно управлять как обычным GPIO —
// это тот же светодиод, что мигал при отправке ИК-кодов (см. worldIrCodes.h).
void blinkDoneIndicator() {
  delay(kDelayBeforeBlinkMs);
  for (uint8_t i = 0; i < kDoneBlinkCount; i++) {
    digitalWrite(IR_TX_PIN, HIGH);
    delay(kDoneBlinkOnMs);
    digitalWrite(IR_TX_PIN, LOW);
    delay(kDoneBlinkOffMs);
  }
}

void ensureIrInitialized() {
  static bool initialized = false;
  if (initialized) return;

  // DISABLE_LED_FEEDBACK: свою индикацию делаем вручную в blinkDoneIndicator() —
  // встроенный feedback-LED библиотеки не подходит (LED_BUILTIN не определён
  // для этой платы, да и физически это тот же пин, что и ИК-передатчик).
  irSender.begin(IR_TX_PIN, DISABLE_LED_FEEDBACK);
  initialized = true;
}

} // namespace

void sendIRCodes() {
  ensureIrInitialized();

  const IrCode* const* codes = (kSelectedRegion == kRegionEU) ? EUpowerCodes : NApowerCodes;
  const uint8_t numNACodes = NUM_ELEM(NApowerCodes);
  const uint8_t numEUCodes = NUM_ELEM(EUpowerCodes);
  const uint8_t numCodes = (kSelectedRegion == kRegionEU) ? numEUCodes : numNACodes;

  for (uint8_t i = 0; i < numCodes; i++) {
    sendSingleCode(codes[i]);
  }

  blinkDoneIndicator();
}



bool TvbGoneApp::Loop() {
  displayBigText("< READY >");
  if (StickCP2.BtnA.wasPressed()) {
    StickCP2.Display.fillRect(0, 0, StickCP2.Display.width(), StickCP2.Display.height(), BLACK);
    displayBigText("work...");
    sendIRCodes();
    updateActivity();
  }
  return true;
}
