#Only change program name and deps
#program name
LAB = lab7
LAB2 = owlsh
#add headers
DEPS = $(empty)
DEPS2 = $(empty)
PROG = $(addsuffix _1, $(LAB)) $(addsuffix _2, $(LAB))

CLFAGS =-g -Wall -Wshadow -Wunreachable-code -Wredundant-decls \
		-Wmissing-declarations -Wold-style-definition -Wmissing-prototypes \
        -Wdeclaration-after-statement

all: $(PROG)

#lab7 config
$(addsuffix _1.o, $(LAB)) : $(addsuffix _1.c, $(LAB)) $(DEPS)
	$(CC) $(CLFAGS) -c $<

$(addsuffix _1, $(LAB)) : $(addsuffix _1.o, $(LAB))
	$(CC) $(CLFAGS) -o $@ $^

#runs owlsh
valgrind1: $(addsuffix _1, $(LAB))
	clear
	valgrind ./$(addsuffix _1, $(LAB))

#owlsh config
$(addsuffix .o, $(LAB2)) : $(addsuffix .c, $(LAB2)) $(DEPS2)
	$(CC) $(CLFAGS) -c $<

$(LAB2)) : $(addsuffix .o, $(LAB2))
	$(CC) $(CLFAGS) -o $@ $^

valgrind2: $(LAB2)
	@echo No reason for valgrind on lab7

compress: clean
	tar -czvf $(addsuffix .tar.gz, $(LAB)) *

clean:
	rm -rf $(PROG) *.o *.dSYM
