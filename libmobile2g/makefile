#
# 2g–≠“È±‡“ÎΩ≈±æ
#
include $(HOME)/make.dir/makefile.std

all:libmobile2g.so

test2g: Mobile2GIntoLayerB.o Mobile2GIntoLayerC.o Mobile2GAppLayer.o DasAppLayer.o Mobile2GNetworkLayer.o Mobile2GControl.o test2g.o
	$(CCLINK)

libmobile2g.so:Mobile2GIntoLayerB.o Mobile2GIntoLayerC.o Mobile2GAppLayer.o DasAppLayer.o Mobile2GNetworkLayer.o Mobile2GControl.o
	${SOLINK}

