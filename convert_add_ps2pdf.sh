#!/bin/bash

for ((i=1;i<=90;i++));
do
  name=$(printf "HV_gains_offsets%i.ps" "$i")
  ps2pdf $name
  echo $name
  namepdf=$(printf "HV_gains_offsets%i.pdf" "$i")
  allname+=$(printf " %s" "$namepdf")
done

echo $allname
pdftk $allname cat output HV_gains_offsets_all.pdf
