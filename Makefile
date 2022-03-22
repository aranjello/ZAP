release : ZAP/*.c ZAP/*.h
	gcc -Wall -Wextra ZAP/*.c -o ZAP/builds/release
	./ZAP/builds/release
