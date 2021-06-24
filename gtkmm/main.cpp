#include <gtkmm.h>
#include "mainWindow.hpp"

using namespace Gtk;

int main(int argc, char *argv[]){
	Main main(argc, argv);
	MainWindow mainWindow;
	main.run(mainWindow);
}

MainWindow::MainWindow(){
	MainWindow::setWindow();

	show_all_children();
}
