#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <iostream>
#include <vector>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

const int pUP = 17;
const int pRIGHT = 5;
const int pLEFT = 16;
const int pDOWN = 4;

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


enum Direction{
  UP,
  DOWN,
  LEFT,
  RIGHT,
};
struct Part{
  int x;
  int y;
};
struct Berry{
  int x;
  int y;
  bool is_alive;
   Berry(int x, int y){
    x=x;
    y=y;
    is_alive=true;
  }
  void render(){
    display.drawRoundRect(display.width()/2 + x*4, display.height()/2 + y*4, 4, 4, 2, SSD1306_WHITE);
  }
};

void render_part(int x, int y){
  display.fillRoundRect(display.width()/2 + x*4, display.height()/2 + y*4, 4, 4, 2, SSD1306_WHITE);
}
void de_render_part(int x, int y){
  display.fillRoundRect(display.width()/2 + x*4, display.height()/2 + y*4, 4, 4, 2, SSD1306_BLACK);
}

struct Snake{
  Direction dir = Direction::RIGHT; 
  std::vector<Part> parts = {Part{x:0,y:0},Part{x:0,y:0},Part{x:0,y:0}};
  bool is_alive = true;

  void render_snake(){
    // display.clearDisplay();
    for (int i=0;i<parts.size();++i){
      render_part(parts[i].x,parts[i].y);
    }
    display.display();
  }
  void wrap_if_needed(){
    if (parts[0].x > 128/8 - 1) parts[0].x = -128/8;
    if (parts[0].x < -128/8) parts[0].x = 128/8;
    if (parts[0].y > 64/8 - 1) parts[0].y = -64/8;
    if (parts[0].y < -64/8) parts[0].y = 64/8;
  }

  void check_collision(){
    for (int i=1;i<parts.size();++i){
      if (parts[0].x == parts[i].x && parts[0].y == parts[i].y){
        is_alive = false;
      }
    }
  }

  void move(){
    de_render_part(parts[parts.size()-1].x,parts[parts.size()-1].y);
    for (int i=parts.size()-1;i>=1;--i){
      parts[i].x = parts[i-1].x;
      parts[i].y = parts[i-1].y;
    }
    if(dir == Direction::UP) parts[0].y--;
    if(dir == Direction::DOWN) parts[0].y++;
    if(dir == Direction::LEFT) parts[0].x--;
    if(dir == Direction::RIGHT) parts[0].x++;
    wrap_if_needed(); 
  }

  void check_input(){
    if (!digitalRead(pUP) && dir != Direction::DOWN) dir = Direction::UP;
    if (!digitalRead(pDOWN) && dir != Direction::UP) dir = Direction::DOWN;
    if (!digitalRead(pLEFT) && dir != Direction::RIGHT) dir = Direction::LEFT;
    if (!digitalRead(pRIGHT) && dir != Direction::LEFT) dir = Direction::RIGHT;
  }

  void await_restart(){
    if (!digitalRead(pUP) || !digitalRead(pDOWN) || !digitalRead(pLEFT) || !digitalRead(pRIGHT)){
      dir = Direction::RIGHT; 
      parts = {Part{x:0,y:0},Part{x:0,y:0},Part{x:0,y:0}};
      is_alive = true;
      display.clearDisplay();
    }
  }
  void animate_death(){
    for (int i=0;i<3;++i){
      for (int y=0;y<parts.size();++y){
        de_render_part(parts[y].x,parts[y].y);
      }
      display.display();
      delay(100);
      for (int y=0;y<parts.size();++y){
        render_part(parts[y].x,parts[y].y);
      }
      display.display();
      delay(100);
    }
    for (int i=parts.size()-1;i>=0;--i){
      de_render_part(parts[i].x,parts[i].y);
      display.display();
      delay(20);
    }
  }
};

int random(int min, int max) //range : [min, max]
{
   static bool first = true;
   if (first) 
   {  
      srand( time(NULL) ); //seeding for the first time only!
      first = false;
   }
   return min + rand() % (max + 1 - min);
}

Snake snake;
Berry berry = Berry(
  random(-64/8,64/8),
  random(-32/8,32/8)
);

void end_game(){
  snake.animate_death();
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,10);
  display.println(F("DEATH"));
  display.setTextSize(2);
  display.setTextColor(0,1);
  display.print(F("score "));
  display.println(snake.parts.size());
  display.setTextSize(1);
  display.setTextColor(1);
  display.print(F("press any button to restart"));
  display.display();
  delay(2000);
}

 
void handle_berry(){
  for (int i=0;i<snake.parts.size();++i){
    if (snake.parts[i].x == berry.x && snake.parts[i].y == berry.y){
      berry.is_alive = false;
      Serial.println("BERRY EATEN");
    }
  }
  if (berry.is_alive == false){
    de_render_part(berry.x,berry.y);
    snake.parts.push_back(snake.parts[snake.parts.size()-1]);
    berry.x = random(-64/8,64/8); 
    berry.y = random(-32/8,32/8);
    berry.is_alive=true;
    berry.render();
  }
}

void setup() {
  Serial.begin(921600);
  pinMode(pUP,INPUT_PULLUP);
  pinMode(pDOWN,INPUT_PULLUP);
  pinMode(pLEFT,INPUT_PULLUP);
  pinMode(pRIGHT,INPUT_PULLUP);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.clearDisplay();

  snake = Snake();

  snake.render_snake();
  delay(1000);
  
  berry.render();
}

void loop(){
  if (snake.is_alive){
    berry.render();
    // Serial.print(berry.x); Serial.print(" "); Serial.println(berry.y);
    snake.check_input();
    snake.move();
    snake.render_snake();
    snake.check_collision();
    if (snake.is_alive == false) end_game();
    handle_berry();
    delay(100);   
  }
  else{
    snake.await_restart();
  }
}