#!/bin/bash
echo "recompile and move updated files"

scriptDir="${PWD}" ##find the real script dir

$scriptDir/recompile
$scriptDir/moveFiles.sh