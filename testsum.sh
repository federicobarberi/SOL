#!/bin/bash

for (( i = 0; i < 50; i++ )); do
  ./client client_$i 1 &
done
wait

for (( i = 0; i < 50; i++ )); do
  if [[ $i -lt 30 ]]; then
    ./client client_$i 2 &
  else
    ./client client_$i 3 &
  fi
done
wait

#Segnale SIGUSR1 -30 su mac(pidof non su mac : ps | grep server), -10 su xubuntu
kill -USR1 $(ps | grep server);
wait

#Scrivo dimensione cartella data su testout.log
du -hs data >> testout.log

#Stampo a schermo l'esito dei test
cat testout.log
wait

rm -f testout.log
