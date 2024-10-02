#include <Adafruit_IS31FL3741.h>

#include "Font3x4N.h"
#include <Fonts/TomThumb.h>

#define MATRIX_WIDTH 13
#define MATRIX_HEIGHT 9

struct ShufflePixel
{
  uint16_t color;
  int x, y;                  // Current position
  int destX, destY;          // Final position for text
  int velX, velY;            // Velocity (-1, 0, 1 for X and Y)
  int speed;                 // Speed of the pixel (how often it moves)
  int moveCounter;           // Counter to control the speed
  int stepsInSameDirection;  // Counter for steps in the same direction
  int maxSteps;              // Maximum steps before changing direction
  bool reachedFinalPosition; // If pixel has reached final position
};

ShufflePixel pixels[MATRIX_WIDTH * MATRIX_HEIGHT];
int totalPixels = 0;

const uint16_t ShuffleColors[] = {
    Adafruit_IS31FL3741::color565(0xBB, 0x30, 0x00),
    Adafruit_IS31FL3741::color565(0xAA, 0x15, 0x00),
    Adafruit_IS31FL3741::color565(0xAA, 0x05, 0x00)};

extern volatile bool display;
TaskHandle_t matrix13x9TaskHandle = NULL;

Adafruit_IS31FL3741_QT_buffered matrix13x9;
char *matrix13x9Message;
int animationPauseMs = 2000;
int animationCounter = 100;
bool animationInit = false;
bool animationOut = false;
bool animationDone = false;
unsigned long animationFinishedAt;

void renderMatrix()
{
  matrix13x9.fillScreen(0);
  for (int i = 0; i < totalPixels; i++)
  {
    matrix13x9.drawPixel(pixels[i].x, pixels[i].y, pixels[i].color);
  }
  matrix13x9.show();
}

void movePixel(ShufflePixel &p, int maxDistToDest)
{
  if (!p.reachedFinalPosition)
  {
    if (p.moveCounter >= p.speed)
    {
      // change direction if max steps reached
      if (p.stepsInSameDirection >= p.maxSteps)
      {
        int distToDest = abs(p.destX - p.x) + abs(p.destY - p.y);
        int distToDestAxis;
        int distToEdge;
        bool awayFromDest = false;

        //
        // choose a velocity direction
        //

        // constrained to one direction if on edge of max distance border
        if (distToDest >= maxDistToDest)
        {
          if (p.destX == p.x || p.destY == p.y)
          {
            p.velX = p.destX == p.x ? 0 : (p.destX > p.x ? 1 : -1);
            p.velY = p.destY == p.y ? 0 : (p.destY > p.y ? 1 : -1);
            distToDestAxis = abs(p.destX - p.x) + abs(p.destY - p.y);
          }
          else
          {
            if (random(2))
            {
              p.velX = p.destX > p.x ? 1 : -1;
              p.velY = 0;
              distToDestAxis = abs(p.destX - p.x);
            }
            else
            {
              p.velX = 0;
              p.velY = p.destY > p.y ? 1 : -1;
              distToDestAxis = abs(p.destY - p.y);
            }
          }
        }
        // can choose any direction not leading directly off an edge
        else
        {
          bool awayFromDest;
          do
          {
            if (random(2))
            {
              p.velX = random(-1, 2);
              p.velY = 0;
              distToEdge = p.velX > 0 ? MATRIX_WIDTH - p.x : p.x;
              awayFromDest = p.destX - p.x > 0 && p.velX < 0 || p.destX - p.x < 0 && p.velX > 0;
            }
            else
            {
              p.velX = 0;
              p.velY = random(-1, 2);
              distToEdge = p.velY > 0 ? MATRIX_HEIGHT - p.y : p.y;
              awayFromDest = p.destY - p.y > 0 && p.velY < 0 || p.destY - p.y < 0 && p.velY > 0;
            }
          } while (distToEdge == 0);
        }

        //
        // choose move distance
        //

        int minMove = 3;
        int maxMove = 8;

        // if not moving, max "steps"
        if (p.velX == 0 && p.velY == 0)
        {
          minMove = 1;
          maxMove = min(maxMove, maxDistToDest - distToDest - 1);
        }

        if (distToDest >= maxDistToDest)
        {
          minMove = min(minMove, distToDestAxis);
          maxMove = min(maxMove, distToDestAxis);
        }
        else
        {
          minMove = min(minMove, distToEdge);
          maxMove = min(maxMove, distToEdge);

          if (awayFromDest)
          {
            minMove = min(minMove, (maxDistToDest - distToDest) / 2); // every step is like 2 because max dist is also decreasing as pixel moves away
            maxMove = min(maxMove, (maxDistToDest - distToDest) / 2);
          }
        }

        p.maxSteps = random(minMove, maxMove);
        p.stepsInSameDirection = 0;
      }

      p.x += p.velX;
      p.y += p.velY;
      p.stepsInSameDirection++;
      p.moveCounter = 0;

      if (maxDistToDest < 3 && p.x == p.destX && p.y == p.destY)
      {
        p.reachedFinalPosition = true;
      }
    }
    p.moveCounter++;
  }
}

void getRandomOuterEdgeCoords(int &x, int &y)
{
  int edge = random(4);
  switch (edge)
  {
  case 0: // Top edge
    x = random(MATRIX_WIDTH);
    y = -1;
    break;
  case 1: // Bottom edge
    x = random(MATRIX_WIDTH);
    y = MATRIX_HEIGHT;
    break;
  case 2: // Left edge
    x = -1;
    y = random(MATRIX_HEIGHT);
    break;
  case 3: // Right edge
    x = MATRIX_WIDTH;
    y = random(MATRIX_HEIGHT);
    break;
  }
}

void spawnOnOuterEdge(ShufflePixel &p)
{
  // Randomly choose an edge: top, bottom, left, or right
  // Initialize velocity to move inside the bounds of the matrix on first step
  int edge = random(4);
  switch (edge)
  {
  case 0: // Top edge
    p.x = random(MATRIX_WIDTH);
    p.y = -1;
    p.velX = 0;
    p.velY = 1;
    break;
  case 1: // Bottom edge
    p.x = random(MATRIX_WIDTH);
    p.y = MATRIX_HEIGHT;
    p.velX = 0;
    p.velY = -1;
    break;
  case 2: // Left edge
    p.x = -1;
    p.y = random(MATRIX_HEIGHT);
    p.velX = 1;
    p.velY = 0;
    break;
  case 3: // Right edge
    p.x = MATRIX_WIDTH;
    p.y = random(MATRIX_HEIGHT);
    p.velX = -1;
    p.velY = 0;
    break;
  }
  p.reachedFinalPosition = false;
  p.speed = 1;                // random(1, 4);     // Set random speed (1: fast, 2: medium, 3: slow)
  p.moveCounter = 0;          // Initialize move counter
  p.stepsInSameDirection = 0; // Initialize step counter for direction
  p.maxSteps = random(3, 8);  // Randomize the max number of steps before changing direction
}

void initializeCharPixels(int16_t x, int16_t y, char c, uint16_t color, uint8_t &glyphWidth)
{
  c -= (uint8_t)pgm_read_byte(&Font3x4N.first); // Adjust the character index

  GFXglyph *glyph = &(((GFXglyph *)pgm_read_ptr(&Font3x4N.glyph))[c]);
  uint8_t *bitmap = (uint8_t *)pgm_read_ptr(&Font3x4N.bitmap);

  uint16_t bo = pgm_read_word(&glyph->bitmapOffset);
  uint8_t w = pgm_read_byte(&glyph->width);
  uint8_t h = pgm_read_byte(&glyph->height);
  int8_t xo = pgm_read_byte(&glyph->xOffset);
  int8_t yo = pgm_read_byte(&glyph->yOffset);
  uint8_t xx, yy, bits = 0, bit = 0;

  for (yy = 0; yy < h; yy++)
  {
    for (xx = 0; xx < w; xx++)
    {
      if (!(bit++ & 7))
      {
        bits = pgm_read_byte(&bitmap[bo++]);
      }
      if (bits & 0x80)
      {
        ShufflePixel &p = pixels[totalPixels];
        p.color = color;
        p.destX = x + xo + xx;
        p.destY = y + yo + yy;
        spawnOnOuterEdge(p);
        totalPixels++;
      }
      bits <<= 1;
    }
  }

  glyphWidth = w;
}

void initializePixels()
{
  totalPixels = 0;
  uint8_t charWidth;

  int cursorX = 0;
  int cursorY = 3;

  String message = String(matrix13x9Message);
  int splitIdx = message.indexOf("|");

  String ticker = message.substring(0, splitIdx);
  for (int i = 0; i < ticker.length(); i++)
  {
    uint16_t textColor = ShuffleColors[i % 2];
    matrix13x9.setTextColor(textColor);
    initializeCharPixels(cursorX, cursorY, ticker[i], textColor, charWidth);
    cursorX += charWidth;
  }

  matrix13x9.setCursor(0, 8);
  cursorX = 0;
  cursorY = 8;

  String price = message.substring(splitIdx + 1);
  bool decimalNotSeen = true;
  for (int i = 0; i < price.length(); i++)
  {
    uint16_t textColor = price[i] != '.'
                             ? ShuffleColors[(i + decimalNotSeen) % 2]
                             : ShuffleColors[2];
    decimalNotSeen &= price[i] != '.';

    matrix13x9.setTextColor(textColor);
    initializeCharPixels(cursorX, cursorY, price[i], textColor, charWidth);
    cursorX += charWidth;
  }
}

bool animatePixels(int maxDistToDest)
{
  bool allReached = true;

  for (int i = 0; i < totalPixels; i++)
  {
    movePixel(pixels[i], maxDistToDest);
    if (!pixels[i].reachedFinalPosition)
    {
      allReached = false;
    }
  }

  renderMatrix();

  return allReached;
}

void Matrix13x9Task(void *parameters);

void Matrix13x9Setup()
{
  matrix13x9Message = new char[100];
  strcpy(matrix13x9Message, "NASB|2214");

  if (!matrix13x9.begin(IS3741_ADDR_DEFAULT))
  {
    log_e("13x9 not found");
    return;
  }

  // Set brightness to max and bring controller out of shutdown state
  matrix13x9.setLEDscaling(0x08);
  matrix13x9.setGlobalCurrent(0xFF);
  matrix13x9.fill(0);
  matrix13x9.enable(true); // bring out of shutdown
  matrix13x9.setRotation(0);
  matrix13x9.setTextWrap(false);
  matrix13x9.setFont(&Font3x4N);

  xTaskCreate(Matrix13x9Task, "Matrix13x9Task", 4096, NULL, 2, &matrix13x9TaskHandle);
  log_d("13x9 set up complete");
}

void Matrix13x9Task(void *parameters)
{
  log_d("Starting Matrix13x9Task");

  bool isEnabled = true;
  while (1)
  {
    if (!display)
    {
      if (isEnabled)
      {
        matrix13x9.enable(false);
        isEnabled = false;
      }
      delay(100);
      continue;
    }
    else if (!isEnabled)
    {
      matrix13x9.enable(true);
      isEnabled = true;
    }

    if (!animationInit)
    {
      initializePixels();
      animationInit = true;
      animationDone = false;
    }

    if (!animationDone)
    {
      animationCounter = max(0, animationCounter - 1);
      animationDone = animatePixels(animationCounter / 2);
      if (animationDone)
      {
        animationFinishedAt = millis();
      }
    }
    else if (millis() - animationFinishedAt > animationPauseMs)
    {
      animationDone = false;

      // set pixel destinations to outside of matrix
      if (!animationOut)
      {
        for (int i = 0; i < totalPixels; i++)
        {
          int x, y;
          getRandomOuterEdgeCoords(x, y);
          pixels[i].destX = x;
          pixels[i].destY = y;
          pixels[i].reachedFinalPosition = false;
        }
        animationOut = true;
        animationCounter = (13 + 9);
        animationPauseMs = 500;
      }
      else
      {
        animationInit = false;
        animationOut = false;
        animationCounter = (13 + 9) * 5;
        animationPauseMs = 2500;
      }
    }

    delay(50);
  }
}