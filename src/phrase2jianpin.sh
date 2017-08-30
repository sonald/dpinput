#!/bin/bash
awk '{print $1}' $1 | awk -F"'" '{s=""; for (i = 1; i <= NF; i++) s= s substr($i, 0, 1); print s}' |sort |uniq
