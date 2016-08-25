include parallelDefs

COMMON = sequence.h parseCommandLine.h parallel.h quickSort.h runTests.py utils.h

kmeansCheck: kmeansCheck.C ${COMMON}
	${PCC} ${PCFLAGS} ${PLFLAGS} -o kmeansCheck kmeansCheck.C

${COMMON}:
	ln -s ../../common/$@

clean:
	rm -f kmeansCheck

cleansrc:
	make -s clean
	rm -f $(COMMON) *.pyc

