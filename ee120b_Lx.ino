#include <LiquidCrystal.h>

byte canvas[8][8]; // 64 bytes
LiquidCrystal lcd(10, 11, 12, 5, 4, 3, 2); // RS~10, RW~11, E~12, D4~5, D5~4, D6~3, D7~2; 34 bytes

/**
 * 0 2 4 6
 * 1 3 5 7
 */

void Clear() {
  for (int k = 0; k < 8; ++k) {
    for (int i = 0; i < 8; ++i ) {
      canvas[k][i] = 0;
    }
  }
}
bool OnScreen(int x, int y) {
  return x >= 0 && x < 20 && y >= 0 && y < 16;
}
bool OnBoundary(int x, int y) {
  return x == 0 && x == 19 && y == 0 && y == 19;
}
void Set(int x, int y) {
  if (!OnScreen(x, y)) {
    return;
  }
  // chunk location (which of the 5x8 chars to draw onto)
  const int cx = x / 5;
  const int cy = y / 8;
  // note: y - cy*8 = y % 8
  // set a bit
  canvas[cy + cx*2][y - cy*8] |= 1 << (4 - (x - cx*5));
}

class Projectile {
 private: // 5 bytes
  char _x, _y, _dx, _dy;
  byte _state; // &1 ~ valid, &2 ~ friendly;
 public:
  Projectile() {}
  Projectile(char x, char y, char dx, char dy, bool friendly): _x(x), _y(y), _dx(dx), _dy(dy) {
    _state = 1 | (2 * friendly);
  }
  void Tick() {
    _x += _dx;
    _y += _dy;
    if (!OnScreen(_x, _y)) {
      _state &= 0xFE; // unset valid bit
    }
  }
  void Draw() {
    Set(_x, _y);
  }
  bool IsValid() const {
    return (_state & 1) == 1;
  }
  bool IsFriendly() const {
    return (_state & 2) == 2;
  }
};

int projectile_n = 0; // how many projectiles
Projectile projectiles[50]; // 250 bytes
inline void ProcessProjectiles() {
  int j = 0; // location of next projectile (how many valid were counted)
  for (int i = 0; i < projectile_n; ++i) {
    Projectile &projectile = projectiles[i];
    projectile.Draw();
    projectile.Tick();
    if (projectile.IsValid()) { // clear out unneeded projectiles
      if (i != j) {
        projectiles[j] = projectile;
      }
      ++j;
    }
  }
  projectile_n = j;
}
#define AddProjectile(p) projectiles[projectile_n++] = p;

class Ship {
 private:
  char _x, _y, _dx, _dy;
  byte _state; // s0 ~ alive, s1..s4 ~ fire cd, s5..s7 ~ move cd
  // 7 6 5 4 3 2 1 0
  // [   ] [     ] 
 public:
  Ship() {}
  Ship(char x, char y): _x(x), _y(y) {
    _dx = _dy = 1;
    _state = 1;
  }
  Ship(char x, char y, char dx, char dy): _x(x), _y(y), _dx(dx), _dy(dy) {
    _state = 1;
  }
  void Tick() {
    _state += 2;
    if ((_state & 0x0E) == 0x0E) { // periodic 
      AddProjectile(Projectile(_x, _y+1, 0, 1, false));
      if (!OnScreen(_x+_dx, 1)) {
        _dx *= -1; // change direction!
      }
      _x += _dx;
      if ((_state & 0xE0) == 0xE0) { // move every few times
        _y += _dy;
      }
      if (!OnScreen(_x, _y)) _state &= 0xFE; // unset valid bit
    }
  }
  void Draw() {
    Set(_x, _y);
    Set(_x, _y-1);
    Set(_x-1, _y-1);
    Set(_x+1, _y-1);
  }
  bool IsValid() const {
    return (_state & 1) == 1;
  }
};

int ship_n = 0;
Ship ships[20];
inline void ProcessShips() {
  int j = 0; // location of next projectile (how many valid were counted)
  for (int i = 0; i < ship_n; ++i) {
    Ship &ship = ships[i];
    ship.Draw();
    ship.Tick();
    if (ship.IsValid()) { // clear out unneeded projectiles
      if (i != j) {
        ships[j] = ship;
      }
      ++j;
    }
  }
  ship_n = j;
}
#define AddShip(x) ships[ship_n++] = x;

void setup() {
  lcd.begin(16, 2);
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 2; ++j) {
      lcd.setCursor(i, j);
      lcd.write(byte(i*2 + j));
    }
  }
  for (int i=0;i<15;i+=4)
  AddShip(Ship(i, 3, 1, 1));
//  AddProjectile(Projectile(2, 2, 0, 1, false));
//  for (int i = 0; i < 10; ++i) {
//    AddProjectile(Projectile(i, i, 0, 1, false));
//  }
//  Serial.begin(9600);
}

void loop() {
  Clear();
  ProcessShips();
  ProcessProjectiles();
  
  // push canvas to lcd
  for (int g = 0; g < 8; ++g) {
    lcd.createChar(g, canvas[g]);
  }
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 2; ++j) {
      lcd.setCursor(i, j);
      lcd.write(byte(i*2 + j));
    }
  }
  lcd.setCursor(15, 0);
  lcd.write("A");
  delay(100);
}
