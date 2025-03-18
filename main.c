#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 320

#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32

void renderBuffer(bool *buffer){
  int xPos = 1;
  int yPos = 1;
  for(int y = 0; y < SCREEN_HEIGHT; y++) {
    xPos = 0;
    for(int x = 0; x < SCREEN_WIDTH; x++) {
      DrawRectangle(xPos, yPos, 9, 9, (buffer[(y * SCREEN_WIDTH + x) * sizeof(bool)]) ? WHITE : BLACK);
      xPos += 10;
    }
    yPos += 10;
  }
}

int main() {
  // NOTE: allocate screen buffer
  bool *displayBuffer = (bool *) malloc(sizeof(bool) * SCREEN_HEIGHT * SCREEN_WIDTH);

  for(int i = 0; i < SCREEN_HEIGHT; i++) {
    for(int j = 0; j < SCREEN_WIDTH; j++) {
      displayBuffer[i * SCREEN_WIDTH + j] = true;
    }
  }

  for(int i = 0; i < SCREEN_HEIGHT; i++) {
    displayBuffer[(i * SCREEN_WIDTH + i)] = false;
  }

  // NOTE: setup raylib window
  InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "CHIP-8");

  // NOTE: Main Loop
  while(!WindowShouldClose()) {

    BeginDrawing();
      renderBuffer(displayBuffer);
    EndDrawing();
    
    WaitTime(1.0/60.0);
  }

  // NOTE: Cleanup
  CloseWindow();
  free(displayBuffer);

  return 0;
}
