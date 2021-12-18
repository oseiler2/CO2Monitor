#ifndef _FEATHER_MATRIX_H
#define _FEATHER_MATRIX_H

#include <model.h>
#include <Adafruit_DotStarMatrix.h>

class FeatherMatrix {
public:
  FeatherMatrix(Model* model);
  ~FeatherMatrix();

  void update();

private:
  Model* model;

  Adafruit_DotStarMatrix* matrix;
};

#endif

