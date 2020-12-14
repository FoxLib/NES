#include "asm.h"

int main(int argc, char** argv) {

    NESASM as;

    cout << as.assemble("ora #$A0");

    return 0;
}
