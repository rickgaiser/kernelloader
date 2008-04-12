/* Copyright (c) 2007 Mega Man */
#ifndef _CONFIGURATION_H_
#define _CONFIGURATION_H_

#ifdef __cplusplus
extern "C" {
#endif
int loadConfiguration(void);
void saveConfiguration(void);
#ifdef __cplusplus
}
#endif
void addConfigTextItem(const char *name, char *value, int maxlen);
void addConfigCheckItem(const char *name, int *value);

#endif
