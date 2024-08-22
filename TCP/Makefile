cc=gcc
EXE=server
OBJS=$(patsubst %.c,%.o,$(wildcard *.c))
CFLAGS=-c -o
LN=-lcurl -lcjson -lpthread -lsqlite3

all:$(EXE)

$(EXE):$(OBJS)
	$(cc) $^ -o $@ $(LN)

%.o:%.c
	$(cc) $(CFLAGS) $@ $^ 

.PHONY:clean

clean:
	@rm $(OBJS) $(EXE)