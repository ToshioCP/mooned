# VAR ?= value はVARが未定義の時のみvalueを代入する、という意味
CC ?= gcc

# which はパスを返す
#  $ which pkg-config
#  /usr/bin/pkg-config
#  $ 
PKGCONFIG = $(shell which pkg-config)
CFLAGS = $(shell $(PKGCONFIG) --cflags gtk+-3.0)
LIBS = $(shell $(PKGCONFIG) --libs gtk+-3.0)

# glib-compile-resoutcesのパスを返す
#  $ pkg-config --variable=glib_compile_resources gio-2.0
#  /usr/lib/x86_64-linux-gnu/glib-2.0/glib-compile-resources
#  $ 
GLIB_COMPILE_RESOURCES = $(shell $(PKGCONFIG) --variable=glib_compile_resources gio-2.0)

SRC = moonedapp.c moonedwin.c main.c resources.c
HEADER = moonedapp.h moonedwin.h
RESOURCES = $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=. --generate-dependencies mooned.gresource.xml)

# $(var:a=b)は変数varの中の文字列aを文字列bに置き換えた文字列を返す
OBJS = $(SRC:.c=.o)

all: mooned

# $(@F)はターゲットがパス名の場合、そのファイル名（の部分）になる。
# 例えば、dir/foo.oに対して、$(@F)はfoo.o。 
mooned: $(OBJS)
	$(CC) -o $(@F) $(OBJS) $(LIBS)

# %.oは、拡張子が.oであるすべてのファイルに一致。%.cも同様。
# $(OBJS):がその前にあると、その中の拡張子が.oのファイル全てに一致。
# $<は最初の必要条件の名前。 
# したがって、下記の1行が下記の６行に等しくなる。
#   main.o: main.c $(HEADER)
#           $(CC) -c -o main.o $(CFLAGS) main.c
#   mooned.o: mooned.c $(HEADER)
#           $(CC) -c -o mooned.o $(CFLAGS) mooned.c
#   moonedwin.o: moonedwin.c $(HEADER)
#           $(CC) -c -o moonedwin.o $(CFLAGS) moonedwin.c
$(OBJS): %.o: %.c $(HEADER)
	$(CC) -fPIC -c -o $(@F) $(CFLAGS) $<

resources.c: mooned.gresource.xml $(RESOURCES)
	$(GLIB_COMPILE_RESOURCES) mooned.gresource.xml --target=$@ --sourcedir=. --generate-source

.Phony: clean

clean:
	rm -f $(OBJS)
	rm -f mooned
	rm -f resources.c
