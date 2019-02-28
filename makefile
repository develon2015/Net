# Makefile
VPATH = bin:src

%: %.c
	$(CC) -o bin/$@ $^

MultiTarget: bin \
	gethostbyname socket news
	@echo OK

bin:
	mkdir bin

.PHONY: clean
clean:
	rm -rf bin/*
