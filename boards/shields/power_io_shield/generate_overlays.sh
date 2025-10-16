#!/bin/sh
for i in 0 1 2 3 4 5 6 7
do
  cp -f power_io_shield.overlay.template \
    power_io_shield_${i}.overlay
  sed -i "s/%id%/${i}/g" power_io_shield_${i}.overlay
done

cp -f power_io_shield.overlay.template \
    power_io_shield.overlay
sed -i "s/%id%/0/g" power_io_shield.overlay
