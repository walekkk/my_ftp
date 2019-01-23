bin=./text
.PHONY:$(bin)
$(bin):mian.cpp
	g++ -g -std=gun++0x $^ -o $@ -pthread
clean:
	rm $(bin)
