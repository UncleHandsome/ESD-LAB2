CFLAGS = -gdwarf-2 -O0
#CFLAGS = -O2
CC=$(CROSS_COMPILE) $(CFLAGS) -I$(LINUXDIR)/include


EXEC = creator-pxa270-fpga
OBJS = creator-pxa270-fpga.o 

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

romfs:
	$(ROMFSINST) /bin/$(EXEC)

clean:
	-rm -f $(EXEC) *.elf *.gdb *.o

