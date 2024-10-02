#include <Adafruit_LEDBackpack.h>
#include "dataTypes.hpp"

#define LED_COUNT 64
#define REFRESHTIME 15
#define BASEDELAY 20
#define MATRIXWIDTH 8
#define MATRIXHEIGHT 8

extern volatile bool display;
TaskHandle_t matrix8x8TaskHandle = NULL;
Adafruit_BicolorMatrix matrix8x8 = Adafruit_BicolorMatrix();

void Matrix8x8Task(void *parameters);

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

struct LocationWithDistance
{
  int x;
  int y;
  int distance;
};

bool operator==(const Location &lhs, const Location &rhs)
{
  return lhs.x == rhs.x && lhs.y == rhs.y;
}

const Direction Left = {-1, 0};
const Direction Right = {1, 0};
const Direction Up = {0, -1};
const Direction Down = {0, 1};
const Direction Directions[] = {Left, Right, Up, Down};

bool maze[MATRIXHEIGHT][MATRIXWIDTH];
Location runnerLoc = {-1, -1};
Location exitLoc = {-1, -1};

void shuffleDirections(Direction *list, int size)
{
  for (int i = 0; i < size; i++)
  {
    int index = random(size);
    Direction temp = list[i];
    list[i] = list[index];
    list[index] = temp;
  }
}

bool isWall(int x, int y)
{
  return maze[y][x];
}

bool isWall(Location loc)
{
  return isWall(loc.x, loc.y);
}

bool isInMazeBounds(int x, int y)
{
  return x >= 0 && x < MATRIXWIDTH && y >= 0 && y < MATRIXHEIGHT;
}

bool isInMazeBounds(Location loc)
{
  return isInMazeBounds(loc.x, loc.y);
}

int getAdjacentWallAndBorderCount(int x, int y)
{
  int count = 0;
  for (int i = 0; i < 4; i++)
  {
    int nx = x + Directions[i].x;
    int ny = y + Directions[i].y;
    if (nx < 0 || nx >= MATRIXWIDTH || ny < 0 || ny >= MATRIXHEIGHT || maze[ny][nx])
    {
      count++;
    }
  }
  return count;
}

int getAdjacentWallAndBorderCount(Location loc)
{
  return getAdjacentWallAndBorderCount(loc.x, loc.y);
}

void generateMaze()
{
  // fill maze with walls (true)
  for (int y = 0; y < MATRIXHEIGHT; y++)
  {
    for (int x = 0; x < MATRIXWIDTH; x++)
    {
      maze[y][x] = true;
    }
  }

  // define starting point randomly
  int edge = random(4);
  int x = random(MATRIXWIDTH);
  int y = random(MATRIXHEIGHT);
  Location start = {x, y};
  maze[start.y][start.x] = false;

  // create traversal stack with starting point
  Stack<Location> path = Stack<Location>();
  path.push(start);
  int maxCycles = 1000;

  while (!path.isEmpty() && maxCycles-- > 0)
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

void placeRunner()
{
  runnerLoc = {-1, -1};

  int attempts = 0;
  while (runnerLoc.x == -1)
  {
    int x = random(MATRIXWIDTH);
    int y = random(MATRIXHEIGHT);
    if (!isWall(x, y))
    {
      runnerLoc = {x, y};
    }
    attempts++;
  }
  log_d("Placing runner at (%d,%d) after %d attempts", runnerLoc.x, runnerLoc.y, attempts);
}

void placeExit()
{
  exitLoc = {-1, -1};

  int attempts = 0;
  while (exitLoc.x == -1)
  {
    int x = random(MATRIXWIDTH);
    int y = random(MATRIXHEIGHT);
    int distance = abs(x - runnerLoc.x) + abs(y - runnerLoc.y);
    int minDistance = max(0, (MATRIXWIDTH + MATRIXHEIGHT) / 2 - (attempts / 10));
    if (!isWall(x, y) && distance > minDistance)
    {
      exitLoc = {x, y};
    }
    attempts++;
  }
  log_d("Placing exit at (%d,%d) after %d attempts", exitLoc.x, exitLoc.y, attempts);
}

Stack<Location> findPathDfs(Location startLoc, Location endLoc)
{
  Stack<LocationWithDistance> locsToVisit = Stack<LocationWithDistance>();
  Set<Location> locsVisited = Set<Location>();
  Stack<Location> curPath = Stack<Location>();

  locsToVisit.push({startLoc.x, startLoc.y, 0});

  // 0 P 0 G
  // toVisit: 100
  // visited: 10,
  // curPath: 10

  // cur: 100

  while (!locsToVisit.isEmpty())
  {
    LocationWithDistance curLocWithDist = locsToVisit.top();
    Location curLoc = {curLocWithDist.x, curLocWithDist.y};

    int distance = curLocWithDist.distance;
    locsToVisit.pop();

    // found end, return path
    if (curLoc == endLoc)
    {
      log_d("Found path from (%d,%d) to (%d,%d)", startLoc.x, startLoc.y, curLoc.x, curLoc.y);
      curPath.push(curLoc);
      return curPath;
    }

    if (locsVisited.contains(curLoc))
    {
      log_d("Skipping visited location (%d,%d)", curLoc.x, curLoc.y);
      continue;
    }
    locsVisited.add(curLoc);

    // if curPath size is greater than distance than we need to unwind path to current distance
    while (curPath.size() > distance)
    {
      curPath.pop();
    }
    curPath.push(curLoc);

    // look in different directions randomly in case of loops for variety of potential paths
    Direction randSteps[4] = {Left, Right, Up, Down};
    shuffleDirections(randSteps, 4);

    for (Direction step : randSteps)
    {
      Location nextLoc = {curLoc.x + step.x, curLoc.y + step.y};
      if (isInMazeBounds(nextLoc) && !isWall(nextLoc) && !locsVisited.contains(nextLoc))
      {
        locsToVisit.push({nextLoc.x, nextLoc.y, distance + 1});
      }
    }
  }

  return Stack<Location>();
}

void MazeRunnerInit()
{
  log_d("Initializing maze");

  generateMaze();
  placeRunner();
  placeExit();

  // log maze in debug
  log_d("*--------*");
  for (int y = 0; y < MATRIXHEIGHT; y++)
  {
    String row = "|";
    for (int x = 0; x < MATRIXWIDTH; x++)
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

void MazeRunnerMove()
{
  if (runnerLoc == exitLoc)
  {
    log_d("Runner reached exit");
    delay(5000); // blocking is fine for now
    MazeRunnerInit();
  }

  // get path to exit
  Stack<Location> path = findPathDfs(runnerLoc, exitLoc);
  if (path.isEmpty())
  {
    log_d("No path found to exit ");
    return;
  }

  if (path.size() < 2)
  {
    log_e("Already at exit");
    return;
  }

  // second location in path is the next move (second last in stack)
  String pathStr = "";
  while (path.size() > 2)
  {
    pathStr += "(" + String(path.top().x) + "," + String(path.top().y) + ")<=";
    path.pop();
  }

  // now stack is [cirLoc, nextLoc]
  Location nextLoc = path.top();

  pathStr += "(" + String(nextLoc.x) + "," + String(nextLoc.y) + ")";
  log_d("Path to exit: %s", pathStr.c_str());

  if (isWall(nextLoc.x, nextLoc.y))
  {
    log_e("Next location is a wall, not moving");
    return;
  }

  int distance = abs(nextLoc.x - runnerLoc.x) + abs(nextLoc.y - runnerLoc.y);
  if (distance > 1)
  {
    log_e("Next location is not adjacent, not moving");
    return;
  }

  log_d("Moving runner from (%d,%d) to (%d,%d)", runnerLoc.x, runnerLoc.y, nextLoc.x, nextLoc.y);
  runnerLoc = nextLoc;
}

void MazeRunnerDraw()
{
  for (int y = 0; y < MATRIXHEIGHT; y++)
  {
    for (int x = 0; x < MATRIXWIDTH; x++)
    {
      matrix8x8.drawPixel(x, y, maze[y][x] ? LED_YELLOW : LED_OFF);
    }
  }

  matrix8x8.drawPixel(exitLoc.x, exitLoc.y, LED_RED);
  matrix8x8.drawPixel(runnerLoc.x, runnerLoc.y, LED_GREEN);
  matrix8x8.writeDisplay();
}

void Matrix8x8Setup()
{
  if (!matrix8x8.begin(0x70))
  {
    log_e("8x8 not found");
    return;
  }
  log_d("8x8 found!");

  matrix8x8.setBrightness(5);

  randomSeed(analogRead(0));

  MazeRunnerInit();

  xTaskCreate(Matrix8x8Task, "Matrix8x8Task", 4096, NULL, 2, &matrix8x8TaskHandle);

  log_d("8x8 setup complete");
}

void Matrix8x8Task(void *parameters)
{
  log_d("Starting Matrix8x8Task");

  while (1)
  {
    if (!display)
    {
      matrix8x8.fillScreen(LED_OFF);
      matrix8x8.writeDisplay();
      delay(100);
      continue;
    }

    MazeRunnerMove();
    MazeRunnerDraw();
    delay(500);
  }
}