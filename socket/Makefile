cc=gcc
RM=rm -fr
LL=-lpthread
C=-std=c99

SE=serv_epoll_mul_pthread.c
CL=cln_mul_pthread.c

SE1=serv
CL1=cln

.PHONY:all
all:$(SE1)  $(CL1)


$(SE1):$(SE)
	$(cc) $(SE) -o $(SE1) $(C) $(LL)

$(CL1):$(CL)
	$(cc) $(CL) -o $(CL1) $(C)  $(LL)

clean:
	$(RM) $(SE1).o  $(CL1).o 
