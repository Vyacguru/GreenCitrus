#include <citro2d.h>
#include <citro3d.h>

#include <stdio.h>

#define UPPER_SCREEN_UNKNOWN "Upper: Unknown"
#define LOWER_SCREEN_UNKNOWN " | Lower: Unknown"
#define UPPER_SCREEN_IPS "Upper: IPS"
#define LOWER_SCREEN_IPS " | Lower: IPS"
#define UPPER_SCREEN_TN "Upper: TN"
#define LOWER_SCREEN_TN " | Lower: TN"

// Checks if the device is a New 3DS model.
[[nodiscard]] bool UtilsIsN3Ds(void) {
  bool is_new_3ds = false;
  return R_SUCCEEDED(APT_CheckNew3DS(&is_new_3ds)) && is_new_3ds;
}

// Determines the screen types and returns a descriptive string.
char* SystemGetScreenType(void) {
  static char screen_type[32] = {0};

  if (UtilsIsN3Ds()) {
    u8 screens = 0;

    if (R_SUCCEEDED(gspLcdInit())) {
      if (R_SUCCEEDED(GSPLCD_GetVendors(&screens))) {
        gspLcdExit();
      }
    }

    const char* upper_screen;
    const char* lower_screen;

    switch ((screens >> 4) & 0xF) {
      case 0x01:
        upper_screen = UPPER_SCREEN_IPS;
        break;
      case 0x0C:
        upper_screen = UPPER_SCREEN_TN;
        break;
      default:
        upper_screen = UPPER_SCREEN_UNKNOWN;
        break;
    }

    switch (screens & 0xF) {
      case 0x01:
        lower_screen = LOWER_SCREEN_IPS;
        break;
      case 0x0C:
        lower_screen = LOWER_SCREEN_TN;
        break;
      default:
        lower_screen = LOWER_SCREEN_UNKNOWN;
        break;
    }

    snprintf(screen_type, sizeof(screen_type), "%s%s", upper_screen, lower_screen);
  } else {
    snprintf(screen_type, sizeof(screen_type), "%s%s", UPPER_SCREEN_TN, LOWER_SCREEN_TN);
  }

  return screen_type;
}

u8 battery_percent = 0;
u8 battery_status = 0;
bool is_connected = false;
bool is_bottom_screen = false;

// Prints static information on the console.
void PrintStaticInfo(void) {
  printf("\x1b[1;0H\x1b[32mGreenCitrus\x1b[0m \nTool for de-yellow'ing TN screens"
        "\x1b[4;0H\x1b[31;1m*\x1b[0m Screen type:\x1b[31;1m %s \x1b[0m"
        "\x1b[6;0H\x1b[34;1m*\x1b[0m Battery percentage:"
        "\x1b[8;0H\x1b[34;1m*\x1b[0m Adapter state:"
        "\x1b[10;0H\x1b[32m*\x1b[0m Selected screen:"
        "\x1b[11;0HPress \x1b[32mUP\x1b[0m or \x1b[32mDOWN\x1b[0m to select screen"
        "\x1b[30;0HPress\x1b[31;1m START\x1b[0m to exit", SystemGetScreenType()
  );
}

// Prints dynamic information on the console.
void PrintDynamicInfo(void) {
  if (R_SUCCEEDED(MCUHWC_GetBatteryLevel(&battery_percent))) {
    printf("\x1b[6;22H\x1b[34;1m%3d%%\x1b[0m ", battery_percent);
  } else {
    printf("\x1b[34;1mN/A\x1b[0m ");
  }

  if (R_SUCCEEDED(PTMU_GetBatteryChargeState(&battery_status))) {
    printf("(\x1b[34;1m%s\x1b[0m)     \n", battery_status ? "charging" : "not charging");
  } else {
    printf("(\x1b[34;1mN/A\x1b[0m)\n");
  }

  printf("\x1b[8;18H");
  if (R_SUCCEEDED(PTMU_GetAdapterState(&is_connected))) {
    printf("\x1b[34;1m%s\x1b[0m\n", is_connected ? "connected   " : "disconnected");
  } else {
    printf("\x1b[34;1mN/A\x1b[0m\n");
  }

  printf("\x1b[10;19H \x1b[32m>\x1b[0m%s", is_bottom_screen ? "bottom" : "top");
}

// Draws a frame with the specified color.
void DrawFrame(C3D_RenderTarget* screen, u32 color) {
  C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
  C2D_TargetClear(screen, color);
  C2D_SceneBegin(screen);
  C3D_FrameEnd(0);
}

// Initializes necessary services.
void InitServices(void) {
  mcuHwcInit();  // Battery info
  ptmuInit();    // Charger info
  gspLcdInit();  // Screen brightness
}

// Terminates previously initialized services.
void TerminateServices(void) {
  ptmuExit();    // Battery info
  mcuHwcExit();  // Charger info
  gspLcdExit();  // Screen brightness
}

int main(void) {
  // Initialize libraries.
  gfxInitDefault(); // Initialize framebuffer
  C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
  C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
  C2D_Prepare();
  PrintConsole console_top, console_bottom;
  consoleInit(GFX_TOP, &console_top);
  consoleInit(GFX_BOTTOM, &console_bottom);
  InitServices();
  GSPLCD_SetBrightness(GSPLCD_SCREEN_BOTH, 5);

  // Create screens.
  C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
  C3D_RenderTarget* bot = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
  C3D_RenderTarget* screen = top;

  // Create colors.
  u32 clr_white = C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF);
  u32 clr_black = C2D_Color32(0x00, 0x00, 0x00, 0x00);

  DrawFrame(screen, clr_white);
  consoleSelect(&console_bottom);
  PrintStaticInfo();

  // Main loop.
  while (aptMainLoop()) {
    hidScanInput();

    // Respond to user input.
    u32 k_down = hidKeysDown();
    if (k_down & KEY_START) break;  // Break to return to hbmenu.

    PrintDynamicInfo();

    if (k_down & KEY_DDOWN || k_down & KEY_DUP) {
      DrawFrame(screen, clr_black);
      screen = (k_down & KEY_DDOWN) ? bot : top;
      consoleSelect((k_down & KEY_DDOWN) ? &console_top : &console_bottom);
      PrintStaticInfo();
      DrawFrame(screen, clr_white);
    }
  }

  // Deinitialize libraries.
  C2D_Fini();
  C3D_Fini();
  TerminateServices();
  gfxExit();
  return 0;
}
