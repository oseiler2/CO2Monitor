#include <featherMatrix.h>
#include <config.h>
#include <configManager.h>
#include <Fonts/TomThumb.h>

#define BRIGHTNESS 20

// MATRIX DECLARATION:
// Parameter 1 = width of DotStar matrix
// Parameter 2 = height of matrix
// Parameter 3 = pin number (most are valid)
// Parameter 4 = matrix layout flags, add together as needed:
//   DS_MATRIX_TOP, DS_MATRIX_BOTTOM, DS_MATRIX_LEFT, DS_MATRIX_RIGHT:
//     Position of the FIRST LED in the matrix; pick two, e.g.
//     DS_MATRIX_TOP + DS_MATRIX_LEFT for the top-left corner.
//   DS_MATRIX_ROWS, DS_MATRIX_COLUMNS: LEDs are arranged in horizontal
//     rows or in vertical columns, respectively; pick one or the other.
//   DS_MATRIX_PROGRESSIVE, DS_MATRIX_ZIGZAG: all rows/columns proceed
//     in the same order, or alternate lines reverse direction; pick one.
//   See example below for these values in action.
// Parameter 5 = pixel type:
//   DOTSTAR_BRG  Pixels are wired for BRG bitstream (most DotStar items)
//   DOTSTAR_GBR  Pixels are wired for GBR bitstream (some older DotStars)
//   DOTSTAR_BGR  Pixels are wired for BGR bitstream (APA102-2020 DotStars)

FeatherMatrix::FeatherMatrix(Model* _model, uint8_t dataPin, uint8_t clockPin) {
  this->model = _model;
  this->matrix = new Adafruit_DotStarMatrix(
    12, 6, dataPin, clockPin,
    DS_MATRIX_BOTTOM + DS_MATRIX_LEFT +
    DS_MATRIX_ROWS + DS_MATRIX_PROGRESSIVE,
    DOTSTAR_BGR);
  matrix->begin();
  matrix->setFont(&TomThumb);
  matrix->setTextWrap(false);
  matrix->setBrightness(BRIGHTNESS);

  for (byte i = 0; i < 3; i++) {
    matrix->fillScreen(matrix->Color(255 * (i == 0 ? 1 : 0), 255 * (i == 1 ? 1 : 0), 255 * (i == 2 ? 1 : 0)));
    matrix->show();
    vTaskDelay(pdMS_TO_TICKS(500));
  }
  matrix->fillScreen(0);
  matrix->show();

  cyclicTimer = new Ticker();
  cyclicTimer->attach(0.5, +[](FeatherMatrix* instance) { instance->timer(); }, this);
}

FeatherMatrix::~FeatherMatrix() {
  if (this->matrix) delete matrix;
  if (cyclicTimer) delete cyclicTimer;
};

void FeatherMatrix::update(uint16_t mask, TrafficLightStatus oldStatus, TrafficLightStatus newStatus) {
  if (newStatus == GREEN) {
    matrix->setTextColor(matrix->Color(0, 255, 0));
  } else if (newStatus == YELLOW) {
    matrix->setTextColor(matrix->Color(200, 255, 0));
  } else if (newStatus == RED) {
    matrix->setTextColor(matrix->Color(255, 0, 0));
  } else if (newStatus == DARK_RED) {
    matrix->setTextColor(matrix->Color(255, 0, 128));
  }
  matrix->fillScreen(0);
  matrix->setCursor(0, 5);
  scrollPosition = 0;
  if (model->getCo2() == 0) {
    strcpy(txt, "---");
  } else {
    sprintf(txt, "%u", model->getCo2());
  }

  int16_t x1, y1 = 0;
  uint16_t h = 0;

  matrix->getTextBounds(txt, 0, 0, &x1, &y1, &textWidth, &h);
  scrollWidth = textWidth - matrix->width();
  if (scrollWidth > 0) {
    scrollWidth += 2;
  }

  matrix->print(txt);
  matrix->show();
}


void FeatherMatrix::timer() {
  matrix->fillScreen(0);

  if (scrollWidth <= 0) return;

  if (scrollPosition == 0) scrollDirection = -1;
  else if (scrollPosition == -scrollWidth) scrollDirection = 1;
  scrollPosition += scrollDirection;

  matrix->setCursor(scrollPosition + (scrollWidth > 0 ? 1 : 0), 5);
  matrix->print(txt);
  matrix->show();
}