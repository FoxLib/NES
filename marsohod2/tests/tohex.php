<?php // php loderom.php > ../init/rom.hex

// Прогрузить по полной программе то, что требуется доказать.
$C = substr(file_get_contents($argv[1]), 0, 32768);
$C = str_pad($C, 32768, chr(0), STR_PAD_RIGHT);

$BRK = 0x8001;
$RST = 0x8000;
$NMI = 0x802C;

$C[ 0x7FFA ] = chr($NMI & 0xFF); $C[ 0x7FFB ] = chr($NMI >> 8); // NMI
$C[ 0x7FFC ] = chr($RST & 0xFF); $C[ 0x7FFD ] = chr($RST >> 8); // RESET
$C[ 0x7FFE ] = chr($BRK & 0xFF); $C[ 0x7FFF ] = chr($BRK >> 8); // BRK

file_put_contents($argv[1], $C);

for ($i = 0; $i < 32768; $i++) echo sprintf("%02X\n", ord($C[$i]));
