#!/usr/bin/php
<?php
$md = file_get_contents("../../README.md");
$sd = new Sundown($md);
echo $sd->to_html();
?>
