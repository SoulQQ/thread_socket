LIB_DIR=/home/soul/from_vs/linux/lib/#存放静态库文件
O_DIR=/home/soul/from_vs/linux/o/#存放中间代码文件.o
BIN_DIR=/home/soul/from_vs/linux/out/#存放可执行文件
APP_DIR=/home/soul/from_vs/linux/#当前目录

$(shell mkdir -p $(LIB_DIR))
$(shell mkdir -p $(O_DIR))
$(shell mkdir -p $(BIN_DIR))

RM=rm -fr
CC=gcc
AR=ar -rc#生成静态库的命令

SRC_OBJ=$(APP_DIR)ThreadPool.c#一些不是main()的.c文件
H_OBJ=$(APP_DIR)ThreadPool.h#头文件
OBJ=ThreadPool.o

DY_OBJ=aThreadPool.o#可执行程序的中间代码文件
DY_SRC_OBJ=main.c

LIB_OBJ=libtemp.a#静态库文件
BIN_OBJ=aThreadPool#最终的可执行程序名

C99=-std=c99
THREAD=-lpthread

all:$(LIB_OBJ)  $(BIN_OBJ)
.PHONY:all




$(OBJ):$(SRC_OBJ) $(H_OBJ)
	$(CC) -c $(SRC_OBJ)  $(C99)
	mv $(OBJ) $(O_DIR)

$(LIB_OBJ):$(OBJ)
	$(AR) $(LIB_OBJ) $(O_DIR)$(OBJ) 
	mv $(LIB_OBJ) $(LIB_DIR)


$(DY_OBJ):$(DY_SRC_OBJ)
	$(CC) -g  -c $(DY_SRC_OBJ) -o $(DY_OBJ) $(C99)
	mv $(DY_OBJ) $(O_DIR)

$(BIN_OBJ):$(DY_OBJ)
	$(CC) -g  $(O_DIR)$(DY_OBJ) $(LIB_DIR)$(LIB_OBJ) -o $(BIN_OBJ)  $(THREAD)
	mv $(BIN_OBJ) $(BIN_DIR)

clean:
	$(RM) $(LIB_DIR) $(O_DIR) $(BIN_DIR)





