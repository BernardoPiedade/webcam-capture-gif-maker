SOURCE	= main.cpp
OUT	= main.exe

all: $(SOURCE)

	g++ -Wall `pkg-config --libs --cflags gtkmm-3.0` $(SOURCE) -o $(OUT) `pkg-config --libs gtkmm-3.0` -L/usr/lib
