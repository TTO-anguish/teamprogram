#指定编译器为gcc
CC=gcc
EXE=cccc
#CC=arm-none-linux-gnueabi-gcc
#指定编译时的参数
AFLAGS= -c -g -m32
#指定手动链接的库
LDFLAGS= -lpthread -m32 -lpaho-mqtt3c -lrt -lm
#指定依赖文件
OBJS=$(patsubst %.c,%.o,$(wildcard *.c)) 

#基于依赖编译生成目标demo
all:$(EXE)

$(EXE):$(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)
#指定依赖文件生成的规则
%.o:%.c
	$(CC) $(AFLAGS) $< -o $@

#指定伪目标
.PHONY:clean lib
clean:
	@rm $(OBJS) $(EXE) 

lib:
	ar crs libmylib.a 
#gcc main.c -o server -L.. -lmylib -lpthread
