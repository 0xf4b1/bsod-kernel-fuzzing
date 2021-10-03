all: syz-bp-cov syz-bp-triage

syz-bp-cov: main.c breakpoint.c
	gcc -o syz-bp-cov main.c breakpoint.c -lvmi

syz-bp-triage: syz-bp-triage.c
	gcc -static -o syz-bp-triage syz-bp-triage.c