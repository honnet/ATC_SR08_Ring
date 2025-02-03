#!/bin/sh
wids=( $(xprop -root | grep '_NET_CLIENT_LIST.*(WINDOW)' | sed 's/^.*#//' | sed 's/,/\n/g' | sort -u ) )
for w in ${wids[@]}; do xprop -id $w | grep _NET_WM_NAME | grep SmartSnippets; done