export LD_LIBRARY_PATH="./verlib"
../gennum 2510040000 10000 | ./verbank -v | wc 
../gennum 2560040000 10000 | ./verbank -v | wc 
export LD_LIBRARY_PATH="./rclib"
../gennum 2510040000 10000 | ./verbank -v | wc 
../gennum 2560040000 10000 | ./verbank -v | wc 
export LD_LIBRARY_PATH="./rcverlib"
../gennum 2510040000 10000 | ./verbank -v | wc 
../gennum 2560040000 10000 | ./verbank -v | wc 