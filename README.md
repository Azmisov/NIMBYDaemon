##Description

This is a daemon for linux. It toggles the NIMBY option of Tractor blades for your render farm. It will disable NIMBY if the computer is idle. A configuraiton file defines what makes a computer idle.

##Usage

- Make sure Tractor Blade is installed on the computer
- Copy the NIMBYDaemon.config file to the **website** directory where your Tractor Engine is installed. The daemon updates its configuration by fetching `http://tractor-engine/NIMBYDaemon.config` periodically.
    ```
    cp NIMBYDaemon.config ENGINE_DIR/website/
    chmod -x ENGINE_DIR/website/NIMBYDaemon.config
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

Most of the customization for the daemon can be done by editing the `NIMBYDaemon.config` file. A couple things are hardcoded, so here are some files you might want to edit:
- *config.h*: a couple define headers for extra logging and debugging
- *config.c*: configuration defaults, if the daemon cannot fetch the `NIMBYDaemon.config` file
- *query.c*: more configuration defaults; also holds the URL of the configuration file (inside `query_config` method)
- *main.c*: default log file location
- *logging.c*: maximum log file size, before the log gets emptied

##Tractor Engine Dashboard

In addition to the daemon, there are several scripts you can use to modify the Tractor engine dashboard. They add another tab on the settings page for modifying the `NIMBYDaemon.config`. The tab is named "NIMBY." If you use this, you won't need to edit the configuration file manually. *Note: most of the code in AppView.js.gz is copyright Pixar; so do not use it illegally.*

    # First, copy the scripts to the website directory
    cp website/NIMBY.html ENGINE_DIR/website/tv/js/
    cp website/AppView.js.gz ENGINE_DIR/website/tv/js/
    cp website/NIMBYWriter.js ENGINE_DIR/website/
    # Use the updated AppView script
    mv ENGINE_DIR/website/debug.html ENGINE_DIR/website/index.html
    # Run the NIMBYWriter mini-server as a node module
    # Optionally, have this run as a startup command in rc.local
    node ENGINE_DIR/website/NIMBYWriter.js &

If you use the dashboard for NIMBY configuration, make sure to remove comments from the NIMBYDaemon.config file. JavaScript does not allow comments in JSON. Also, note that `AppView.js.gz` was adapted from the one provided in version 1.7.2. If you have a newer version of Tractor Engine, you may need to manually port over the changes.
