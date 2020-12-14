#ifndef DISPLAYH
#define DISPLAYH

#define WIDTH       1024
#define HEIGHT      864

// Тип отображения дампа памяти
#define DUMP_ZP     1
#define DUMP_STACK  2
#define DUMP_OAM    3

// 1 пиксель на экране
struct sRGB {

    unsigned char r;
    unsigned char g;
    unsigned char b;

};

// Глобальная палитра
struct sRGB globalPalette[ 64 ];

// RGB x ScanlineY x ScanlineX
struct   sRGB frame [ HEIGHT ][ WIDTH ];
unsigned char opaque[ HEIGHT ][ WIDTH ];

// Режим отображения дампа памяти
char dump_mode;
int  justRedrawAll;
int  redrawDump;

// Спрайты
unsigned char   ctrl0;
unsigned char   ctrl1;
unsigned char   spraddr;
unsigned char   objvar;
int             sprite[256];
unsigned char   ppu_status;
unsigned char   firstWrite;
int             regFV, regVT, regV, regH, regFH, regHT,
                ppu_address,
                cntFV, cntV, cntH, cntVT, cntHT, b1, b2;

/* Буферизированные параметры скроллинга */
int vb_HT[262], vb_VT[262], vb_FH[262], vb_FV[262];
           
// Текущий видео адрес
int VRAMAddress;
int HMirroring;     /* При X + ScrollX > 256 использовать второй экран */
int VMirroring;     /* При Y + ScrollY > 256 использовать второй экран */

// Прототипы
// ---------------------------------------------------------------------

void setBigPixel(int, int, int);
void printString(int, int, char*, int, int);
void printHex(int, int, unsigned int, int, int, int);
void drawLine(int, int, int, int, int);
void printScreen();
void printRegisters();
void display();
void swap();
void fontsLoad();
void initGlobalPal();
void setPalette(int, int, int, int);

#endif
