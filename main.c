#include "raylib.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 640
#define DEBUG_PADDING 0

#define INSTRUCTIONS_PER_SECOND 700
#define FRAMES_PER_SECOND 60

#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32

#define STACK_SIZE 16

#define RAM_SIZE 4096
#define RAM_OFFSET 512

#define FONT_OFFSET 0x050

// NOTE: Stack
typedef struct {
  uint16_t arr[STACK_SIZE];
  int top;
} Stack;

bool isFull(Stack *stack) { return stack->top == STACK_SIZE - 1; }

bool isEmpty(Stack *stack) { return stack->top == -1; }

void push(Stack *stack, uint16_t value) {

  if (isFull(stack)) {
    printf("Stack Overflow\n");
    exit(-1);
  }
  stack->arr[++stack->top] = value;
}

uint16_t pop(Stack *stack) {
  if (isEmpty(stack)) {
    printf("Stack Underflow\n");
    exit(-1);
  }
  return stack->arr[stack->top--];
}

#ifdef DEBUG_MODE
int count = 0;
#endif

void pollKeyPressed(bool *keys) {
  keys[0x0] = IsKeyDown(KEY_X);
  keys[0x1] = IsKeyDown(KEY_ONE);
  keys[0x2] = IsKeyDown(KEY_TWO);
  keys[0x3] = IsKeyDown(KEY_THREE);
  keys[0x4] = IsKeyDown(KEY_Q);
  keys[0x5] = IsKeyDown(KEY_W);
  keys[0x6] = IsKeyDown(KEY_E);
  keys[0x7] = IsKeyDown(KEY_A);
  keys[0x8] = IsKeyDown(KEY_S);
  keys[0x9] = IsKeyDown(KEY_D);
  keys[0xA] = IsKeyDown(KEY_Z);
  keys[0xB] = IsKeyDown(KEY_C);
  keys[0xC] = IsKeyDown(KEY_FOUR);
  keys[0xD] = IsKeyDown(KEY_R);
  keys[0xE] = IsKeyDown(KEY_F);
  keys[0xF] = IsKeyDown(KEY_V);
#ifdef DEBUG_MODE
  count++;
  for (int i = 0; i < 16; i++) {
    printf("%X : %d\n", i, keys[i]);
  }
  printf("%d \n\n", count);
#endif
}

// NOTE: renders the passed buffer to the screen
void renderBuffer(bool *buffer) {
  int blockWidth = WINDOW_WIDTH / SCREEN_WIDTH;
  int blockHeight = WINDOW_HEIGHT / SCREEN_HEIGHT;

  int xPos = DEBUG_PADDING;
  int yPos = DEBUG_PADDING;

  BeginDrawing();
  for (int y = 0; y < SCREEN_HEIGHT; y++) {
    xPos = DEBUG_PADDING;
    for (int x = 0; x < SCREEN_WIDTH; x++) {
      DrawRectangle(xPos, yPos, blockWidth - (DEBUG_PADDING * 2),
                    blockHeight - (DEBUG_PADDING * 2),
                    (buffer[(y * SCREEN_WIDTH + x) * sizeof(bool)]) ? GOLD 
                                                                    : ORANGE);
      xPos += blockWidth;
    }
    yPos += blockHeight;
  }
  EndDrawing();
}

int main(int argc, char **argv) {
  // NOTE: RAM
  unsigned char *ram = (unsigned char *)malloc(sizeof(char) * RAM_SIZE);

  // NOTE: Load Font into ram
  char font[] = {
      0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
      0x20, 0x60, 0x20, 0x20, 0x70, // 1
      0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
      0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
      0x90, 0x90, 0xF0, 0x10, 0x10, // 4
      0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
      0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
      0xF0, 0x10, 0x20, 0x40, 0x40, // 7
      0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
      0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
      0xF0, 0x90, 0xF0, 0x90, 0x90, // A
      0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
      0xF0, 0x80, 0x80, 0x80, 0xF0, // C
      0xE0, 0x90, 0x90, 0x90, 0xE0, // D
      0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
      0xF0, 0x80, 0xF0, 0x80, 0x80  // F
  };

  for (int i = 0; i < 80; i++) {
    ram[FONT_OFFSET + i] = font[i];
  }

  // NOTE: Allocate Stack
  Stack stack = {.top = -1};

  // NOTE: Program Counter
  uint16_t PC = 0x200;

  // NOTE: Index Register
  uint16_t I = 0;

  // NOTE: General Purpose Variable Register
  unsigned char V[16];

  // NOTE: allocate screen buffer
  char *fileName;

  // NOTE: Timers
  uint8_t delayTimer = 0;
  uint8_t soundTimer = 0;

  // NOTE: keys
  bool *keys = (bool *)malloc(sizeof(bool) * 16);

  if (argc == 2) {
    fileName = argv[1];
    printf("%s\n", fileName);
  } else {
    printf("Please input a rom filename.\n");
    return -1;
  }

  // NOTE: Open the rom file into
  printf("Opening File\n");
  FILE *romPtr = fopen(fileName, "rb");

  if (romPtr == NULL) {
    printf("Could Not Open File.\n");
    return EXIT_FAILURE;
  }

  int numReadBytes = fread(&ram[RAM_OFFSET], 1, RAM_SIZE - RAM_OFFSET, romPtr);
  printf("%d\n", numReadBytes);

  // NOTE: Dump ram
  for (int i = 0; i < RAM_OFFSET + numReadBytes; i++) {
    printf("%d : %d | ", i, ram[i]);
  }

  bool *displayBuffer =
      (bool *)malloc(sizeof(bool) * SCREEN_HEIGHT * SCREEN_WIDTH);

  for (int i = 0; i < SCREEN_HEIGHT; i++) {
    for (int j = 0; j < SCREEN_WIDTH; j++) {
      displayBuffer[i * SCREEN_WIDTH + j] = false;
    }
  }

  // NOTE: setup raylib window
  InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "window");

  // NOTE: Main Loop
  while (!WindowShouldClose()) {

    for (int Instruction = 0; Instruction < INSTRUCTIONS_PER_SECOND / FRAMES_PER_SECOND;
         Instruction++) {
      // NOTE: fetch
      uint16_t firstByte = ram[PC];
      uint16_t secondByte = ram[(PC + 1)];
      uint16_t instruction = (firstByte << 8) | secondByte;

      // NOTE: decode & exicute
      uint8_t nibble = (uint8_t)((instruction & 0xF000) >> 12);
      uint8_t X = (uint8_t)((instruction & 0x0F00) >> 8);
      uint8_t Y = (uint8_t)((instruction & 0x00F0) >> 4);
      uint8_t N = (uint8_t)((instruction & 0x000F));
      uint8_t NN = (uint8_t)((instruction & 0x00FF));
      uint16_t NNN = (uint16_t)((instruction & 0x0FFF));

      PC += 2;

#ifdef DEBUG_MODE
      printf("X: %X\n", X);
      printf("Y: %X\n", Y);

      printf("N: %X\n", N);
      printf("NN: %X\n", NN);
      printf("NNN: %X\n", NNN);
#endif

      renderBuffer(displayBuffer);
      pollKeyPressed(keys);
      switch (nibble) {
      // NOTE: Clear Screen
      case 0x0: {
        switch (NNN) {
        case 0x0E0: {
          for (int i = 0; i < SCREEN_HEIGHT; i++) {
            for (int j = 0; j < SCREEN_WIDTH; j++) {
              displayBuffer[i * SCREEN_WIDTH + j] = false;
            }
          }
          renderBuffer(displayBuffer);
#ifdef DEBUG_MODE
          printf("Clear Screen Instruction: %X - PC: %X\n", instruction, PC);
#endif
        } break;

        case 0x0EE: {
          PC = pop(&stack);
#ifdef DEBUG_MODE
          printf("Return Instruction: %X - PC: %X\n", instruction, PC);
#endif
        } break;

        default: {
#ifdef DEBUG_MODE
          printf("Unknown Instruction: %X - PC: %X\n", instruction, PC);
#endif
        } break;
        }
      } break;

      case 0x1: {
        PC = NNN;
#ifdef DEBUG_MODE
        printf("Jump Instruction: %X - PC: %X\n", instruction, PC);
#endif
      } break;

      case 0x2: {
        push(&stack, PC);
        PC = NNN;
#ifdef DEBUG_MODE
        printf("Call Instruction: %X - PC: %X\n", instruction, PC);
#endif
      } break;

      case 0x3: {
        if (V[X] == NN)
          PC += 2;
#ifdef DEBUG_MODE
        printf("Skip if equal Instrucwtion: %X - PC: %X\n", instruction, PC);
#endif
      } break;

      case 0x4: {
        if (V[X] != NN)
          PC += 2;
#ifdef DEBUG_MODE
        printf("Skip if not equal Instrucwtion: %X - PC: %X\n", instruction,
               PC);
#endif
      } break;

      case 0x5: {
        if (V[X] == V[Y])
          PC += 2;
#ifdef DEBUG_MODE
        printf("Skip if registers equal Instrucwtion: %X - PC: %X\n",
               instruction, PC);
#endif
      } break;

      case 0x9: {
        if (V[X] != V[Y])
          PC += 2;
#ifdef DEBUG_MODE
        printf("Skip if registers not equal Instrucwtion: %X - PC: %X\n",
               instruction, PC);
#endif
      } break;

      case 0x6: {
        V[X] = NN;
#ifdef DEBUG_MODE
        printf("Set Register Instruction: %X - PC: %X\n", instruction, PC);
#endif
      } break;

      case 0x7: {
        V[X] += NN;
#ifdef DEBUG_MODE
        printf("Add Register Instruction: %X - PC: %X\n", instruction, PC);
#endif
      } break;

      // NOTE: Arithmetic Instructions
      case 0x8: {
        switch (N) {
        case 0x0: {
          V[X] = V[Y];
#ifdef DEBUG_MODE
          printf("set VX to VY Instruction: %X - PC: %X\n", instruction, PC);
#endif
        } break;

        case 0x1: {
          V[X] = V[X] | V[Y];
#ifdef DEBUG_MODE
          printf("set VX to VX | VY Instruction: %X - PC: %X\n", instruction,
                 PC);
#endif
        } break;

        case 0x2: {
          V[X] = V[X] & V[Y];
#ifdef DEBUG_MODE
          printf("set VX to VX & VY Instruction: %X - PC: %X\n", instruction,
                 PC);
#endif
        } break;

        case 0x3: {
          V[X] = V[X] ^ V[Y];
#ifdef DEBUG_MODE
          printf("set VX to VX ^ VY Instruction: %X - PC: %X\n", instruction,
                 PC);
#endif
        } break;

        case 0x4: {
          uint8_t tempY = V[Y];
          uint8_t tempX = V[X];
          V[X] = V[X] + V[Y];
          if ((tempY + tempX) > 255) {
            V[0xF] = 1;
          } else {
            V[0xF] = 0;
          }
#ifdef DEBUG_MODE
          printf("set VX to VX + VY Instruction: %X - PC: %X\n", instruction,
                 PC);
#endif
        } break;

        case 0x5: {
          uint8_t tempY = V[Y];
          uint8_t tempX = V[X];
          V[X] = V[X] - V[Y];
          V[0xF] = 1;
          if (tempY > tempX)
            V[0xF] = 0;
#ifdef DEBUG_MODE
          printf("set VX to VX - VY Instruction: %X - PC: %X\n", instruction,
                 PC);
#endif
        } break;

        case 0x7: {
          uint8_t tempY = V[Y];
          uint8_t tempX = V[X];
          V[X] = V[Y] - V[X];
          V[0xF] = 1;
          if (tempX > tempY)
            V[0xF] = 0;
#ifdef DEBUG_MODE
          printf("set VX to VY - VX Instruction: %X - PC: %X\n", instruction,
                 PC);
#endif
        } break;

        case 0x6: {
          V[X] = V[Y];
          uint8_t tempX = V[X];
          V[X] = V[X] >> 1;
          V[0xF] = tempX & 0x0001;
#ifdef DEBUG_MODE
          printf("bit shift right Instruction: %X - PC: %X\n", instruction, PC);
#endif
        } break;

        case 0xE: {
          V[X] = V[Y];
          uint8_t tempX = V[X];
          V[X] = V[X] << 1;
          V[0xF] = (tempX & 0x80) >> 7;
#ifdef DEBUG_MODE
          printf("bit shift left Instruction: %X - PC: %X\n", instruction, PC);
#endif
        } break;
        }
      } break;

      case 0xA: {
        I = NNN;
#ifdef DEBUG_MODE
        printf("Set I Instruction: %X - PC: %X\n", instruction, PC);
#endif
      } break;

      case 0xB: {
        PC = NNN + V[0x0];
#ifdef DEBUG_MODE
        printf("Jump with offset Instruction: %X - PC: %X\n", instruction, PC);
#endif
      } break;

      case 0xC: {
        int random = (rand() % 256);
        V[X] = random & NN;
#ifdef DEBUG_MODE
        printf("Random Instruction: %X - PC: %X\n", instruction, PC);
#endif
      } break;

      case 0xD: {
        uint8_t xCoords = (V[X] % 64);
        uint8_t yCoords = (V[Y] % 32);

        V[0xF] = 0;

        for (int i = 0; i < N; i++) {
          unsigned char byte = ram[I + i];

          unsigned char mask = 128;
          xCoords = (V[X] % 64);
          for (int j = 0; j < 8; j++) {
            if ((byte & mask) > 0) {
              bool pixle = displayBuffer[yCoords * SCREEN_WIDTH + xCoords];
              if (pixle) {
                displayBuffer[yCoords * SCREEN_WIDTH + xCoords] = false;
                V[0xF] = 1;
              } else {
                displayBuffer[yCoords * SCREEN_WIDTH + xCoords] = true;
              }
            }
            xCoords++;
            if (xCoords >= 64) {
              break;
            }
            mask = mask >> 1;
          }
          yCoords++;
          if (yCoords >= 32) {
            break;
          }
        }

#ifdef DEBUG_MODE
        printf("Draw Instruction: %X - PC: %X\n", instruction, PC);
#endif
      } break;

      case 0xE: {
        switch (NN) {
        case 0x9E: {
          if (keys[V[X]]) {
            PC += 2;
          }
#ifdef DEBUG_MODE
          printf("Skip if key Instruction: %X - PC: %X\n", instruction, PC);
#endif
        } break;

        case 0xA1: {
          if (!keys[V[X]]) {
            PC += 2;
          }
#ifdef DEBUG_MODE
          printf("Skip if not key Instruction: %X - PC: %X\n", instruction, PC);
#endif
        } break;
        }
      } break;

      case 0xF: {
        switch (NN) {
        case 0x07: {
          V[X] = delayTimer;
#ifdef DEBUG_MODE
          printf("set delay timer Instruction: %X - PC: %X\n", instruction, PC);
#endif
        } break;

        case 0x15: {
          delayTimer = V[X];
#ifdef DEBUG_MODE
          printf("get delay timer Instruction: %X - PC: %X\n", instruction, PC);
#endif
        } break;

        case 0x18: {
          soundTimer = V[X];
#ifdef DEBUG_MODE
          printf("set sound timer Instruction: %X - PC: %X\n", instruction, PC);
#endif
        } break;

        case 0x1E: {
          if (I + V[X] > 0xFFF) {
            V[0xF] = 1;
          }
          I += V[X];
#ifdef DEBUG_MODE
          printf("Add to index Instruction: %X - PC: %X\n", instruction, PC);
#endif
        } break;

        case 0x0A: {
          PC -= 2;
          for (int i = 0; i < 0x10; i++) {
            if (keys[i]) {
              V[X] = i;
              PC += 2;
              break;
            }
          }
#ifdef DEBUG_MODE
          printf("Get key Instruction: %X - PC: %X\n", instruction, PC);
#endif
        } break;

        case 0x29: {
          I = FONT_OFFSET + V[X] * 5;
#ifdef DEBUG_MODE
          printf("Font Character Instruction: %X - PC: %X\n", instruction, PC);
#endif
        } break;

        case 0x33: {
          ram[I] = (uint8_t)(V[X] / 100);
          ram[I + 1] = (uint8_t)((V[X] % 100) / 10);
          ram[I + 2] = (uint8_t)((V[X] % 100) % 10);
#ifdef DEBUG_MODE
          printf("Binary-coded decimal conversion Instruction: %X - PC: %X\n",
                 instruction, PC);
#endif
        } break;

        case 0x55: {
          for (int i = 0; i <= X; i++) {
            ram[I + i] = V[i];
          }
#ifdef DEBUG_MODE
          printf("Store registers to mem Instruction: %X - PC: %X\n",
                 instruction, PC);
#endif
        } break;

        case 0x65: {
          for (int i = 0; i <= X; i++) {
            V[i] = ram[I + i];
          }
#ifdef DEBUG_MODE
          printf("load registers from mem Instruction: %X - PC: %X\n",
                 instruction, PC);
#endif
        } break;

        default: {
#ifdef DEBUG_MODE
          printf("Unknown Instruction: %X - PC: %X\n", instruction, PC);
#endif
        } break;
        }
      } break;

      default: {
#ifdef DEBUG_MODE
        printf("Unknown Instruction: %X - PC: %X\n", instruction, PC);
#endif
      } break;
      }
    }

    // NOTE: Update Timers
    if (delayTimer > 0) {
      delayTimer--;
    }
    if (soundTimer > 0) {
      soundTimer--;
      printf("beep");
    }

    WaitTime(1.0 / (float)FRAMES_PER_SECOND);
    // getchar();
  }

  // NOTE: Cleanup
  CloseWindow();
  free(displayBuffer);
  free(ram);

  return 0;
}
