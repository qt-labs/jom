#!/bin/perl
$jomVersion = `../bin/jom.exe /VERSION`;
$jomVersion = substr($jomVersion, 12);
$jomVersion = substr($jomVersion, 0, -1);
$jomVersion =~ s/\.//g;
system("copy /Y jom.zip jom" . $jomVersion . ".zip");
