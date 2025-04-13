include ../cpu.mk

TARGETS=receiver sender 
UTILS=util.o 

all: $(TARGETS)

$(UTILS): %.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

%.o: %.c util.h
	$(CC) $(CFLAGS)  -c $< 

$(TARGETS): %:%.o util.o
	$(CC) $(CFLAGS) $^ -o $@

run_sender: sender 
	taskset -c $(SENDER_CPU) ./sender

run_receiver: receiver
	taskset -c $(RECEIVER_CPU) ./receiver

.PHONY:	clean

clean:
	$(RM) *.o $(HELPERS) $(TARGETS) 

sender_bit: sender_bit.c util.c
	$(CC) -o sender_bit sender_bit.c util.c -O2

receiver_bit: receiver_bit.c util.c
	$(CC) -o receiver_bit receiver_bit.c util.c -O2

sender_8bit_from_working: sender_8bit_from_working.c util.c
	$(CC) -o sender_8bit_from_working sender_8bit_from_working.c util.c -O2

receiver_8bit_from_working: receiver_8bit_from_working.c util.c
	$(CC) -o receiver_8bit_from_working receiver_8bit_from_working.c util.c -O2
