#pragma once

#include <Adafruit_LEDBackpack.h>
#include <queue>
#include <stack>
#include <tuple>
#include <unordered_map>
#include <unordered_set>

#include "task_handler.h"

using namespace std;

extern volatile bool display;

struct Direction
{
  int x;
  int y;
};

struct Location
{
  int x;
  int y;
};

bool operator==(const Location &lhs, const Location &rhs)
{
  return lhs.x == rhs.x && lhs.y == rhs.y;
}

bool operator!=(const Location &lhs, const Location &rhs)
{
  return !(lhs == rhs);
}

namespace std
{
  template <>
  struct hash<Location>
  {
    size_t operator()(const Location &loc) const
    {
      return hash<int>()(loc.x) ^ hash<int>()(loc.y);
    }
  };
}

const Direction Left = {-1, 0};
const Direction Right = {1, 0};
const Direction Up = {0, -1};
const Direction Down = {0, 1};
const Direction Directions[] = {Left, Right, Up, Down};

class Matrix8x8TaskHandler : public TaskHandler
{
private:
  static const uint8_t I2C_ADDR = 0x70;
  static const uint8_t WIDTH = 8;
  static const uint8_t HEIGHT = 8;

  Adafruit_BicolorMatrix matrix = Adafruit_BicolorMatrix();
  bool maze[HEIGHT][WIDTH];
  Location runnerLoc = {-1, -1};
  Location exitLoc = {-1, -1};
  int distanceToExit = -1;

public:
  Matrix8x8TaskHandler() {}
  bool createTask() override;

private:
  void task(void *parameters) override;
  void shuffleDirections(Direction *list, int size);
  bool isWall(int x, int y);
  bool isWall(Location loc);
  bool isInMazeBounds(int x, int y);
  bool isInMazeBounds(Location loc);
  int getAdjacentWallAndBorderCount(int x, int y);
  int getAdjacentWallAndBorderCount(Location loc);
  void generateMaze();
  void placeRunner();
  void placeExit();

  vector<Location> findPathDfs(Location startLoc, Location endLoc, int maxDistToEnd = -1);
  vector<Location> findPathBfs(Location startLoc, Location endLoc);

  void mazeRunnerInit();
  void mazeRunnerMove();
  void mazeRunnerDraw();
};

bool Matrix8x8TaskHandler::createTask()
{
  if (_taskHandle != NULL)
  {
    log_w("Task already started");
    return false;
  }

  if (!matrix.begin(I2C_ADDR))
  {
    log_e("Matrix not found");
    return false;
  }

  matrix.setBrightness(5);

  randomSeed(analogRead(0));

  mazeRunnerInit();

  xTaskCreate(taskWrapper, "Matrix8x8Task", 4096, this, 2, &_taskHandle);

  log_d("Matrix setup complete");

  return true;
}

void Matrix8x8TaskHandler::task(void *parameters)
{
  log_d("Starting Matrix8x8Task");

  while (1)
  {
    if (!display)
    {
      matrix.fillScreen(LED_OFF);
      matrix.writeDisplay();
      delay(100);
      continue;
    }

    mazeRunnerMove();
    mazeRunnerDraw();
    delay(50);
  }
}

void Matrix8x8TaskHandler::shuffleDirections(Direction *list, int size)
{
  for (int i = 0; i < size; i++)
  {
    int index = random(size);
    Direction temp = list[i];
    list[i] = list[index];
    list[index] = temp;
  }
}

bool Matrix8x8TaskHandler::isWall(int x, int y)
{
  return maze[y][x];
}

bool Matrix8x8TaskHandler::isWall(Location loc)
{
  return isWall(loc.x, loc.y);
}

bool Matrix8x8TaskHandler::isInMazeBounds(int x, int y)
{
  return x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT;
}

bool Matrix8x8TaskHandler::isInMazeBounds(Location loc)
{
  return isInMazeBounds(loc.x, loc.y);
}

int Matrix8x8TaskHandler::getAdjacentWallAndBorderCount(int x, int y)
{
  int count = 0;
  for (int i = 0; i < 4; i++)
  {
    int nx = x + Directions[i].x;
    int ny = y + Directions[i].y;
    if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT || maze[ny][nx])
    {
      count++;
    }
  }
  return count;
}

int Matrix8x8TaskHandler::getAdjacentWallAndBorderCount(Location loc)
{
  return getAdjacentWallAndBorderCount(loc.x, loc.y);
}

void Matrix8x8TaskHandler::generateMaze()
{
  // fill maze with walls (true)
  for (int y = 0; y < HEIGHT; y++)
  {
    for (int x = 0; x < WIDTH; x++)
    {
      maze[y][x] = true;
    }
  }

  // define starting point randomly if exit is not previously defined (ensures space for runner start loc)
  Location start = {-1, -1};
  if (exitLoc.x == -1 || exitLoc.y == -1)
  {
    int edge = random(4);
    int x = random(WIDTH);
    int y = random(HEIGHT);
    start = {x, y};
  }
  else
  {
    start = exitLoc;
  }
  maze[start.y][start.x] = false;

  // create traversal stack with starting point
  stack<Location> path = stack<Location>();
  path.push(start);
  int maxCycles = 1000;

  while (!path.empty() && maxCycles-- > 0)
  {
    Location cur = path.top();

    // shuffle directions randomly
    Direction randSteps[4] = {Left, Right, Up, Down};
    shuffleDirections(randSteps, 4);

    // try to move in each direction, pop location if no path forward
    bool foundPath = false;
    for (int i = 0; i < 4; i++)
    {
      Location nextLoc = {cur.x + randSteps[i].x, cur.y + randSteps[i].y};
      if (isInMazeBounds(nextLoc) && isWall(nextLoc) && getAdjacentWallAndBorderCount(nextLoc) >= 3)
      {
        maze[nextLoc.y][nextLoc.x] = false;
        path.push(nextLoc);
        foundPath = true;
        break;
      }
    }

    if (!foundPath)
    {
      path.pop();
    }
  }
  log_d("Maze generation complete");
}

void Matrix8x8TaskHandler::placeRunner()
{
  runnerLoc = {-1, -1};

  if (exitLoc.x != -1 && exitLoc.y != -1)
  {
    runnerLoc = exitLoc; // using last exit as start location
    return;
  }

  int attempts = 0;
  while (runnerLoc.x == -1)
  {
    int x = random(WIDTH);
    int y = random(HEIGHT);
    if (!isWall(x, y))
    {
      runnerLoc = {x, y};
    }
    attempts++;
  }
  log_d("Placing runner at (%d,%d) after %d attempts", runnerLoc.x, runnerLoc.y, attempts);
}

void Matrix8x8TaskHandler::placeExit()
{
  exitLoc = {-1, -1};

  int attempts = 0;
  while (exitLoc.x == -1)
  {
    int x = random(WIDTH);
    int y = random(HEIGHT);
    int distance = abs(x - runnerLoc.x) + abs(y - runnerLoc.y);
    int minDistance = max(0, (WIDTH + HEIGHT) / 2 - (attempts / 10));
    if (!isWall(x, y) && distance > minDistance)
    {
      exitLoc = {x, y};
    }
    attempts++;
  }
  log_d("Placing exit at (%d,%d) after %d attempts", exitLoc.x, exitLoc.y, attempts);
}

vector<Location> Matrix8x8TaskHandler::findPathDfs(Location startLoc, Location endLoc, int maxDistToEnd)
{
  stack<pair<Location, int>> locsToVisit = stack<pair<Location, int>>();
  unordered_set<Location> locsVisited = unordered_set<Location>();
  stack<Location> curPath = stack<Location>();

  locsToVisit.push({startLoc, 0});

  while (!locsToVisit.empty())
  {
    pair<Location, int> curLocAndDist = locsToVisit.top();
    Location curLoc = curLocAndDist.first;
    int distFromStart = curLocAndDist.second;
    locsToVisit.pop();

    // if curPath size is greater than distance than we need to unwind path to current distance
    while (curPath.size() > distFromStart)
    {
      curPath.pop();
    }
    curPath.push(curLoc);

    // found end, return path in vector form
    if (curLoc == endLoc)
    {
      log_v("Found path from (%d,%d) to (%d,%d)", startLoc.x, startLoc.y, curLoc.x, curLoc.y);

      std::vector<Location> path;
      while (!curPath.empty())
      {
        path.push_back(curPath.top());
        curPath.pop();
      }

      std::reverse(path.begin(), path.end());
      return path;
    }

    locsVisited.insert(curLoc);

    // don't visit locations further than maxDistToEnd
    if (maxDistToEnd > 0 && (distFromStart + 1) > maxDistToEnd)
    {
      continue;
    }

    // look in different directions randomly in case of loops for variety of potential paths
    Direction randSteps[4] = {Left, Right, Up, Down};
    shuffleDirections(randSteps, 4);

    for (Direction step : randSteps)
    {
      Location nextLoc = {curLoc.x + step.x, curLoc.y + step.y};
      if (isInMazeBounds(nextLoc) && !isWall(nextLoc) && !locsVisited.count(nextLoc))
      {
        locsToVisit.push({nextLoc, distFromStart + 1});
      }
    }
  }

  return vector<Location>();
}

vector<Location> Matrix8x8TaskHandler::findPathBfs(Location startLoc, Location endLoc)
{
  queue<Location> locsToVisit = queue<Location>();
  unordered_set<Location> locsVisited = unordered_set<Location>();
  unordered_map<Location, Location> visitedFrom = unordered_map<Location, Location>();

  locsToVisit.push(startLoc);
  visitedFrom[startLoc] = startLoc; // special case start location, visited from itself

  while (!locsToVisit.empty())
  {
    Location curLoc = locsToVisit.front();
    locsToVisit.pop();

    // found end, return path in vector form
    if (curLoc == endLoc)
    {
      log_d("Found path from (%d,%d) to (%d,%d)", startLoc.x, startLoc.y, curLoc.x, curLoc.y);

      vector<Location> path = vector<Location>();
      while (curLoc != startLoc)
      {
        path.push_back(curLoc);
        curLoc = visitedFrom[curLoc];
      }
      path.push_back(startLoc);

      std::reverse(path.begin(), path.end());
      return path;
    }

    locsVisited.insert(curLoc);

    // look in different directions randomly in case of loops for variety of potential paths
    Direction randSteps[4] = {Left, Right, Up, Down};
    shuffleDirections(randSteps, 4);

    for (Direction step : randSteps)
    {
      Location nextLoc = {curLoc.x + step.x, curLoc.y + step.y};
      if (isInMazeBounds(nextLoc) && !isWall(nextLoc) && !locsVisited.count(nextLoc))
      {
        locsToVisit.push(nextLoc);
        visitedFrom[nextLoc] = curLoc;
      }
    }
  }

  return vector<Location>();
}

void Matrix8x8TaskHandler::mazeRunnerInit()
{
  log_d("Initializing maze");

  generateMaze();
  placeRunner();
  placeExit();

  // log maze in debug
  log_d("*--------*");
  for (int y = 0; y < HEIGHT; y++)
  {
    String row = "|";
    for (int x = 0; x < WIDTH; x++)
    {
      char c = isWall(x, y) ? '#' : ' ';
      c = (runnerLoc.x == x && runnerLoc.y == y) ? 'S' : c;
      c = (exitLoc.x == x && exitLoc.y == y) ? 'E' : c;
      row += c;
    }
    row += "|";
    log_d("%s", row);
  }
  log_d("*--------*");
}

void Matrix8x8TaskHandler::mazeRunnerMove()
{
  if (runnerLoc == exitLoc)
  {
    log_d("Runner reached exit");
    mazeRunnerInit();
    return;
  }

  try
  {
    // vector<Location> path = findPathBfs(runnerLoc, exitLoc);
    vector<Location> path = findPathDfs(runnerLoc, exitLoc, distanceToExit);

    if (path.size() < 2)
    {
      log_d("No path found from (%d,%d) to (%d,%d)", runnerLoc.x, runnerLoc.y, exitLoc.x, exitLoc.y);
      return;
    }

    if (runnerLoc != path[0])
    {
      log_d("Runner is not at expected location (%d,%d)", path[0].x, path[0].y);
      return;
    }

    int stepDistance = abs(path[1].x - path[0].x) + abs(path[1].y - path[0].y);
    if (stepDistance != 1)
    {
      log_d("First step is not one step: (%d,%d) to (%d,%d)", path[0].x, path[0].y, path[1].x, path[1].y);
      return;
    }

    runnerLoc = path[1];
    distanceToExit = path.size() - 2; // -1 for included start, -1 for next step

    log_d("Moved runner from (%d,%d) to (%d,%d) with dist %d", path[0].x, path[0].y, path[1].x, path[1].y, distanceToExit);
  }
  catch (exception &e)
  {
    log_e("MazeRunnerMove Exception: %s", e.what());
  }
}

void Matrix8x8TaskHandler::mazeRunnerDraw()
{
  for (int y = 0; y < HEIGHT; y++)
  {
    for (int x = 0; x < WIDTH; x++)
    {
      matrix.drawPixel(x, y, maze[y][x] ? LED_YELLOW : LED_OFF);
    }
  }

  matrix.drawPixel(exitLoc.x, exitLoc.y, LED_RED);
  matrix.drawPixel(runnerLoc.x, runnerLoc.y, LED_GREEN);
  matrix.writeDisplay();
}
