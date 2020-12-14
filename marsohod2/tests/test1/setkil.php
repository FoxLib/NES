<?php 

$f = fopen("rom.bin", "rb+");

// 16Kb 
$offset = hexdec($argv[1]) - 0xc000; 

// Пишем KIL 
fseek($f, $offset, SEEK_SET);
fwrite($f, chr(2));

fclose($f);