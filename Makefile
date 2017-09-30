
pman:
	@gcc -o pman pman.c

.PHONY clean:
clean:
	@rm -rf pman
