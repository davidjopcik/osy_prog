for n in {1..50}; do
  echo "n=$n"
  ./binary "$n" | xxd -b -c1
done