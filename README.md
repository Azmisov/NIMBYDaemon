##Description

This is a daemon for linux. It toggles the NIMBY option of Tractor blades for your render farm. It will disable NIMBY if the computer is idle. A configuraiton file defines what makes a computer idle.

##Usage

- Make sure Tractor Blade is installed on the computer
- Copy the NIMBYDaemon.config file to the **website** directory where your Tractor Engine is installed. The daemon updates its configuration by fetching `http://tractor-engine/NIMBYDaemon.config` periodically.
    ```
    cp source/NIMBYDaemon.config ENGINE_DIR/website/
    ```
  The config file gives a good description for each of the configuration options. *Warning: do not remove options from the config file, or you'll start getting errors in the log file.*

- Compile the code and move the binary to the Tractor Blade installation directory. Make sure you have development headers for cURL, X11, and XScreenSaver. The daemon uses these libraries.
    ```
    make
    cp NIMBYDaemon BLADE_DIR/
    chmod +x BLADE_DIR/NIMBYDaemon
    ```
- Setup daemon to run on startup. Since it uses X11 for some configuration options, I recommend having it run as a startup script from your dispay manager. For example, with GDM, add the following to `/etc/gdm/Init/Default`

    ```
    BLADE_DIR/NIMBYDaemon &
    ```
    The daemon automatically exits when it can no longer connect to X. Currently, the daemon won't behave very well when there are multiple displays/daemons running.

##Customization

Most of the customization for the daemon can be done by editing the `NIMBYDaemon.config` file. A couple things are hardcoded, which you might want to change. Here are some files you might want to edit:
- *config.h*: a couple define headers for extra logging and debugging
- *config.c*: configuration defaults, if the daemon cannot fetch the `NIMBYDaemon.config` file
- *query.c*: more configuration defaults; also holds the URL of the configuration file (inside `query_config` method)
- *main.c*: default log file location
- *logging.c*: maximum log file size, before the log gets emptied
