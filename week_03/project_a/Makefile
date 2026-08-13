CFLAGS = -Wall -Wextra -Werror -std=c11

all: poll_O0 poll_O2

poll_O0: main.c flag.c flag.h
	gcc -O0 $(CFLAGS) main.c flag.c -o $@

poll_O2: main.c flag.c flag.h
	gcc -O2 $(CFLAGS) main.c flag.c -o $@

# -O0 terminates; -O2 spins forever (timeout exits 124)
test: all
	@echo "== -O0 =="; timeout 5 ./poll_O0; echo "exit=$$?"
	@echo "== -O2 =="; timeout 5 ./poll_O2; echo "exit=$$?"

disas: all
	@objdump -d --no-show-raw-insn poll_O0 | awk '/<main>:/,/^$$/'
	@objdump -d --no-show-raw-insn poll_O2 | awk '/<main>:/,/^$$/'

clean:
	rm -f poll_O0 poll_O2

.PHONY: all test disas clean

