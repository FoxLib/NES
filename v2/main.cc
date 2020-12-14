#include "nes.h"

int main(int argc, char** argv) {

    NES nes(1024, 960, "Nintendo Entertainment System [Funcode]");

    if (argc > 1) {
        nes.load_nes_file(argv[1]);
    }

    nes.update_screen();
    nes.loop();

    return 0;
}
