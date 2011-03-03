#
# menconfig/xconfig/... targets for configuring firmly.0cpm
#
# This works just like it does for the Linux kernel, from which this
# code has been shared in.
#


xconfig: bin/qconf
	$< $(Kconfig)

gconfig: bin/gconf
	$< $(Kconfig)

menuconfig: bin/mconf
	$< $(Kconfig)

bin/mconf: bin/kconfig/mconf.c
	flex -o bin/lex.zconf.c bin/kconfig/zconf.l
	cp bin/kconfig/zconf.tab.c_shipped bin/zconf.tab.c
	cp bin/kconfig/zconf.hash.c_shipped bin/zconf.hash.c
	# bison -o bin/zconf.tab.c bin/kconfig/zconf.y
	gcc -Wall -Wstrict-prototypes -O2 -fomit-frame-pointer   -DCURSES_LOC="<ncurses.h>" -DLOCALE   -c -o bin/kconfig/kxgettext.o bin/kconfig/kxgettext.c
	gcc -Wall -Wstrict-prototypes -O2 -fomit-frame-pointer   -DCURSES_LOC="<ncurses.h>" -DLOCALE  -Ibin/kconfig -c -o bin/zconf.tab.o bin/zconf.tab.c
	gcc -Wall -Wstrict-prototypes -O2 -fomit-frame-pointer   -DCURSES_LOC="<ncurses.h>" -DLOCALE   -c -o bin/mconf.o bin/kconfig/mconf.c
	gcc -Wall -Wstrict-prototypes -O2 -fomit-frame-pointer   -DCURSES_LOC="<ncurses.h>" -DLOCALE   -c -o bin/checklist.o bin/kconfig/lxdialog/checklist.c
	gcc -Wall -Wstrict-prototypes -O2 -fomit-frame-pointer   -DCURSES_LOC="<ncurses.h>" -DLOCALE   -c -o bin/inputbox.o bin/kconfig/lxdialog/inputbox.c
	gcc -Wall -Wstrict-prototypes -O2 -fomit-frame-pointer   -DCURSES_LOC="<ncurses.h>" -DLOCALE   -c -o bin/menubox.o bin/kconfig/lxdialog/menubox.c
	gcc -Wall -Wstrict-prototypes -O2 -fomit-frame-pointer   -DCURSES_LOC="<ncurses.h>" -DLOCALE   -c -o bin/textbox.o bin/kconfig/lxdialog/textbox.c
	gcc -Wall -Wstrict-prototypes -O2 -fomit-frame-pointer   -DCURSES_LOC="<ncurses.h>" -DLOCALE   -c -o bin/yesno.o bin/kconfig/lxdialog/yesno.c
	gcc -Wall -Wstrict-prototypes -O2 -fomit-frame-pointer   -DCURSES_LOC="<ncurses.h>" -DLOCALE   -c -o bin/util.o bin/kconfig/lxdialog/util.c
	gcc  -o bin/mconf bin/mconf.o bin/zconf.tab.o bin/checklist.o bin/util.o bin/inputbox.o bin/textbox.o bin/yesno.o bin/menubox.o -lncurses

bin/qconf: bin/kconfiq/qconf.cc
	@echo TODO: Copy from Linux kernel cmd_scripts
	@exit 1

bin/gconf.o: bin/gconf.c
	@echo TODO: Copy from Linux kernel cmd_scripts
	@exit 1

