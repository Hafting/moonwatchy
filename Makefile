
CFLAGS=-O2
CXXFLAGS=-O2
DEPS := include/UechiGothic20num.h include/Gogh_ExtraBold7A_T.h include/Engine5pt7.h include/Engine5pt8.h include/FreeSansBold15num.h include/leaguegothic12pt7.h include/leaguegothic12pt8.h include/FreeSerif8pt8.h include/FreeSerif12pt8.h

all: $(DEPS)
	pio -f -c vim run

upload: $(DEPS)
	pio -f -c vim run --target upload

clean: $(DEPS)
	pio -f -c vim run --target clean

program: $(DEPS)
	pio -f -c vim run --target program

uploadfs: $(DEPS)
	pio -f -c vim run --target uploadfs

update: $(DEPS)
	pio -f -c vim update

include/UechiGothic20num.h: fonts/Uechi\ Gothic.ttf
	fontconvert fonts/Uechi\ Gothic.ttf 20 48 57 > include/UechiGothic20num.h

include/Gogh_ExtraBold7A_T.h: fonts/Gogh-ExtraBold.ttf
	fontconvert fonts/Gogh-ExtraBold.ttf 7 65 84 > include/Gogh_ExtraBold7A_T.h

include/Engine5pt7.h: fonts/Engine.ttf
	fontconvert fonts/Engine.ttf 5 65 90 > include/Engine5pt7.h

include/Engine5pt8.h: fonts/Engine.ttf
	fontconvert fonts/Engine.ttf 5 192 223 > include/Engine5pt8.h

include/FreeSansBold15num.h: fonts/FreeSansBold.ttf
	fontconvert fonts/FreeSansBold.ttf 15 48 57 > include/FreeSansBold15num.h

include/leaguegothic12pt7.h: fonts/leaguegothic-regular-webfont.ttf
	fontconvert fonts/leaguegothic-regular-webfont.ttf 12 32 126 > include/leaguegothic12pt7.h

include/leaguegothic12pt8.h: fonts/leaguegothic-regular-webfont.ttf
	fontconvert fonts/leaguegothic-regular-webfont.ttf 12 160 383 > include/leaguegothic12pt8.h

#Some symbols in smaller size, so they fit in a crowded calendar
include/FreeSerif8pt8.h: fonts/FreeSerif.ttf
	fontconvert fonts/FreeSerif.ttf 8 9673 9681 > include/FreeSerif8pt8.h

include/FreeSerif12pt8.h: fonts/FreeSerif.ttf
	fontconvert fonts/FreeSerif.ttf 12 9728 9928 > include/FreeSerif12pt8.h

fonts/Gogh-ExtraBold.ttf: fonts/gogh.zip
	cd fonts;unzip -DD gogh.zip Gogh-ExtraBold.ttf 

fonts/Uechi\ Gothic.ttf: fonts/uechi.zip
	cd fonts;unzip -DD uechi.zip Uechi\ Gothic.ttf

fonts/leaguegothic-regular-webfont.ttf: fonts/league-gothic.zip
	cd fonts;unzip -j -DD league-gothic.zip theleagueof-league-gothic-64c3ede/webfonts/leaguegothic-regular-webfont.ttf
	
