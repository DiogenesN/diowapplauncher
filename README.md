# DioWAppLauncher
A Wayland application to quicky search for and launch applications installed on your system.
It looks for .desktop files in '~/.local/share/applications', '/usr/share/applicartions' and '/usr/local/share/applicartions'.
It was tested on Debian 12 on Wayfire.

# What you can do with DioWAppLauncher
   1. Quickly search for the installed applications.
   2. Launch the application on Enter key press.

   also you need to install the following libs:

		make
		pkgconf
	 	librsvg2-dev
		libcairo2-dev
		libwayland-dev
		libxkbcommon-dev
		libc6-dev (spawn.h)

   on Debian run the following command:

		sudo apt install make pkgconf librsvg2-dev libcairo2-dev libwayland-dev libxkbcommon-dev libc6-dev

   after going through all the above steps, go to the next step in this tutorial.

# Installation/Usage
  1. Open a terminal and run:
 
		 chmod +x ./configure
		 ./configure

  2. if all went well then run:

		 make
		 sudo make install
		 
		 (if you just want to test it then run: make run)
		
  3. Run the application:
  
		 diowapplauncher

# Configuration
The application creates the following configuration file

		.config/diowapplauncher/diowapplauncher.conf

   add the full path to your current theme directory, e.g.:

		icons_theme=/usr/share/icons/Lyra-blue-dark

   any change in the configuration file requires application restart, right click on the panel will close the panel after that launch it again.

# Screenshots

![Alt text](https://raw.githubusercontent.com/DiogenesN/diowapplauncher/main/diowapplauncher.png)

That's it!

# Support

   My Libera IRC support channel: #linuxfriends

   Matrix: https://matrix.to/#/#linuxfriends2:matrix.org

   Email: nicolas.dio@protonmail.com
