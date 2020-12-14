<?php // php loderom.php > ../init/rom.hex

// Прогрузить по полной программе то, что требуется доказать.
$C = substr(file_get_contents("loderunner.nes"), 16, 16384);
for ($i = 0; $i < 16384; $i++) echo sprintf("%02X\n", ord($C[$i]));
