SUBDIRS = SMSUTILS SMSCDVD eromdrvloader

all:
	for dir in $(SUBDIRS); do \
		make -C $$dir || exit -1; \
	done

clean:
	for dir in $(SUBDIRS); do \
		make -C $$dir clean || exit -1; \
	done
