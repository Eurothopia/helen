enum connection_state {
    UNKNOWN,    // initial / uninitialized
    OFF,        // radio fully stopped
    STARTING,   // init in progress
    WIFI_CONNECTED,  // successfWIFI_CONNECTEDected
    ERROR       // failed to connect
};