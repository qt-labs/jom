#!/bin/perl
$jomVersion = `../bin/jom.exe /VERSION`;
$jomVersion = substr($jomVersion, 12);
$jomVersion = substr($jomVersion, 0, -1);
$jomVersion =~ /(\d+)\.(\d+)\.(\d+)/;
$jomVersion = ${1} . "_" . ${2} . "_" . ${3};
system("copy /Y jom.zip jom_" . $jomVersion . ".zip");
