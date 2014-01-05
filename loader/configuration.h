/* Copyright (c) 2007 Mega Man */
#ifndef _CONFIGURATION_H_
#define _CONFIGURATION_H_

/** FIle where configuration is stored. */
#define CONFIG_FILE CONFIG_DIR "/config.txt"
/** Directory where configuration is stored. */
#define CONFIG_DIR "mc0:kloader"
/** Directory where configuration is stored on second mc. */
#define CONFIG_DIR2 "mc1:kloader"
/** Config file used for DVDs. */
#define DVD_CONFIG_FILE "cdfs:config.txt"
/** Config file used for NetSurf from USB storage device. */
#define PS2NS_CONFIG_FILE "mass0:PS2NS/CONFIG.TXT"
/** Config file from USB storage device. */
#define USB_CONFIG_FILE "mass0:CONFIG.TXT"

#ifdef __cplusplus
extern "C" {
#endif
int loadConfiguration(const char *configfile);
void saveMcIcons(const char *config_dir);
void saveConfiguration(const char *configfile);
#ifdef __cplusplus
}
#endif
void addConfigTextItem(const char *name, char *value, int maxlen);
void addConfigCheckItem(const char *name, int *value);
void addConfigVideoItem(const char *name, int *value);

#endif
