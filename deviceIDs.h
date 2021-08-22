// deviceIDs.h
// Used by BLE to send the ID of a device with the BLE message, so
// the gateway knows the destination. Checked in with some dummy values.

// Do not add future updates with
// git update-index --skip-worktree deviceIDs.h
// Then enable again if needed with
// git update-index --skip-worktree deviceIDs.h

// Structure of names & IDs. Used for sending BLE messages
struct devID {
  String id;
  String name;
};

// Testing if I can obscure these
devID deviceIDs[] = {
  {"xxxxxxxxxxxxxxxxx", "BLE-Xen1"},
  {"yyyyyyyyyyyyyyyyy", "BLE-Xen6"}
};
