./gennum 3000030000 
./gennum 5000050000 100 > nums.txt

./verbank -v < nums.txt
./gennum 2000020000 500 | ./verbank -v
./gennum 2000020000 100 -b | ./verbank -v -b