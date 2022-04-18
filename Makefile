cloud:cloud.cpp bundle.cpp
	g++ $^ -o $@ -std=c++14 -lpthread -lstdc++fs


.PHONY:
clean:
	rm cloud
