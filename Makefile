run: release
	./ZAP2/builds/release

release : ZAP2/*.c ZAP2/*.h
	gcc -Wall -Wextra ZAP2/*.c -o ZAP2/builds/release
