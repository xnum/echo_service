all: echo_s

echo_s:
	gcc -o $@ main.c -luv

install:
	cp echo_s /opt
	cp echod /etc/init.d
	chmod 755 /etc/init.d/echod
