sht:
	@echo " Compile ht_main ...";
	gcc -I ./include/ -L ./lib/ -Wl,-rpath,./lib/ ./examples/sht_main.c ./src/hash_file.c ./src/sht_file.c -lbf -lm -o ./build/runner -O2

run:
	./build/runner

clean:
	rm -f ./build/runner data1.db \
	rm -f data.db
	rm -f sdata.db
	rm -f data2.db
	rm -f sdata2.db