release : ZAP3/*.c ZAP3/*.h
	gcc -Wall -Wextra ZAP3/*.c -o ZAP3/builds/release
	./ZAP3/builds/release
