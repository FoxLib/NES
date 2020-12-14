// 46 Кб общедоступной SRAM

//  МАППЕР ДЛЯ СОБСТВЕННОЙ РАЗРАБОТКИ
//  $0000 - $1FFF 8k  RAM (+STACK)
//  $2000 - $3FFF 6K  VIDEO 
//                    $2000-$23FF VRAM-1 1K: основная
//                    $2400-$27FF VRAM-2 1K: дополнительная
//                    $2800-$2FFF FONT   2K: шрифты
//                    $3000-$37FF VRAM-3 2k: расширение шрифтов
//                    $3800-$3FFF FAILED 2k: нет места в SRAM на ПЛИС
//  $8000 - $BFFF 16K PRG-ROM (1)
//  $C000 - $FFFF 32K PRG-ROM (0) 
// $10000             Системная память для шрифтов

#ifndef CPUH
#define CPUH

#define TRACER          0
#define BREAKPOINT      0x0000  // 0xC000

#define EXEC_QUANT      27167   // 25Mhz=416667, 1.75Mhz=29167 | Сколько инструкции выполнить за 1/60 с
#define MAPPER_NES      1       // NES-маппер
#define MAPPER_OWN      2       // Свой маппер

// Операнды
#define ___      0          // -- ошибка --
#define NDX      1          // (b8,X)
#define ZP       2          // b8
#define IMM      3          // #b8
#define ABS      4          // b16
#define NDY      5          // (b8),Y
#define ZPX      6          // b8,X
#define ABY      7          // b16,Y
#define ABX      8          // b16,X
#define REL      9          // b8 (адрес)
#define ACC     10          // A
#define IMP     11          // -- нет --
#define ZPY     12          // b8,Y
#define IND     13          // (b16)

// Инструкциия
#define BRK      1
#define ORA      2
#define AND      3
#define EOR      4
#define ADC      5
#define STA      6
#define LDA      7
#define CMP      8
#define SBC      9
#define BPL     10
#define BMI     11
#define BVC     12
#define BVS     13
#define BCC     14
#define BCS     15
#define BNE     16
#define BEQ     17
#define JSR     18
#define RTI     19
#define RTS     20
#define LDY     21
#define CPY     22
#define CPX     23
#define ASL     24
#define PHP     25
#define CLC     26
#define BIT     27
#define ROL     28
#define PLP     29
#define SEC     30
#define LSR     31
#define PHA     32
#define PLA     33
#define JMP     34
#define CLI     35
#define ROR     36
#define SEI     37
#define STY     38
#define STX     39
#define DEY     40
#define TXA     41
#define TYA     42
#define TXS     43
#define LDX     44
#define TAY     45
#define TAX     46
#define CLV     47
#define TSX     48
#define DEC     49
#define INY     50
#define DEX     51
#define CLD     52
#define INC     53
#define INX     54
#define NOP     55
#define DOP     55
#define SED     56

// Расширенные инструкции
// --------------------------
#define AAC     57
#define SLO     58
#define RLA     59
#define RRA     60
#define SRE     61
#define DCP     62
#define ISC     63
#define LAX     64
#define AAX     65
#define ASR     66
#define ARR     67
#define ATX     68 
#define AXS     69

// --------- 
#define XAA     DOP 
#define AXA     DOP
#define SYA     DOP
#define SXA     DOP
// ---------

/* Макрос для перехода к определенной метке с учетом +1/+2 цикла */
#define BRANCH  { if ( (addr & 0xff00) != (iaddr & 0xff00)) cycles_per_instr++; addr = iaddr; cycles_per_instr++;  }

unsigned int  reg_A;
unsigned int  reg_X;
unsigned int  reg_Y;
unsigned int  reg_P;
unsigned int  reg_S;
unsigned int  reg_PC;

// 64Кб общей + 16Кб CHR-ROM/VRAM
unsigned char sram[ 128*1024 ]; // PRG-ROM (32k) + CHR-ROM (8k) + VRAM(4)
unsigned char spriteRam[ 256 ]; // Память спрайтов

// Статус CPU, если 0 - остановлен для отладки
int  cpu_running;

int  deb_top;
int  deb_bottom;
int  deb_addr;
int  zp_base;

// Декодированная линия в отладчике
char debLine[32];

// Адреса по каждой линии
int debAddr[64];
int cycles_ext;

// Список точек останова
int breakpoints[256];
int breakpointsMax;     // Максимальная точка в массиве breakpoints

// Текущая линия отладчика
int debCurrentLine;
int debugOldPC;

int Joy1Strobe, Joy1Latch, Joy1;
int Joy2Strobe, Joy2Latch, Joy2;

// Используемый маппер памяти
int mapper;

void initCPU();
int decodeLine(int);
void disassembleAll();
unsigned int getEffectiveAddress(int);
int exec();
unsigned char readB(int);
unsigned int readW(int);
void writeB(int, unsigned char);
unsigned char PULL();
void nmi_exec();
void updateDebugger();

#endif
