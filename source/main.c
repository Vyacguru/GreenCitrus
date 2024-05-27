#include <citro2d.h>
#include <citro3d.h>

#include <string.h>
#include <stdio.h>


// [[nodiscard]] attribute indicates that the return value of the function should not be ignored.
[[nodiscard]] bool UtilsIsN3Ds(void) {
  bool is_new_3ds = false;
  return R_SUCCEEDED(APT_CheckNew3DS(&is_new_3ds)) && is_new_3ds;
}

char* SystemGetScreenType(void) {
  static char upper_screen[20];
  static char lower_screen[20];
  static char screen_type[32];

  if (UtilsIsN3Ds()) {
    u8 screens = 0;

    if (R_SUCCEEDED(gspLcdInit())) {
      if (R_SUCCEEDED(GSPLCD_GetVendors(&screens))) gspLcdExit();
    }

    switch ((screens >> 4) & 0xF) {
      case 0x01:  // 0x01 = JDI => IPS
        sprintf(upper_screen, "Upper: IPS");
        break;
      case 0x0C:  // 0x0C = SHARP => TN
        sprintf(upper_screen, "Upper: TN");
        break;
      default:
        sprintf(upper_screen, "Upper: Unknown");
        break;
    }

    switch (screens & 0xF) {
      case 0x01:  // 0x01 = JDI => IPS
        sprintf(lower_screen, " | Lower: IPS");
        break;
      case 0x0C:  // 0x0C = SHARP => TN
        sprintf(lower_screen, " | Lower: TN");
        break;
      default:
        sprintf(lower_screen, " | Lower: Unknown");
        break;
    }

    strcpy(screen_type, upper_screen);
    strcat(screen_type, lower_screen);
  } else {
    sprintf(screen_type, "Upper: TN | Lower: TN");
  }

  return screen_type;
}

u8 battery_percent = 0;
u8 battery_status = 0;
bool is_connected = false;
bool is_bottom_screen = false;

void PrintStaticInfo(void) {
  printf("\x1b[1;0H\x1b[32mGreenCitrus\x1b[0m \nTool for de-yellow'ing TN screens");
  printf("\x1b[4;0H\x1b[31;1m*\x1b[0m Screen type:\x1b[31;1m %s \x1b[0m", SystemGetScreenType());
  printf("\x1b[6;0H\x1b[34;1m*\x1b[0m Battery percentage:");
  printf("\x1b[8;0H\x1b[34;1m*\x1b[0m Adapter state:");
  printf("\x1b[10;0H\x1b[32m*\x1b[0m Selected screen:");
  printf("\x1b[11;0HPress \x1b[32mUP\x1b[0m or \x1b[32mDOWN\x1b[0m to select screen");
  printf("\x1b[30;0HPress\x1b[31;1m START\x1b[0m to exit");
}

void PrintDynamicInfo(void) {
  if (R_SUCCEEDED(MCUHWC_GetBatteryLevel(&battery_percent)))
    printf("\x1b[6;22H\x1b[34;1m%3d%%\x1b[0m ", battery_percent);
  else
    printf("\x1b[34;1mN/A\x1b[0m ");

  if (R_SUCCEEDED(PTMU_GetBatteryChargeState(&battery_status)))
    printf("(\x1b[34;1m%s\x1b[0m)     \n", battery_status ? "charging" : "not charging");
  else
    printf("(\x1b[34;1mN/A\x1b[0m)\n");

  printf("\x1b[8;18H");
  if (R_SUCCEEDED(PTMU_GetAdapterState(&is_connected)))
    printf("\x1b[34;1m%s\x1b[0m\n", is_connected ? "connected   " : "disconnected");
  else
    printf("\x1b[34;1mN/A\x1b[0m\n");

  printf("\x1b[10;19H \x1b[32m>\x1b[0m%s", is_bottom_screen ? "bottom" : "top");
}

void DrawFrame(C3D_RenderTarget* screen, u32 color) {
  C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
  C2D_TargetClear(screen, color);
  C2D_SceneBegin(screen);
  C3D_FrameEnd(0);
}

void InitServices(void) {
  mcuHwcInit();  // Battery info
  ptmuInit();    // Charger info
  gspLcdInit();  // Screen brightness
}

void TerminateServices(void) {
  ptmuExit();    // Battery info
  mcuHwcExit();  // Charger info
  gspLcdExit();  // Screen brightness
}

int main(void) {
  // Initialize libraries.
  gfxInitDefault();
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

    if (k_down & KEY_DDOWN) {
      DrawFrame(screen, clr_black);
      screen = bot;
      is_bottom_screen = true;
      consoleSelect(&console_top);
      PrintStaticInfo();
      DrawFrame(screen, clr_white);
    }
    if (k_down & KEY_DUP) {
      DrawFrame(screen, clr_black);
      screen = top;
      is_bottom_screen = false;
      consoleSelect(&console_bottom);
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
