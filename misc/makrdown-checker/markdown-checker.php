#!/usr/bin/php
<?php
$target_file = "../../README.md";
if ($argc >= 2)
    $target_file = $argv[1];
$md = file_get_contents($target_file);
$sd = new Sundown($md);
echo $sd->to_html();
?>
