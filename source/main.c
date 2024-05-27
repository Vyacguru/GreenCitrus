#include <citro2d.h>
#include <citro3d.h>

#include <string.h>
#include <stdio.h>

bool UtilsIsN3Ds(void) {
  bool isNew3DS = false;
  return R_SUCCEEDED(APT_CheckNew3DS(&isNew3DS)) && isNew3DS;
}

char *SystemGetScreenType(void) {
  static char upperScreen[20];
  static char lowerScreen[20];

  static char screenType[32];

  if (UtilsIsN3Ds()) {
    u8 screens = 0;

    if (R_SUCCEEDED(gspLcdInit())) {
      if (R_SUCCEEDED(GSPLCD_GetVendors(&screens)))
        gspLcdExit();
    }

    switch ((screens >> 4) & 0xF) {
      case 0x01: // 0x01 = JDI => IPS
        sprintf(upperScreen, "Upper: IPS");
        break;
      case 0x0C: // 0x0C = SHARP => TN
        sprintf(upperScreen, "Upper: TN");
        break;
      default:sprintf(upperScreen, "Upper: Unknown");
        break;
    }
    switch (screens & 0xF) {
      case 0x01: // 0x01 = JDI => IPS
        sprintf(lowerScreen, " | Lower: IPS");
        break;
      case 0x0C: // 0x0C = SHARP => TN
        sprintf(lowerScreen, " | Lower: TN");
        break;
      default:sprintf(lowerScreen, " | Lower: Unknown");
        break;
    }

    strcpy(screenType, upperScreen);
    strcat(screenType, lowerScreen);
  } else
    sprintf(screenType, "Upper: TN | Lower: TN");

  return screenType;
}

u8 batteryPercent = 0;
u8 batteryStatus = 0;
bool isConnected = false;
bool isBottomScreen = false;

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
  if (R_SUCCEEDED(MCUHWC_GetBatteryLevel(&batteryPercent)))
    printf("\x1b[6;22H\x1b[34;1m%3d%%\x1b[0m ", batteryPercent);
  else
    printf("\x1b[34;1mN/A\x1b[0m ");

  if (R_SUCCEEDED(PTMU_GetBatteryChargeState(&batteryStatus)))
    printf("(\x1b[34;1m%s\x1b[0m)     \n", batteryStatus ? "charging" : "not charging");
  else
    printf("(\x1b[34;1mN/A\x1b[0m)\n");

  printf("\x1b[8;18H");
  if (R_SUCCEEDED(PTMU_GetAdapterState(&isConnected)))
    printf("\x1b[34;1m%s\x1b[0m\n", isConnected ? "connected   " : "disconnected");
  else
    printf("\x1b[34;1mN/A\x1b[0m\n");

  printf("\x1b[10;19H \x1b[32m>\x1b[0m%s", isBottomScreen ? "bottom" : "top");
}

void DrawFrame(C3D_RenderTarget *screen, u32 color) {
  C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
  C2D_TargetClear(screen, color);
  C2D_SceneBegin(screen);
  C3D_FrameEnd(0);
}

int main() {

  // Init libs
  gfxInitDefault();
  C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
  C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
  C2D_Prepare();
  PrintConsole consoleTop, consoleBottom;
  consoleInit(GFX_TOP, &consoleTop);
  consoleInit(GFX_BOTTOM, &consoleBottom);
  mcuHwcInit(); // Battery info
  ptmuInit(); // Charger info
  gspLcdInit(); // Screen brightness
  GSPLCD_SetBrightness(GSPLCD_SCREEN_BOTH, 5);


  // Create screens
  C3D_RenderTarget *top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
  C3D_RenderTarget *bot = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
  C3D_RenderTarget *screen = top;

  // Create colors
  u32 clrWhite = C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF);
  u32 clrBlack = C2D_Color32(0x00, 0x00, 0x00, 0x00);

  DrawFrame(screen, clrWhite);
  consoleSelect(&consoleBottom);
  PrintStaticInfo();

  // Main loop
  while (aptMainLoop()) {
    hidScanInput();

    // Respond to user input
    u32 kDown = hidKeysDown();
    if (kDown & KEY_START)
      break; // break in order to return to hbmenu

    PrintDynamicInfo();

    if (kDown & KEY_DDOWN) {
      DrawFrame(screen, clrBlack);
      screen = bot;
      isBottomScreen = true;
      consoleSelect(&consoleTop);
      PrintStaticInfo();
      DrawFrame(screen, clrWhite);
    }
    if (kDown & KEY_DUP) {
      DrawFrame(screen, clrBlack);
      screen = top;
      isBottomScreen = false;
      consoleSelect(&consoleBottom);
      PrintStaticInfo();
      DrawFrame(screen, clrWhite);
    }
  }

  // Deinit libs
  C2D_Fini();
  C3D_Fini();
  ptmuExit();
  mcuHwcExit();
  gspLcdExit();
  gfxExit();
  return 0;
}