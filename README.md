##Description

This is a daemon for linux. It toggles the NIMBY option of Tractor blades for your render farm. It will disable NIMBY if the computer is idle. A configuraiton file defines what makes a computer idle.

##Usage

- Make sure Tractor Blade is installed on the computer
- Copy the NIMBYDaemon.config file to the **website** directory where your Tractor Engine is installed.
       cp source/NIMBYDaemon.config ENGINE_DIR/website/
  The config file gives a good description for each of the configuration options. *Warning: do not remove options from the config file, or you'll start getting errors in the log file.*

- Compile the code and move the binary to the Tractor Blade installation directory. Make sure you have development headers for cURL, X11, and XScreenSaver. The daemon uses these libraries.
        make
        cp NIMBYDaemon BLADE_DIR/
        chmod +x BLADE_DIR/NIMBYDaemon

- Setup daemon to run on startup. Since it uses X11 for some configuration options, I recommend having it run as a startup script from your dispay manager. For example, with GDM, add the following to `/etc/gdm/Init/Default`
        pkill -9 -f BLADE_DIR/NIMBYDaemon
        BLADE_DIR/NIMBYDaemon &
    Currently, the daemon doesn't behave correctly when multiple instances are running (e.g. with multiple X displays). So for now, we just kill any existing NIMBYDaemon processes before starting the new one.
