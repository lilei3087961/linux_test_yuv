TOP_DIR := $(shell pwd)
APP = test

#CC = arm-none-linux-gnueabi-gcc
CC = gcc
#CFLAGS = -g 
#LIBS = -lpthread -lx264 -lm 
#LIBS = -lpthread 
#DEP_LIBS = -L$(TOP_DIR)/lib
HEADER =
#OBJS = testYuv.o color.o utils.o 
OBJS = test.o
all:  $(OBJS)
	$(CC)  -o $(APP) $(OBJS) 

clean:
		rm -f *.o a.out $(APP) core *~ *.avi ./out/*


