#!/usr/bin/env bash
NUM=24

for n in $(seq 1 $NUM)
do
  for v in $(echo -e "ready\nworking\nproblem\ndisconnected")
  do
    FILENAME=icons/numbered/"$v"_"$n".ico
    echo $FILENAME
    magick \
      icons/$v.ico \
      -font 'Fira-Sans-Bold' \
      -fill white \
      -gravity center \
      -pointsize 80 \
      -annotate 0 $n \
      $FILENAME
  done
done
