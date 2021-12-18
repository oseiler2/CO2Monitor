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

FeatherMatrix::FeatherMatrix(Model* _model) {
  this->model = _model;
#if defined(FEATHER_MATRIX_DATAPIN) && defined(FEATHER_MATRIX_CLOCKPIN)
  this->matrix = new Adafruit_DotStarMatrix(
    12, 6, FEATHER_MATRIX_DATAPIN, FEATHER_MATRIX_CLOCKPIN,
    DS_MATRIX_BOTTOM + DS_MATRIX_LEFT +
    DS_MATRIX_ROWS + DS_MATRIX_PROGRESSIVE,
    DOTSTAR_BGR);
#endif
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
}

FeatherMatrix::~FeatherMatrix() {
  if (this->matrix) delete matrix;
};

void FeatherMatrix::update() {
  if (model->getCo2() < config.yellowThreshold) {
    matrix->setTextColor(matrix->Color(0, 255, 0));
  } else if (model->getCo2() < config.redThreshold) {
    matrix->setTextColor(matrix->Color(200, 255, 0));
  } else if (model->getCo2() < config.darkRedThreshold) {
    matrix->setTextColor(matrix->Color(255, 0, 0));
  } else {
    matrix->setTextColor(matrix->Color(255, 0, 128));
  }
  matrix->fillScreen(0);
  matrix->setCursor(0, 5);
  if (model->getCo2() == 0) {
    matrix->print("----");
  } else {
    matrix->printf("%u", model->getCo2());
  }
  matrix->show();
}
